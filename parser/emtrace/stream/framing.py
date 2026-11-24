"""Stage 1 of decoding the runtime stream: splitting event bytes into byte trees.

How the bytes of a trace event are split up is fully determined by the type
tree of its ``TRCE`` record — sizes, size flags, padding, and children — and
by the runtime data version. Type ids play no role here (see
``TRACE_FORMAT.md``); they are only used in stage 2
(:mod:`emtrace.stream.decoding`) to turn the byte tree into Python values.

The framer is a pure function of a type tree and a :class:`ByteReader`, so it
can equally be applied to a stream, a COBS frame, or a byte range that has
already been framed (e.g. to re-interpret it with a different type tree).
"""

from __future__ import annotations

from collections.abc import Callable
from dataclasses import dataclass
from typing import Literal, NoReturn

from ..errors import Issue
from ..section.records import MAX_NESTING_DEPTH, TraceRecord
from ..section.type_tree import TypeNode

__all__ = [
    "ByteNode",
    "ByteReader",
    "FramedEvent",
    "FramingError",
    "dump_byte_tree",
    "frame_args",
    "frame_value",
]

#: Upper bound for single reads from a stream (garbage sizes must not cause
#: huge allocations up front).
_MAX_CHUNK = 1 << 16
#: LEB128 length prefixes longer than this are rejected (fits 128-bit sizes).
_MAX_LEB128_BYTES = 20
#: Length prefixes of the runtime data version 1.0 wider than this are rejected.
_MAX_V1_PREFIX_WIDTH = 16
#: Element counts above this are rejected for elements that may consume no
#: bytes at all (they could otherwise never be bounded by the input size).
_MAX_ZERO_SIZE_ELEMENTS = 1 << 16
#: Upper bound for the number of values per event that consume no bytes
#: (nested repetitions of those could otherwise multiply without bound).
_MAX_ZERO_SIZE_VALUES = 1 << 16


class FramingError(Exception):
    """The event's bytes don't match its type tree (or ended prematurely)."""

    def __init__(
        self, message: str, offset: int, length: int, path: tuple[str, ...]
    ) -> None:
        super().__init__(message)
        self.message: str = message
        self.offset: int = offset
        self.length: int = length
        self.path: tuple[str, ...] = path

    def to_issue(self, base_path: tuple[str, ...] = ()) -> Issue:
        return Issue.error(
            self.message, self.offset, self.length, (*base_path, *self.path)
        )


class ByteReader:
    """Reads the bytes of one event, keeping all of them in :attr:`buffer`.

    *source* is either the complete bytes of the event (e.g. a COBS frame),
    or a ``read(n)`` callable of a stream, which must only return fewer than
    *n* bytes at the end of the stream. Bytes are only requested from a
    stream when they are needed, so no bytes of the following event are
    consumed — except when the current event is malformed.
    """

    def __init__(self, source: bytes | bytearray | Callable[[int], bytes]) -> None:
        self._read: Callable[[int], bytes] | None
        if isinstance(source, (bytes, bytearray)):
            self.buffer: bytearray = bytearray(source)
            self._read = None
        else:
            self.buffer = bytearray()
            self._read = source
        self.pos: int = 0
        self._eof: bool = False

    @property
    def bounded(self) -> bool:
        """Whether the total number of bytes is known up front."""
        return self._read is None

    @property
    def remaining(self) -> int | None:
        """Number of unread bytes, or ``None`` when reading from a stream."""
        return len(self.buffer) - self.pos if self._read is None else None

    def ensure(self, n: int) -> bool:
        """Make ``n`` bytes after :attr:`pos` available; False if impossible."""
        missing = self.pos + n - len(self.buffer)
        while missing > 0 and self._read is not None and not self._eof:
            chunk = self._read(min(missing, _MAX_CHUNK))
            if not chunk:
                self._eof = True
                break
            self.buffer += chunk
            missing -= len(chunk)
        return self.pos + n <= len(self.buffer)

    def available(self) -> int:
        """Number of bytes after :attr:`pos` that have been read so far."""
        return len(self.buffer) - self.pos

    def take(self, n: int) -> bytes | None:
        """Consume and return ``n`` bytes, or ``None`` when there are fewer."""
        if not self.ensure(n):
            return None
        data = bytes(self.buffer[self.pos : self.pos + n])
        self.pos += n
        return data


@dataclass
class ByteNode:
    """The bytes of one value, split according to its :class:`TypeNode`.

    Offsets are relative to the start of the event. ``[start, content_start)``
    holds the length prefix (if any), ``[content_start, content_end)`` the
    content, followed by the null terminator (if any) and the padding up to
    ``end``. Leaves carry their content in ``data``; composites carry one
    list of child nodes per element in ``elements``.
    """

    type: TypeNode
    start: int
    content_start: int
    content_end: int
    end: int
    data: bytes | None = None
    elements: list[list[ByteNode]] | None = None

    @property
    def is_leaf(self) -> bool:
        return self.elements is None


