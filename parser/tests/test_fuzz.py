"""Property-based fuzz tests: the parsers must be total and lossless.

- Parsing arbitrary or corrupted section bytes never raises, terminates, and
  only reports byte ranges inside the data.
- Every valid record (of random content and layout) is parsed back exactly.
- Stage 1 of runtime decoding is lossless on valid streams, and total on
  arbitrary bytes (bounded frames as well as streams).
- Stage 2 and formatting never raise.
- The run engine never raises, whatever the stream contains.

Run long fuzzing sessions with ``just fuzz`` (Hypothesis profile ``fuzz``).
"""

from __future__ import annotations

import contextlib
import tempfile
from collections.abc import Callable
from functools import cache
from io import BytesIO
from pathlib import Path
from typing import Any, Literal, cast

from emtrace.errors import Issue
from emtrace.section.export import export_section
from emtrace.section.inspector import check_section, summarize_records
from emtrace.section.records import (
    TraceRecord,
    dump_record,
    find_mgic,
    iter_records,
    parse_record,
)
from emtrace.section.type_tree import Size, SizeKind, TypeNode
from emtrace.stream.decoding import BUILTIN_DECODERS, default_registry
from emtrace.stream.engine import emtrace
from emtrace.stream.formatters import FORMATTERS
from emtrace.stream.framing import (
    ByteNode,
    ByteReader,
    FramingError,
    frame_args,
    frame_value,
)
from emtrace.stream.values import MyDict, MyList
from hypothesis import given, reject
from hypothesis import strategies as st

from .builders import (
    AmbiguousValue,
    Layout,
    RawValue,
    build_mgic_record,
    build_section,
    build_trce_record,
    cobs_encode,
    encode_pointer,
    encode_value,
    record_offsets,
)

# ─────────────────────────────────────────────────────────────────────────────
# Strategies
# ─────────────────────────────────────────────────────────────────────────────

_BYTEORDERS: tuple[Literal["little", "big"], ...] = ("little", "big")
byteorders = st.sampled_from(_BYTEORDERS)
c_strings = st.text(
    alphabet=st.characters(exclude_characters="\x00", exclude_categories=["Cs"]),
    max_size=10,
)
type_ids = st.one_of(st.sampled_from(sorted(BUILTIN_DECODERS)), c_strings)

leaf_sizes = st.one_of(
    st.tuples(st.just("fixed"), st.integers(0, 9)),
    st.tuples(st.just("length_prefixed"), st.integers(1, 4)),
    st.tuples(st.just("null_terminated"), st.integers(1, 4)),
)
composite_sizes = st.one_of(
    st.tuples(st.just("fixed"), st.integers(0, 3)),
    st.tuples(st.just("length_prefixed"), st.integers(1, 4)),
    st.tuples(st.just("null_terminated"), st.integers(1, 4)),
)


def _node(
    size: tuple[str, int],
    padding: int,
    type_id: str,
    name: str,
    children: list[TypeNode] | None = None,
) -> TypeNode:
    kind = cast(SizeKind, size[0])
    return TypeNode(type_id, Size(kind, size[1]), padding, name, children or [])


leaf_nodes = st.builds(_node, leaf_sizes, st.integers(0, 3), type_ids, c_strings)


def _composites(children: st.SearchStrategy[TypeNode]) -> st.SearchStrategy[TypeNode]:
    return st.builds(
        _node,
        composite_sizes,
        st.integers(0, 3),
        type_ids,
        c_strings,
        st.lists(children, min_size=1, max_size=3),
    )


type_trees = st.recursive(leaf_nodes, _composites, max_leaves=8)


def raw_values(node: TypeNode) -> st.SearchStrategy[RawValue]:
    """Raw values (see :func:`encode_value`) of type *node*."""
    kind, size = node.size.kind, node.size.size
    if node.is_leaf:
        if kind == "fixed":
            return st.binary(min_size=size, max_size=size)
        if kind == "length_prefixed":
            return st.binary(max_size=12)
        chunk = st.binary(min_size=size, max_size=size).filter(any)
        return st.lists(chunk, max_size=4).map(b"".join)
    element = st.tuples(*(raw_values(c) for c in node.children)).map(list)
    if kind == "fixed":
        return st.lists(element, min_size=size, max_size=size)
    return st.lists(element, max_size=3)


layouts = st.builds(
    Layout,
    st.permutations(["size", "probe", "payload"]).map(tuple),
    st.tuples(st.integers(0, 5), st.integers(0, 5), st.integers(0, 5)),
    st.integers(0, 255),
)


@st.composite
def trace_records(draw: st.DrawFn) -> tuple[bytes, dict[str, Any]]:
    """A valid TRCE record, and the parameters it was built from."""
    params: dict[str, Any] = {
        "fmt_string": draw(c_strings),
        "formatter_id": draw(st.integers(0, 2)),
        "file": draw(c_strings),
        "line": draw(st.integers(0, 2**16)),
        "args": draw(st.lists(type_trees, max_size=3)),
        "size_t_size": draw(st.sampled_from([1, 2, 4, 8])),
        "byteorder": draw(byteorders),
        "layout": draw(layouts),
        "string_gap": draw(st.integers(0, 4)),
    }
    try:
        record = build_trce_record(**params)
    except (ValueError, OverflowError):  # doesn't fit the size_t width / offsets
        reject()
    return record, params


