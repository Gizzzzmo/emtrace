"""Builders for spec-conformant section bytes and runtime streams (test helpers).

All record builders produce valid, self-contained records as described in
TRACE_FORMAT.md. The runtime stream encoder is the inverse of stage 1 of
decoding (:mod:`emtrace.stream.framing`).
"""

from __future__ import annotations

from collections.abc import Callable, Sequence
from dataclasses import dataclass, field
from typing import Literal, cast

from emtrace.section.records import FRAMING_SIZE, MAGIC_CONSTANT
from emtrace.section.type_tree import (
    FLAG_LENGTH_PREFIXED,
    FLAG_NULL_TERMINATED,
    FLAG_STATIC,
    Size,
    SizeKind,
    TypeNode,
)

Byteorder = Literal["little", "big"]

# ─────────────────────────────────────────────────────────────────────────────
# Records
# ─────────────────────────────────────────────────────────────────────────────


def probe(size_t_size: int, byteorder: Byteorder) -> bytes:
    """The endianness probe for a *size_t_size*-byte size_t."""
    b = bytes(range(size_t_size))
    return b if byteorder == "little" else b[::-1]


def _pack(value: int, sz: int, byteorder: Byteorder) -> bytes:
    return value.to_bytes(sz, byteorder=byteorder)


@dataclass
class Layout:
    """Where the pointed-to pieces of a record go (after the 14-byte framing).

    The pieces are placed in *order*, each followed by the number of gap
    bytes given in *gaps*. The type tag is always part of the framing.
    """

    order: tuple[str, str, str] = ("size", "probe", "payload")
    gaps: tuple[int, int, int] = (0, 0, 0)
    gap_byte: int = 0


def _place(sz: int, payload_len: int, layout: Layout) -> tuple[dict[str, int], int]:
    """Offsets of the pieces (relative to the record start) and the record size."""
    lengths = {"size": sz, "probe": sz, "payload": payload_len}
    offsets: dict[str, int] = {}
    pos = FRAMING_SIZE
    for piece, gap in zip(layout.order, layout.gaps, strict=True):
        offsets[piece] = pos
        pos += lengths[piece] + gap
    for piece in ("size", "probe", "payload"):
        if offsets[piece] > 255:
            raise ValueError(f"{piece} offset {offsets[piece]} doesn't fit in a byte")
    return offsets, pos


def _framing(
    record_type: bytes,
    offsets: dict[str, int],
    size_t_size: int,
    version: tuple[int, int],
) -> bytes:
    return (
        b"EMT"
        + bytes(
            [
                10,
                offsets["size"],
                offsets["payload"],
                offsets["probe"],
                size_t_size,
                version[0],
                version[1],
            ]
        )
        + record_type
    )


def _build(
    record_type: bytes,
    make_payload: Callable[[int], bytes],
    payload_len: int,
    size_t_size: int,
    byteorder: Byteorder,
    version: tuple[int, int],
    layout: Layout,
) -> bytes:
    offsets, total = _place(size_t_size, payload_len, layout)
    record = bytearray([layout.gap_byte]) * total
    record[0:FRAMING_SIZE] = _framing(record_type, offsets, size_t_size, version)
    record[offsets["size"] : offsets["size"] + size_t_size] = _pack(
        total, size_t_size, byteorder
    )
    record[offsets["probe"] : offsets["probe"] + size_t_size] = probe(
        size_t_size, byteorder
    )
    payload = make_payload(offsets["payload"])
    assert len(payload) == payload_len
    record[offsets["payload"] : offsets["payload"] + payload_len] = payload
    return bytes(record)


def build_mgic_record(
    size_t_size: int = 8,
    byteorder: Byteorder = "little",
    ptr_size: int = 8,
    alignment_power: int = 0,
    encoding_id: int = 0,
    magic_version: tuple[int, int] = (1, 0),
    prefix: bytes = b"",
    layout: Layout | None = None,
) -> bytes:
    """Build a spec-correct MGIC record, prefixed with *prefix*."""
    payload = MAGIC_CONSTANT + bytes([ptr_size, alignment_power, encoding_id])
    record = _build(
        b"MGIC",
        lambda _offset: payload,
        len(payload),
        size_t_size,
        byteorder,
        magic_version,
        layout or Layout(),
    )
    return bytes(prefix) + record


