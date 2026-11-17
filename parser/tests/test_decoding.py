"""Unit tests for stage 2 of decoding (byte trees → values) and the formatters."""

from __future__ import annotations

import struct
from typing import Any, Literal, cast

import pytest
from emtrace.section.records import TraceRecord
from emtrace.section.type_tree import TypeNode
from emtrace.stream.decoding import (
    DecodeContext,
    TypeRegistry,
    default_registry,
)
from emtrace.stream.formatters import FORMATTERS, c_formatter, py_formatter
from emtrace.stream.framing import ByteNode, ByteReader, frame_args
from emtrace.stream.values import Char, Placeholder, SChar

from .builders import RawValue, composite, encode_value, leaf, leb128


def record(
    *args: TypeNode,
    byteorder: Literal["little", "big"] = "little",
    fmt: str = "",
    formatter: int = 0,
) -> TraceRecord:
    return TraceRecord(
        record_start=0,
        fmt_string=fmt,
        formatter=formatter,
        args=list(args),
        file="test.c",
        line=1,
        version=(3, 0),
        runtime_data_version=(3, 0),
        byteorder=byteorder,
        size_t_size=8,
    )


def decode(
    rec: TraceRecord, data: bytes, registry: TypeRegistry | None = None
) -> tuple[list[Any], list[str]]:
    event = frame_args(rec, ByteReader(data))
    assert event.error is None
    decoded = (registry or default_registry()).decode_args(event, rec)
    return decoded.values, [p.message for p in decoded.problems]


def test_integers_respect_record_byteorder():
    rec_le = record(leaf("int", 4), leaf("unsigned", 2))
    values, problems = decode(rec_le, struct.pack("<iH", -5, 0xFFFE))
    assert values == [-5, 0xFFFE]
    assert problems == []
    rec_be = record(leaf("int", 4), leaf("unsigned", 2), byteorder="big")
    values, _ = decode(rec_be, struct.pack(">iH", -5, 0xFFFE))
    assert values == [-5, 0xFFFE]


def test_floats():
    rec = record(leaf("float", 4), leaf("double", 8), leaf("float", 2))
    values, _ = decode(rec, struct.pack("<fde", 1.5, -2.25, 0.5))
    assert values == [1.5, -2.25, 0.5]
    values, _ = decode(
        record(leaf("double", 8), byteorder="big"), struct.pack(">d", 3.0)
    )
    assert values == [3.0]


def test_chars_strings_and_bools():
    rec = record(
        leaf("char", 1),
        leaf("int8_t", 1),
        leaf("string", 1, "null_terminated"),
        leaf("bool", 1),
    )
    values, _ = decode(rec, b"A\xff" + b"hi\x00" + b"\x01")
    assert format(values[0], "") == "A"
    assert format(values[1], "d") == "-1"
    assert values[2:] == ["hi", True]


def test_lists_and_multi_child_elements():
    ints = composite("list", 2, [leaf("int", 4)])
    pairs = composite("list", 1, [leaf("int", 4, name="x"), leaf("char", 1, name="y")])
    rec = record(ints, pairs)
    values, _ = decode(rec, struct.pack("<ii", 1, 2) + struct.pack("<i", 3) + b"z")
    assert format(values[0], "-*d") == "1-2"
    assert values[1].list[0][0] == 3


def test_unknown_type_id_becomes_placeholder():
    rec = record(leaf("int", 4), leaf("my_type", 3))
    values, problems = decode(rec, struct.pack("<i", 7) + b"\x01\x02\x03")
    assert values[0] == 7
    assert isinstance(values[1], Placeholder)
    assert str(values[1]) == "<my_type: unknown type id: 01 02 03>"
    assert problems == ["unknown type id"]


def test_wrong_shape_becomes_placeholder_inside_list():
    # A "float" of 3 bytes cannot be decoded; the list around it still is.
    rec = record(composite("list", 2, [leaf("float", 3)]))
    values, problems = decode(rec, bytes(6))
    assert all(isinstance(v, Placeholder) for v in values[0].list)
    assert problems == ["expected 2, 4, or 8 bytes, got 3"] * 2


def test_list_type_id_on_leaf_becomes_placeholder():
    values, problems = decode(record(leaf("list", 2)), b"ab")
    assert isinstance(values[0], Placeholder)
    assert "single value" in problems[0]


