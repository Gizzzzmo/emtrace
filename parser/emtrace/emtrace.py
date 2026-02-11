#!/usr/bin/env python3

from __future__ import annotations

import sys
import json
import os
from typing import Callable, Literal, Any
from pathlib import Path

from .cobs import cobs_get_frame
from .format_info import FmtInfo
from .parser import (
    Parser,
    translation_le,
    translation_be,
)
from .formatters import FORMATTERS
from .record_parser import RecordParser, detect_byteorder as detect_byteorder

try:
    import lief

    Binary = lief.MachO.Binary | lief.ELF.Binary | lief.PE.Binary

    def parse_exe(
        trace_info_file: Path, section_name: str, parse: Callable[[Path], Binary | None]
    ) -> bytes | None:
        exe = parse(trace_info_file)
        if exe is None:
            return None
        section = exe.get_section(section_name)

        if section is None:
            return None
        return bytes(section.content)

    def parse_macho(x: Path):
        exe = lief.parse(x) if lief.is_macho(x) else None
        assert isinstance(exe, lief.MachO.Binary)
        return exe

except ImportError:
    from . import lief_stub as lief

    def parse_exe(
        trace_info_file: Path, section_name: str, parse: Callable[[Path], Any]
    ) -> None:
        _ = trace_info_file, section_name, parse
        return None

    def parse_macho(_: Path):
        return None





def error(*args: Any, **kwargs: Any):
    print(
        " ".join(
            [
                "\n".join("[error] " + line for line in str(arg).split("\n"))
                for arg in args
            ]
        ),
        file=sys.stderr,
        **kwargs,
    )


def _run_test_comparison(captured_output: bytearray, expected_output: bytes) -> None:
    """Compare captured output against expected output, print diff on mismatch, exit 1 on failure."""
    actual_output = bytes(captured_output)
    if actual_output == expected_output:
        print("Test passed!")
    else:
        import difflib

        print("Test failed!", file=sys.stderr)
        diff = difflib.unified_diff(
            expected_output.decode("utf-8").splitlines(keepends=True),
            actual_output.decode("utf-8").splitlines(keepends=True),
            fromfile="expected",
            tofile="actual",
        )
        _ = sys.stdout.writelines(diff)
        sys.exit(1)


def default_ostream(b: bytes) -> None:
    _ = sys.stdout.buffer.write(b)
    return sys.stdout.buffer.flush()


