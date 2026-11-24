"""Parsing of the records in an emtrace section (see ``TRACE_FORMAT.md``).

This is the single record parser used by every consumer (``check``,
``inspect``, ``export``, and ``run``). It never raises for malformed input:
every problem is recorded as an :class:`~emtrace.errors.Issue` carrying the
byte offset and length it refers to, and a breadcrumb path describing what
was being parsed (e.g. ``TRCE record at 0x100 › arg 1 › child 0 › type id``).

Problems come in two severities:

- errors: the record violates the spec. If the record cannot be parsed any
  further, :attr:`ParsedRecord.value` is ``None``; otherwise parsing continues
  so that all problems are reported, but the record must not be used for
  decoding (:attr:`ParsedRecord.ok` is ``False``).
- warnings: the record is usable, but something is off (e.g. a string that is
  not valid UTF-8 and was decoded with replacement characters).

Every field read (and every pointed-to string) is additionally recorded as a
:class:`Span`, which is used for annotated debug dumps of records.
"""

from __future__ import annotations

from collections.abc import Generator, Iterator
from contextlib import contextmanager
from dataclasses import dataclass, field
from typing import Any, Literal, NoReturn

from ..errors import Issue, Severity
from .type_tree import FLAG_KINDS, Size, TypeNode

Byteorder = Literal["little", "big"]

#: Every record starts with these bytes.
FRAMING_PREFIX = b"EMT"
#: 'EMT' + 7 single-byte numbers + the 4-byte record type.
FRAMING_SIZE = 14

#: Record type versions this parser can decode. Record types are versioned
#: independently; a record is decodable when its major version matches a known
#: one and its minor version is at least the known minor version.
KNOWN_VERSIONS: dict[bytes, list[tuple[int, int]]] = {
    b"MGIC": [(1, 0)],
    b"TRCE": [(1, 0), (2, 0), (3, 0)],
}
KNOWN_RECORD_TYPES: list[bytes] = list(KNOWN_VERSIONS)

#: Versions of the runtime data format (the bytes emitted per trace event).
KNOWN_RUNTIME_DATA_VERSIONS: list[tuple[int, int]] = [(1, 0), (2, 0), (3, 0)]

#: 32-byte magic constant at the start of the MGIC record's payload.
MAGIC_CONSTANT: bytes = bytes.fromhex(
    "d197f522d9269fd1ad703392f659dfd0fbecbd60971325e89201b25a385d9ec7"
)
#: magic constant + sizeof(emt_ptr_t) + alignment power + encoding id.
MGIC_PAYLOAD_SIZE = 35

VALID_ENCODING_IDS = (0, 1, 2)
MAX_ALIGNMENT_POWER = 63
#: Limit on the nesting depth of type trees (a parser limit, not a spec one).
MAX_NESTING_DEPTH = 64


def supported_version(
    known: list[tuple[int, int]], major: int, minor: int
) -> tuple[int, int] | None:
    """The highest version in *known* that can decode version *major*.*minor*.

    Returns ``None`` when no known version is compatible.
    """
    best: tuple[int, int] | None = None
    for known_major, known_minor in known:
        if (
            known_major == major
            and minor >= known_minor
            and (best is None or best[1] < known_minor)
        ):
            best = (known_major, known_minor)
    return best


def detect_byteorder(b: bytes) -> Literal["little", "big", "unknown"] | None:
    """Detect endianness from a size_t-shaped byte-order probe value.

    The probe value is ``0x..0706050403020100`` truncated to ``len(b)`` bytes
    and stored in the record's byte order. Returns ``"little"`` or ``"big"``
    on success, ``"unknown"`` if the bytes are a permutation of the probe
    bytes in neither order, or ``None`` if *b* is empty, longer than 256
    bytes, or not a permutation of the probe bytes at all.
    """
    if len(b) > 256 or len(b) < 1:
        return None
    if sorted(b) != list(range(len(b))):
        return None
    if list(b) == list(range(len(b))):
        return "little"
    if list(b) == list(range(len(b) - 1, -1, -1)):
        return "big"
    return "unknown"


