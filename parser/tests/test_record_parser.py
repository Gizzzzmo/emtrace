"""Unit tests for emtrace.record_parser."""

from __future__ import annotations

import pytest
from typing import Literal

from emtrace.record_parser import (
    RecordParser,
    RecordInfo,
    RecordVersionError,
    detect_byteorder,
    _MAGIC_CONSTANT,
    _FRAMING_SIZE,
)
from emtrace.format_info import Size


# ─────────────────────────────────────────────────────────────────────────────
# Binary record builders
#
# All builders produce a bytearray that is a valid, self-contained record (or
# section containing one) as described in TRACE_FORMAT.md.
#
# Concrete layout chosen for simplicity (no internal padding):
#
#   Record framing (14 bytes):
#     [0:3]  b"EMT"
#     [3]    type_offset       = 10        (type tag starts right after framing)
#     [4]    size_offset       = 14        (record_size starts after type tag)
#     [5]    payload_offset    = 14 + size_t_size
#     [6]    endianness_offset = 14 + 2 * size_t_size
#     [7]    size_t width
#     [8]    major version
#     [9]    minor version
#     [10:14] type tag
# ─────────────────────────────────────────────────────────────────────────────

_BYTEORDER_PROBE: dict[int, dict[Literal["little", "big"], bytes]] = {
    1: {"little": b"\x00", "big": b"\x00"},
    2: {"little": b"\x00\x01", "big": b"\x01\x00"},
    4: {"little": b"\x00\x01\x02\x03", "big": b"\x03\x02\x01\x00"},
    8: {
        "little": b"\x00\x01\x02\x03\x04\x05\x06\x07",
        "big": b"\x07\x06\x05\x04\x03\x02\x01\x00",
    },
}


def _pack_size_t(value: int, sz: int, byteorder: Literal["little", "big"]) -> bytes:
    return value.to_bytes(sz, byteorder=byteorder)


def _write_framing(
    record: bytearray,
    record_type: bytes,
    size_offset: int,
    payload_offset: int,
    endianness_offset: int,
    size_t_size: int,
    major: int = 1,
    minor: int = 0,
) -> None:
    record[0:3] = b"EMT"
    record[3] = 10  # type tag directly after the framing
    record[4] = size_offset
    record[5] = payload_offset
    record[6] = endianness_offset
    record[7] = size_t_size
    record[8] = major
    record[9] = minor
    record[10:14] = record_type


def build_mgic_record(
    size_t_size: int = 8,
    byteorder: Literal["little", "big"] = "little",
    ptr_size: int = 8,
    alignment_power: int = 0,
    encoding_id: int = 0,
    magic_version: tuple[int, int] = (1, 0),
    prefix: bytes = b"",
) -> bytes:
    """Build a minimal, spec-correct MGIC record.

    *prefix* is prepended verbatim before the record bytes — useful for
    testing that ``find_and_parse_mgic`` scans past leading garbage.

    Returns the complete byte sequence.  The record itself always starts at
    ``len(prefix)``.
    """
    sz = size_t_size

    # Header offsets (all relative to record start).
    size_offset = 14  # record_size (size_t) right after the framing
    endianness_offset = 14 + sz
    payload_offset = 14 + 2 * sz  # magic constant starts here

    # MGIC payload: magic(32) + ptr_width(1) + alignment_power(1) + encoding_id(1).
    total_size = payload_offset + 32 + 3

    record = bytearray(total_size)
    _write_framing(
        record,
        b"MGIC",
        size_offset,
        payload_offset,
        endianness_offset,
        sz,
        magic_version[0],
        magic_version[1],
    )

    # Record size.
    record[size_offset : size_offset + sz] = _pack_size_t(total_size, sz, byteorder)

    # Endianness probe.
    record[endianness_offset : endianness_offset + sz] = _BYTEORDER_PROBE[sz][byteorder]

    # Magic constant.
    mo = payload_offset  # magic_offset within the record
    record[mo : mo + 32] = _MAGIC_CONSTANT

    # Three single-byte metadata fields.
    record[mo + 32] = ptr_size
    record[mo + 33] = alignment_power
    record[mo + 34] = encoding_id

    return bytes(prefix) + bytes(record)


# Child type descriptor: (name, type_id, size, flag, grandchildren).
# *grandchildren* is again a list of children, allowing arbitrarily deep
# nesting (pre-order serialization as described in TRACE_FORMAT.md).
Child = tuple[str, str, int, int, list["Child"]]


