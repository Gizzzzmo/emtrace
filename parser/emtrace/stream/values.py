"""Python value types produced by the builtin decoders.

Besides the standard format specs of their contents, the container types
support these format specs:

- ``MyList``: ``SEP*ELEMENT_SPEC`` joins the elements, each formatted with
  ``ELEMENT_SPEC``, with ``SEP``.
- ``MyDict``: ``ENTRY_SEP*KV_SEP*VALUE_SPEC`` joins the entries, each
  formatted as key, ``KV_SEP``, and the value formatted with ``VALUE_SPEC``,
  with ``ENTRY_SEP``.
- both: ``#`` pretty-prints the value (recursively) over multiple lines,
  indented by 4 spaces, or by the number of spaces given after the ``#``
  (e.g. ``#2``).

Fields of a ``MyDict`` are accessible in format strings with ``{0.name}`` or
``{0[name]}``, elements of a ``MyList`` with ``{0[index]}``.
"""

from __future__ import annotations

import re
from collections.abc import Iterator, Mapping
from dataclasses import dataclass
from typing import Any, override


class SChar:
    """A wrapper for a single byte that can be formatted as a character or an integer."""

    def __init__(self, value: bytes) -> None:
        """Initialize the Char with a single byte."""
        self.value: int = value[0]
        if self.value >= 128:
            self.value -= 256

    @override
    def __format__(self, format_spec: str, /) -> str:
        if len(format_spec) == 0 or not format_spec[-1].isalpha():
            return ("{:" + format_spec + "c}").format(self.value)
        return ("{:" + format_spec + "}").format(self.value)

    @override
    def __repr__(self) -> str:
        return f"char({hex(self.value)})"

    @override
    def __eq__(self, other: object) -> bool:
        return isinstance(other, SChar) and other.value == self.value

    @override
    def __hash__(self) -> int:
        return hash((SChar, self.value))


class Char:
    """A wrapper for a single byte that can be formatted as a character or an integer."""

    def __init__(self, value: bytes) -> None:
        """Initialize the Char with a single byte."""
        self.value: int = value[0]

    @override
    def __format__(self, format_spec: str, /) -> str:
        if len(format_spec) == 0 or not format_spec[-1].isalpha():
            return ("{:" + format_spec + "c}").format(self.value)
        return ("{:" + format_spec + "}").format(self.value)

    @override
    def __repr__(self) -> str:
        return f"char({hex(self.value)})"

    @override
    def __eq__(self, other: object) -> bool:
        return isinstance(other, Char) and other.value == self.value

    @override
    def __hash__(self) -> int:
        return hash((Char, self.value))


_PRETTY_SPEC = re.compile(r"#(\d*)")
_DEFAULT_INDENT = 4


def _pretty_indent(format_spec: str) -> int | None:
    """The indentation of a pretty-printing format spec, or None for other specs."""
    match = _PRETTY_SPEC.fullmatch(format_spec)
    if match is None:
        return None
    return int(match[1]) if match[1] else _DEFAULT_INDENT


def pretty(value: Any, indent: int = _DEFAULT_INDENT, level: int = 0) -> str:
    """Render *value* like its ``repr``, with containers spread over multiple lines."""
    if isinstance(value, MyDict):
        items = [
            f"{k!r}: {pretty(v, indent, level + 1)}" for k, v in value._fields.items()
        ]
        return _pretty_container("{", items, "}", indent, level)
    if isinstance(value, MyList):
        items = [pretty(v, indent, level + 1) for v in value.list]
        return _pretty_container("[", items, "]", indent, level)
    if isinstance(value, tuple):
        items = [pretty(v, indent, level + 1) for v in value]
        if len(items) == 1:
            items[0] += ","
        return _pretty_container("(", items, ")", indent, level)
    return repr(value)


def _pretty_container(
    open_: str, items: list[str], close: str, indent: int, level: int
) -> str:
    if not items:
        return open_ + close
    inner = " " * (indent * (level + 1))
    lines = [f"{inner}{item}" for item in items]
    return f"{open_}\n" + ",\n".join(lines) + f"\n{' ' * (indent * level)}{close}"