@cache
def _real_sections() -> list[bytes]:
    """.emtrace sections of built example binaries, when available."""
    from emtrace.exe._lief import HAS_LIEF, find_section_bytes, parse_binary

    if not HAS_LIEF:
        return []
    root = Path(__file__).resolve().parents[2]
    candidates = [
        root / "c/build/clang/rel/examples/test_mixed",
        root / "c/build/clang/rel/examples/test_arr_cobs",
        root / "c/build/clang/rel/examples/test_strings",
        root / "rust/target/debug/examples/test_mixed",
    ]
    sections: list[bytes] = []
    for path in candidates:
        if path.is_file():
            data = find_section_bytes(
                path, ".emtrace", lambda p: parse_binary("exe", p)
            )
            if data:
                sections.append(data)
    return sections


@st.composite
def sections(draw: st.DrawFn) -> bytes:
    """A valid section: built from random records, or a real one."""
    real = _real_sections()
    if real and draw(st.booleans()):
        return draw(st.sampled_from(real))
    records = [build_mgic_record(encoding_id=draw(st.integers(0, 2)))]
    records += [r for r, _ in draw(st.lists(trace_records(), max_size=3))]
    return build_section(records, draw(st.integers(0, 3)))


@st.composite
def corrupted(draw: st.DrawFn, data: bytes) -> bytes:
    """*data* with random byte flips, insertions, deletions, and truncation."""
    out = bytearray(data)
    for _ in range(draw(st.integers(1, 8))):
        if not out:
            break
        pos = draw(st.integers(0, len(out) - 1))
        op = draw(st.sampled_from(["set", "insert", "delete", "truncate", "emt"]))
        if op == "set":
            out[pos] = draw(st.integers(0, 255))
        elif op == "insert":
            out[pos:pos] = draw(st.binary(min_size=1, max_size=8))
        elif op == "delete":
            del out[pos : pos + draw(st.integers(1, 8))]
        elif op == "truncate":
            del out[pos:]
        else:
            out[pos:pos] = b"EMT"
    return bytes(out)


# Arbitrary bytes, with 'EMT' sprinkled in to reach the framing logic.
garbage = st.lists(st.one_of(st.binary(max_size=40), st.just(b"EMT")), max_size=12).map(
    b"".join
)

# ─────────────────────────────────────────────────────────────────────────────
# Record parsing
# ─────────────────────────────────────────────────────────────────────────────


def _check_issue_bounds(issue: Issue, size: int) -> None:
    if issue.offset is not None:
        assert 0 <= issue.offset <= size, issue
        assert issue.length >= 0
        assert issue.offset + issue.length <= size, issue


def _check_records_total(data: bytes) -> None:
    previous = -1
    for record in iter_records(data):
        assert record.start > previous
        assert record.next_offset > record.start
        previous = record.start
        for issue in record.issues:
            _check_issue_bounds(issue, len(data))
        for span in record.spans:
            assert 0 <= span.start <= span.end <= len(data)
        if record.ok:
            assert record.value is not None
        _ = dump_record(data, record)
    mgic = find_mgic(data)
    if mgic is not None:
        assert mgic.framing is not None
    report = check_section(data, error_on_minor=True, debug_trace=lambda _line: None)
    for issue in [*report.errors, *report.warnings]:
        _check_issue_bounds(issue, len(data))
    _ = summarize_records(data)
    _ = export_section(data)


@given(garbage)
def test_parsing_arbitrary_bytes_is_total(data: bytes):
    _check_records_total(data)


@given(st.data())
def test_parsing_corrupted_sections_is_total(data: st.DataObject):
    _check_records_total(data.draw(corrupted(data.draw(sections()))))


def _normalized(args: list[TypeNode]) -> list[TypeNode]:
    """Top-level argument names aren't serialized."""
    return [TypeNode(a.type_id, a.size, a.padding, "", a.children) for a in args]


@given(trace_records())
def test_valid_records_round_trip(built: tuple[bytes, dict[str, Any]]):
    record, params = built
    parsed = parse_record(record, 0)
    assert parsed.issues == []
    value = parsed.value
    assert isinstance(value, TraceRecord)
    assert value.fmt_string == params["fmt_string"]
    assert value.formatter == params["formatter_id"]
    assert value.file == params["file"]
    assert value.line == params["line"]
    assert value.args == _normalized(params["args"])
    assert value.size_t_size == params["size_t_size"]
    if params["size_t_size"] > 1:  # a 1-byte probe can't tell byte orders apart
        assert value.byteorder == params["byteorder"]
    assert TraceRecord.from_dict(value.to_dict(), 0) == value
    # The same record inside a section with garbage around it is found again.
    section = b"\xa5" * 7 + record + b"\x5a" * 5
    found = [r for r in iter_records(section) if r.ok]
    assert [r.start for r in found] == [7]


