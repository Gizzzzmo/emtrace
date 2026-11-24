"""Stage 2 of decoding the runtime stream: turning byte trees into Python values.

Decoders are looked up by type id in a :class:`TypeRegistry`. A decoder is a
callable ``decoder(node, ctx) -> value`` that receives a
:class:`~emtrace.stream.framing.ByteNode` and a :class:`DecodeContext`. It
decodes child nodes through :meth:`DecodeContext.decode` (never by calling
other decoders directly), and reports values it cannot decode by calling
:meth:`DecodeContext.fail`. A value that cannot be decoded — because no
decoder is registered for its type id, or because the decoder failed — is
replaced by a :class:`~emtrace.stream.values.Placeholder`.

Custom type ids are supported by registering additional decoders::

    registry = default_registry()

    @registry.register("point")
    def decode_point(node, ctx):
        x, y = (ctx.decode(child) for child in node.elements[0])
        return f"({x}, {y})"
"""

from __future__ import annotations

import struct
from collections.abc import Callable, Mapping
from dataclasses import dataclass, field
from typing import Any, Literal, NoReturn, overload

from ..section.records import TraceRecord
from .framing import ByteNode, FramedEvent
from .values import Char, MyDict, MyList, Placeholder, SChar

__all__ = [
    "BUILTIN_DECODERS",
    "DecodeContext",
    "DecodeError",
    "DecodeProblem",
    "DecodedArgs",
    "Decoder",
    "TypeRegistry",
    "default_registry",
]


class DecodeError(Exception):
    """Raised by decoders (via :meth:`DecodeContext.fail`) for undecodable values."""


@dataclass(frozen=True)
class DecodeProblem:
    """Something that went wrong while decoding a value.

    ``placeholder`` tells whether the value was replaced by a
    :class:`Placeholder` (as opposed to a decoder warning about a value it
    could still decode, e.g. a string with invalid UTF-8).
    """

    path: tuple[str, ...]
    type_id: str
    message: str
    placeholder: bool
    offset: int
    length: int


@dataclass
class DecodedArgs:
    """The decoded arguments of one event, and the problems encountered."""

    values: list[Any]
    problems: list[DecodeProblem] = field(default_factory=list)

    @property
    def has_placeholders(self) -> bool:
        return any(p.placeholder for p in self.problems)


class DecodeContext:
    """What a decoder knows about the value it decodes."""

    def __init__(
        self,
        registry: TypeRegistry,
        record: TraceRecord,
        problems: list[DecodeProblem],
        path: tuple[str, ...] = (),
    ) -> None:
        self.registry: TypeRegistry = registry
        #: The trace record the decoded event belongs to.
        self.record: TraceRecord = record
        #: Byte order of the values (that of the trace record).
        self.byteorder: Literal["little", "big"] = record.byteorder
        #: Breadcrumbs of the value being decoded, e.g. ``("arg 0", "[2]")``.
        self.path: tuple[str, ...] = path
        self._problems: list[DecodeProblem] = problems

    def decode(self, node: ByteNode, label: str | None = None) -> Any:
        """Decode a child *node* with the decoder registered for its type id."""
        path = (*self.path, label) if label is not None else self.path
        child = DecodeContext(self.registry, self.record, self._problems, path)
        return self.registry.decode(node, child)

    def fail(self, message: str) -> NoReturn:
        """Give up on the current value; it will be shown as a placeholder."""
        raise DecodeError(message)

    def warn(self, node: ByteNode, message: str) -> None:
        """Report a problem with a value that could still be decoded."""
        self._problems.append(
            DecodeProblem(
                self.path,
                node.type.type_id,
                message,
                False,
                node.start,
                node.end - node.start,
            )
        )

    def placeholder(self, node: ByteNode, reason: str) -> Placeholder:
        self._problems.append(
            DecodeProblem(
                self.path,
                node.type.type_id,
                reason,
                True,
                node.start,
                node.end - node.start,
            )
        )
        return Placeholder(node.type.type_id, reason, node.data)


Decoder = Callable[[ByteNode, DecodeContext], Any]


