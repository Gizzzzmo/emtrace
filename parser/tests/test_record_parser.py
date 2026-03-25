"""Unit tests for emtrace.record_parser."""

from __future__ import annotations

import struct
import pytest
from typing import Literal

from emtrace.record_parser import (
    RecordParser,
    MgicInfo,
    detect_byteorder,
    _MAGIC_CONSTANT,
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
#   MGIC / TRCE record header (6 bytes):
#     [0:3]  b"EMT"
#     [3]    type_offset  = 6   (type string starts right after header)
#     [4]    size_offset  = 10  (record_size field starts after type string)
#     [5]    payload_offset     (= 10 + size_t_size, right after record_size)
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


def build_mgic_record(
    size_t_size: int = 8,
    byteorder: Literal["little", "big"] = "little",
    ptr_size: int = 8,
    alignment_power: int = 0,
    encoding_id: int = 0,
    version: int = 0,
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
    type_offset = 6       # b"MGIC" at byte 6
    size_offset = 10      # record_size (size_t) at byte 10
    payload_offset = 10 + sz  # magic constant starts immediately after record_size

    # size_t_meta lives right after the 6 single-byte metadata fields that
    # follow the magic constant.
    # magic is at payload_offset; metadata bytes occupy [+32 .. +37]; so:
    size_t_meta_record_offset = payload_offset + 32 + 6  # = 10 + sz + 38

    total_size = size_t_meta_record_offset + 2 * sz

    version_lo = version & 0xFF
    version_hi = (version >> 8) & 0xFF

    record = bytearray(total_size)

    # Framing header.
    record[0:3] = b"EMT"
    record[3] = type_offset
    record[4] = size_offset
    record[5] = payload_offset

    # Type string.
    record[6:10] = b"MGIC"

    # Record size.
    record[10 : 10 + sz] = _pack_size_t(total_size, sz, byteorder)

    # Magic constant.
    mo = payload_offset  # magic_offset within the record
    record[mo : mo + 32] = _MAGIC_CONSTANT

    # Seven single-byte metadata fields.
    record[mo + 32] = version_lo
    record[mo + 33] = version_hi
    record[mo + 34] = size_t_meta_record_offset
    record[mo + 35] = sz
    record[mo + 36] = ptr_size
    record[mo + 37] = alignment_power

    # size_t_meta[].
    stm = size_t_meta_record_offset
    record[stm : stm + sz] = _BYTEORDER_PROBE[sz][byteorder]
    record[stm + sz : stm + 2 * sz] = _pack_size_t(encoding_id, sz, byteorder)

    return bytes(prefix) + bytes(record)


def build_trce_record(
    fmt_string: str = "hello",
    formatter_id: int = 0,
    file: str = "test.c",
    line: int = 42,
    args: list[tuple[str, int, int, list[tuple[str, int, int, str]]]] | None = None,
    size_t_size: int = 8,
    byteorder: Literal["little", "big"] = "little",
    prefix: bytes = b"",
) -> bytes:
    """Build a minimal, spec-correct TRCE record.

    *args* is a list of ``(type_id, size, flag, children)`` tuples where each
    child is ``(child_name, child_size, child_flag, child_type_id)``.

    Returns the complete byte sequence.  The record itself always starts at
    ``len(prefix)``.
    """
    if args is None:
        args = []

    sz = size_t_size

    def pack(v: int) -> bytes:
        return _pack_size_t(v, sz, byteorder)

    # ── Step 1: build the layout array (size_t values only) ──────────────────
    # We need to know string offsets, but those depend on the layout size.
    # So first count layout entries, then compute string section, then fill.

    # Header: num_args, fmt_offset  (2 entries)
    # Per arg: type_id_offset, size, flag, num_children  (4 entries)
    # Per child of arg: name_offset, child_size, child_flag, type_id_offset (4 entries)
    # Trailer: formatter_id, file_offset, line  (3 entries)

    num_layout_entries = 2  # num_args, fmt_offset
    for _, _, _, children in args:
        num_layout_entries += 4  # arg descriptor
        num_layout_entries += 4 * len(children)  # child descriptors
    num_layout_entries += 3  # formatter_id, file_offset, line

    # Header offsets (relative to record start).
    type_offset = 6
    size_offset = 10
    payload_offset = 10 + sz  # layout array starts here

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

    # Collect (type_id_offset, children_offsets) per arg in order.
    arg_type_id_offsets: list[int] = []
    arg_children_offsets: list[list[tuple[int, int]]] = []  # [(name_off, type_id_off), ...]

    for type_id, _, _, children in args:
        arg_type_id_offsets.append(add_string(type_id))
        child_offsets: list[tuple[int, int]] = []
        for child_name, _, _, child_type_id in children:
            name_off = add_string(child_name)
            tid_off = add_string(child_type_id)
            child_offsets.append((name_off, tid_off))
        arg_children_offsets.append(child_offsets)

    file_offset = add_string(file)

    # ── Step 3: assemble layout array ─────────────────────────────────────────
    layout: bytearray = bytearray()

    def append_size_t(v: int) -> None:
        layout.extend(pack(v))

    append_size_t(len(args))   # num_args
    append_size_t(fmt_offset)  # fmt_offset

    for i, (_, arg_size, arg_flag, children) in enumerate(args):
        append_size_t(arg_type_id_offsets[i])  # type_id_offset
        append_size_t(arg_size)                 # size
        append_size_t(arg_flag)                 # flag
        append_size_t(len(children))            # num_children
        for j, (_, child_size, child_flag, _) in enumerate(children):
            name_off, tid_off = arg_children_offsets[i][j]
            append_size_t(name_off)    # name_offset
            append_size_t(child_size)  # child_size
            append_size_t(child_flag)  # child_flag
            append_size_t(tid_off)     # type_id_offset

    append_size_t(formatter_id)  # formatter_id
    append_size_t(file_offset)   # file_offset
    append_size_t(line)          # line

    assert len(layout) == layout_bytes

    # ── Step 4: assemble the full record ──────────────────────────────────────
    total_size = payload_offset + len(layout) + len(strings)
    record = bytearray(total_size)

    record[0:3] = b"EMT"
    record[3] = type_offset
    record[4] = size_offset
    record[5] = payload_offset

    record[6:10] = b"TRCE"
    record[10 : 10 + sz] = pack(total_size)
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
        version=0x0005,
    )
    # Record always starts at offset 0 when no prefix.
    record_start = 0
    payload_offset = 10 + size_t_size  # per our builder layout

    parser = make_parser(data, size_t_size=size_t_size, byteorder=byteorder)
    mgic = parser.parse_mgic_payload(record_start, payload_offset)

    assert mgic.record_start == record_start
    assert mgic.magic_offset == payload_offset
    assert mgic.version == 0x0005
    assert mgic.size_t_size == size_t_size
    assert mgic.ptr_size == 4
    assert mgic.alignment_power == 2
    assert mgic.byteorder == byteorder
    assert mgic.encoding_id == 1


def test_parse_mgic_payload_version_encoding():
    """Version is stored little-endian in two bytes regardless of data byteorder."""
    data = build_mgic_record(version=0x0102)
    record_start = 0
    payload_offset = 10 + 8
    parser = make_parser(data)
    mgic = parser.parse_mgic_payload(record_start, payload_offset)
    assert mgic.version == 0x0102


def test_parse_mgic_payload_truncated():
    """AssertionError when data is cut short before metadata."""
    data = build_mgic_record()
    # Truncate to just the magic constant — cuts off metadata bytes.
    truncated = data[: 10 + 8 + 32]
    parser = make_parser(truncated)
    with pytest.raises(AssertionError):
        parser.parse_mgic_payload(0, 10 + 8)


def test_parse_mgic_payload_bad_magic():
    """AssertionError when magic constant bytes are wrong."""
    data = bytearray(build_mgic_record())
    payload_offset = 10 + 8
    data[payload_offset] ^= 0xFF  # corrupt first byte of magic
    parser = make_parser(bytes(data))
    with pytest.raises(AssertionError):
        parser.parse_mgic_payload(0, payload_offset)


def test_parse_mgic_payload_size_t_size_zero():
    """AssertionError when size_t_size byte is 0."""
    data = bytearray(build_mgic_record())
    payload_offset = 10 + 8
    data[payload_offset + 35] = 0  # size_t_size byte
    parser = make_parser(bytes(data))
    with pytest.raises(AssertionError):
        parser.parse_mgic_payload(0, payload_offset)


def test_parse_mgic_payload_bad_byteorder_probe():
    """AssertionError when the byteorder probe is unrecognisable."""
    data = bytearray(build_mgic_record())
    # Overwrite the byteorder probe with an all-zeros value (ambiguous for sz>1).
    sz = 8
    payload_offset = 10 + sz
    size_t_meta_record_offset = payload_offset + 32 + 6
    # All-zeros looks like a valid little-endian probe for sz=1 but not sz=8.
    # Use a value that has a duplicate byte to force None from detect_byteorder.
    data[size_t_meta_record_offset : size_t_meta_record_offset + sz] = b"\x00" * sz
    parser = make_parser(bytes(data))
    with pytest.raises(AssertionError):
        parser.parse_mgic_payload(0, payload_offset)


# ─────────────────────────────────────────────────────────────────────────────
# parse_record_header
# ─────────────────────────────────────────────────────────────────────────────


def test_parse_record_header_trce():
    data = build_trce_record()
    parser = make_parser(data)
    rec_type, payload_start, record_end = parser.parse_record_header(0)
    assert rec_type == b"TRCE"
    assert payload_start == 10 + 8  # payload_offset per builder
    assert record_end == len(data)


def test_parse_record_header_mgic():
    data = build_mgic_record()
    parser = make_parser(data)
    rec_type, payload_start, record_end = parser.parse_record_header(0)
    assert rec_type == b"MGIC"
    assert payload_start == 10 + 8
    assert record_end == len(data)


def test_parse_record_header_unknown_type():
    # Build a TRCE record and overwrite the type string with b"UNKN".
    data = bytearray(build_trce_record())
    data[6:10] = b"UNKN"
    parser = make_parser(bytes(data))
    rec_type, payload_start, record_end = parser.parse_record_header(0)
    assert rec_type is None
    assert record_end == len(data)


def test_parse_record_header_no_emt_prefix():
    data = b"\x00" * 20
    parser = make_parser(data)
    rec_type, next_off, _ = parser.parse_record_header(0)
    assert rec_type is None
    assert next_off == 1  # advances by one byte


def test_parse_record_header_truncated_before_offsets():
    # Only 4 bytes — has b"EMT" prefix but not the full 6-byte header.
    data = b"EMT\x06"
    parser = make_parser(data)
    rec_type, next_off, _ = parser.parse_record_header(0)
    assert rec_type is None
    assert next_off == len(data)


def test_parse_record_header_record_size_overflow():
    # Build a valid record then make record_size absurdly large.
    data = bytearray(build_trce_record())
    sz = 8
    # record_size is at offset 10.
    data[10 : 10 + sz] = (9999).to_bytes(sz, "little")
    parser = make_parser(bytes(data))
    rec_type, _, next_off = parser.parse_record_header(0)
    assert rec_type is None
    assert next_off == len(data)


# ─────────────────────────────────────────────────────────────────────────────
# parse_mgic_record
# ─────────────────────────────────────────────────────────────────────────────


def test_parse_mgic_record_valid():
    data = build_mgic_record(encoding_id=2, version=3)
    parser = make_parser(data)
    mgic, record_end = parser.parse_mgic_record(0)
    assert mgic is not None
    assert mgic.encoding_id == 2
    assert mgic.version == 3
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
    mgic, next_off = parser.parse_mgic_record(0)
    assert mgic is None
    assert next_off == 1


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
        version=7,
    )
    parser = RecordParser(memoryview(data))
    mgic = parser.find_and_parse_mgic()
    assert mgic is not None
    assert mgic.size_t_size == size_t_size
    assert mgic.byteorder == byteorder
    assert mgic.ptr_size == 4
    assert mgic.alignment_power == 1
    assert mgic.encoding_id == 2
    assert mgic.version == 7


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
    data = b"\xAA" * 10 + _MAGIC_CONSTANT + b"\x00" * 20
    parser = RecordParser(memoryview(data))
    assert parser.find_and_parse_mgic() is None


