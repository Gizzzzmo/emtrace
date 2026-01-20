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
from .format_parser import FormatParser

try:
    import lief

    Binary = lief.MachO.Binary | lief.ELF.Binary | lief.PE.Binary

    def parse_exe(
        trace_info_file: Path, section_name: str, parse: Callable[[Path], Binary | None]
    ) -> memoryview | None:
        exe = parse(trace_info_file)
        if exe is None:
            return None
        section = exe.get_section(section_name)
        if section is None:
            return None
        return section.content

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


try:
    from elftools.elf.elffile import ELFFile
    from elftools.common.exceptions import ELFError
except ImportError:
    ELFFile: None | type = None
    ELFError: type[Exception] = ImportError


def detect_byteorder(b: bytes) -> Literal["little", "big", "unknown"] | None:
    """Given a sequence of bytes, this function returns the byte order.

    None is returned if the sequence is longer than 256, shorter than 1, or if there is a
    duplicate byte in the sequence.
    "little" is returned if b[i] = b[i-1] + 1 and b[0] = 0.
    "big" is returned if b[i] = b[i+1] + 1 and b[0] = len(b) - 1.
    "unknown" is returned otherwise.
    """
    if len(b) > 256 or len(b) < 1:
        return None
    found = [False for _ in range(len(b))]
    if int(b[0]) == 0:
        byteorder = "little"
    elif int(b[0]) == len(b) - 1:
        byteorder = "big"
    else:
        byteorder = "unknown"
    for i, byte in enumerate(b):
        if int(byte) >= len(b):
            return None
        if found[int(byte)]:
            return None
        found[int(byte)] = True
        if byteorder == "unknown":
            continue

        if byteorder == "little" and i != int(byte):
            byteorder = "unknown"
        elif byteorder == "big" and len(b) - i - 1 != int(byte):
            byteorder = "unknown"

    return byteorder


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


def default_ostream(b: bytes) -> None:
    _ = sys.stdout.buffer.write(b)
    return sys.stdout.buffer.flush()


def get_format_info(
    trace_info_file: Path,
    file_format: Literal["auto", "exe", "elf", "pe", "macho", "oat", "json", "binary"],
    section_name: str,
) -> dict[str, Any] | memoryview | None:
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
                return memoryview(open(trace_info_file, "rb").read())
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
                return memoryview(open(trace_info_file, "rb").read())
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

    if isinstance(format_info, dict):
        error(f"Dict parsing not yet implemented\nFormat info: {format_info}")
        sys.exit(1)

    expected_output: bytes | None = None
    test_section = None
    with trace_info_file.open("rb") as fd:
        try:
            if ELFFile is None:
                trace("pyelftools was not found")
                raise ELFError()
            elffile = ELFFile(fd)

            if test_section_name is not None:
                test_section = elffile.get_section_by_name(test_section_name)
                expected_output = test_section.data().rstrip(b"\x00")
                trace(f"Expected test output: {expected_output}")

        except ELFError:
            trace("Could not interpret file as ELF, reading raw binary...")
            _ = fd.seek(0)
            data = fd.read()

    data = bytes(format_info)

    if test_section_name is not None and test_section is None:
        error(f"Section '{test_section_name}' not found in {trace_info_file}")
        sys.exit(1)

    magic_constant = bytes.fromhex(
        "d197f522d9269fd1ad703392f659dfd0fbecbd60971325e89201b25a385d9ec7"
    )
    magic_offset = data.find(magic_constant)
    trace(f"{magic_offset=}")
    if magic_offset == -1:
        trace(
            "emtrac magic constant not found. Assuming info lies at beginning of section / data."
        )
    info_location: int = magic_offset + 32
    rest_info_loc = int(data[info_location]) + magic_offset
    trace(f"{info_location=} {rest_info_loc=}")

    size_t_size = int(data[info_location + 1])
    ptr_size = int(data[info_location + 2])
    alignment_power = int(data[info_location + 3])
    trace(f"{size_t_size=} {ptr_size=} {alignment_power=}")

    byteorder_id: bytes = data[rest_info_loc : rest_info_loc + size_t_size]
    trace(f"{byteorder_id=}")
    byteorder = detect_byteorder(byteorder_id)
    trace(f"detected byteorder: {byteorder}")
    if byteorder is None or byteorder == "unknown":
        error(
            f"Unable to detect byteorder based on byteorder-id: {byteorder_id} ({size_t_size=})."
        )
        sys.exit(1)

    null_terminated: int = int.from_bytes(
        data[rest_info_loc + size_t_size : rest_info_loc + size_t_size * 2],
        byteorder=byteorder,
    )
    length_prefixed: int = int.from_bytes(
        data[rest_info_loc + size_t_size * 2 : rest_info_loc + size_t_size * 3],
        byteorder=byteorder,
    )
    encoding_id: int = int.from_bytes(
        data[rest_info_loc + size_t_size * 3 : rest_info_loc + size_t_size * 4],
        byteorder=byteorder,
    )

    cobs_passthrough = None
    match encoding_id:
        case 0:
            encoding = "none"
        case 1:
            encoding = "cobs"
        case 2:
            encoding = "cobs"
            cobs_passthrough = ostream
        case _:
            error(f"Unknown encoding: {encoding_id}")
            sys.exit(1)

    encoding = "none" if encoding_id == 0 else "cobs"
    trace(
        f"{hex(null_terminated)=} {hex(length_prefixed)=} {encoding=} passthrough={cobs_passthrough is not None}"
    )

    format_parser = FormatParser(
        memoryview(data),
        ptr_size,
        size_t_size,
        null_terminated,
        length_prefixed,
        debug_trace=trace,
    )

    all_format_info = format_parser.parse_all_fmt_infos()
    with open("test.json", "w") as test_out:
        json.dump(all_format_info, test_out, default=lambda x: x.to_dict())

    if encoding == "none":
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
    aslr_offset = magic_offset - magic_address

    cache: dict[int, FmtInfo] = {}
    min_path_length: int = 0
    new_line_missing = True

    original_istream = istream
    frame_buffer: None | bytearray = None
    if encoding == "cobs":
        frame_buffer = bytearray()

        def istream(n: int) -> bytes:
            nonlocal frame_buffer
            assert frame_buffer is not None
            result = bytes(frame_buffer[:n])
            frame_buffer = frame_buffer[n:]
            return result

    parser = Parser(
        translation_le if byteorder == "little" else translation_be,
        istream,
        trace,
        size_t_size,
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
        if address in cache:
            trace("Associated format info already parsed into cache.")
            info = cache[address]
        else:
            trace("Not cached yet.")
            info, _ = format_parser.parse_fmt_info(address + aslr_offset)
            cache[address] = info

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