class TypeRegistry:
    """Maps type ids to decoders."""

    def __init__(self, decoders: Mapping[str, Decoder] | None = None) -> None:
        self._decoders: dict[str, Decoder] = dict(decoders or {})

    @overload
    def register(
        self, type_id: str, decoder: Decoder, *, replace: bool = False
    ) -> Decoder: ...

    @overload
    def register(
        self, type_id: str, decoder: None = None, *, replace: bool = False
    ) -> Callable[[Decoder], Decoder]: ...

    def register(
        self, type_id: str, decoder: Decoder | None = None, *, replace: bool = False
    ) -> Decoder | Callable[[Decoder], Decoder]:
        """Register *decoder* for *type_id* (usable as a decorator).

        Raises ``ValueError`` if a decoder is already registered for
        *type_id*, unless *replace* is set.
        """

        def add(d: Decoder) -> Decoder:
            if type_id in self._decoders and not replace:
                raise ValueError(
                    f"a decoder for type id {type_id!r} is already registered"
                )
            self._decoders[type_id] = d
            return d

        return add if decoder is None else add(decoder)

    def unregister(self, type_id: str) -> None:
        _ = self._decoders.pop(type_id, None)

    def get(self, type_id: str) -> Decoder | None:
        return self._decoders.get(type_id)

    def __contains__(self, type_id: object) -> bool:
        return type_id in self._decoders

    def type_ids(self) -> list[str]:
        return sorted(self._decoders)

    def copy(self) -> TypeRegistry:
        return TypeRegistry(self._decoders)

    def decode(self, node: ByteNode, ctx: DecodeContext) -> Any:
        """Decode *node*; never raises (failures become placeholders)."""
        decoder = self._decoders.get(node.type.type_id)
        if decoder is None:
            return ctx.placeholder(node, "unknown type id")
        try:
            return decoder(node, ctx)
        except DecodeError as e:
            return ctx.placeholder(node, str(e))
        except Exception as e:  # noqa: BLE001  # user decoders must not abort decoding
            return ctx.placeholder(node, f"decoder raised {type(e).__name__}: {e}")

    def decode_args(self, event: FramedEvent, record: TraceRecord) -> DecodedArgs:
        """Decode all (framed) arguments of *event*."""
        problems: list[DecodeProblem] = []
        ctx = DecodeContext(self, record, problems)
        values = [ctx.decode(node, f"arg {i}") for i, node in enumerate(event.args)]
        return DecodedArgs(values, problems)


# ─────────────────────────────────────────────────────────────────────────────
# Builtin decoders
# ─────────────────────────────────────────────────────────────────────────────


def _leaf(node: ByteNode, ctx: DecodeContext) -> bytes:
    if node.data is None:
        assert node.elements is not None
        ctx.fail(f"expected a single value, got {len(node.elements)} element(s)")
    return node.data


def decode_signed(node: ByteNode, ctx: DecodeContext) -> int:
    return int.from_bytes(_leaf(node, ctx), ctx.byteorder, signed=True)


def decode_unsigned(node: ByteNode, ctx: DecodeContext) -> int:
    return int.from_bytes(_leaf(node, ctx), ctx.byteorder, signed=False)


def _single_byte(node: ByteNode, ctx: DecodeContext) -> bytes:
    data = _leaf(node, ctx)
    if len(data) != 1:
        ctx.fail(f"expected 1 byte, got {len(data)}")
    return data


def decode_char(node: ByteNode, ctx: DecodeContext) -> Char:
    return Char(_single_byte(node, ctx))


def decode_signed_char(node: ByteNode, ctx: DecodeContext) -> SChar:
    return SChar(_single_byte(node, ctx))


def decode_string(node: ByteNode, ctx: DecodeContext) -> str:
    data = _leaf(node, ctx)
    try:
        return data.decode("utf-8")
    except UnicodeDecodeError as e:
        ctx.warn(node, f"string is not valid UTF-8 ({e.reason})")
        return data.decode("utf-8", "replace")


def decode_bool(node: ByteNode, ctx: DecodeContext) -> bool:
    return any(_leaf(node, ctx))


_FLOAT_FORMATS = {2: "e", 4: "f", 8: "d"}


def decode_float(node: ByteNode, ctx: DecodeContext) -> float:
    data = _leaf(node, ctx)
    code = _FLOAT_FORMATS.get(len(data))
    if code is None:
        ctx.fail(f"expected 2, 4, or 8 bytes, got {len(data)}")
    prefix = "<" if ctx.byteorder == "little" else ">"
    return struct.unpack(prefix + code, data)[0]