# ─────────────────────────────────────────────────────────────────────────────
# Runtime decoding
# ─────────────────────────────────────────────────────────────────────────────


def _to_raw(node: ByteNode) -> RawValue:
    if node.data is not None:
        return node.data
    assert node.elements is not None
    return [[_to_raw(child) for child in element] for element in node.elements]


def _streaming(data: bytes, chunk: int) -> Callable[[int], bytes]:
    stream = BytesIO(data)
    return lambda n: stream.read(min(n, chunk))


def _check_node_bounds(node: ByteNode, size: int) -> None:
    assert 0 <= node.start <= node.content_start <= node.content_end <= node.end <= size
    for element in node.elements or []:
        for child in element:
            _check_node_bounds(child, size)


@given(st.data())
def test_stage1_is_lossless(data: st.DataObject):
    node = data.draw(type_trees)
    value = data.draw(raw_values(node))
    byteorder = data.draw(byteorders)
    try:
        encoded = encode_value(node, value, byteorder)
    except AmbiguousValue:
        reject()
    trailer = data.draw(st.binary(max_size=4))
    for reader in (
        ByteReader(encoded + trailer),
        ByteReader(_streaming(encoded + trailer, data.draw(st.integers(1, 5)))),
    ):
        framed = frame_value(node, reader, (3, 0), byteorder)
        assert _to_raw(framed) == value
        assert (framed.start, framed.end) == (0, len(encoded))
        assert reader.pos == len(encoded)


@given(type_trees, st.binary(max_size=200), st.integers(1, 7))
def test_stage1_is_total(node: TypeNode, data: bytes, chunk: int):
    for reader in (ByteReader(data), ByteReader(_streaming(data, chunk))):
        try:
            framed = frame_value(node, reader, (3, 0), "little")
        except FramingError as e:
            assert 0 <= e.offset <= len(data)
            assert e.offset + e.length <= len(reader.buffer)
            continue
        _check_node_bounds(framed, len(data))


@given(st.lists(type_trees, max_size=3), st.binary(max_size=200), c_strings)
def test_stage2_and_formatting_are_total(args: list[TypeNode], data: bytes, fmt: str):
    record = TraceRecord(0, fmt, 0, args, "f.c", 1, (3, 0), (3, 0), "little", 8)
    event = frame_args(record, ByteReader(data))
    if event.error is not None:
        return
    decoded = default_registry().decode_args(event, record)
    for value in decoded.values:
        _ = format(value, "")
        _ = repr(value)
        if isinstance(value, (MyDict, MyList)):
            _ = format(value, "#")
    for formatter in FORMATTERS.values():
        # Formatting errors are reported by the engine; they just must not hang.
        with contextlib.suppress(Exception):
            _ = formatter(fmt, decoded.values)


# ─────────────────────────────────────────────────────────────────────────────
# Engine
# ─────────────────────────────────────────────────────────────────────────────

_TMP = Path(tempfile.mkdtemp(prefix="emtrace-fuzz-"))


@st.composite
def streams(draw: st.DrawFn) -> tuple[bytes, bytes]:
    """A section and a (possibly corrupted) stream of events for it."""
    encoding_id = draw(st.integers(0, 2))
    built = draw(st.lists(trace_records(), max_size=3))
    records = [
        build_mgic_record(ptr_size=4, encoding_id=encoding_id, alignment_power=2)
    ]
    records += [r for r, _ in built]
    section = build_section(records, 2)
    offsets = record_offsets(records, 2)
    events = [encode_pointer(offsets[0], 4, 2)]
    for _ in range(draw(st.integers(0, 4))):
        if len(records) > 1 and draw(st.booleans()):
            index = draw(st.integers(1, len(records) - 1))
            args: list[TypeNode] = _normalized(built[index - 1][1]["args"])
            payload = b""
            for arg in args:
                try:
                    payload += encode_value(arg, draw(raw_values(arg)))
                except AmbiguousValue:
                    payload += b"\x00"
            events.append(encode_pointer(offsets[index], 4, 2) + payload)
        else:
            events.append(draw(st.binary(max_size=24)))
    if encoding_id == 0:
        stream = b"".join(events)
    else:
        stream = b"".join(cobs_encode(e) for e in events)
    if draw(st.booleans()):
        stream = draw(corrupted(stream))
    return section, stream


def _run(section: bytes, stream: bytes, **kwargs: Any) -> None:
    path = _TMP / "section.bin"
    _ = path.write_bytes(section)
    out = bytearray()
    report = emtrace(
        path, "binary", istream=BytesIO(stream).read, ostream=out.extend, **kwargs
    )
    for issue in [*report.errors, *report.warnings]:
        assert isinstance(issue.message, str)


@given(streams(), st.booleans())
def test_engine_is_total(section_and_stream: tuple[bytes, bytes], drop: bool):
    section, stream = section_and_stream
    _run(section, stream, drop_undecodable=drop)


@given(st.data())
def test_engine_is_total_on_corrupted_sections(data: st.DataObject):
    section, stream = data.draw(streams())
    _run(data.draw(corrupted(section)), stream)