def test_invalid_utf8_string_is_decoded_with_a_warning():
    rec = record(leaf("string", 1, "null_terminated"))
    event = frame_args(rec, ByteReader(b"a\xffb\x00"))
    decoded = default_registry().decode_args(event, rec)
    assert decoded.values == ["a\ufffdb"]
    assert len(decoded.problems) == 1
    assert not decoded.has_placeholders


def test_custom_decoder_and_recursion_via_context():
    registry = default_registry()

    @registry.register("point")
    def decode_point(node: ByteNode, ctx: DecodeContext) -> str:
        assert node.elements is not None
        x, y = (
            ctx.decode(child, name)
            for child, name in zip(node.elements[0], "xy", strict=True)
        )
        return f"({x}, {y})"

    point = composite("point", 1, [leaf("int", 4, name="x"), leaf("int", 4, name="y")])
    values, problems = decode(record(point), struct.pack("<ii", 1, -1), registry)
    assert values == ["(1, -1)"]
    assert problems == []
    with pytest.raises(ValueError, match="already registered"):
        _ = registry.register("point", decode_point)
    _ = registry.register("point", decode_point, replace=True)


def test_raising_custom_decoder_becomes_placeholder():
    registry = TypeRegistry()

    def broken(node: ByteNode, ctx: DecodeContext) -> Any:
        _ = node, ctx
        raise RuntimeError("oops")

    _ = registry.register("int", broken)
    values, problems = decode(record(leaf("int", 4)), bytes(4), registry)
    assert isinstance(values[0], Placeholder)
    assert problems == ["decoder raised RuntimeError: oops"]


def test_default_registry_is_independent():
    registry = default_registry()
    registry.unregister("int")
    assert "int" not in registry
    assert "int" in default_registry()


def test_padding_bytes_are_ignored():
    node = leaf("int", 4, padding=4)
    rec = record(composite("list", 4, [node]))
    raw: RawValue = cast(RawValue, [[struct.pack("<i", 1)]] * 4)
    data = encode_value(rec.args[0], raw)
    values, _ = decode(rec, data)
    assert format(values[0], "-*d") == "1-1-1-1"


# ─────────────────────────────────────────────────────────────────────────────
# Structs and dicts
# ─────────────────────────────────────────────────────────────────────────────


def point(*extra: TypeNode, name: str = "") -> TypeNode:
    fields = [leaf("int", 4, name="x"), leaf("int", 4, name="y"), *extra]
    return composite("struct", 1, fields, name=name)


def test_struct_fields_by_name():
    rec = record(point(leaf("char", 1, name="c", padding=3)))
    values, problems = decode(rec, struct.pack("<ii", 1, -2) + b"z\xee\xee\xee")
    assert problems == []
    assert values[0] == {"x": 1, "y": -2, "c": Char(b"z")}
    assert repr(values[0]) == "{'x': 1, 'y': -2, 'c': char(0x7a)}"


def test_struct_anonymous_members_padding_fillers_and_positional_fields():
    anonymous = composite("struct", 1, [leaf("short", 2, name="a")])
    rec = record(point(anonymous, leaf("pad", 0, padding=2), leaf("double", 8)))
    data = struct.pack("<iih", 1, 2, 3) + b"\xee\xee" + struct.pack("<d", 0.5)
    values, problems = decode(rec, data)
    assert problems == []
    # The anonymous struct is merged, the padding filler skipped, and the
    # unnamed double keyed by its position.
    assert values[0] == {"x": 1, "y": 2, "a": 3, 4: 0.5}


def test_struct_duplicate_field_warns_and_keeps_last():
    rec = record(point(leaf("int", 4, name="x")))
    values, problems = decode(rec, struct.pack("<iii", 1, 2, 3))
    assert values[0] == {"x": 3, "y": 2}
    assert problems == ["duplicate key 'x'; the last value is kept"]


def test_nested_structs_and_lists_of_structs():
    line = composite("struct", 1, [point(name="start"), point(name="end")])
    polygon = composite("list", 0, [point()], "length_prefixed")
    rec = record(line, polygon)
    data = struct.pack("<4i", 1, 2, 3, 4) + leb128(2) + struct.pack("<4i", 5, 6, 7, 8)
    values, problems = decode(rec, data)
    assert problems == []
    assert values[0] == {"start": {"x": 1, "y": 2}, "end": {"x": 3, "y": 4}}
    assert values[1] == [{"x": 5, "y": 6}, {"x": 7, "y": 8}]