def build_trce_record(
    fmt_string: str = "hello",
    formatter_id: int = 0,
    file: str = "test.c",
    line: int = 42,
    args: list[tuple[str, int, int, list[Child]]] | None = None,
    size_t_size: int = 8,
    byteorder: Literal["little", "big"] = "little",
    trace_version: tuple[int, int] = (1, 0),
    prefix: bytes = b"",
) -> bytes:
    """Build a minimal, spec-correct TRCE record.

    *args* is a list of ``(type_id, size, flag, children)`` tuples where each
    child is ``(name, type_id, size, flag, grandchildren)``.

    Returns the complete byte sequence.  The record itself always starts at
    ``len(prefix)``.
    """
    if args is None:
        args = []

    sz = size_t_size

    def pack(v: int) -> bytes:
        return _pack_size_t(v, sz, byteorder)

    # ── Step 1: count layout entries ──────────────────────────────────────────
    # Header: num_args, fmt_offset  (2 entries)
    # Per arg: type_id_offset, size, flag, num_children  (4 entries)
    # Per child (pre-order): name, type_id, size, flag, num_children (5 entries)
    # Trailer: formatter_id, file_offset, line  (3 entries)

    def subtree_entries(children: list[Child]) -> int:
        return sum(5 + subtree_entries(child[4]) for child in children)

    num_layout_entries = 2  # num_args, fmt_offset
    for _, _, _, children in args:
        num_layout_entries += 4  # arg descriptor
        num_layout_entries += subtree_entries(children)  # child descriptors
    num_layout_entries += 3  # formatter_id, file_offset, line

    # Header offsets (relative to record start).
    size_offset = 14
    endianness_offset = 14 + sz
    payload_offset = 14 + 2 * sz  # layout array starts here

    layout_bytes = num_layout_entries * sz
    strings_start = payload_offset + layout_bytes  # offset within record

    # ── Step 2: assign string offsets and build string section ────────────────
    strings: bytearray = bytearray()

    def add_string(s: str) -> int:
        """Append *s* (null-terminated UTF-8) and return its offset from record start."""
        offset_in_record = strings_start + len(strings)
        strings.extend(s.encode("utf-8") + b"\x00")
        return offset_in_record

    fmt_offset = add_string(fmt_string)

    # Collect string offsets for all child descriptors, in pre-order (the same
    # order in which the child blocks are emitted below).
    arg_type_id_offsets: list[int] = []
    child_string_offsets: list[tuple[int, int]] = []  # [(name_off, type_id_off), ...]

    def collect_child_strings(children: list[Child]) -> None:
        for name, child_type_id, _, _, grandchildren in children:
            name_off = add_string(name)
            tid_off = add_string(child_type_id)
            child_string_offsets.append((name_off, tid_off))
            collect_child_strings(grandchildren)

    for type_id, _, _, children in args:
        arg_type_id_offsets.append(add_string(type_id))
        collect_child_strings(children)

    file_offset = add_string(file)

    # ── Step 3: assemble layout array ─────────────────────────────────────────
    layout: bytearray = bytearray()

    def append_size_t(v: int) -> None:
        layout.extend(pack(v))

    child_offsets_iter = iter(child_string_offsets)

    def emit_children(children: list[Child]) -> None:
        for _, _, child_size, child_flag, grandchildren in children:
            name_off, tid_off = next(child_offsets_iter)
            append_size_t(name_off)  # name_offset
            append_size_t(tid_off)  # type_id_offset
            append_size_t(child_size)  # size
            append_size_t(child_flag)  # flag
            append_size_t(len(grandchildren))  # num_children
            emit_children(grandchildren)

    append_size_t(len(args))  # num_args
    append_size_t(fmt_offset)  # fmt_offset

    for i, (_, arg_size, arg_flag, children) in enumerate(args):
        append_size_t(arg_type_id_offsets[i])  # type_id_offset
        append_size_t(arg_size)  # size
        append_size_t(arg_flag)  # flag
        append_size_t(len(children))  # num_children
        emit_children(children)

    append_size_t(formatter_id)  # formatter_id
    append_size_t(file_offset)  # file_offset
    append_size_t(line)  # line

    assert len(layout) == layout_bytes

    # ── Step 4: assemble the full record ──────────────────────────────────────
    total_size = payload_offset + len(layout) + len(strings)
    record = bytearray(total_size)

    _write_framing(
        record,
        b"TRCE",
        size_offset,
        payload_offset,
        endianness_offset,
        sz,
        trace_version[0],
        trace_version[1],
    )

    record[size_offset : size_offset + sz] = pack(total_size)
    record[endianness_offset : endianness_offset + sz] = _BYTEORDER_PROBE[sz][byteorder]
    record[payload_offset : payload_offset + len(layout)] = layout
    record[strings_start : strings_start + len(strings)] = strings

    return bytes(prefix) + bytes(record)


def make_parser(
    data: bytes,
    size_t_size: int = 8,
    byteorder: Literal["little", "big"] = "little",
    ptr_size: int = 8,
) -> RecordParser:
    return RecordParser(
        memoryview(data),
        size_t_size=size_t_size,
        byteorder=byteorder,
        ptr_size=ptr_size,
    )