@dataclass
class FramedEvent:
    """The byte trees of all arguments of one event.

    ``data`` holds all bytes of the event read so far (starting with the
    event's record pointer). When framing failed, ``error`` is set and
    ``args`` contains the arguments that were framed before the failure.
    """

    data: bytes
    args: list[ByteNode]
    error: FramingError | None = None


class _Framer:
    def __init__(
        self,
        reader: ByteReader,
        runtime_data_version: tuple[int, int],
        byteorder: Literal["little", "big"],
    ) -> None:
        self.r: ByteReader = reader
        self.v1_prefix: bool = runtime_data_version < (2, 0)
        self.byteorder: Literal["little", "big"] = byteorder
        self.path: list[str] = []
        self._min_sizes: dict[int, int] = {}
        self._zero_size_values: int = 0

    def fail(self, message: str, offset: int, length: int) -> NoReturn:
        raise FramingError(message, offset, length, tuple(self.path))

    def _where(self) -> str:
        return "the frame" if self.r.bounded else "the stream"

    def read(self, n: int, what: str) -> int:
        """Consume *n* bytes; returns the offset of the first one."""
        start = self.r.pos
        if not self.r.ensure(n):
            available = self.r.available()
            self.fail(
                f"{what} needs {n} byte(s), but {self._where()} ends after "
                f"{available} more byte(s)",
                start,
                available,
            )
        self.r.pos += n
        return start

    def count_prefix(self, node: TypeNode) -> int:
        start = self.r.pos
        if self.v1_prefix:
            width = node.size.size
            if width > _MAX_V1_PREFIX_WIDTH:
                self.fail(
                    f"length prefix width {width} is unreasonably large", start, 0
                )
            pos = self.read(width, "length prefix")
            return int.from_bytes(self.r.buffer[pos : pos + width], self.byteorder)
        value = 0
        for i in range(_MAX_LEB128_BYTES):
            byte = self.r.buffer[self.read(1, "length prefix")]
            value |= (byte & 0x7F) << (7 * i)
            if byte & 0x80 == 0:
                return value
        self.fail(
            f"LEB128 length prefix is longer than {_MAX_LEB128_BYTES} bytes",
            start,
            _MAX_LEB128_BYTES,
        )

    def min_size(self, node: TypeNode) -> int:
        """The minimum number of bytes a value of type *node* occupies."""
        key = id(node)
        cached = self._min_sizes.get(key)
        if cached is not None:
            return cached
        kind, size = node.size.kind, node.size.size
        if kind == "null_terminated":
            result = size
        elif kind == "length_prefixed":
            result = size if self.v1_prefix else 1
        elif node.is_leaf:
            result = size
        else:
            result = size * self.element_min_size(node)
        result += node.padding
        self._min_sizes[key] = result
        return result

    def element_min_size(self, node: TypeNode) -> int:
        return sum(self.min_size(child) for child in node.children)

    def check_count(self, count: int, node: TypeNode, offset: int) -> None:
        element_size = self.element_min_size(node)
        remaining = self.r.remaining
        if element_size == 0 and count > _MAX_ZERO_SIZE_ELEMENTS:
            self.fail(
                f"refusing to decode {count} elements that may each consume 0 bytes",
                offset,
                self.r.pos - offset,
            )
        if remaining is not None and count * element_size > remaining:
            self.fail(
                f"{count} element(s) of at least {element_size} byte(s) each don't "
                f"fit into the {remaining} byte(s) left in the frame",
                offset,
                self.r.pos - offset,
            )

    def terminator_at_pos(self, n: int, what: str) -> bool:
        if not self.r.ensure(n):
            available = self.r.available()
            self.fail(
                f"{what} has no {n}-byte null terminator before the end of "
                f"{self._where()}",
                self.r.pos,
                available,
            )
        return not any(self.r.buffer[self.r.pos : self.r.pos + n])

    def leaf_until_terminator(self, n: int) -> tuple[int, int]:
        """Content of a null-terminated leaf (terminator aligned to *n* bytes)."""
        start = self.r.pos
        if self.r.bounded and n == 1:
            end = self.r.buffer.find(0, start)
            if end < 0:
                self.fail(
                    "value has no null terminator before the end of the frame",
                    start,
                    len(self.r.buffer) - start,
                )
            self.r.pos = end + 1
            return start, end
        while not self.terminator_at_pos(n, "value"):
            self.r.pos += n
        end = self.r.pos
        self.r.pos += n
        return start, end

    def node(self, t: TypeNode, depth: int) -> ByteNode:
        start = self.r.pos
        if depth > MAX_NESTING_DEPTH:
            self.fail(
                f"type tree is nested deeper than {MAX_NESTING_DEPTH} levels", start, 0
            )
        kind, size = t.size.kind, t.size.size
        if t.is_leaf:
            if kind == "fixed":
                content_start = self.read(size, "value")
                content_end = content_start + size
            elif kind == "length_prefixed":
                n = self.count_prefix(t)
                content_start = self.read(n, f"{n}-byte value")
                content_end = content_start + n
            else:
                if size == 0:
                    self.fail("null terminator length is 0", start, 0)
                content_start, content_end = self.leaf_until_terminator(size)
            data = bytes(self.r.buffer[content_start:content_end])
            elements = None
        else:
            data = None
            elements: list[list[ByteNode]] | None = []
            if kind == "null_terminated":
                if size == 0:
                    self.fail("null terminator length is 0", start, 0)
                content_start = self.r.pos
                while not self.terminator_at_pos(size, "sequence"):
                    element_start = self.r.pos
                    elements.append(self.element(t, len(elements), depth))
                    if self.r.pos == element_start:
                        self.fail(
                            "null-terminated sequence of elements that consume 0 bytes",
                            element_start,
                            0,
                        )
                content_end = self.r.pos
                self.r.pos += size
            else:
                count = self.count_prefix(t) if kind == "length_prefixed" else size
                self.check_count(count, t, start)
                content_start = self.r.pos
                for i in range(count):
                    elements.append(self.element(t, i, depth))
                content_end = self.r.pos
        if t.padding:
            _ = self.read(t.padding, "padding")
        if self.r.pos == start:
            self._zero_size_values += 1
            if self._zero_size_values > _MAX_ZERO_SIZE_VALUES:
                self.fail(
                    f"refusing to decode more than {_MAX_ZERO_SIZE_VALUES} values that "
                    f"consume 0 bytes",
                    start,
                    0,
                )
        return ByteNode(
            t, start, content_start, content_end, self.r.pos, data, elements
        )

    def element(self, t: TypeNode, index: int, depth: int) -> list[ByteNode]:
        self.path.append(f"[{index}]")
        nodes: list[ByteNode] = []
        for j, child in enumerate(t.children):
            label = child.name or f"child {j}"
            if len(t.children) > 1:
                self.path.append(label)
            nodes.append(self.node(child, depth + 1))
            if len(t.children) > 1:
                _ = self.path.pop()
        _ = self.path.pop()
        return nodes