def decode_list(node: ByteNode, ctx: DecodeContext) -> MyList[Any]:
    if node.elements is None:
        ctx.fail("expected elements, got a single value")
    items: list[Any] = []
    for i, element in enumerate(node.elements):
        if len(element) == 1:
            items.append(ctx.decode(element[0], f"[{i}]"))
        else:
            items.append(
                tuple(
                    ctx.decode(child, f"[{i}].{child.type.name or j}")
                    for j, child in enumerate(element)
                )
            )
    return MyList(items)


def _put(
    fields: dict[Any, Any], key: Any, value: Any, node: ByteNode, ctx: DecodeContext
) -> None:
    if key in fields:
        ctx.warn(node, f"duplicate key {key!r}; the last value is kept")
    fields[key] = value


def decode_struct(node: ByteNode, ctx: DecodeContext) -> MyDict:
    """A record of named fields: the children of the node's single element.

    Named children become entries keyed by their name. Unnamed children are
    merged into the struct when they decode to a mapping (anonymous members),
    skipped when they have no content (padding fillers), and keyed by their
    position otherwise.
    """
    if node.elements is None:
        ctx.fail("expected fields, got a single value")
    if len(node.elements) != 1:
        ctx.fail(
            f"expected exactly one element, got {len(node.elements)} (arrays of "
            f"structs are lists of structs)"
        )
    fields: dict[Any, Any] = {}
    for j, child in enumerate(node.elements[0]):
        name = child.type.name
        if name:
            _put(fields, name, ctx.decode(child, name), node, ctx)
            continue
        if child.data is not None and child.content_start == child.content_end:
            continue
        value = ctx.decode(child, f"#{j}")
        if isinstance(value, MyDict):
            for key in value:
                _put(fields, key, value[key], node, ctx)
        else:
            _put(fields, j, value, node, ctx)
    return MyDict(fields)


def _hashable_key(key: Any, node: ByteNode, ctx: DecodeContext) -> Any:
    if isinstance(key, (MyList, tuple)):
        key = tuple(_hashable_key(k, node, ctx) for k in key)
    try:
        _ = hash(key)
    except TypeError:
        ctx.warn(node, f"key {key!r} is not hashable; using its repr as key")
        return repr(key)
    return key


def decode_dict(node: ByteNode, ctx: DecodeContext) -> MyDict:
    """A mapping: every element of the node is a (key, value) pair of children."""
    if node.elements is None:
        ctx.fail("expected key/value elements, got a single value")
    if len(node.type.children) != 2:
        ctx.fail(
            f"expected elements of 2 children (key and value), got "
            f"{len(node.type.children)}"
        )
    fields: dict[Any, Any] = {}
    for i, (key_node, value_node) in enumerate(node.elements):
        key = _hashable_key(ctx.decode(key_node, f"[{i}].key"), node, ctx)
        _put(fields, key, ctx.decode(value_node, f"[{i}].value"), node, ctx)
    return MyDict(fields)


_SIGNED = [
    "signed",
    "int",
    "signed int",
    "int32_t",
    "long",
    "signed long",
    "long long",
    "signed long long",
    "int64_t",
    "int128_t",
    "short",
    "signed short",
    "int16_t",
    "ssize_t",
    "ptrdiff_t",
    "intptr_t",
]
_UNSIGNED = [
    "unsigned",
    "unsigned int",
    "uint32_t",
    "unsigned long",
    "unsigned long long",
    "uint64_t",
    "uint128_t",
    "uint16_t",
    "size_t",
    "uintptr_t",
    "*",
]

#: The decoders of the builtin type ids.
BUILTIN_DECODERS: dict[str, Decoder] = {
    **dict.fromkeys(_SIGNED, decode_signed),
    **dict.fromkeys(_UNSIGNED, decode_unsigned),
    "signed char": decode_signed_char,
    "int8_t": decode_signed_char,
    "unsigned char": decode_char,
    "char": decode_char,
    "uint8_t": decode_char,
    "string": decode_string,
    "bool": decode_bool,
    "_Bool": decode_bool,
    "float": decode_float,
    "double": decode_float,
    "list": decode_list,
    "struct": decode_struct,
    "dict": decode_dict,
}


def default_registry() -> TypeRegistry:
    """A new registry containing the decoders of all builtin type ids."""
    return TypeRegistry(BUILTIN_DECODERS)
