"""Trace-processing engine (the ``run`` command's library core).

Reads the trace information (MGIC record plus trace records) from an
executable section, a JSON export, or raw section bytes, then decodes a
stream of trace events read through the injected *istream* callable and
writes the formatted traces through the injected *ostream* callable.

Each event is decoded in two stages: its bytes are first split into a byte
tree according to the type tree of its trace record
(:mod:`emtrace.stream.framing`), which is then turned into Python values by
the decoders registered for the type ids (:mod:`emtrace.stream.decoding`).

All failures are returned as :class:`RunReport` issues (and optionally passed
to an *issue_sink* as they occur) — this module never prints and never raises
for input problems.
"""

from __future__ import annotations

import os
import sys
from collections.abc import Callable
from dataclasses import dataclass
from functools import partial
from pathlib import Path
from typing import Any, Literal

from ..errors import Issue, Report
from ..exe._lief import find_section_bytes, parse_binary
from ..hexdump import context_hexdump
from ..section.export import load_export
from ..section.loading import TraceInfoFormat, load_format_info
from ..section.records import (
    MgicRecord,
    ParsedRecord,
    TraceRecord,
    find_mgic,
    parse_record,
)
from .cobs import cobs_get_frame
from .decoding import DecodedArgs, TypeRegistry, default_registry
from .formatters import FORMATTERS
from .framing import ByteReader, FramedEvent, dump_byte_tree, frame_args

__all__ = ["IssueSink", "RunReport", "default_ostream", "emtrace"]

IssueSink = Callable[[Issue], None]


@dataclass
class RunReport(Report):
    """Outcome of processing a trace stream (Tier 2)."""

    #: Number of traces written to the output.
    records: int = 0
    #: Number of traces not written because they contained undecodable values.
    dropped: int = 0
    #: Captured output (only in test mode, when *test_section_name* is set).
    captured: bytes | None = None
    #: Expected output (only in test mode).
    expected: bytes | None = None


def default_ostream(b: bytes) -> None:
    """Write *b* to stdout (the default output sink)."""
    _ = sys.stdout.buffer.write(b)
    return sys.stdout.buffer.flush()


def _read_fully(read: Callable[[int], bytes]) -> Callable[[int], bytes]:
    """Wrap *read* so that it only returns fewer than n bytes at the end of input.

    Some sources (e.g. sockets) return fewer bytes than requested whenever
    less data is available at the moment.
    """

    def wrapped(n: int) -> bytes:
        chunks: list[bytes] = []
        got = 0
        while got < n:
            chunk = read(n - got)
            if not chunk:
                break
            chunks.append(chunk)
            got += len(chunk)
        return b"".join(chunks)

    return wrapped


def _describe_record(record: TraceRecord) -> str:
    return f"trace point {record.fmt_string!r} at {record.location}"


class _TraceInfo:
    """The MGIC record plus on-demand access to the trace records."""

    def __init__(
        self,
        mgic: MgicRecord,
        section: bytes | None,
        records: dict[int, TraceRecord] | None = None,
    ) -> None:
        self.mgic: MgicRecord = mgic
        self._section: bytes | None = section
        self._exported: dict[int, TraceRecord] | None = records
        self._parsed: dict[int, ParsedRecord] = {}

    def lookup(self, section_offset: int) -> TraceRecord | list[Issue]:
        """The trace record at *section_offset*, or the issues preventing its use."""
        where = f"record at section offset {section_offset:#x}"
        if self._exported is not None:
            record = self._exported.get(section_offset)
            if record is None:
                return [Issue.error(f"no trace point in the JSON export at {where}")]
            return record
        assert self._section is not None
        if not 0 <= section_offset < len(self._section):
            return [
                Issue.error(
                    f"{where} is outside of the section (size {len(self._section):#x})"
                )
            ]
        parsed = self._parsed.get(section_offset)
        if parsed is None:
            parsed = parse_record(self._section, section_offset)
            self._parsed[section_offset] = parsed
        if not isinstance(parsed.value, TraceRecord):
            if parsed.framing is not None and parsed.ok:
                return [Issue.error(f"{parsed.label} is not a trace record")]
            return parsed.errors or [Issue.error(f"no trace record at {where}")]
        if not parsed.ok:
            return parsed.errors
        return parsed.value