def test_find_and_parse_mgic_with_leading_garbage():
    """find_and_parse_mgic works when garbage bytes precede the MGIC record."""
    prefix = b"\xDE\xAD\xBE\xEF" * 8  # 32 bytes of garbage
    data = build_mgic_record(size_t_size=8, byteorder="little", prefix=prefix)
    parser = RecordParser(memoryview(data))
    mgic = parser.find_and_parse_mgic()
    assert mgic is not None
    assert mgic.record_start == len(prefix)
    assert mgic.magic_offset == len(prefix) + 10 + 8


# ─────────────────────────────────────────────────────────────────────────────
# parse_trace_payload / parse_trace_record
# ─────────────────────────────────────────────────────────────────────────────


def test_parse_trace_payload_no_args():
    data = build_trce_record(fmt_string="hello world", formatter_id=0, file="src/a.c", line=10)
    parser = make_parser(data)
    info = parser.parse_trace_payload(0, 10 + 8)
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
    info = parser.parse_trace_payload(0, 10 + 8)
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
    info = parser.parse_trace_payload(0, 10 + 8)
    _, type_info = info.type_infos[0]
    assert type_info.size == Size(kind="null_terminated", size=0)


def test_parse_trace_payload_length_prefixed_arg():
    data = build_trce_record(
        fmt_string="{0}",
        args=[("uint8[]", 0, 2, [])],  # flag=2 => EMT_FLAG_LENGTH_PREFIXED
    )
    parser = make_parser(data)
    info = parser.parse_trace_payload(0, 10 + 8)
    _, type_info = info.type_infos[0]
    assert type_info.size == Size(kind="length_prefixed", size=0)