def payload_offset_for(size_t_size: int) -> int:
    """Payload offset of the builder records for the given size_t width."""
    return 14 + 2 * size_t_size


def framing_info(parser: RecordParser, offset: int = 0) -> RecordInfo:
    """Parse the framing at *offset*; asserts validity."""
    info = parser._read_framing(offset)
    assert info is not None
    return info


# ─────────────────────────────────────────────────────────────────────────────
# detect_byteorder
# ─────────────────────────────────────────────────────────────────────────────


def test_detect_byteorder_little_1byte():
    assert detect_byteorder(b"\x00") == "little"


def test_detect_byteorder_little_4byte():
    assert detect_byteorder(b"\x00\x01\x02\x03") == "little"


def test_detect_byteorder_little_8byte():
    assert detect_byteorder(b"\x00\x01\x02\x03\x04\x05\x06\x07") == "little"


def test_detect_byteorder_big_4byte():
    assert detect_byteorder(b"\x03\x02\x01\x00") == "big"


def test_detect_byteorder_big_8byte():
    assert detect_byteorder(b"\x07\x06\x05\x04\x03\x02\x01\x00") == "big"


def test_detect_byteorder_unknown():
    # First byte is neither 0 nor len-1.
    assert detect_byteorder(b"\x02\x01\x00\x03") == "unknown"


def test_detect_byteorder_empty():
    assert detect_byteorder(b"") is None


def test_detect_byteorder_too_long():
    assert detect_byteorder(bytes(257)) is None


def test_detect_byteorder_duplicate_byte():
    assert detect_byteorder(b"\x00\x00") is None


# ─────────────────────────────────────────────────────────────────────────────
# parse_mgic_payload
# ─────────────────────────────────────────────────────────────────────────────


@pytest.mark.parametrize(
    "size_t_size,byteorder",
    [
        (4, "little"),
        (8, "little"),
        (4, "big"),
        (8, "big"),
    ],
)
def test_parse_mgic_payload_fields(
    size_t_size: int, byteorder: Literal["little", "big"]
):
    """parse_mgic_payload extracts all MgicInfo fields correctly."""
    data = build_mgic_record(
        size_t_size=size_t_size,
        byteorder=byteorder,
        ptr_size=4,
        alignment_power=2,
        encoding_id=1,
        magic_version=(1, 2),
    )
    # Record always starts at offset 0 when no prefix.
    record_start = 0

    parser = make_parser(data, size_t_size=size_t_size, byteorder=byteorder)
    info = framing_info(parser)
    mgic = parser.parse_mgic_payload(info)

    assert mgic.record_start == record_start
    assert mgic.magic_offset == payload_offset_for(size_t_size)
    assert mgic.version == (1, 2)
    assert mgic.size_t_size == size_t_size
    assert mgic.byteorder == byteorder
    assert mgic.ptr_size == 4
    assert mgic.alignment_power == 2
    assert mgic.encoding_id == 1


def test_parse_mgic_payload_truncated():
    """AssertionError when data is cut short before metadata."""
    data = build_mgic_record()
    # Truncate to just the magic constant — cuts off metadata bytes.
    truncated = data[: 14 + 2 * 8 + 32]
    # The framing itself cannot be read from the truncated data (record_size
    # extends past the end), so drive the framing manually:
    full_parser = make_parser(data)
    full_info = framing_info(full_parser)
    truncated_info = RecordInfo(
        record_start=full_info.record_start,
        record_size=len(truncated),
        type_offset=full_info.type_offset,
        size_offset=full_info.size_offset,
        payload_offset=full_info.payload_offset,
        endianness_offset=full_info.endianness_offset,
        size_t_size=full_info.size_t_size,
        major=full_info.major,
        minor=full_info.minor,
        byteorder=full_info.byteorder,
        record_type=b"MGIC",
    )
    truncated_parser = make_parser(truncated)
    with pytest.raises(AssertionError):
        truncated_parser.parse_mgic_payload(truncated_info)


def test_parse_mgic_payload_bad_magic():
    """AssertionError when magic constant bytes are wrong."""
    data = bytearray(build_mgic_record())
    payload_offset = payload_offset_for(8)
    data[payload_offset] ^= 0xFF  # corrupt first byte of magic
    parser = make_parser(bytes(data))
    info = framing_info(parser)
    with pytest.raises(AssertionError):
        parser.parse_mgic_payload(info)


def test_parse_mgic_payload_ptr_size_zero():
    """AssertionError when the ptr size byte is 0."""
    data = bytearray(build_mgic_record())
    data[payload_offset_for(8) + 32] = 0  # ptr size byte
    parser = make_parser(bytes(data))
    info = framing_info(parser)
    with pytest.raises(AssertionError):
        parser.parse_mgic_payload(info)


