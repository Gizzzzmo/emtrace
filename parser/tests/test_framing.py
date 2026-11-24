"""Unit tests for stage 1 of decoding: framing event bytes into byte trees."""

from __future__ import annotations

import struct
from typing import Literal

import pytest
from emtrace.section.records import TraceRecord
from emtrace.section.type_tree import TypeNode
from emtrace.stream.framing import (
    ByteNode,
    ByteReader,
    FramingError,
    dump_byte_tree,
    frame_args,
    frame_value,
)

from .builders import composite, encode_value, leaf, leb128


def frame(
    node: TypeNode,
    data: bytes,
    version: tuple[int, int] = (3, 0),
    byteorder: Literal["little", "big"] = "little",
) -> ByteNode:
    return frame_value(node, ByteReader(data), version, byteorder)


def record(*args: TypeNode, version: tuple[int, int] = (3, 0)) -> TraceRecord:
    return TraceRecord(
        record_start=0,
        fmt_string="",
        formatter=0,
        args=list(args),
        file="test.c",
        line=1,
        version=(3, 0),
        runtime_data_version=version,
        byteorder="little",
        size_t_size=8,
    )


def leaves(node: ByteNode) -> list[bytes]:
    """The data of all leaves, in order."""
    if node.data is not None:
        return [node.data]
    assert node.elements is not None
    return [d for element in node.elements for child in element for d in leaves(child)]


# ─────────────────────────────────────────────────────────────────────────────
# Leaves
# ─────────────────────────────────────────────────────────────────────────────


def test_fixed_leaf():
    node = frame(leaf("int", 4), struct.pack("<i", -2))
    assert node.data == struct.pack("<i", -2)
    assert (node.start, node.content_start, node.content_end, node.end) == (0, 0, 4, 4)
    assert node.is_leaf


def test_length_prefixed_leaf_leb128():
    content = b"x" * 300
    data = leb128(300) + content
    node = frame(leaf("string", 0, "length_prefixed"), data)
    assert node.data == content
    assert node.content_start == 2
    assert node.end == len(data)


def test_length_prefixed_leaf_v1_prefix_uses_size_as_width():
    data = struct.pack(">H", 3) + b"abc"
    node = frame(leaf("string", 2, "length_prefixed"), data, (1, 0), "big")
    assert node.data == b"abc"


def test_null_terminated_string():
    node = frame(leaf("string", 1, "null_terminated"), b"hello\x00rest")
    assert node.data == b"hello"
    assert node.end == 6


def test_null_terminated_leaf_terminator_is_aligned():
    # A two-byte terminator is only recognized at even offsets: the zeros at
    # offsets 1..2 are content.
    data = b"a\x00\x00b\x00\x00"
    node = frame(leaf("u16string", 2, "null_terminated"), data)
    assert node.data == b"a\x00\x00b"
    assert node.end == 6


def test_null_terminated_leaf_without_terminator():
    with pytest.raises(FramingError, match="null terminator"):
        _ = frame(leaf("string", 1, "null_terminated"), b"abc")


def test_trailing_padding_is_consumed_after_the_content():
    data = struct.pack("<i", 42) + b"\xee\xee\xee\xee" + struct.pack("<i", 7)
    reader = ByteReader(data)
    first = frame_value(leaf("int", 4, padding=4), reader, (3, 0), "little")
    second = frame_value(leaf("int", 4), reader, (3, 0), "little")
    assert first.data == struct.pack("<i", 42)
    assert (first.content_end, first.end) == (4, 8)
    assert second.data == struct.pack("<i", 7)


def test_padding_follows_terminator_and_prefix_precedes_content():
    node = frame(leaf("string", 1, "null_terminated", padding=2), b"ab\x00\xee\xee")
    assert node.data == b"ab"
    assert (node.content_end, node.end) == (2, 5)
    node = frame(leaf("string", 0, "length_prefixed", padding=1), b"\x02ab\xee")
    assert (node.start, node.content_start, node.end) == (0, 1, 4)


# ─────────────────────────────────────────────────────────────────────────────
# Composites
# ─────────────────────────────────────────────────────────────────────────────


def nested_static_list(depth: int, leaf_size: int = 4) -> TypeNode:
    """A static list tree of the given depth whose leaves are fixed-size ints."""
    node = leaf("int", leaf_size)
    for _ in range(depth):
        node = composite("list", 2, [node])
    return node


def test_nested_static_lists():
    node = frame(nested_static_list(3), struct.pack("<8i", *range(8)))
    assert [int.from_bytes(d, "little") for d in leaves(node)] == list(range(8))
    assert node.elements is not None and len(node.elements) == 2


def test_length_prefixed_outer_and_inner_lists():
    inner = composite("list", 0, [leaf("int", 4)], "length_prefixed")
    outer = composite("list", 2, [inner])
    data = leb128(2) + struct.pack("<2i", 1, 2) + leb128(1) + struct.pack("<i", 3)
    node = frame(outer, data)
    assert [int.from_bytes(d, "little") for d in leaves(node)] == [1, 2, 3]