@dataclass(frozen=True)
class Span:
    """A piece of a record that was read by the parser (for debug dumps)."""

    start: int
    end: int
    label: str
    value: str = ""


@dataclass(frozen=True)
class Framing:
    """The generic framing every record starts with (absolute offsets)."""

    record_start: int
    record_size: int
    record_type: bytes
    type_offset: int
    size_offset: int
    payload_offset: int
    endianness_offset: int
    size_t_size: int
    major: int
    minor: int
    byteorder: Byteorder

    @property
    def record_end(self) -> int:
        return self.record_start + self.record_size

    @property
    def version(self) -> tuple[int, int]:
        return (self.major, self.minor)

    @property
    def type_name(self) -> str:
        """The record type as text (non-ASCII bytes escaped)."""
        return self.record_type.decode("ascii", "backslashreplace")

    @property
    def newer_minor_than_known(self) -> bool:
        """Whether the record's minor version is higher than any known one."""
        known = supported_version(
            KNOWN_VERSIONS.get(self.record_type, []), self.major, self.minor
        )
        return known is not None and known[1] < self.minor


@dataclass(frozen=True)
class MgicRecord:
    """Parsed contents of the MGIC record."""

    record_start: int
    magic_offset: int
    size_t_size: int
    byteorder: Byteorder
    version: tuple[int, int]
    ptr_size: int
    alignment_power: int
    encoding_id: int

    def to_dict(self) -> dict[str, Any]:
        return {
            "record_start": self.record_start,
            "magic_offset": self.magic_offset,
            "size_t_size": self.size_t_size,
            "byteorder": self.byteorder,
            "version": list(self.version),
            "ptr_size": self.ptr_size,
            "alignment_power": self.alignment_power,
            "encoding_id": self.encoding_id,
        }

    @staticmethod
    def from_dict(data: dict[str, Any]) -> MgicRecord:
        byteorder = data["byteorder"]
        if byteorder not in ("little", "big"):
            raise ValueError(f"invalid byteorder {byteorder!r}")
        major, minor = data["version"]
        return MgicRecord(
            record_start=int(data["record_start"]),
            magic_offset=int(data["magic_offset"]),
            size_t_size=int(data["size_t_size"]),
            byteorder=byteorder,
            version=(int(major), int(minor)),
            ptr_size=int(data["ptr_size"]),
            alignment_power=int(data["alignment_power"]),
            encoding_id=int(data["encoding_id"]),
        )


@dataclass
class TraceRecord:
    """Parsed contents of a TRCE record: everything needed to decode a trace."""

    record_start: int
    fmt_string: str
    formatter: int
    args: list[TypeNode]
    file: str
    line: int
    version: tuple[int, int]
    runtime_data_version: tuple[int, int]
    byteorder: Byteorder
    size_t_size: int

    @property
    def location(self) -> str:
        return f"{self.file}:{self.line}"

    def to_dict(self) -> dict[str, Any]:
        return {
            "fmt_string": self.fmt_string,
            "formatter": self.formatter,
            "file": self.file,
            "line": self.line,
            "version": list(self.version),
            "runtime_data_version": list(self.runtime_data_version),
            "byteorder": self.byteorder,
            "size_t_size": self.size_t_size,
            "args": [arg.to_dict() for arg in self.args],
        }

    @staticmethod
    def from_dict(data: dict[str, Any], record_start: int) -> TraceRecord:
        byteorder = data["byteorder"]
        if byteorder not in ("little", "big"):
            raise ValueError(f"invalid byteorder {byteorder!r}")
        major, minor = data["version"]
        rt_major, rt_minor = data["runtime_data_version"]
        return TraceRecord(
            record_start=record_start,
            fmt_string=str(data["fmt_string"]),
            formatter=int(data["formatter"]),
            args=[TypeNode.from_dict(arg) for arg in data["args"]],
            file=str(data["file"]),
            line=int(data["line"]),
            version=(int(major), int(minor)),
            runtime_data_version=(int(rt_major), int(rt_minor)),
            byteorder=byteorder,
            size_t_size=int(data["size_t_size"]),
        )