# ─────────────────────────────────────────────────────────────────────────────
# parse_record_header
# ─────────────────────────────────────────────────────────────────────────────


def test_parse_record_header_trce():
    data = build_trce_record()
    parser = make_parser(data)
    rec_type, info, next_offset = parser.parse_record_header(0)
    assert rec_type == b"TRCE"
    assert info is not None
    assert info.payload_offset == payload_offset_for(8)
    assert info.record_end == len(data)
    assert info.size_t_size == 8
    assert info.version == (1, 0)
    assert next_offset == len(data)


def test_parse_record_header_mgic():
    data = build_mgic_record()
    parser = make_parser(data)
    rec_type, info, next_offset = parser.parse_record_header(0)
    assert rec_type == b"MGIC"
    assert info is not None
    assert info.payload_offset == payload_offset_for(8)
    assert info.record_end == len(data)
    assert next_offset == len(data)


def test_parse_record_header_unknown_type():
    # Build a TRCE record and overwrite the type string with b"UNKN".
    data = bytearray(build_trce_record())
    data[10:14] = b"UNKN"
    parser = make_parser(bytes(data))
    rec_type, info, next_offset = parser.parse_record_header(0)
    assert rec_type is None
    assert info is not None  # framing still parsed, so callers can skip
    assert info.record_end == len(data)
    assert next_offset == len(data)


def test_parse_record_header_no_emt_prefix():
    data = b"\x00" * 20
    parser = make_parser(data)
    rec_type, info, next_offset = parser.parse_record_header(0)
    assert rec_type is None
    assert info is None
    assert next_offset == 1  # advances by one byte


def test_parse_record_header_truncated_before_offsets():
    # Only 4 bytes — has b"EMT" prefix but not the full 14-byte framing.
    data = b"EMT\x0a"
    parser = make_parser(data)
    rec_type, info, next_offset = parser.parse_record_header(0)
    assert rec_type is None
    assert info is None
    assert next_offset == len(data)


def test_parse_record_header_bad_size_t_width():
    data = bytearray(build_trce_record())
    data[7] = 0  # size_t width 0 is invalid
    parser = make_parser(bytes(data))
    rec_type, info, next_offset = parser.parse_record_header(0)
    assert rec_type is None
    assert info is None
    assert next_offset == len(data)  # EMT prefix present → bail out


def test_parse_record_header_bad_probe():
    """An undetectable endianness probe makes the framing invalid."""
    data = bytearray(build_trce_record())
    endi_offset = 14 + 8
    data[endi_offset : endi_offset + 8] = b"\x02\x01\x00\x03\x00\x00\x00\x00"
    parser = make_parser(bytes(data))
    rec_type, info, next_offset = parser.parse_record_header(0)
    assert rec_type is None
    assert info is None
    assert next_offset == len(data)  # EMT prefix present → bail out


def test_parse_record_header_record_size_overflow():
    # Build a valid record then make record_size absurdly large.
    data = bytearray(build_trce_record())
    sz = 8
    # record_size is at offset 14.
    data[14 : 14 + sz] = (9999).to_bytes(sz, "little")
    parser = make_parser(bytes(data))
    rec_type, info, next_offset = parser.parse_record_header(0)
    assert rec_type is None
    assert info is None
    assert next_offset == len(data)


def test_parse_record_header_record_size_too_small():
    """A record_size smaller than the framing is rejected."""
    data = bytearray(build_trce_record())
    data[14 : 14 + 8] = (8).to_bytes(8, "little")
    parser = make_parser(bytes(data))
    rec_type, info, next_offset = parser.parse_record_header(0)
    assert rec_type is None
    assert info is None
    assert next_offset == len(data)


def test_parse_record_header_big_endian():
    """The framing metadata is read with the record's own byte order."""
    data = build_trce_record(
        fmt_string="be",
        size_t_size=4,
        byteorder="big",
        args=[("int", 4, 0, [])],
    )
    parser = make_parser(data)
    rec_type, info, next_offset = parser.parse_record_header(0)
    assert rec_type == b"TRCE"
    assert info is not None
    assert info.byteorder == "big"
    assert info.size_t_size == 4
    assert info.record_end == len(data)
    assert next_offset == len(data)


# ─────────────────────────────────────────────────────────────────────────────
# parse_mgic_record
# ─────────────────────────────────────────────────────────────────────────────


def test_parse_mgic_record_valid():
    data = build_mgic_record(encoding_id=2, magic_version=(1, 3))
    parser = make_parser(data)
    mgic, record_end = parser.parse_mgic_record(0)
    assert mgic is not None
    assert mgic.encoding_id == 2
    assert mgic.version == (1, 3)
    assert record_end == len(data)


