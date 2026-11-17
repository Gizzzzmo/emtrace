"""Tests of the value types: field access, format specs, and pretty printing."""

from __future__ import annotations

import copy

import pytest
from emtrace.stream.formatters import c_formatter, py_formatter
from emtrace.stream.values import Char, MyDict, MyList, Placeholder, SChar, pretty


def point(x: int = 1, y: int = -2) -> MyDict:
    return MyDict({"x": x, "y": y})


def test_default_rendering_is_python_style():
    value = MyDict({"name": "a", "xs": MyList([1, 2]), "p": point()})
    expected = "{'name': 'a', 'xs': [1, 2], 'p': {'x': 1, 'y': -2}}"
    assert repr(value) == expected
    assert str(value) == expected
    assert f"{value}" == expected
    assert c_formatter("%s", [value]) == expected


def test_field_access_in_format_strings():
    value = MyDict({"p": point(), "xs": MyList([7, 8]), 2: "two", "_r": 0})
    assert py_formatter("{0.p.x} {0[p][y]}", [value]) == "1 -2"
    assert py_formatter("{0.xs[1]} {0[2]} {0._r}", [value]) == "8 two 0"
    assert py_formatter("{0.p.x:+04d}", [value]) == "+001"


def test_field_access_errors():
    value = point()
    with pytest.raises(AttributeError):
        _ = py_formatter("{0.z}", [value])
    with pytest.raises(KeyError):
        _ = py_formatter("{0[z]}", [value])
    assert not hasattr(value, "z")
    assert not hasattr(value, "__missing_dunder__")


def test_fields_named_like_methods_are_accessible():
    value = MyDict({"items": 1, "keys": 2, "values": 3, "get": 4, "copy": 5})
    assert (
        py_formatter("{0.items} {0.keys} {0.values} {0.get} {0.copy}", [value])
        == "1 2 3 4 5"
    )


def test_mapping_behaviour():
    value = point()
    assert list(value) == ["x", "y"]
    assert len(value) == 2
    assert "x" in value
    assert value == {"x": 1, "y": -2}
    assert value == point()
    assert value != point(y=3)
    assert {key: value[key] for key in value} == {"x": 1, "y": -2}
    with pytest.raises(TypeError):
        _ = hash(value)


def test_copy():
    value = MyDict({"p": point()})
    assert copy.copy(value) == value
    assert copy.deepcopy(value) == value


def test_separator_format_spec():
    assert f"{point():, *=*}" == "x=1, y=-2"
    assert f"{point():, *: *+d}" == "x: +1, y: -2"
    assert f"{point():\n*: *}" == "x: 1\ny: -2"
    value = MyDict({"xs": MyList([1, 2, 3]), "n": MyList([4])})
    assert f"{value:, *=*-*d}" == "xs=1-2-3, n=4"
    structs = MyList([point(1, 2), point(3, 4)])
    assert f"{structs:; *, *=*d}" == "x=1, y=2; x=3, y=4"


def test_invalid_dict_format_spec():
    with pytest.raises(ValueError, match="ENTRY_SEP"):
        _ = f"{point():d}"
    with pytest.raises(ValueError, match="ENTRY_SEP"):
        _ = f"{point():, *d}"


def test_list_index_access():
    value = MyList([10, 20, 30])
    assert py_formatter("{0[0]} {0[2]}", [value]) == "10 30"
    assert len(value) == 3
    assert list(value) == [10, 20, 30]
    assert value == [10, 20, 30]
    assert value == MyList([10, 20, 30])


def test_pretty_printing():
    value = MyDict(
        {
            "name": "a",
            "xs": MyList([1, 2]),
            "p": point(),
            "empty": MyDict(),
            "none": MyList([]),
            "pair": (1, "b"),
            "single": (Char(b"c"),),
        }
    )
    assert f"{value:#}" == (
        "{\n"
        "    'name': 'a',\n"
        "    'xs': [\n"
        "        1,\n"
        "        2\n"
        "    ],\n"
        "    'p': {\n"
        "        'x': 1,\n"
        "        'y': -2\n"
        "    },\n"
        "    'empty': {},\n"
        "    'none': [],\n"
        "    'pair': (\n"
        "        1,\n"
        "        'b'\n"
        "    ),\n"
        "    'single': (\n"
        "        char(0x63),\n"
        "    )\n"
        "}"
    )
    assert f"{MyList([point()]):#2}" == "[\n  {\n    'x': 1,\n    'y': -2\n  }\n]"
    assert f"{point():#0}" == "{\n'x': 1,\n'y': -2\n}"
    assert pretty(5) == "5"


def test_chars_compare_and_hash_by_value():
    assert Char(b"a") == Char(b"a")
    assert Char(b"a") != Char(b"b")
    assert Char(b"\xff") != SChar(b"\xff")
    assert len({Char(b"a"), Char(b"a"), SChar(b"a")}) == 2


def test_placeholders_ignore_specs_inside_containers():
    value = MyDict({"x": Placeholder("frob", "unknown type id", b"\x01")})
    assert f"{value:, *=*08x}" == "x=<frob: unknown type id: 01>"
    assert py_formatter("{0.x:d}", [value]) == "<frob: unknown type id: 01>"