def test_parse_trace_payload_compound_arg_with_children():
    children = [
        ("x", 4, 0, "float"),
        ("y", 4, 0, "float"),
    ]
    data = build_trce_record(
        fmt_string="{0}",
        args=[("vec2", 8, 0, children)],
    )
    parser = make_parser(data)
    info = parser.parse_trace_payload(0, 10 + 8)
    _, type_info = info.type_infos[0]
    assert set(type_info.children.keys()) == {"x", "y"}
    assert type_info.children["x"][0] == "float"
    assert type_info.children["x"][1].size == Size(kind="fixed", size=4)
    assert type_info.children["y"][0] == "float"


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
    info = parser.parse_trace_payload(0, 10 + 8)
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
    parser = make_parser(data, size_t_size=4, byteorder="little")
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
    parser = make_parser(data, size_t_size=8, byteorder="big")
    info, _ = parser.parse_trace_record(0)
    assert info is not None
    assert info.fmt_string == "be"
    assert info.type_infos[0][0] == "int"


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


def test_parse_all_trace_records_garbage_between():
    """Garbage bytes between records are skipped; both TRCE records are found."""
    r1 = build_trce_record(fmt_string="first")
    garbage = b"\xDE\xAD\xBE\xEF" * 4
    r2 = build_trce_record(fmt_string="second")
    data = r1 + garbage + r2
    parser = make_parser(data)
    result = parser.parse_all_trace_records()
    assert len(result) == 2
    fmt_strings = {info.fmt_string for info in result.values()}
    assert fmt_strings == {"first", "second"}