def test_parse_mgic_record_wrong_type_returns_none():
    """A TRCE record at the given offset makes parse_mgic_record return None."""
    data = build_trce_record()
    parser = make_parser(data)
    mgic, record_end = parser.parse_mgic_record(0)
    assert mgic is None
    assert record_end == len(data)


def test_parse_mgic_record_no_emt_prefix():
    data = b"\x00" * 32
    parser = make_parser(data)
    mgic, next_offset = parser.parse_mgic_record(0)
    assert mgic is None
    assert next_offset == 1


def test_parse_mgic_record_does_not_mutate_state():
    """parse_mgic_record must not update parser internal state."""
    data = build_mgic_record(size_t_size=4, ptr_size=4, byteorder="big")
    parser = make_parser(data, size_t_size=4, byteorder="big")
    original_sz = parser.size_t_size
    original_bo = parser.byteorder
    original_ptr = parser.ptr_size
    parser.parse_mgic_record(0)
    assert parser.size_t_size == original_sz
    assert parser.byteorder == original_bo
    assert parser.ptr_size == original_ptr


def test_parse_mgic_record_unsupported_major_version():
    data = build_mgic_record(magic_version=(2, 0))
    parser = make_parser(data)
    with pytest.raises(RecordVersionError) as exc_info:
        parser.parse_mgic_record(0)
    assert exc_info.value.record_type == b"MGIC"
    assert (exc_info.value.major, exc_info.value.minor) == (2, 0)


def test_parse_trce_record_unsupported_major_version():
    data = build_trce_record(trace_version=(2, 1))
    parser = make_parser(data)
    with pytest.raises(RecordVersionError) as exc_info:
        parser.parse_trace_record(0)
    assert exc_info.value.record_type == b"TRCE"
    assert (exc_info.value.major, exc_info.value.minor) == (2, 1)


def test_parse_trce_record_higher_minor_version_accepted():
    """Minor versions higher than known are decoded with the known rules."""
    data = build_trce_record(fmt_string="minor bump", trace_version=(1, 5))
    parser = make_parser(data)
    info, record_end = parser.parse_trace_record(0)
    assert info is not None
    assert info.fmt_string == "minor bump"
    assert info.version == (1, 5)
    assert record_end == len(data)


# ─────────────────────────────────────────────────────────────────────────────
# find_and_parse_mgic
# ─────────────────────────────────────────────────────────────────────────────


@pytest.mark.parametrize(
    "size_t_size,byteorder",
    [
        (8, "little"),
        (4, "little"),
        (8, "big"),
        (4, "big"),
    ],
)
def test_find_and_parse_mgic_valid(
    size_t_size: int, byteorder: Literal["little", "big"]
):
    data = build_mgic_record(
        size_t_size=size_t_size,
        byteorder=byteorder,
        ptr_size=4,
        alignment_power=1,
        encoding_id=2,
        magic_version=(1, 7),
    )
    parser = RecordParser(memoryview(data))
    mgic = parser.find_and_parse_mgic()
    assert mgic is not None
    assert mgic.size_t_size == size_t_size
    assert mgic.byteorder == byteorder
    assert mgic.version == (1, 7)
    assert mgic.ptr_size == 4
    assert mgic.alignment_power == 1
    assert mgic.encoding_id == 2


def test_find_and_parse_mgic_updates_parser_state():
    """find_and_parse_mgic must update size_t_size, ptr_size, and byteorder."""
    data = build_mgic_record(size_t_size=4, byteorder="big", ptr_size=2)
    parser = RecordParser(memoryview(data))  # defaults: sz=8, little
    mgic = parser.find_and_parse_mgic()
    assert mgic is not None
    assert parser.size_t_size == 4
    assert parser.ptr_size == 2
    assert parser.byteorder == "big"


def test_find_and_parse_mgic_does_not_update_on_failure():
    """Parser state must remain unchanged when find_and_parse_mgic returns None."""
    data = b"\x00" * 64
    parser = RecordParser(memoryview(data))  # defaults: sz=8, little
    result = parser.find_and_parse_mgic()
    assert result is None
    assert parser.size_t_size == 8
    assert parser.byteorder == "little"
    assert parser.ptr_size == 8


def test_find_and_parse_mgic_no_magic():
    data = b"\x00" * 64
    parser = RecordParser(memoryview(data))
    assert parser.find_and_parse_mgic() is None


def test_find_and_parse_mgic_magic_without_framing():
    """Magic constant present in data but not preceded by a valid EMT header."""
    # Just the raw magic bytes with no EMT framing anywhere before them.
    data = b"\xaa" * 10 + _MAGIC_CONSTANT + b"\x00" * 20
    parser = RecordParser(memoryview(data))
    assert parser.find_and_parse_mgic() is None