def test_multiple_children_form_one_element():
    # A struct-like composite: one element consisting of all children in order,
    # with the struct's tail padding carried by its last member.
    point = composite(
        "struct",
        1,
        [
            leaf("char", 1, name="a", padding=3),
            leaf("int", 4, name="b"),
            leaf("char", 1, name="c", padding=3),
        ],
    )
    data = b"A\xee\xee\xee" + struct.pack("<i", 5) + b"C\xee\xee\xee"
    node = frame(point, data)
    assert node.elements is not None
    assert len(node.elements) == 1
    assert [child.data for child in node.elements[0]] == [
        b"A",
        struct.pack("<i", 5),
        b"C",
    ]
    assert node.end == 12


def test_array_of_structs():
    pair = composite(
        "list", 2, [leaf("short", 2, name="x"), leaf("char", 1, name="y", padding=1)]
    )
    data = b"\x01\x00a\xee\x02\x00b\xee"
    node = frame(pair, data)
    assert node.elements is not None
    assert [[c.data for c in e] for e in node.elements] == [
        [b"\x01\x00", b"a"],
        [b"\x02\x00", b"b"],
    ]


def test_null_terminated_list_terminator_at_element_boundary():
    # A list of 8-byte pointers terminated by a null pointer: the zero bytes
    # inside the first element must not be taken for the terminator.
    pointers = composite("list", 8, [leaf("*", 8)], "null_terminated")
    data = struct.pack("<QQ", 0x100, 0x200) + bytes(8)
    node = frame(pointers, data)
    assert [int.from_bytes(d, "little") for d in leaves(node)] == [0x100, 0x200]
    assert node.end == 24


def test_empty_null_terminated_list():
    node = frame(composite("list", 1, [leaf("char", 1)], "null_terminated"), b"\x00")
    assert node.elements == []


def test_zero_size_element_repetition_is_rejected():
    zero = composite("list", 0, [leaf("empty", 0)], "length_prefixed")
    with pytest.raises(FramingError, match="0 bytes"):
        _ = frame(zero, leb128(1 << 60))


def test_count_beyond_remaining_bytes_is_rejected_early():
    ints = composite("list", 0, [leaf("int", 4)], "length_prefixed")
    with pytest.raises(FramingError, match="don't fit"):
        _ = frame(ints, leb128(1000) + bytes(8))


def test_overlong_leb128_prefix():
    with pytest.raises(FramingError, match="LEB128"):
        _ = frame(leaf("string", 0, "length_prefixed"), b"\xff" * 30)


def test_truncated_value_reports_offset_and_path():
    rec = record(leaf("int", 4), composite("list", 3, [leaf("int", 4)]))
    # In a frame the element count is checked against the remaining bytes up front.
    event = frame_args(rec, ByteReader(bytes(4 + 8)))
    assert event.error is not None
    assert len(event.args) == 1
    assert event.error.path == ("arg 1",)
    assert event.error.offset == 4
    # In a stream it is only noticed when the stream ends.
    data = iter([bytes(4 + 8), b""])
    event = frame_args(rec, ByteReader(lambda _n: next(data)))
    assert event.error is not None
    assert event.error.path == ("arg 1", "[2]")
    assert event.error.offset == 12


# ─────────────────────────────────────────────────────────────────────────────
# Readers
# ─────────────────────────────────────────────────────────────────────────────


def test_stream_reader_only_reads_what_is_needed():
    data = struct.pack("<ii", 1, 2)
    pos = 0

    def read(n: int) -> bytes:
        nonlocal pos
        chunk = data[pos : pos + n]
        pos += len(chunk)
        return chunk

    reader = ByteReader(read)
    node = frame_value(leaf("int", 4), reader, (3, 0), "little")
    assert node.data == struct.pack("<i", 1)
    assert pos == 4
    assert reader.remaining is None


def test_stream_reader_null_terminator():
    data = b"abc\x00next"
    pos = 0

    def read(n: int) -> bytes:
        nonlocal pos
        chunk = data[pos : pos + n]
        pos += len(chunk)
        return chunk

    node = frame_value(
        leaf("string", 1, "null_terminated"), ByteReader(read), (3, 0), "little"
    )
    assert node.data == b"abc"
    assert pos == 4


def test_huge_fixed_size_on_a_stream_fails_at_end_of_stream():
    reads: list[int] = []

    def read(n: int) -> bytes:
        reads.append(n)
        return b"\x01" * min(n, 10) if len(reads) == 1 else b""

    with pytest.raises(FramingError, match="ends after 10"):
        _ = frame_value(leaf("blob", 1 << 60), ByteReader(read), (3, 0), "little")
    assert max(reads) <= 1 << 16


def test_frame_args_and_dump():
    rec = record(leaf("int", 4), composite("list", 2, [leaf("int", 4)]))
    data = encode_value(rec.args[0], b"\x01\x00\x00\x00") + encode_value(
        rec.args[1], [[b"\x02\x00\x00\x00"], [b"\x03\x00\x00\x00"]]
    )
    event = frame_args(rec, ByteReader(data))
    assert event.error is None
    assert event.data == data
    dump = "\n".join(dump_byte_tree(event))
    assert "arg 0: 'int'" in dump
    assert "  [1]: 'int'" in dump
