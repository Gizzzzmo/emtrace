"""Trace-processing engine (the ``run`` command's library core).

Reads the format information (MGIC record plus trace-point records) from an
executable section, a JSON description, or raw section bytes, then decodes a
stream of trace events through the injected *istream* callable and formats
them through the injected *ostream* callable.

All failures are returned as :class:`RunReport` / :class:`FormatInfoResult`
issues — this module never prints and never raises for input problems
(internal control flow may still use exceptions, folded at the boundary).
"""

from __future__ import annotations

import json
import os
import sys
from dataclasses import dataclass
from functools import partial
from pathlib import Path
from typing import Any, Callable, Literal

from . import _lief
from ._lief import find_section_bytes, lief_missing_error, parse_binary
from .cobs import cobs_get_frame
from .errors import Report
from .format_info import FmtInfo
from .formatters import FORMATTERS
from .parser import Parser, translation_be, translation_le
from .record_parser import RecordParser, RecordVersionError
from .record_parser import detect_byteorder as detect_byteorder

__all__ = [
    "FormatInfoResult",
    "RunReport",
    "default_ostream",
    "detect_byteorder",
    "load_format_info",
]


@dataclass
class FormatInfoResult(Report):
    """Outcome of loading format information (Tier 2).

    ``payload`` is the loaded data: a JSON-derived dict, or raw section
    bytes. ``None`` with errors set means loading failed.
    """

    payload: dict[str, Any] | bytes | None = None


@dataclass
class RunReport(Report):
    """Outcome of processing a trace stream (Tier 2)."""

    records: int = 0
    #: Captured output (only in test mode, when *test_section_name* is set).
    captured: bytes | None = None
    #: Expected output (only in test mode).
    expected: bytes | None = None


def default_ostream(b: bytes) -> None:
    """Write *b* to stdout (the default output sink)."""
    _ = sys.stdout.buffer.write(b)
    return sys.stdout.buffer.flush()


def _parse_callable(
    file_format: str,
) -> Callable[[Path], Any | None] | None:
    """The binary parser for an explicit executable *file_format*."""
    if file_format in ("exe", "elf", "pe", "macho", "oat"):
        return partial(parse_binary, file_format)
    return None


def load_format_info(
    trace_info_file: Path,
    file_format: Literal["auto", "exe", "elf", "pe", "macho", "oat", "json", "binary"],
    section_name: str,
) -> FormatInfoResult:
    """Load format information from *trace_info_file* (Tier 2).

    ``auto`` tries JSON, then the embedded executable section, then raw
    binary — degrading gracefully when lief is not installed.
    """
    result = FormatInfoResult()
    if file_format == "auto":
        try:
            with open(trace_info_file) as f:
                result.payload = json.load(f)
                return result
        except json.JSONDecodeError:
            pass
        except UnicodeDecodeError:
            pass

        section_bytes = find_section_bytes(
            trace_info_file, section_name, partial(parse_binary, "exe")
        )
        if section_bytes is not None:
            result.payload = section_bytes
            return result
        try:
            result.payload = open(trace_info_file, "rb").read()
            return result
        except FileNotFoundError:
            result.fail(
                f"Unable to get section {section_name} from file {trace_info_file} "
                f"as {file_format}."
            )
            return result

    if file_format == "json":
        try:
            with open(trace_info_file) as f:
                result.payload = json.load(f)
                return result
        except json.JSONDecodeError:
            result.fail(
                f"Unable to get section {section_name} from file {trace_info_file} "
                f"as {file_format}."
            )
            return result
    elif file_format == "binary":
        try:
            result.payload = open(trace_info_file, "rb").read()
            return result
        except FileNotFoundError:
            result.fail(
                f"Unable to get section {section_name} from file {trace_info_file} "
                f"as {file_format}."
            )
            return result
    else:
        parse = _parse_callable(file_format)
        if parse is None:
            result.fail(f"Unknown format: {file_format}.")
            return result
        if not _lief.HAS_LIEF:
            result.fail(lief_missing_error(f"reading {file_format} executables"))
            return result
        section_bytes = find_section_bytes(trace_info_file, section_name, parse)
        if section_bytes is None:
            result.fail(
                f"Unable to get section {section_name} from file {trace_info_file} "
                f"as {file_format}."
            )
            return result
        result.payload = section_bytes
        return result
    return result