def test_find_and_parse_mgic_with_leading_garbage():
    """find_and_parse_mgic works when garbage bytes precede the MGIC record."""
    prefix = b"\xde\xad\xbe\xef" * 8  # 32 bytes of garbage
    data = build_mgic_record(size_t_size=8, byteorder="little", prefix=prefix)
    parser = RecordParser(memoryview(data))
    mgic = parser.find_and_parse_mgic()
    assert mgic is not None
    assert mgic.record_start == len(prefix)
    assert mgic.magic_offset == len(prefix) + 14 + 2 * 8


def test_find_and_parse_mgic_unsupported_major_version():
    data = build_mgic_record(magic_version=(3, 0))
    parser = RecordParser(memoryview(data))
    with pytest.raises(RecordVersionError):
        parser.find_and_parse_mgic()


# ─────────────────────────────────────────────────────────────────────────────
# parse_trace_payload / parse_trace_record
# ─────────────────────────────────────────────────────────────────────────────


def test_parse_trace_payload_no_args():
    data = build_trce_record(
        fmt_string="hello world", formatter_id=0, file="src/a.c", line=10
    )
    parser = make_parser(data)
    info = parser.parse_trace_payload(0, payload_offset_for(8))
    assert info.fmt_string == "hello world"
    assert info.formatter == 0
    assert info.file == "src/a.c"
    assert info.line == 10
    assert info.type_infos == []


def test_parse_trace_payload_one_leaf_arg_static():
    data = build_trce_record(
        fmt_string="{0}",
        args=[("int", 4, 0, [])],  # flag=0 => EMT_FLAG_STATIC
    )
    parser = make_parser(data)
    info = parser.parse_trace_payload(0, payload_offset_for(8))
    assert len(info.type_infos) == 1
    type_id, type_info = info.type_infos[0]
    assert type_id == "int"
    assert type_info.size == Size(kind="fixed", size=4)
    assert type_info.children == {}


def test_parse_trace_payload_null_terminated_arg():
    data = build_trce_record(
        fmt_string="{0}",
        args=[("char*", 0, 1, [])],  # flag=1 => EMT_FLAG_NULL_TERMINATED
    )
    parser = make_parser(data)
    info = parser.parse_trace_payload(0, payload_offset_for(8))
    _, type_info = info.type_infos[0]
    assert type_info.size == Size(kind="null_terminated", size=0)


def test_parse_trace_payload_length_prefixed_arg():
    data = build_trce_record(
        fmt_string="{0}",
        args=[("uint8[]", 0, 2, [])],  # flag=2 => EMT_FLAG_LENGTH_PREFIXED
    )
    parser = make_parser(data)
    info = parser.parse_trace_payload(0, payload_offset_for(8))
    _, type_info = info.type_infos[0]
    assert type_info.size == Size(kind="length_prefixed", size=0)


def test_parse_trace_payload_compound_arg_with_children():
    children = [
        ("x", "float", 4, 0, []),
        ("y", "float", 4, 0, []),
    ]
    data = build_trce_record(
        fmt_string="{0}",
        args=[("vec2", 8, 0, children)],
    )
    parser = make_parser(data)
    info = parser.parse_trace_payload(0, payload_offset_for(8))
    _, type_info = info.type_infos[0]
    assert set(type_info.children.keys()) == {"x", "y"}
    assert type_info.children["x"][0] == "float"
    assert type_info.children["x"][1].size == Size(kind="fixed", size=4)
    assert type_info.children["y"][0] == "float"


def test_parse_trace_payload_nested_list_two_levels():
    # list of list of int: outer list -> inner list -> int leaf.
    args = [("list", 2, 0, [("", "list", 2, 0, [("", "int", 4, 0, [])])])]
    data = build_trce_record(fmt_string="{0}", args=args)
    parser = make_parser(data)
    info = parser.parse_trace_payload(0, payload_offset_for(8), record_end=len(data))
    _, outer = info.type_infos[0]
    assert outer.size == Size(kind="fixed", size=2)
    inner_id, inner = outer.children[""]
    assert inner_id == "list"
    assert inner.size == Size(kind="fixed", size=2)
    leaf_id, leaf = inner.children[""]
    assert leaf_id == "int"
    assert leaf.size == Size(kind="fixed", size=4)
    assert leaf.children == {}


def test_parse_trace_payload_nested_list_three_levels():
    # list of list of list of int16_t.
    args = [
        (
            "list",
            2,
            0,
            [("", "list", 2, 0, [("", "list", 2, 0, [("", "int16_t", 2, 0, [])])])],
        )
    ]
    data = build_trce_record(fmt_string="{0}", args=args)
    parser = make_parser(data)
    info = parser.parse_trace_payload(0, payload_offset_for(8), record_end=len(data))
    _, l1 = info.type_infos[0]
    l2_id, l2 = l1.children[""]
    l3_id, l3 = l2.children[""]
    leaf_id, leaf = l3.children[""]
    assert (l2_id, l3_id, leaf_id) == ("list", "list", "int16_t")
    assert leaf.size == Size(kind="fixed", size=2)
    assert leaf.children == {}