# Old-style child descriptor: (name, type_id, size, flag, grandchildren).
Child = tuple[str, str, int, int, list["Child"]]
# Old-style argument descriptor: (type_id, size, flag, children).
Arg = tuple[str, int, int, list[Child]]


@dataclass
class Entry:
    """A type entry with a raw flag value (which may be invalid)."""

    type_id: str
    size: int
    flag: int
    padding: int = 0
    name: str = ""
    children: list[Entry] = field(default_factory=list)


_KIND_FLAGS = {
    "fixed": FLAG_STATIC,
    "null_terminated": FLAG_NULL_TERMINATED,
    "length_prefixed": FLAG_LENGTH_PREFIXED,
}


def entry_from(arg: TypeNode | Arg | Child | Entry, named: bool = False) -> Entry:
    """Convert a TypeNode or an old-style tuple to an :class:`Entry`."""
    if isinstance(arg, Entry):
        return arg
    if isinstance(arg, TypeNode):
        return Entry(
            arg.type_id,
            arg.size.size,
            _KIND_FLAGS[arg.size.kind],
            arg.padding,
            arg.name,
            [entry_from(c, True) for c in arg.children],
        )
    if named:
        name, type_id, size, flag, children = cast(Child, arg)
        return Entry(
            type_id, size, flag, 0, name, [entry_from(c, True) for c in children]
        )
    type_id, size, flag, children = cast(Arg, arg)
    return Entry(type_id, size, flag, 0, "", [entry_from(c, True) for c in children])


def build_trce_record(
    fmt_string: str = "hello",
    formatter_id: int = 0,
    file: str = "test.c",
    line: int = 42,
    args: Sequence[TypeNode | Arg | Entry] | None = None,
    size_t_size: int = 8,
    byteorder: Byteorder = "little",
    trace_version: tuple[int, int] = (3, 0),
    runtime_version: tuple[int, int] | None = None,
    prefix: bytes = b"",
    layout: Layout | None = None,
    string_gap: int = 0,
) -> bytes:
    """Build a spec-correct TRCE record, prefixed with *prefix*.

    *args* are TypeNodes or old-style ``(type_id, size, flag, children)``
    tuples, whose children are ``(name, type_id, size, flag, grandchildren)``.
    The runtime data version defaults to the record version (for records of
    version 2.0 and newer). *string_gap* bytes are inserted between the
    layout table and the strings.
    """
    entries = [entry_from(a) for a in (args or [])]
    sz = size_t_size
    has_runtime_version = trace_version[0] >= 2
    has_padding = trace_version[0] >= 3
    if runtime_version is None:
        runtime_version = trace_version if has_runtime_version else (1, 0)

    per_entry = 4 + int(has_padding)

    def count(e: Entry, named: bool) -> int:
        return per_entry + int(named) + sum(count(c, True) for c in e.children)

    num_values = 2 * int(has_runtime_version) + 2 + 3
    num_values += sum(count(e, False) for e in entries)
    layout_len = num_values * sz

    strings_list: list[str] = [fmt_string]

    def collect(e: Entry, named: bool) -> None:
        if named:
            strings_list.append(e.name)
        strings_list.append(e.type_id)
        for c in e.children:
            collect(c, True)

    for e in entries:
        collect(e, False)
    strings_list.append(file)
    encoded = [s.encode("utf-8") + b"\x00" for s in strings_list]
    payload_len = layout_len + string_gap + sum(len(s) for s in encoded)

    def make_payload(payload_offset: int) -> bytes:
        string_offsets: list[int] = []
        pos = payload_offset + layout_len + string_gap
        for s in encoded:
            string_offsets.append(pos)
            pos += len(s)
        strings = iter(string_offsets)
        out = bytearray()

        def put(v: int) -> None:
            out.extend(_pack(v, sz, byteorder))

        if has_runtime_version:
            put(runtime_version[0])
            put(runtime_version[1])
        put(len(entries))
        put(next(strings))  # format string

        def emit(e: Entry, named: bool) -> None:
            if named:
                put(next(strings))
            put(next(strings))
            put(e.size)
            put(e.flag)
            if has_padding:
                put(e.padding)
            put(len(e.children))
            for c in e.children:
                emit(c, True)

        for e in entries:
            emit(e, False)
        put(formatter_id)
        put(next(strings))  # file
        put(line)
        assert len(out) == layout_len
        out.extend(bytes(string_gap))
        for s in encoded:
            out.extend(s)
        return bytes(out)

    record = _build(
        b"TRCE",
        make_payload,
        payload_len,
        sz,
        byteorder,
        trace_version,
        layout or Layout(),
    )
    return bytes(prefix) + record