def _load_trace_info(
    payload: dict[str, Any] | bytes, issues: _Issues
) -> _TraceInfo | None:
    if isinstance(payload, dict):
        loaded = load_export(payload)
        issues.extend(loaded.errors)
        if loaded.mgic is None:
            return None
        mgic, section, records = loaded.mgic, None, loaded.records
    else:
        section = bytes(payload)
        parsed = find_mgic(section)
        if parsed is None or not isinstance(parsed.value, MgicRecord):
            issues.fail("Failed to find or parse MGIC record in section data.")
            if parsed is not None:
                issues.extend(parsed.errors)
            return None
        if not parsed.ok:
            issues.extend(parsed.errors)
            return None
        mgic, records = parsed.value, None

    if mgic.ptr_size == 0:
        issues.fail("MGIC record: sizeof(emt_ptr_t) is 0.")
        return None
    if mgic.encoding_id not in (0, 1, 2):
        issues.fail(f"Unknown encoding: {mgic.encoding_id}")
        return None
    return _TraceInfo(mgic, section, records)


class _Output:
    """Writes formatted traces, optionally prefixed with their source location."""

    def __init__(
        self,
        ostream: Callable[[bytes], Any],
        with_src_loc: Literal["none", "absolute", "relative"],
    ) -> None:
        self.ostream: Callable[[bytes], Any] = ostream
        self.with_src_loc: Literal["none", "absolute", "relative"] = with_src_loc
        self.min_path_length: int = 0
        self.new_line_missing: bool = True

    def write(self, formatted: str, record: TraceRecord) -> None:
        if self.with_src_loc == "none":
            _ = self.ostream(formatted.encode("utf-8"))
            return
        if self.with_src_loc == "absolute":
            path = record.file
        else:
            path = os.path.relpath(record.file, os.getcwd())

        location_string = f"{path}:{record.line}"
        self.min_path_length = max(self.min_path_length, len(location_string))
        location_string = location_string.ljust(self.min_path_length)
        if self.new_line_missing:
            _ = self.ostream(f"{location_string}: ".encode())
            location_missing = False
        else:
            location_missing = True

        lines = formatted.split("\n")
        self.new_line_missing = False
        if lines[-1] == "":
            lines = lines[:-1]
            self.new_line_missing = True

        for i, line in enumerate(lines):
            if i == 0:
                pass
            elif location_missing and i == 1:
                _ = self.ostream(f"\n{location_string}: ".encode())
            else:
                _ = self.ostream(("\n" + " " * (2 + self.min_path_length)).encode())
            _ = self.ostream(line.encode("utf-8"))

        if self.new_line_missing:
            _ = self.ostream(b"\n")


class _Issues:
    """Reports issues, collapsing repetitions of the same issue."""

    def __init__(self, report: RunReport, sink: IssueSink | None) -> None:
        self.report: RunReport = report
        self.sink: IssueSink | None = sink
        self.seen: dict[tuple[str, str], int] = {}

    def emit(self, issue: Issue, dedup_key: str | None = None) -> None:
        if dedup_key is not None:
            key = (issue.severity, dedup_key)
            count = self.seen.get(key, 0)
            self.seen[key] = count + 1
            if count:
                return
        self.report.add(issue)
        if self.sink is not None:
            self.sink(issue)

    def fail(self, message: str) -> None:
        self.emit(Issue.error(message))

    def extend(self, issues: list[Issue]) -> None:
        for issue in issues:
            self.emit(issue)

    def finish(self) -> None:
        repeated = sum(count - 1 for count in self.seen.values() if count > 1)
        if repeated:
            self.emit(
                Issue.warning(
                    f"{repeated} repetition(s) of the issues above were not shown."
                )
            )