@dataclass
class ParsedRecord:
    """The outcome of parsing the record starting at ``start``.

    ``framing`` is ``None`` when the bytes at ``start`` are not a valid record
    framing; ``value`` is ``None`` when the record could not be parsed (see
    ``issues`` for why).
    """

    start: int
    framing: Framing | None = None
    value: MgicRecord | TraceRecord | None = None
    issues: list[Issue] = field(default_factory=list)
    spans: list[Span] = field(default_factory=list)

    @property
    def errors(self) -> list[Issue]:
        return [i for i in self.issues if i.severity == "error"]

    @property
    def warnings(self) -> list[Issue]:
        return [i for i in self.issues if i.severity == "warning"]

    @property
    def ok(self) -> bool:
        """Whether the record was parsed and is free of errors."""
        return self.value is not None and not self.errors

    @property
    def next_offset(self) -> int:
        """Where a scan for the next record should continue."""
        if self.framing is not None:
            return self.framing.record_end
        return self.start + 1

    @property
    def label(self) -> str:
        if self.framing is None:
            return f"record at {self.start:#x}"
        return f"{self.framing.type_name} record at {self.start:#x}"


class _Fatal(Exception):
    """The record cannot be parsed any further (the issue is already recorded)."""


class _RecordReader:
    """Bounds-checked reads of size_t values and strings inside one record."""

    def __init__(self, data: bytes, framing: Framing, parsed: ParsedRecord) -> None:
        self.data: bytes = data
        self.framing: Framing = framing
        self.parsed: ParsedRecord = parsed
        self.start: int = framing.record_start
        self.end: int = framing.record_end
        self.sz: int = framing.size_t_size
        self.byteorder: Byteorder = framing.byteorder
        self.pos: int = framing.payload_offset
        self.path: list[str] = [parsed.label]

    @contextmanager
    def scope(self, label: str) -> Generator[None]:
        self.path.append(label)
        try:
            yield
        finally:
            _ = self.path.pop()

    def issue(
        self,
        severity: Severity,
        message: str,
        offset: int | None,
        length: int,
        what: str | None = None,
    ) -> None:
        path = (*self.path, what) if what else tuple(self.path)
        self.parsed.issues.append(Issue(severity, message, offset, length, path))

    def error(
        self, message: str, offset: int | None, length: int, what: str | None = None
    ) -> None:
        self.issue("error", message, offset, length, what)

    def warn(
        self, message: str, offset: int | None, length: int, what: str | None = None
    ) -> None:
        self.issue("warning", message, offset, length, what)

    def fatal(
        self, message: str, offset: int | None, length: int, what: str | None = None
    ) -> NoReturn:
        self.error(message, offset, length, what)
        raise _Fatal

    def span(self, start: int, end: int, what: str, value: str) -> None:
        label = " › ".join([*self.path[1:], what])
        self.parsed.spans.append(Span(start, end, label, value))

    def size_t(self, what: str) -> int:
        start = self.pos
        end = start + self.sz
        if end > self.end:
            self.fatal(
                f"{self.sz}-byte value at {start:#x} extends past the end of the "
                f"record at {self.end:#x}",
                start,
                max(0, self.end - start),
                what,
            )
        value = int.from_bytes(self.data[start:end], self.byteorder)
        self.pos = end
        self.span(start, end, what, str(value))
        return value

    def string_field(self, what: str) -> str:
        """Read a pointer (relative to the record start) to a C string."""
        pointer_start = self.pos
        rel = self.size_t(f"{what} pointer")
        abs_start = self.start + rel
        if abs_start >= self.end:
            self.error(
                f"pointer {rel:#x} points outside the record (record size "
                f"{self.end - self.start:#x})",
                pointer_start,
                self.sz,
                what,
            )
            return ""
        terminator = self.data.find(b"\x00", abs_start, self.end)
        if terminator < 0:
            self.error(
                f"string at {abs_start:#x} is not null-terminated within the record",
                abs_start,
                self.end - abs_start,
                what,
            )
            return ""
        raw = self.data[abs_start:terminator]
        try:
            text = raw.decode("utf-8")
        except UnicodeDecodeError as e:
            self.warn(
                f"string at {abs_start:#x} is not valid UTF-8 ({e.reason}); decoded "
                f"with replacement characters",
                abs_start + e.start,
                max(1, e.end - e.start),
                what,
            )
            text = raw.decode("utf-8", "replace")
        self.span(abs_start, terminator + 1, what, repr(text))
        return text