def emtrace(
    trace_info_file: Path,
    file_format: Literal[
        "auto", "exe", "elf", "pe", "macho", "json", "binary"
    ] = "auto",
    istream: Callable[[int], bytes] = sys.stdin.buffer.read,
    ostream: Callable[[bytes], Any] = default_ostream,
    section_name: str = ".emtrace",
    with_src_loc: Literal["none", "absolute", "relative"] = "none",
    src_hyperlinks: bool = False,
    debug_script: bool = False,
    test_section_name: str | None = None,
    trace_sink: Callable[[str], Any] | None = None,
) -> RunReport:
    """Process a trace stream; return a :class:`RunReport` (Tier 2).

    *trace_sink* receives internal debug lines when *debug_script* is set
    (defaults to writing them to stderr).
    """

    report = RunReport()

    def trace(*args: Any, **kwargs: Any):
        if debug_script:
            text = " ".join(
                [
                    "\n".join("[trace] " + line for line in str(arg).split("\n"))
                    for arg in args
                ]
            )
            if trace_sink is not None:
                trace_sink(text)
            else:
                print(text, file=sys.stderr, **kwargs)

    captured_output: None | bytearray = None
    if test_section_name is not None:
        captured_output = bytearray()

        def test_ostream(b: bytes):
            assert captured_output is not None
            captured_output.extend(b)

        ostream = test_ostream

    expected_output_from_file: bytes | None = None
    if test_section_name is not None and Path(test_section_name).is_file():
        expected_output_from_file = Path(test_section_name).read_bytes().rstrip(b"\x00")

    trace(
        f"Main args: {trace_info_file=} {istream=} {ostream=} {section_name=} {with_src_loc=} {src_hyperlinks=} {debug_script=} {test_section_name=}"
    )

    loaded = load_format_info(trace_info_file, file_format, section_name)  # type: ignore[arg-type]
    report.errors.extend(loaded.errors)
    report.warnings.extend(loaded.warnings)
    format_info = loaded.payload

    if format_info is None:
        if not report.errors:
            report.fail(
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

    if test_section_name is not None and expected_output is None:
        report.fail(f"Section '{test_section_name}' not found in {trace_info_file}")
        return report

    if isinstance(format_info, dict):
        j = format_info
        size_t_size: int = j["size_t_size"]
        ptr_size: int = j["ptr_size"]
        alignment_power: int = j["alignment_power"]
        byteorder = j["byteorder"]
        encoding_id: int = j["encoding_id"]
        record_start: int = j["mgic_record_start"]

        record_parser = None
        record_cache: dict[int, FmtInfo] = {
            int(i): FmtInfo.from_dict(fmt_info)
            for i, fmt_info in j["trace_points"].items()
        }
    else:
        data = bytes(format_info)
        record_parser = RecordParser(memoryview(data), debug_trace=trace)
        try:
            mgic = record_parser.find_and_parse_mgic()
        except RecordVersionError as e:
            report.fail(str(e))
            return report
        if mgic is None:
            report.fail("Failed to find or parse MGIC record in section data.")
            return report

        size_t_size: int = mgic.size_t_size
        ptr_size: int = mgic.ptr_size
        alignment_power: int = mgic.alignment_power
        byteorder = mgic.byteorder
        encoding_id: int = mgic.encoding_id
        record_start: int = mgic.record_start
        trace(
            f"version={mgic.version_major}.{mgic.version_minor} {size_t_size=} {ptr_size=} "
            f"{alignment_power=} byteorder={byteorder} {encoding_id=} {record_start=}"
        )
        record_cache = {}

    cobs_passthrough = None
    match encoding_id:
        case 0:
            encoding = "raw"
        case 1:
            encoding = "cobs"
        case 2:
            encoding = "cobs"
            cobs_passthrough = ostream
        case _:
            report.fail(f"Unknown encoding: {encoding_id}")
            return report

    encoding = "raw" if encoding_id == 0 else "cobs"
    trace(f"{encoding=} passthrough={cobs_passthrough is not None}")

    if encoding == "raw":
        magic_address = int.from_bytes(istream(ptr_size), byteorder=byteorder)
    else:
        frame = cobs_get_frame(istream, cobs_passthrough)
        if frame is None:
            report.fail("Stream ended before reading the first COBS frame.")
            return report
        trace(f"First COBS frame: {frame}")
        if len(frame) < ptr_size:
            report.fail(
                "Stream ended in the middle of reading the bytes for the magic address.",
            )
            report.fail(f"Leftover bytes: {frame}")
            return report
        magic_address = int.from_bytes(frame[:ptr_size], byteorder=byteorder)
    trace(f"{hex(magic_address)=}")
    magic_address *= 2**alignment_power
    # magic_address is the runtime address of the MGIC record start (record_start in section data).
    aslr_offset = record_start - magic_address

    min_path_length: int = 0
    new_line_missing = True

    original_istream = istream
    frame_buffer: None | bytearray = None

    if encoding == "cobs":
        frame_buffer = bytearray()

        def _istream(n: int) -> bytes:
            nonlocal frame_buffer
            assert frame_buffer is not None
            result = bytes(frame_buffer[:n])
            frame_buffer = frame_buffer[n:]
            return result

        istream = _istream

    parser = Parser(
        translation_le if byteorder == "little" else translation_be,
        istream,
        trace,
        ptr_size,
        byteorder,
        byteorder,
    )
    while True:
        if frame_buffer is not None:
            if len(frame_buffer) != 0:
                report.fail(
                    "Leftover bytes in COBS frame buffer before reading next frame.",
                )
                report.fail(f"Leftover bytes: {frame_buffer}")
                frame_buffer.clear()
            frame = cobs_get_frame(original_istream, cobs_passthrough)
            if frame is None:
                trace("End of stream reached.")
                break
            trace(f"Next COBS frame: {frame}")
            frame_buffer.extend(frame)

        b = istream(ptr_size)
        if len(b) == 0:
            break
        if len(b) < ptr_size:
            report.fail(
                "Stream ended in the middle of reading the bytes for the next format info location.",
            )
            report.fail(f"Leftover bytes: {b}")
            if encoding == "cobs":
                continue
            return report
        trace(f"Next format info location bytes: {b}")
        address = int.from_bytes(b, byteorder="little")
        trace(f"as address: {hex(address)}")
        address *= 2**alignment_power
        trace(f"adjusted address: {hex(address)}")
        section_offset = address + aslr_offset
        if section_offset in record_cache:
            trace("Associated format info already parsed into cache.")
            info = record_cache[section_offset]
        else:
            trace("Not cached yet.")
            if record_parser is None:
                report.fail(
                    f"Record parser not initialized and record not in cache / json at section_offset={section_offset:#x}."
                )
                return report

            try:
                info, _ = record_parser.parse_trace_record(section_offset)
            except RecordVersionError as e:
                report.fail(
                    f"Skipping trace point at section_offset={section_offset:#x}: {e}"
                )
                if encoding != "cobs":
                    return report
                continue
            if info is None:
                report.fail(
                    f"No trace point found for address={address:#x} (section_offset={section_offset})."
                )
                if encoding != "cobs":
                    return report
                continue
            record_cache[section_offset] = info

        trace(hex(address))
        formatted = parser.format(info, FORMATTERS)
        match formatted:
            case tuple():
                report.fail(
                    f"Stream ended in the middle of parsing bytes associated with format string {info.fmt_string}.",
                )
                report.fail(f"from {info.file}:{info.line}")
                report.fail(
                    " ".join(
                        ["successfully parsed arguments: "]
                        + [str(a) for a in formatted[0]]
                    )
                )
                report.fail(f"Leftover bytes: {formatted[1]}")
                if encoding == "cobs":
                    continue
                return report
            case list():
                report.fail("Failed to format")
                report.fail(f"```\n{info.fmt_string}\n```")
                report.fail(f"from {info.file}:{info.line}")
                report.fail("with arguments")
                report.fail(" ".join(["    "] + [str(a) for a in formatted[:-1]]))
                report.fail(str(formatted[-1]))
                continue
            case str():
                pass

        assert isinstance(formatted, str)

        path = None
        if with_src_loc == "absolute":
            path = info.file
        elif with_src_loc == "relative":
            path = os.path.relpath(info.file, os.getcwd())

        if path is None:
            _ = ostream(formatted.encode("utf-8"))
            report.records += 1
            continue

        location_string = f"{path}:{info.line}"
        min_path_length = max(min_path_length, len(location_string))

        location_string = location_string + " " * (
            min_path_length - len(location_string)
        )
        if new_line_missing:
            _ = ostream(f"{location_string}: ".encode("utf-8"))
            location_missing = False
        else:
            location_missing = True

        lines = formatted.split("\n")

        new_line_missing = False
        if lines[-1] == "":
            lines = lines[:-1]
            new_line_missing = True

        for i, line in enumerate(lines):
            if i == 0:
                pass
            elif location_missing and i == 1:
                _ = ostream(f"\n{location_string}: ".encode("utf-8"))
            else:
                _ = ostream(("\n" + " " * (2 + min_path_length)).encode("utf-8"))

            _ = ostream(line.encode("utf-8"))

        if new_line_missing:
            _ = ostream(b"\n")
        report.records += 1

    if test_section_name is not None:
        assert expected_output is not None
        assert captured_output is not None
        report.captured = bytes(captured_output)
        report.expected = expected_output
    return report