def emtrace(
    trace_info_file: Path,
    file_format: TraceInfoFormat = "auto",
    istream: Callable[[int], bytes] = sys.stdin.buffer.read,
    ostream: Callable[[bytes], Any] = default_ostream,
    section_name: str = ".emtrace",
    with_src_loc: Literal["none", "absolute", "relative"] = "none",
    src_hyperlinks: bool = False,
    debug_script: bool = False,
    test_section_name: str | None = None,
    trace_sink: Callable[[str], Any] | None = None,
    *,
    registry: TypeRegistry | None = None,
    drop_undecodable: bool = False,
    issue_sink: IssueSink | None = None,
) -> RunReport:
    """Process a trace stream; return a :class:`RunReport` (Tier 2).

    *trace_sink* receives internal debug lines when *debug_script* is set
    (defaults to writing them to stderr). *registry* holds the decoders used
    for the type ids (defaults to :func:`default_registry`). Traces containing
    values that could not be decoded are written with placeholders in place
    of those values, or skipped entirely when *drop_undecodable* is set.
    *issue_sink* receives every issue as soon as it occurs (repetitions of an
    issue are collapsed).
    """
    report = RunReport()
    issues = _Issues(report, issue_sink)
    if registry is None:
        registry = default_registry()

    def trace(*args: Any) -> None:
        if debug_script:
            text = " ".join(
                "\n".join("[trace] " + line for line in str(arg).split("\n"))
                for arg in args
            )
            if trace_sink is not None:
                trace_sink(text)
            else:
                print(text, file=sys.stderr)

    captured_output: bytearray | None = None
    if test_section_name is not None:
        captured_output = bytearray()
        ostream = captured_output.extend

    expected_output_from_file: bytes | None = None
    if test_section_name is not None and Path(test_section_name).is_file():
        expected_output_from_file = Path(test_section_name).read_bytes().rstrip(b"\x00")

    trace(
        f"Main args: {trace_info_file=} {section_name=} {with_src_loc=} "
        f"{src_hyperlinks=} {test_section_name=} {drop_undecodable=}"
    )

    loaded = load_format_info(trace_info_file, file_format, section_name)
    issues.extend(loaded.errors)
    issues.extend(loaded.warnings)
    if loaded.payload is None:
        if not report.errors:
            issues.fail(
                f"Unable to get section {section_name} from file {trace_info_file} "
                f"as {file_format}."
            )
        return report

    expected_output: bytes | None = None
    if test_section_name is not None:
        if expected_output_from_file is not None:
            expected_output = expected_output_from_file
        else:
            section_bytes = find_section_bytes(
                trace_info_file, test_section_name, partial(parse_binary, "exe")
            )
            if section_bytes is not None:
                expected_output = section_bytes.rstrip(b"\x00")
        trace(f"Expected test output: {expected_output}")
        if expected_output is None:
            issues.fail(f"Section '{test_section_name}' not found in {trace_info_file}")
            return report

    info = _load_trace_info(loaded.payload, issues)
    if info is None:
        return report
    mgic = info.mgic
    trace(
        f"MGIC: version={mgic.version[0]}.{mgic.version[1]} ptr_size={mgic.ptr_size} "
        f"alignment_power={mgic.alignment_power} encoding_id={mgic.encoding_id} "
        f"record_start={mgic.record_start:#x}"
    )

    cobs = mgic.encoding_id != 0
    passthrough = ostream if mgic.encoding_id == 2 else None
    istream = _read_fully(istream)
    ptr_size = mgic.ptr_size
    scale = 2**mgic.alignment_power

    def next_reader() -> ByteReader | None:
        """A reader over the next event, or None at the end of the stream."""
        if not cobs:
            reader = ByteReader(istream)
            return reader if reader.ensure(1) else None
        frame = cobs_get_frame(istream, passthrough)
        if frame is None:
            return None
        trace(f"COBS frame: {frame.hex(' ')}")
        return ByteReader(frame)

    # The first event is the runtime address of the MGIC record (always
    # little-endian, like all record pointers in the stream).
    first = next_reader()
    if first is None:
        issues.fail("Stream ended before the address of the MGIC record was read.")
        return report
    magic_bytes = first.take(ptr_size)
    if magic_bytes is None:
        issues.fail(
            f"Stream ended in the middle of the address of the MGIC record "
            f"(got {first.buffer.hex(' ')})."
        )
        return report
    aslr_offset = mgic.record_start - int.from_bytes(magic_bytes, "little") * scale
    trace(f"MGIC runtime address bytes: {magic_bytes.hex(' ')}")

    output = _Output(ostream, with_src_loc)
    event_index = 0
    while True:
        reader = next_reader()
        if reader is None:
            trace("End of stream reached.")
            break
        event_index += 1
        where = f"event {event_index}"
        pointer = reader.take(ptr_size)
        if pointer is None:
            issues.emit(
                Issue.error(
                    f"{where}: stream ended in the middle of a record pointer "
                    f"(got {reader.buffer.hex(' ')})."
                )
            )
            if cobs:
                continue
            break
        section_offset = int.from_bytes(pointer, "little") * scale + aslr_offset
        trace(
            f"{where}: pointer {pointer.hex(' ')} → section offset {section_offset:#x}"
        )

        record = info.lookup(section_offset)
        if isinstance(record, list):
            for issue in record:
                issues.emit(
                    Issue.error(
                        f"{where}: cannot decode the trace at section offset "
                        f"{section_offset:#x}: {issue.describe()}"
                    ),
                    dedup_key=f"{section_offset}:{issue.describe()}",
                )
            if cobs:
                continue
            break

        event = frame_args(record, reader)
        if debug_script:
            trace(f"{where}: {_describe_record(record)}")
            for line in dump_byte_tree(event):
                trace(f"  {line}")
        if event.error is not None:
            issues.emit(_framing_issue(where, record, event))
            if cobs:
                continue
            break

        decoded = registry.decode_args(event, record)
        trace(f"{where}: values {decoded.values!r}")
        _report_decode_problems(issues, record, decoded)

        formatter = FORMATTERS.get(record.formatter)
        try:
            if formatter is None:
                raise ValueError(f"unknown formatter id {record.formatter}")
            formatted = formatter(record.fmt_string, decoded.values)
        except Exception as e:  # noqa: BLE001  # format strings are arbitrary input
            issues.emit(
                Issue.error(
                    f"Failed to format {_describe_record(record)} with arguments "
                    f"{decoded.values!r}: {type(e).__name__}: {e}"
                ),
                dedup_key=f"format:{record.record_start}:{type(e).__name__}:{e}",
            )
            formatted = None

        if formatted is not None:
            if drop_undecodable and decoded.has_placeholders:
                report.dropped += 1
            else:
                output.write(formatted, record)
                report.records += 1

        if cobs and reader.remaining:
            leftover = bytes(reader.buffer[reader.pos :])
            issues.emit(
                Issue.error(
                    f"{where}: {len(leftover)} leftover byte(s) in the COBS frame after "
                    f"decoding {_describe_record(record)}: {leftover.hex(' ')}"
                ),
                dedup_key=f"leftover:{record.record_start}",
            )

    issues.finish()
    if test_section_name is not None:
        assert expected_output is not None
        assert captured_output is not None
        report.captured = bytes(captured_output)
        report.expected = expected_output
    return report


def _framing_issue(where: str, record: TraceRecord, event: FramedEvent) -> Issue:
    error = event.error
    assert error is not None
    lines = [
        f"{where}: the bytes of {_describe_record(record)} don't match its trace "
        f"record: {' › '.join(error.path)}: {error.message}"
    ]
    if event.args:
        lines.append(f"  successfully split arguments: {len(event.args)}")
    lines += context_hexdump(event.data, error.offset, error.length, indent="  ")
    return Issue.error("\n".join(lines))


def _report_decode_problems(
    issues: _Issues, record: TraceRecord, decoded: DecodedArgs
) -> None:
    for problem in decoded.problems:
        suffix = " (shown as placeholder)" if problem.placeholder else ""
        issues.emit(
            Issue.warning(
                f"{_describe_record(record)} › {' › '.join(problem.path)}: "
                f"{problem.message} (type id {problem.type_id!r}){suffix}"
            ),
            dedup_key=f"decode:{record.record_start}:{problem.path}:{problem.message}",
        )