def _framing_error(
    parsed: ParsedRecord, message: str, offset: int, length: int
) -> None:
    parsed.issues.append(Issue.error(message, offset, length, (parsed.label,)))


def _parse_framing(data: bytes, offset: int, parsed: ParsedRecord) -> Framing | None:
    """Parse and validate the record framing at *offset*."""
    n = len(data)
    if data[offset : offset + 3] != FRAMING_PREFIX:
        _framing_error(parsed, "no 'EMT' prefix", offset, min(3, n - offset))
        return None
    if offset + FRAMING_SIZE > n:
        _framing_error(
            parsed,
            f"framing truncated: only {n - offset} of {FRAMING_SIZE} bytes left in "
            f"the section",
            offset,
            n - offset,
        )
        return None

    type_off, size_off, payload_off, endi_off, sz, major, minor = data[
        offset + 3 : offset + 10
    ]
    spans = [
        Span(offset, offset + 3, "'EMT' prefix", ""),
        Span(offset + 3, offset + 4, "type tag offset", str(type_off)),
        Span(offset + 4, offset + 5, "record size offset", str(size_off)),
        Span(offset + 5, offset + 6, "payload offset", str(payload_off)),
        Span(offset + 6, offset + 7, "endianness probe offset", str(endi_off)),
        Span(offset + 7, offset + 8, "size_t width", str(sz)),
        Span(offset + 8, offset + 9, "major version", str(major)),
        Span(offset + 9, offset + 10, "minor version", str(minor)),
    ]

    if sz == 0:
        _framing_error(parsed, "size_t width is 0", offset + 7, 1)
        return None

    probe_start = offset + endi_off
    if probe_start + sz > n:
        _framing_error(
            parsed,
            f"endianness probe at {probe_start:#x} ({sz} bytes) extends past the end "
            f"of the section",
            offset + 6,
            1,
        )
        return None
    probe = data[probe_start : probe_start + sz]
    byteorder = detect_byteorder(probe)
    if byteorder != "little" and byteorder != "big":
        _framing_error(
            parsed,
            f"endianness probe {probe.hex(' ')} is not a valid probe for a {sz}-byte "
            f"size_t",
            probe_start,
            sz,
        )
        return None

    size_start = offset + size_off
    if size_start + sz > n:
        _framing_error(
            parsed,
            f"record size at {size_start:#x} ({sz} bytes) extends past the end of the "
            f"section",
            offset + 4,
            1,
        )
        return None
    record_size = int.from_bytes(data[size_start : size_start + sz], byteorder)
    if record_size < FRAMING_SIZE:
        _framing_error(
            parsed,
            f"record size {record_size} is smaller than the {FRAMING_SIZE}-byte framing",
            size_start,
            sz,
        )
        return None
    if offset + record_size > n:
        _framing_error(
            parsed,
            f"record size {record_size:#x} extends past the end of the section "
            f"({n - offset:#x} bytes left)",
            size_start,
            sz,
        )
        return None
    record_end = offset + record_size

    type_start = offset + type_off
    pieces = (
        ("type tag", type_start, 4, offset + 3),
        ("record size", size_start, sz, offset + 4),
        ("endianness probe", probe_start, sz, offset + 6),
        ("payload", offset + payload_off, 0, offset + 5),
    )
    for name, start, length, pointer in pieces:
        if start + length > record_end:
            _framing_error(
                parsed,
                f"{name} at {start:#x} lies outside the record (which ends at "
                f"{record_end:#x})",
                pointer,
                1,
            )
            return None

    framing = Framing(
        record_start=offset,
        record_size=record_size,
        record_type=data[type_start : type_start + 4],
        type_offset=type_start,
        size_offset=size_start,
        payload_offset=offset + payload_off,
        endianness_offset=probe_start,
        size_t_size=sz,
        major=major,
        minor=minor,
        byteorder=byteorder,
    )
    parsed.framing = framing
    spans += [
        Span(type_start, type_start + 4, "type tag", repr(framing.type_name)),
        Span(size_start, size_start + sz, "record size", str(record_size)),
        Span(probe_start, probe_start + sz, "endianness probe", byteorder),
    ]
    parsed.spans.extend(spans)

    # The pointed-to pieces must not overlap the framing (except for the type
    # tag, which is part of it) or each other. The payload's length is not
    # known here, so it is treated as one byte long.
    for name, start, pointer in (
        ("record size", size_start, offset + 4),
        ("payload", framing.payload_offset, offset + 5),
        ("endianness probe", probe_start, offset + 6),
    ):
        if start < offset + FRAMING_SIZE:
            _framing_error(
                parsed,
                f"{name} starts at {start:#x}, inside the {FRAMING_SIZE}-byte framing",
                pointer,
                1,
            )
    ranges = [
        ("type tag", type_start, type_start + 4),
        ("record size", size_start, size_start + sz),
        ("payload", framing.payload_offset, framing.payload_offset + 1),
        ("endianness probe", probe_start, probe_start + sz),
    ]
    for i, (name_a, a_start, a_end) in enumerate(ranges):
        for name_b, b_start, b_end in ranges[i + 1 :]:
            if a_start < b_end and b_start < a_end:
                _framing_error(
                    parsed,
                    f"{name_a} and {name_b} overlap",
                    max(a_start, b_start),
                    min(a_end, b_end) - max(a_start, b_start),
                )
    return framing


