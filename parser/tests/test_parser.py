"""Unit tests for emtrace.parser (runtime stream decoding) with nested types."""

from __future__ import annotations

import struct
from io import BytesIO

from emtrace.format_info import FmtInfo, Size, TypeInfo
from emtrace.formatters import FORMATTERS
from emtrace.parser import Parser, translation_le


def make_parser(data: bytes) -> Parser:
    stream = BytesIO(data)
    return Parser(translation_le, stream.read)


def nested_static_list(depth: int, leaf_size: int = 4) -> TypeInfo:
    """A static list tree of the given depth whose leaves are fixed-size ints."""
    leaf = TypeInfo(Size(kind="fixed", size=leaf_size))
    node = leaf
    for i in range(depth):
        # The innermost wrapper's child is the int leaf; every wrapper above
        # it wraps a list.
        child_id = "int" if i == 0 else "list"
        node = TypeInfo(Size(kind="fixed", size=2), {"": (child_id, node)})
    return node


def test_nested_static_lists_two_levels():
    """list of list of int, 2 outer x 2 inner elements."""
    info = FmtInfo("{0}")
    info.add_param("list", nested_static_list(depth=2))
    stream = struct.pack("<4i", 1, 2, 3, 4)
    formatted = make_parser(stream).format(info, FORMATTERS)
    assert formatted == "[[1, 2], [3, 4]]"


def test_nested_static_lists_three_levels():
    """list of list of list of int, 2 x 2 x 2 elements."""
    info = FmtInfo("{0}")
    info.add_param("list", nested_static_list(depth=3))
    stream = struct.pack("<8i", 1, 2, 3, 4, 5, 6, 7, 8)
    formatted = make_parser(stream).format(info, FORMATTERS)
    assert formatted == "[[[1, 2], [3, 4]], [[5, 6], [7, 8]]]"


def test_length_prefixed_outer_list():
    """A length-prefixed outer list of static inner lists."""
    leaf = TypeInfo(Size(kind="fixed", size=4))
    inner = TypeInfo(Size(kind="fixed", size=2), {"": ("int", leaf)})
    outer = TypeInfo(Size(kind="length_prefixed", size=4), {"": ("list", inner)})
    info = FmtInfo("{0}")
    info.add_param("list", outer)
    stream = struct.pack("<I", 3) + struct.pack("<6i", 1, 2, 3, 4, 5, 6)
    formatted = make_parser(stream).format(info, FORMATTERS)
    assert formatted == "[[1, 2], [3, 4], [5, 6]]"


def test_length_prefixed_inner_lists():
    """A static outer list whose inner lists are length-prefixed."""
    leaf = TypeInfo(Size(kind="fixed", size=4))
    inner = TypeInfo(Size(kind="length_prefixed", size=4), {"": ("int", leaf)})
    outer = TypeInfo(Size(kind="fixed", size=2), {"": ("list", inner)})
    info = FmtInfo("{0}")
    info.add_param("list", outer)
    stream = (
        struct.pack("<I", 2)
        + struct.pack("<2i", 1, 2)
        + struct.pack("<I", 1)
        + struct.pack("<1i", 3)
    )
    formatted = make_parser(stream).format(info, FORMATTERS)
    assert formatted == "[[1, 2], [3]]"


def test_nested_lists_with_separator_format():
    """Nested lists honor the '*' separator format spec at each level."""
    info = FmtInfo("{0}")
    info.add_param("list", nested_static_list(depth=2))
    stream = struct.pack("<4i", 1, 2, 3, 4)
    parser = make_parser(stream)
    parsed = parser.parse("list", info.type_infos[0][1])
    # Outer separator '-', inner separator '-', leaves formatted as decimals.
    assert format(parsed, "-*-*d") == "1-2-3-4"