@pytest.mark.parametrize(
    ("node", "data", "reason"),
    [
        (leaf("struct", 4), bytes(4), "expected fields, got a single value"),
        (
            composite("struct", 2, [leaf("int", 4, name="x")]),
            bytes(8),
            "expected exactly one element, got 2",
        ),
    ],
)
def test_struct_wrong_shape_becomes_placeholder(
    node: TypeNode, data: bytes, reason: str
):
    values, problems = decode(record(node), data)
    assert isinstance(values[0], Placeholder)
    assert problems[0].startswith(reason)


def string_to_int_dict() -> TypeNode:
    children = [leaf("string", 1, "null_terminated"), leaf("int", 4)]
    return composite("dict", 0, children, "length_prefixed")


def test_dict_key_value_pairs():
    data = (
        leb128(2)
        + b"one\x00"
        + struct.pack("<i", 1)
        + b"two\x00"
        + struct.pack("<i", 2)
    )
    values, problems = decode(record(string_to_int_dict()), data)
    assert problems == []
    assert values[0] == {"one": 1, "two": 2}
    assert values[0].one == 1


def test_empty_dict():
    values, problems = decode(record(string_to_int_dict()), leb128(0))
    assert problems == []
    assert values[0] == {}
    assert repr(values[0]) == "{}"


def test_dict_duplicate_keys_warn_and_keep_last():
    data = leb128(2) + b"k\x00" + struct.pack("<i", 1) + b"k\x00" + struct.pack("<i", 2)
    values, problems = decode(record(string_to_int_dict()), data)
    assert values[0] == {"k": 2}
    assert problems == ["duplicate key 'k'; the last value is kept"]


def test_dict_keys_are_made_hashable():
    # Char keys compare by value; list keys become tuples; struct keys can't
    # be hashed and are replaced by their repr.
    chars = composite("dict", 2, [leaf("char", 1), leaf("int8_t", 1)])
    lists = composite(
        "dict", 1, [composite("list", 2, [leaf("int8_t", 1)]), leaf("int8_t", 1)]
    )
    structs = composite("dict", 1, [point(), leaf("int8_t", 1)])
    rec = record(chars, lists, structs)
    data = b"a\x01a\x02" + b"\x01\x02\x03" + struct.pack("<ii", 1, 2) + b"\x04"
    values, problems = decode(rec, data)
    assert values[0] == {Char(b"a"): SChar(b"\x02")}
    assert values[1] == {(SChar(b"\x01"), SChar(b"\x02")): SChar(b"\x03")}
    assert values[2] == {"{'x': 1, 'y': 2}": SChar(b"\x04")}
    assert problems == [
        "duplicate key char(0x61); the last value is kept",
        "key {'x': 1, 'y': 2} is not hashable; using its repr as key",
    ]


@pytest.mark.parametrize(
    ("node", "data", "reason"),
    [
        (leaf("dict", 4), bytes(4), "expected key/value elements, got a single value"),
        (
            composite("dict", 1, [leaf("int", 1), leaf("int", 1), leaf("int", 1)]),
            bytes(3),
            "expected elements of 2 children (key and value), got 3",
        ),
    ],
)
def test_dict_wrong_shape_becomes_placeholder(node: TypeNode, data: bytes, reason: str):
    values, problems = decode(record(node), data)
    assert isinstance(values[0], Placeholder)
    assert problems == [reason]


# ─────────────────────────────────────────────────────────────────────────────
# Formatters
# ─────────────────────────────────────────────────────────────────────────────


def test_py_formatter_with_placeholder_ignores_spec():
    p = Placeholder("foo", "unknown type id", b"\x01")
    assert py_formatter("{0:08x}|{1:d}", [p, 3]) == "<foo: unknown type id: 01>|3"


def test_c_formatter_rewrites_placeholder_conversions():
    p = Placeholder("foo", "unknown type id")
    assert c_formatter("%d %5.2f %%", [p, 1.0]) == "<foo: unknown type id>  1.00 %"
    # '*' width arguments of a placeholder's conversion are dropped with it.
    assert c_formatter("%*d|%s", [5, p, "x"]) == "<foo: unknown type id>|x"
    assert c_formatter("%*d|%s", [5, 1, "x"]) == "    1|x"


def test_raw_formatter_rejects_arguments():
    assert FORMATTERS[1]("text", []) == "text"
    with pytest.raises(ValueError):
        _ = FORMATTERS[1]("text", [1])