class MyList[T]:
    """A decoded sequence that supports extra format specs (see the module docs)."""

    def __init__(self, list_arg: list[T]) -> None:
        self.list: list[T] = list_arg

    @override
    def __format__(self, format_spec: str, /) -> str:
        indent = _pretty_indent(format_spec)
        if indent is not None:
            return pretty(self, indent)
        parts = format_spec.split("*", 1)
        match parts:
            case [_]:
                return self.list.__format__(format_spec)
            case [sep, el_spec]:
                return sep.join([el.__format__(el_spec) for el in self.list])
            case _:
                assert False  # noqa: B011  # unreachable

    def __getitem__(self, index: int) -> T:
        return self.list[index]

    def __len__(self) -> int:
        return len(self.list)

    def __iter__(self) -> Iterator[T]:
        return iter(self.list)

    @override
    def __eq__(self, other: object) -> bool:
        if isinstance(other, MyList):
            return self.list == other.list
        return isinstance(other, list) and self.list == other

    @override
    def __repr__(self) -> str:
        return self.list.__repr__()


class MyDict:
    """A decoded mapping (a struct or a dict) that supports extra format specs.

    Entries are accessible as attributes (``value.name``, ``{0.name}`` in
    format strings) and by key (``value["name"]``, ``{0[name]}``). To avoid
    clashes with field names, the class has no public methods: iterate over it
    for the keys (e.g. ``{key: value[key] for key in value}``).
    """

    def __init__(self, fields: Mapping[Any, Any] | None = None) -> None:
        self._fields: dict[Any, Any] = dict(fields or {})

    def __getattr__(self, name: str) -> Any:
        # Only called when normal attribute lookup fails; dunders and the
        # storage attribute itself must not be looked up in the fields (e.g.
        # while copying, before ``_fields`` exists).
        if name == "_fields" or (name.startswith("__") and name.endswith("__")):
            raise AttributeError(name)
        try:
            return self._fields[name]
        except KeyError:
            raise AttributeError(f"no field {name!r}") from None

    def __getitem__(self, key: Any) -> Any:
        return self._fields[key]

    def __contains__(self, key: object) -> bool:
        return key in self._fields

    def __len__(self) -> int:
        return len(self._fields)

    def __iter__(self) -> Iterator[Any]:
        return iter(self._fields)

    @override
    def __eq__(self, other: object) -> bool:
        if isinstance(other, MyDict):
            return self._fields == other._fields
        return isinstance(other, Mapping) and self._fields == dict(other)

    @override
    def __repr__(self) -> str:
        return repr(self._fields)

    @override
    def __format__(self, format_spec: str, /) -> str:
        if not format_spec:
            return repr(self)
        indent = _pretty_indent(format_spec)
        if indent is not None:
            return pretty(self, indent)
        parts = format_spec.split("*", 2)
        if len(parts) != 3:
            raise ValueError(
                f"invalid format spec {format_spec!r} for a dict: expected "
                f"'ENTRY_SEP*KV_SEP*VALUE_SPEC' or '#[INDENT]'"
            )
        entry_sep, kv_sep, value_spec = parts
        return entry_sep.join(
            f"{format(key, '')}{kv_sep}{format(value, value_spec)}"
            for key, value in self._fields.items()
        )


#: Number of content bytes shown in a placeholder.
_PLACEHOLDER_BYTES = 16


@dataclass(frozen=True)
class Placeholder:
    """Stands in for a value that could not be decoded.

    It ignores any format spec, so that it can be formatted wherever the
    decoded value would have been.
    """

    type_id: str
    reason: str
    data: bytes | None = None

    @override
    def __str__(self) -> str:
        text = f"<{self.type_id}: {self.reason}"
        if self.data:
            text += ": " + self.data[:_PLACEHOLDER_BYTES].hex(" ")
            if len(self.data) > _PLACEHOLDER_BYTES:
                text += " …"
        return text + ">"

    @override
    def __repr__(self) -> str:
        return str(self)

    @override
    def __format__(self, format_spec: str, /) -> str:
        return str(self)