def test_parse_trace_payload_nested_arg_then_sibling():
    # A nested subtree must be fully consumed in pre-order before the next
    # top-level argument's descriptor is read.
    args = [
        ("list", 2, 0, [("", "list", 2, 0, [("", "int", 4, 0, [])])]),
        ("double", 8, 0, []),
    ]
    data = build_trce_record(
        fmt_string="{0} {1}", formatter_id=2, file="nested.c", line=7, args=args
    )
    parser = make_parser(data)
    info = parser.parse_trace_payload(0, payload_offset_for(8), record_end=len(data))
    assert len(info.type_infos) == 2
    arg0_id, arg0 = info.type_infos[0]
    assert arg0_id == "list"
    inner_id, inner = arg0.children[""]
    assert inner_id == "list"
    assert inner.children[""][0] == "int"
    arg1_id, arg1 = info.type_infos[1]
    assert arg1_id == "double"
    assert arg1.size == Size(kind="fixed", size=8)
    assert arg1.children == {}
    assert info.formatter == 2
    assert info.file == "nested.c"
    assert info.line == 7


def test_parse_trace_payload_wide_and_deep():
    # One parent with two children; the first child has two grandchildren.
    args = [
        (
            "struct",
            12,
            0,
            [
                ("i", "int", 4, 0, [("", "list", 2, 0, [("", "int", 4, 0, [])])]),
                ("d", "double", 8, 0, []),
            ],
        )
    ]
    data = build_trce_record(fmt_string="{0}", args=args)
    parser = make_parser(data)
    info = parser.parse_trace_payload(0, payload_offset_for(8), record_end=len(data))
    _, parent = info.type_infos[0]
    assert set(parent.children.keys()) == {"i", "d"}
    i_id, i = parent.children["i"]
    d_id, d = parent.children["d"]
    assert i_id == "int"
    assert d_id == "double"
    assert d.children == {}
    list_id, list_info = i.children[""]
    assert list_id == "list"
    assert list_info.children[""][0] == "int"


def test_parse_trace_payload_nested_4byte_size_t_big_endian():
    args = [("list", 2, 0, [("", "list", 2, 0, [("", "uint16_t", 2, 0, [])])])]
    data = build_trce_record(
        fmt_string="{0}",
        args=args,
        size_t_size=4,
        byteorder="big",
    )
    parser = make_parser(data)
    # The per-record context (4-byte size_t, big endian) comes from the
    # framing; the parser's global state is irrelevant.
    info, record_end = parser.parse_trace_record(0)
    assert info is not None
    _, l1 = info.type_infos[0]
    _, l2 = l1.children[""]
    assert l2.children[""][0] == "uint16_t"
    assert record_end == len(data)


def test_parse_trace_payload_overlong_consumption_rejected():
    # A num_children count that would consume more entries than the record
    # contains must be rejected instead of silently mis-parsing.
    data = bytearray(
        build_trce_record(
            fmt_string="{0}", args=[("list", 2, 0, [("", "int", 4, 0, [])])]
        )
    )
    sz = 8
    payload_offset = payload_offset_for(sz)
    # The child descriptor starts after the 2 header entries and the 4-entry
    # arg descriptor; its num_children is the 5th of its 5 size_t values.
    child_desc_start = payload_offset + 2 * sz + 4 * sz
    data[child_desc_start + 4 * sz : child_desc_start + 5 * sz] = _pack_size_t(
        1 << 40, sz, "little"
    )
    parser = make_parser(bytes(data))
    with pytest.raises(AssertionError):
        parser.parse_trace_record(0)


def test_parse_trace_payload_multiple_args():
    data = build_trce_record(
        fmt_string="{0} {1}",
        formatter_id=2,
        args=[
            ("int", 4, 0, []),
            ("double", 8, 0, []),
        ],
        file="multi.c",
        line=99,
    )
    parser = make_parser(data)
    info = parser.parse_trace_payload(0, payload_offset_for(8))
    assert len(info.type_infos) == 2
    assert info.type_infos[0][0] == "int"
    assert info.type_infos[1][0] == "double"
    assert info.formatter == 2
    assert info.line == 99


def test_parse_trace_record_valid():
    data = build_trce_record(fmt_string="test", line=7)
    parser = make_parser(data)
    info, record_end = parser.parse_trace_record(0)
    assert info is not None
    assert info.fmt_string == "test"
    assert info.line == 7
    assert info.version == (1, 0)
    assert record_end == len(data)


def test_parse_trace_record_on_mgic_returns_none():
    """parse_trace_record returns None when encountering a MGIC record."""
    data = build_mgic_record()
    parser = make_parser(data)
    info, record_end = parser.parse_trace_record(0)
    assert info is None
    assert record_end == len(data)