def _parse_mgic(r: _RecordReader) -> MgicRecord:
    start = r.framing.payload_offset
    left = r.end - start
    if left < MGIC_PAYLOAD_SIZE:
        r.fatal(
            f"payload truncated: {MGIC_PAYLOAD_SIZE} bytes needed, {left} left in the "
            f"record",
            start,
            left,
        )
    if r.data[start : start + 32] != MAGIC_CONSTANT:
        r.fatal("magic constant mismatch", start, 32)
    ptr_size, alignment_power, encoding_id = r.data[start + 32 : start + 35]
    r.span(start, start + 32, "magic constant", "")
    r.span(start + 32, start + 33, "sizeof(emt_ptr_t)", str(ptr_size))
    r.span(start + 33, start + 34, "alignment power", str(alignment_power))
    r.span(start + 34, start + 35, "encoding id", str(encoding_id))
    if ptr_size == 0:
        r.error("sizeof(emt_ptr_t) is 0", start + 32, 1)
    if alignment_power > MAX_ALIGNMENT_POWER:
        r.error(
            f"alignment power {alignment_power} is unreasonably large", start + 33, 1
        )
    if encoding_id not in VALID_ENCODING_IDS:
        r.error(
            f"unknown encoding_id={encoding_id} (expected one of "
            f"{list(VALID_ENCODING_IDS)})",
            start + 34,
            1,
        )
    return MgicRecord(
        record_start=r.start,
        magic_offset=start,
        size_t_size=r.sz,
        byteorder=r.byteorder,
        version=r.framing.version,
        ptr_size=ptr_size,
        alignment_power=alignment_power,
        encoding_id=encoding_id,
    )