def frame_value(
    node: TypeNode,
    reader: ByteReader,
    runtime_data_version: tuple[int, int],
    byteorder: Literal["little", "big"],
) -> ByteNode:
    """Frame a single value of type *node*; raises :class:`FramingError`."""
    return _Framer(reader, runtime_data_version, byteorder).node(node, 0)


def frame_args(record: TraceRecord, reader: ByteReader) -> FramedEvent:
    """Frame all arguments of *record* from *reader* (never raises)."""
    framer = _Framer(reader, record.runtime_data_version, record.byteorder)
    args: list[ByteNode] = []
    error: FramingError | None = None
    try:
        for i, arg in enumerate(record.args):
            framer.path = [f"arg {i}"]
            args.append(framer.node(arg, 0))
    except FramingError as e:
        error = e
    return FramedEvent(bytes(reader.buffer), args, error)


def _describe_node(node: ByteNode) -> str:
    t = node.type
    span = f"[{node.start:#x}..{node.end:#x}]"
    if node.data is not None:
        shown = node.data[:16].hex(" ") + (" …" if len(node.data) > 16 else "")
        return f"{t.type_id!r} {span} {len(node.data)} byte(s): {shown}"
    assert node.elements is not None
    return f"{t.type_id!r} {span} {len(node.elements)} element(s)"


def dump_byte_tree(event: FramedEvent) -> list[str]:
    """An indented listing of the byte trees of an event (for debugging)."""
    lines: list[str] = []
    stack: list[tuple[str, ByteNode, int]] = [
        (f"arg {i}", node, 0) for i, node in reversed(list(enumerate(event.args)))
    ]
    while stack:
        label, node, depth = stack.pop()
        lines.append(f"{'  ' * depth}{label}: {_describe_node(node)}")
        if node.elements is not None:
            for i, element in reversed(list(enumerate(node.elements))):
                for j, child in reversed(list(enumerate(element))):
                    name = (
                        f"[{i}]"
                        if len(element) == 1
                        else f"[{i}].{child.type.name or j}"
                    )
                    stack.append((name, child, depth + 1))
    if event.error is not None:
        lines.append(f"error: {event.error.to_issue().describe()}")
    return lines