def test_parse_trace_record_4byte_size_t():
    data = build_trce_record(
        fmt_string="sz4",
        size_t_size=4,
        byteorder="little",
        args=[("uint32_t", 4, 0, [])],
    )
    parser = make_parser(data)
    info, record_end = parser.parse_trace_record(0)
    assert info is not None
    assert info.fmt_string == "sz4"
    assert record_end == len(data)


def test_parse_trace_record_big_endian():
    data = build_trce_record(
        fmt_string="be",
        size_t_size=8,
        byteorder="big",
        args=[("int", 4, 0, [])],
    )
    parser = make_parser(data)
    info, _ = parser.parse_trace_record(0)
    assert info is not None
    assert info.fmt_string == "be"
    assert info.type_infos[0][0] == "int"


# ─────────────────────────────────────────────────────────────────────────────
# Mixed-size sections
# ─────────────────────────────────────────────────────────────────────────────


def test_mixed_size_t_widths_in_one_section():
    """Records with different size_t widths are decoded independently."""
    r4 = build_trce_record(fmt_string="four", size_t_size=4, byteorder="little")
    r8 = build_trce_record(fmt_string="eight", size_t_size=8, byteorder="little")
    data = r4 + r8
    parser = make_parser(data)
    infos = parser.parse_all_trace_records()
    assert set(infos.keys()) == {0, len(r4)}
    assert infos[0].fmt_string == "four"
    assert infos[len(r4)].fmt_string == "eight"


def test_mixed_byteorder_in_one_section():
    """Records with different endianness are decoded independently."""
    r_le = build_trce_record(fmt_string="le", byteorder="little", size_t_size=4)
    r_be = build_trce_record(fmt_string="be", byteorder="big", size_t_size=4)
    data = r_le + r_be
    parser = make_parser(data)
    infos = parser.parse_all_trace_records()
    assert set(infos.keys()) == {0, len(r_le)}
    assert infos[0].fmt_string == "le"
    assert infos[len(r_le)].fmt_string == "be"


# ─────────────────────────────────────────────────────────────────────────────
# parse_all_trace_records
# ─────────────────────────────────────────────────────────────────────────────


def test_parse_all_trace_records_empty_data():
    parser = make_parser(b"\x00" * 32)
    assert parser.parse_all_trace_records() == {}


def test_parse_all_trace_records_single():
    data = build_trce_record(fmt_string="one", line=1)
    parser = make_parser(data)
    result = parser.parse_all_trace_records()
    assert list(result.keys()) == [0]
    assert result[0].fmt_string == "one"


def test_parse_all_trace_records_two_back_to_back():
    r1 = build_trce_record(fmt_string="first", line=1)
    r2 = build_trce_record(fmt_string="second", line=2)
    data = r1 + r2
    parser = make_parser(data)
    result = parser.parse_all_trace_records()
    assert len(result) == 2
    assert 0 in result
    assert len(r1) in result
    assert result[0].fmt_string == "first"
    assert result[len(r1)].fmt_string == "second"


def test_parse_all_trace_records_mgic_skipped():
    """MGIC records are not included in parse_all_trace_records output."""
    mgic = build_mgic_record()
    trce = build_trce_record(fmt_string="after_mgic")
    data = mgic + trce
    parser = make_parser(data)
    result = parser.parse_all_trace_records()
    assert len(result) == 1
    assert len(mgic) in result
    assert result[len(mgic)].fmt_string == "after_mgic"


def test_parse_all_trace_records_unknown_type_skipped():
    """Records with unknown type tags are skipped without aborting the walk."""
    known = build_trce_record(fmt_string="known")
    unknown = bytearray(build_trce_record(fmt_string="unknown"))
    unknown[10:14] = b"BLUB"
    data = bytes(unknown) + known
    parser = make_parser(data)
    result = parser.parse_all_trace_records()
    assert set(result.keys()) == {len(unknown)}
    assert result[len(unknown)].fmt_string == "known"


def test_parse_all_trace_records_garbage_between():
    """Garbage bytes between records are skipped; both TRCE records are found."""
    r1 = build_trce_record(fmt_string="first")
    garbage = b"\xde\xad\xbe\xef" * 4
    r2 = build_trce_record(fmt_string="second")
    data = r1 + garbage + r2
    parser = make_parser(data)
    result = parser.parse_all_trace_records()
    assert len(result) == 2
    fmt_strings = {info.fmt_string for info in result.values()}
    assert fmt_strings == {"first", "second"}


# ─────────────────────────────────────────────────────────────────────────────
# Framing size sanity
# ─────────────────────────────────────────────────────────────────────────────


def test_framing_size_is_14():
    assert _FRAMING_SIZE == 14