def get_format_info(
    trace_info_file: Path,
    file_format: Literal["auto", "exe", "elf", "pe", "macho", "oat", "json", "binary"],
    section_name: str,
) -> dict[str, Any] | bytes | None:
    if file_format == "auto":
        try:
            with open(trace_info_file, "r") as f:
                json_info = json.load(f)
                return json_info
        except json.JSONDecodeError:
            pass
        except UnicodeDecodeError:
            pass

        section_bytes = parse_exe(trace_info_file, section_name, lief.parse)
        if section_bytes is not None:
            return section_bytes
        else:
            try:
                return open(trace_info_file, "rb").read()
            except FileNotFoundError:
                return None

    match file_format:
        case "exe":
            return parse_exe(trace_info_file, section_name, lief.parse)
        case "elf":
            return parse_exe(trace_info_file, section_name, lief.ELF.parse)
        case "pe":
            return parse_exe(trace_info_file, section_name, lief.PE.parse)
        case "macho":
            return parse_exe(trace_info_file, section_name, parse_macho)
        case "oat":
            return parse_exe(trace_info_file, section_name, lief.OAT.parse)
        case "json":
            try:
                with open(trace_info_file, "r") as f:
                    json_info = json.load(f)
                    return json_info
            except json.JSONDecodeError:
                return None
        case "binary":
            try:
                return open(trace_info_file, "rb").read()
            except FileNotFoundError:
                return None


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
) -> None:
    """Main function for the emtrace script."""

    captured_output: None | bytearray = None
    if test_section_name is not None:
        captured_output = bytearray()

        def test_ostream(b: bytes):
            captured_output.extend(b)

        ostream = test_ostream

    expected_output_from_file: bytes | None = None
    if test_section_name is not None and Path(test_section_name).is_file():
        expected_output_from_file = Path(test_section_name).read_bytes().rstrip(b"\x00")

    def trace(*args: Any, **kwargs: Any):
        if debug_script:
            print(
                " ".join(
                    [
                        "\n".join("[trace] " + line for line in str(arg).split("\n"))
                        for arg in args
                    ]
                ),
                file=sys.stderr,
                **kwargs,
            )

    trace(
        f"Main args: {trace_info_file=} {istream=} {ostream=} {section_name=} {with_src_loc=} {src_hyperlinks=} {debug_script=} {test_section_name=}"
    )

    format_info = get_format_info(trace_info_file, file_format, section_name)

    if format_info is None:
        error(
            f"Unable to get section {section_name} from file {trace_info_file} as {file_format}."
        )
        sys.exit(1)

    expected_output: bytes | None = None
    if test_section_name is not None:
        if expected_output_from_file is not None:
            expected_output = expected_output_from_file
        else:
            section_bytes = parse_exe(trace_info_file, test_section_name, lief.parse)
            if section_bytes is not None:
                expected_output = section_bytes.rstrip(b"\x00")
        trace(f"Expected test output: {expected_output}")

    if test_section_name is not None and expected_output is None:
        error(f"Section '{test_section_name}' not found in {trace_info_file}")
        sys.exit(1)

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
        mgic = record_parser.find_and_parse_mgic()
        if mgic is None:
            error("Failed to find or parse MGIC record in section data.")
            sys.exit(1)

        size_t_size: int = mgic.size_t_size
        ptr_size: int = mgic.ptr_size
        alignment_power: int = mgic.alignment_power
        byteorder = mgic.byteorder
        encoding_id: int = mgic.encoding_id
        record_start: int = mgic.record_start
        trace(
            f"version={mgic.version} {size_t_size=} {ptr_size=} {alignment_power=} "
            f"byteorder={byteorder} {encoding_id=} {record_start=}"
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
            error(f"Unknown encoding: {encoding_id}")
            sys.exit(1)

    encoding = "raw" if encoding_id == 0 else "cobs"
    trace(f"{encoding=} passthrough={cobs_passthrough is not None}")
    # all_format_info = record_parser.parse_all_fmt_infos()
    # with open("test.json", "w") as test_out:
    #     json.dump(all_format_info, test_out, default=lambda x: x.to_dict())

    if encoding == "raw":
        magic_address = int.from_bytes(istream(ptr_size), byteorder=byteorder)
    else:
        frame = cobs_get_frame(istream, cobs_passthrough)
        if frame is None:
            error("Stream ended before reading the first COBS frame.")
            sys.exit(1)
        trace(f"First COBS frame: {frame}")
        if len(frame) < ptr_size:
            error(
                "Stream ended in the middle of reading the bytes for the magic address.",
            )
            error(f"Leftover bytes: {frame}")
            sys.exit(1)
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
                error(
                    "Leftover bytes in COBS frame buffer before reading next frame.",
                )
                error(f"Leftover bytes: {frame_buffer}")
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
            error(
                "Stream ended in the middle of reading the bytes for the next format info location.",
            )
            error(f"Leftover bytes: {b}")
            if encoding == "cobs":
                continue
            sys.exit(1)
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
                error(
                    f"Record parser not initialized and record not in cache / json at section_offset={section_offset:#x}."
                )
                sys.exit(1)

            info, _ = record_parser.parse_trace_record(section_offset)
            if info is None:
                error(
                    f"No trace point found for address={address:#x} (section_offset={section_offset})."
                )
                if encoding != "cobs":
                    sys.exit(1)
                continue
            record_cache[section_offset] = info

        trace(hex(address))
        formatted = parser.format(info, FORMATTERS)
        match formatted:
            case tuple():
                error(
                    f"Stream ended in the middle of parsing bytes associated with format string {info.fmt_string}.",
                )
                error(f"from {info.file}:{info.line}")
                error("successfully parsed arguments: ", *formatted[0])
                error(f"Leftover bytes: {formatted[1]}")
                if encoding == "cobs":
                    continue
                sys.exit(1)
            case list():
                error("Failed to format")
                error(f"```\n{info.fmt_string}\n```")
                error(f"from {info.file}:{info.line}")
                error("with arguments")
                error("    ", *formatted[:-1])
                error(formatted[-1])
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

    if test_section_name is not None:
        assert expected_output is not None
        assert captured_output is not None
        _run_test_comparison(captured_output, expected_output)