def _parse_type_entry(
    r: _RecordReader, named: bool, has_padding: bool, padding_allowed: bool
) -> tuple[TypeNode, int]:
    """Read one type entry; returns the node and its number of children."""
    name = r.string_field("name") if named else ""
    type_id = r.string_field("type id")
    size_pos = r.pos
    size = r.size_t("size")
    flag_pos = r.pos
    flag = r.size_t("size flag")
    padding_pos = r.pos
    padding = r.size_t("padding") if has_padding else 0
    num_children = r.size_t("child count")

    kind = FLAG_KINDS.get(flag)
    if kind is None:
        r.error(
            f"invalid flag={flag} (expected one of 0 = static, 1 = null-terminated, "
            f"2 = length-prefixed)",
            flag_pos,
            r.sz,
            "size flag",
        )
        kind = "fixed"
    elif kind == "null_terminated" and size == 0:
        r.error(
            "null-terminated value with a 0-byte terminator", size_pos, r.sz, "size"
        )
    if padding and not padding_allowed:
        r.error(
            f"padding {padding} requires runtime data version 3.0 or newer",
            padding_pos,
            r.sz,
            "padding",
        )
    return TypeNode(type_id, Size(kind, size), padding, name), num_children


def _parse_type_tree(
    r: _RecordReader, has_padding: bool, padding_allowed: bool
) -> TypeNode:
    """Read a type tree serialized in pre-order (iteratively, depth-limited)."""
    root, num_children = _parse_type_entry(r, False, has_padding, padding_allowed)
    base = len(r.path)
    # One [node, number of children, children read so far] entry per open subtree.
    stack: list[list[Any]] = [[root, num_children, 0]] if num_children else []
    try:
        while stack:
            top = stack[-1]
            if top[2] == top[1]:
                _ = stack.pop()
                continue
            top[2] += 1
            r.path[base:] = [f"child {entry[2] - 1}" for entry in stack]
            child, num_children = _parse_type_entry(
                r, True, has_padding, padding_allowed
            )
            top[0].children.append(child)
            if num_children:
                if len(stack) >= MAX_NESTING_DEPTH:
                    r.fatal(
                        f"type tree is nested deeper than {MAX_NESTING_DEPTH} levels "
                        f"(a limit of this parser)",
                        r.pos - r.sz,
                        r.sz,
                    )
                stack.append([child, num_children, 0])
    finally:
        del r.path[base:]
    return root


def _parse_trce(r: _RecordReader, version: tuple[int, int]) -> TraceRecord:
    runtime_version: tuple[int, int]
    if version[0] >= 2:
        rt_pos = r.pos
        rt_major = r.size_t("runtime data major version")
        rt_minor = r.size_t("runtime data minor version")
        supported = supported_version(KNOWN_RUNTIME_DATA_VERSIONS, rt_major, rt_minor)
        if supported is not None:
            runtime_version = supported
        else:
            r.error(
                f"unsupported runtime data version {rt_major}.{rt_minor} (this "
                f"version of emtrace understands {KNOWN_RUNTIME_DATA_VERSIONS})",
                rt_pos,
                2 * r.sz,
            )
            runtime_version = (rt_major, rt_minor)
    else:
        runtime_version = (1, 0)

    num_args = r.size_t("argument count")
    fmt_string = r.string_field("format string")
    args: list[TypeNode] = []
    for i in range(num_args):
        with r.scope(f"arg {i}"):
            args.append(
                _parse_type_tree(
                    r,
                    has_padding=version[0] >= 3,
                    padding_allowed=runtime_version[0] >= 3,
                )
            )
    formatter = r.size_t("formatter id")
    file = r.string_field("source file")
    line = r.size_t("source line")
    return TraceRecord(
        record_start=r.start,
        fmt_string=fmt_string,
        formatter=formatter,
        args=args,
        file=file,
        line=line,
        version=r.framing.version,
        runtime_data_version=runtime_version,
        byteorder=r.byteorder,
        size_t_size=r.sz,
    )