def build_section(records: Sequence[bytes], alignment_power: int = 0) -> bytes:
    """Concatenate *records*, padding so that each starts aligned."""
    step = 2**alignment_power
    out = bytearray()
    for record in records:
        out.extend(bytes((-len(out)) % step))
        out.extend(record)
    return bytes(out)


def record_offsets(records: Sequence[bytes], alignment_power: int = 0) -> list[int]:
    """The start offsets of *records* in :func:`build_section`'s output."""
    step = 2**alignment_power
    offsets: list[int] = []
    pos = 0
    for record in records:
        pos += (-pos) % step
        offsets.append(pos)
        pos += len(record)
    return offsets


# ─────────────────────────────────────────────────────────────────────────────
# Runtime streams
# ─────────────────────────────────────────────────────────────────────────────

#: A raw value: the content bytes of a leaf, or the elements of a composite
#: (each element holding one raw value per child).
RawValue = bytes | list[list["RawValue"]]


class AmbiguousValue(ValueError):
    """The value can't be encoded unambiguously (it contains its terminator)."""


def leb128(value: int) -> bytes:
    out = bytearray()
    while True:
        byte = value & 0x7F
        value >>= 7
        if value:
            out.append(byte | 0x80)
        else:
            out.append(byte)
            return bytes(out)


def encode_value(
    node: TypeNode,
    value: RawValue,
    byteorder: Byteorder = "little",
    runtime_version: tuple[int, int] = (3, 0),
    padding_byte: int = 0xEE,
) -> bytes:
    """Encode *value* as the runtime bytes of a value of type *node*."""
    kind, size = node.size.kind, node.size.size

    def prefix(count: int) -> bytes:
        if runtime_version < (2, 0):
            return count.to_bytes(size, byteorder)
        return leb128(count)

    if node.is_leaf:
        assert isinstance(value, bytes)
        if kind == "fixed":
            assert len(value) == size, (len(value), size)
            out = value
        elif kind == "length_prefixed":
            out = prefix(len(value)) + value
        else:
            assert len(value) % size == 0
            for i in range(0, len(value), size):
                if not any(value[i : i + size]):
                    raise AmbiguousValue("leaf contains an aligned terminator")
            out = value + bytes(size)
    else:
        assert isinstance(value, list)
        elements = [
            b"".join(
                encode_value(child, v, byteorder, runtime_version, padding_byte)
                for child, v in zip(node.children, element, strict=True)
            )
            for element in value
        ]
        content = b"".join(elements)
        if kind == "fixed":
            assert len(value) == size
            out = content
        elif kind == "length_prefixed":
            out = prefix(len(value)) + content
        else:
            # The terminator is only looked for at element boundaries.
            pos = 0
            for element in elements:
                # A partial window at the end is followed by the terminator's zeros.
                if not any(content[pos : pos + size]):
                    raise AmbiguousValue("element starts with the terminator")
                pos += len(element)
            out = content + bytes(size)
    return out + bytes([padding_byte]) * node.padding


def encode_pointer(
    section_offset: int, ptr_size: int, alignment_power: int = 0
) -> bytes:
    """The little-endian record pointer of an event (with an ASLR offset of 0)."""
    assert section_offset % 2**alignment_power == 0
    return (section_offset >> alignment_power).to_bytes(ptr_size, "little")


def cobs_encode(data: bytes) -> bytes:
    """COBS-encode *data* into one frame, including the trailing zero byte."""
    out = bytearray()
    block = bytearray()
    for byte in data:
        if byte == 0:
            out.append(len(block) + 1)
            out.extend(block)
            block.clear()
            continue
        block.append(byte)
        if len(block) == 254:
            out.append(0xFF)
            out.extend(block)
            block.clear()
    out.append(len(block) + 1)
    out.extend(block)
    out.append(0)
    return bytes(out)


def leaf(
    type_id: str,
    size: int,
    kind: SizeKind = "fixed",
    padding: int = 0,
    name: str = "",
) -> TypeNode:
    return TypeNode(type_id, Size(kind, size), padding, name)


def composite(
    type_id: str,
    size: int,
    children: list[TypeNode],
    kind: SizeKind = "fixed",
    padding: int = 0,
    name: str = "",
) -> TypeNode:
    return TypeNode(type_id, Size(kind, size), padding, name, children)