def parse_record(data: bytes, offset: int) -> ParsedRecord:
    """Parse the record starting at *offset* in the section bytes *data*."""
    parsed = ParsedRecord(start=offset)
    framing = _parse_framing(data, offset, parsed)
    if framing is None:
        return parsed

    path = (parsed.label,)
    known = KNOWN_VERSIONS.get(framing.record_type)
    if known is None:
        parsed.issues.append(
            Issue.error(
                f"unknown record type {framing.record_type!r} (not known to this "
                f"version of emtrace)",
                framing.type_offset,
                4,
                path,
            )
        )
        return parsed
    version = supported_version(known, framing.major, framing.minor)
    if version is None:
        parsed.issues.append(
            Issue.error(
                f"unsupported record version {framing.major}.{framing.minor}; this "
                f"version of emtrace understands {framing.type_name} records of "
                f"versions {known} (and higher minor versions of the same major)",
                offset + 8,
                2,
                path,
            )
        )
        return parsed

    reader = _RecordReader(data, framing, parsed)
    try:
        if framing.record_type == b"MGIC":
            parsed.value = _parse_mgic(reader)
        else:
            parsed.value = _parse_trce(reader, version)
    except _Fatal:
        parsed.value = None
    return parsed


def iter_records(data: bytes) -> Iterator[ParsedRecord]:
    """Parse every record in the section bytes *data*, in order.

    Every ``EMT`` sequence outside of a valid record starts a (possibly
    invalid) record — the spec reserves the sequence for that purpose — so
    invalid framings are yielded as records with errors, and the scan resumes
    right after their ``EMT`` prefix.
    """
    data = bytes(data)
    pos = 0
    while (start := data.find(FRAMING_PREFIX, pos)) >= 0:
        parsed = parse_record(data, start)
        yield parsed
        pos = parsed.next_offset


def find_mgic(data: bytes) -> ParsedRecord | None:
    """Locate and parse the MGIC record via its magic constant.

    For every occurrence of the magic constant, the framing whose payload
    offset points exactly at it is searched backwards. Returns the first
    record found that way (which may still carry errors), or ``None``.
    """
    data = bytes(data)
    pos = 0
    while (magic := data.find(MAGIC_CONSTANT, pos)) >= 0:
        # The payload offset is a single byte, so the framing is at most 255
        # bytes before the payload.
        for candidate in range(magic - 1, max(magic - 256, -1), -1):
            if (
                data.startswith(FRAMING_PREFIX, candidate)
                and candidate + 5 < len(data)
                and candidate + data[candidate + 5] == magic
            ):
                parsed = parse_record(data, candidate)
                if parsed.framing is not None and parsed.framing.record_type == b"MGIC":
                    return parsed
        pos = magic + 1
    return None


def dump_record(data: bytes, parsed: ParsedRecord) -> list[str]:
    """An annotated listing of every piece of a parsed record (for debugging)."""
    framing = parsed.framing
    if framing is None:
        header = f"{parsed.label}: invalid framing"
    else:
        header = (
            f"{parsed.label}: version {framing.major}.{framing.minor}, "
            f"{framing.record_size} bytes, {framing.size_t_size}-byte size_t, "
            f"{framing.byteorder} endian"
        )
    lines = [header]
    for span in sorted(parsed.spans, key=lambda s: (s.start, s.end)):
        raw = data[span.start : min(span.end, span.start + 8)].hex(" ")
        if span.end - span.start > 8:
            raw += " …"
        value = f" = {span.value}" if span.value else ""
        lines.append(
            f"  {span.start:#07x} [{span.end - span.start:>3}] {raw:<25} {span.label}{value}"
        )
    for issue in parsed.issues:
        lines.append(f"  {issue.severity}: {issue.describe()}")
    return lines
