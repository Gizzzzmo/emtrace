#!/usr/bin/env python3

from __future__ import annotations
from typing import Callable, Literal, Any, override
from argparse import ArgumentParser, ArgumentTypeError
from pathlib import Path
import sys
import re
import os
import socket
import struct

try:
    from elftools.elf.elffile import ELFFile
    from elftools.common.exceptions import ELFError
except ImportError:
    ELFFile: None | type = None
    ELFError: type[Exception] = ImportError


def detect_byteorder(b: bytes) -> Literal["little", "big", "unknown"] | None:
    """Given a sequence of bytes, this function returns the byte order.

    None is returned if the sequence is longer than 256 or if there is a
    duplicate byte in the sequence.
    "little" is returned if b[i] = b[i-1] + 1 and b[0] = 0.
    "big" is returned if b[i] = b[i+1] + 1 and b[0] = len(b) - 1.
    "unknown" is returned otherwise.
    """
    if len(b) > 256:
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


def signed_le(b: bytes) -> int:
    """Interpret bytes as a little-endian signed integer."""
    return int.from_bytes(b, byteorder="little", signed=True)


def signed_be(b: bytes) -> int:
    """Interpret bytes as a big-endian signed integer."""
    return int.from_bytes(b, byteorder="big", signed=True)


def unsigned_le(b: bytes) -> int:
    """Interpret bytes as a little-endian unsigned integer."""
    return int.from_bytes(b, byteorder="little", signed=False)


def unsigned_be(b: bytes) -> int:
    """Interpret bytes as a big-endian unsigned integer."""
    return int.from_bytes(b, byteorder="big", signed=False)


def string(b: bytes) -> str:
    """Interpret bytes as a UTF-8 string."""
    return b.decode("utf-8")


def to_bool(b: bytes) -> bool:
    """Interpret a byte as a bool."""
    return b[0] != 0


def float_le(b: bytes) -> float:
    """Interpret bytes as a little-endian float."""
    return struct.unpack("<f", b)[0]


def double_le(b: bytes) -> float:
    """Interpret bytes as a little-endian double."""
    return struct.unpack("<d", b)[0]


def float_be(b: bytes) -> float:
    """Interpret bytes as a big-endian float."""
    return struct.unpack(">f", b)[0]


def double_be(b: bytes) -> float:
    """Interpret bytes as a big-endian double."""
    return struct.unpack(">d", b)[0]


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


def _py_formatter(fmt: str, args: list[Any]) -> str:
    return fmt.format(*args)


class FmtInfo:
    """Contains all information about a format string and its parameters."""

    def __init__(
        self,
        fmt_string: str,
        size_t_size: int = 8,
        size_t_byteorder: Literal["little", "big"] = "little",
        formatter: Callable[[str, list[Any]], str] = _py_formatter,
    ) -> None:
        """Initialize the FmtInfo."""
        self.fmt_string: str = fmt_string
        self.size_t_size: int = size_t_size
        self.size_t_byteorder: Literal["little", "big"] = size_t_byteorder
        self.formatter: Callable[[str, list[Any]], str] = formatter
        self.parsers: list[Callable[[bytes], Any]] = []
        self.param_sizes: list[int | Literal["null_terminated", "length_prefixed"]] = []
        self.file: str = ""
        self.line: int = -1

    def add_source_info(self, file: str, line: int) -> None:
        """Add source location information to the format info."""
        self.file = file
        self.line = line

    def add_param(
        self,
        parser: Callable[[bytes], Any],
        size: int | Literal["null_terminated", "length_prefixed"],
    ) -> None:
        """Add a parameter to the format info."""
        self.parsers.append(parser)
        self.param_sizes.append(size)

    def format(
        self, stream: Callable[[int], bytes]
    ) -> str | list[Any] | tuple[list[Any], bytes]:
        """Format the trace message from the stream."""
        args: list[Any] = []
        for parser, size in zip(self.parsers, self.param_sizes):
            match size:
                case "null_terminated":
                    arr = bytearray()
                    while len(arr) == 0 or arr[-1] != 0:
                        b = stream(1)
                        if len(b) == 0:
                            return (args, bytes(arr))
                        arr.append(b[0])
                    args.append(parser(bytes(arr[:-1])))
                    continue

                case "length_prefixed":
                    b = stream(self.size_t_size)
                    if len(b) < self.size_t_size:
                        return (args, b)
                    size = int.from_bytes(b, byteorder=self.size_t_byteorder)
                case int(size):
                    pass

            b = stream(size)
            if len(b) < size:
                return (args, b)

            args.append(parser(b))

        try:
            formatted = self.formatter(self.fmt_string, args)
        except (IndexError, ValueError) as err:
            return [*args, err]

        return formatted


class Emtrace:
    """Parses and formats emtrace data."""

    remove_qualifiers: re.Pattern[str] = re.compile(r"\b(const|volatile)\b")

    def __init__(
        self,
        data: bytes,
        ptr_size: int = 8,
        size_t_size: int = 8,
        null_terminated: int | None = None,
        length_prefixed: int | None = None,
        byteorder: Literal["little", "big"] = "little",
        debug_trace: Callable[[*tuple[Any, ...]], None] = lambda *args: None,
    ) -> None:
        """Initialize the Emtrace parser."""
        unsigned = unsigned_le if byteorder == "little" else unsigned_be
        signed = signed_le if byteorder == "little" else signed_be
        float32 = float_le if byteorder == "little" else float_be
        float64 = double_le if byteorder == "little" else double_be
        self.type_mapping: dict[str, Callable[[bytes], Any]] = {
            # signed
            "signed": signed,
            "int": signed,
            "signed int": signed,
            "int32_t": signed,
            "long": signed,
            "signed long": signed,
            "long long": signed,
            "signed long long": signed,
            "int64_t": signed,
            "int128_t": signed,
            "short": signed,
            "signed short": signed,
            "int16_t": signed,
            "ssize_t": signed,
            "ptrdiff_t": signed,
            "intptr_t": signed,
            # char
            "signed char": SChar,
            "int8_t": SChar,
            "unsigned char": Char,
            "char": Char,
            "uint8_t": Char,
            # unsigned
            "unsigned": unsigned,
            "unsigned int": unsigned,
            "uint32_t": unsigned,
            "unsigned long": unsigned,
            "unsigned long long": unsigned,
            "uint64_t": unsigned,
            "uint128_t": unsigned,
            "uint16_t": unsigned,
            "size_t": unsigned,
            "uintptr_t": unsigned,
            "*": unsigned,
            # string
            "string": string,
            # other
            "bool": to_bool,
            "_Bool": to_bool,
            "float": float32,
            "double": float64,
        }
        self.ptr_size: int = ptr_size
        self.size_t_size: int = size_t_size

        if length_prefixed is None:
            self.length_prefixed: int = (2**size_t_size) - 2
        else:
            self.length_prefixed = length_prefixed

        if null_terminated is None:
            self.null_terminated: int = (2**size_t_size) - 1
        else:
            self.null_terminated = null_terminated

        self.data: bytes = data
        self.offset: int = 0
        self.byteorder: Literal["little", "big"] = byteorder
        self.debug_trace: Callable[[*tuple[Any, ...]], None] = debug_trace

    def set_offset(self, offset: int) -> None:
        """Set the offset for parsing format info."""
        self.offset = offset

    def _c_style_formatter(self, fmt: str, args: list[Any]) -> str:
        return fmt % tuple(x for x in args)

    def _no_format_formatter(self, fmt: str, _: list[Any]) -> str:
        return fmt

    def parse_fmt_info(
        self, ptr: int, with_src_loc: bool = True, offset: int | None = None
    ) -> FmtInfo:
        """Parse the format info from the data."""
        self.debug_trace("parse_fmt_info:")
        if offset is None:
            offset = self.offset

        pos: int = ptr + offset
        self.debug_trace(f"  {pos=}")

        def consume(n: int) -> bytes:
            nonlocal pos
            s = self.data[pos : pos + n]
            pos += n
            return s

        def consume_size_t() -> int:
            return int.from_bytes(consume(self.size_t_size), byteorder=self.byteorder)

        def consume_until(b: bytes = b"\x00") -> bytes:
            nonlocal pos
            s = self.data[pos : pos + len(b)]
            start = pos
            while s != b and len(s) == len(b):
                s = self.data[pos : pos + len(b)]
                pos += 1

            return self.data[start:pos]

        num_args = consume_size_t()
        self.debug_trace(f"  {num_args=}")

        format_offset = consume_size_t()
        type_info_offsets: list[tuple[int, int]] = []
        for i in range(num_args):
            self.debug_trace(f"  {i + 1}:")
            offset_type_desc = consume_size_t()
            self.debug_trace(f"    {offset_type_desc=}")
            type_size = consume_size_t()
            self.debug_trace(f"    {type_size=}")
            type_info_offsets.append((offset_type_desc, type_size))

        formatter_id = consume_size_t()
        match formatter_id:
            case 0:
                formatter = _py_formatter
            case 2:
                formatter = self._c_style_formatter
            case 1 | _:
                formatter = self._no_format_formatter

        if with_src_loc:
            file_offset = consume_size_t()
            line = consume_size_t()
            pos = ptr + offset + file_offset
            file = consume_until()[:-1].decode("utf-8")
        else:
            file = ""
            line = -1

        pos = ptr + offset + format_offset
        fmt_string = consume_until()[:-1].decode("utf-8")
        self.debug_trace(f"  {fmt_string=}")
        info: FmtInfo = FmtInfo(fmt_string, self.size_t_size, self.byteorder, formatter)
        info.add_source_info(file, line)

        for offset_type_desc, type_size in type_info_offsets:
            pos = ptr + offset + offset_type_desc
            type_desc = consume_until()[:-1]
            type_desc = Emtrace.remove_qualifiers.sub(
                "", type_desc.decode("utf-8")
            ).strip()
            if type_desc and type_desc[-1] == "*":
                type_desc = "*"
            self.debug_trace(f"    {type_desc=}")
            if type_size == self.null_terminated:
                type_size = "null_terminated"
            elif type_size == self.length_prefixed:
                type_size = "length_prefixed"

            info.add_param(self.type_mapping[type_desc], type_size)

        self.debug_trace()

        return info


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


def main(
    elf: Path,
    istream: Callable[[int], bytes] = sys.stdin.buffer.read,
    ostream: Callable[[bytes], Any] = sys.stdin.buffer.write,
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
        f"Main args: {elf=} {istream=} {ostream=} {section_name=} {with_src_loc=} {src_hyperlinks=} {debug_script=} {test_section_name=}"
    )

    expected_output: bytes | None = None
    test_section = None
    with elf.open("rb") as fd:
        try:
            if ELFFile is None:
                trace("pyelftools was not found")
                raise ELFError()
            elffile = ELFFile(fd)
            section = elffile.get_section_by_name(section_name)
            if section is None:
                error(f"Section {section_name} not found in {elf}")
                sys.exit(1)
            data: bytes = section.data()

            if test_section_name is not None:
                test_section = elffile.get_section_by_name(test_section_name)
                expected_output = test_section.data().rstrip(b"\x00")
                trace(f"Expected test output: {expected_output}")

        except ELFError:
            trace("Could not interpret file as ELF, reading raw binary...")
            _ = fd.seek(0)
            data = fd.read()

    if test_section_name is not None and test_section is None:
        error(f"Section '{test_section_name}' not found in {elf}")
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
    trace(f"{size_t_size=} {ptr_size=}")

    byteorder_id: bytes = data[rest_info_loc : rest_info_loc + size_t_size]
    trace(f"{byteorder_id=}")
    byteorder = detect_byteorder(byteorder_id)
    trace(f"detected byteorder: {byteorder}")
    if byteorder is None or byteorder == "unknown":
        error(
            f"Unable to detect byteorder based on byteorder-id: {byteorder_id} ({size_t_size=})."
        )
        sys.exit(0)

    null_terminated: int = int.from_bytes(
        data[rest_info_loc + size_t_size : rest_info_loc + size_t_size * 2],
        byteorder=byteorder,
    )
    length_prefixed: int = int.from_bytes(
        data[rest_info_loc + size_t_size * 2 : rest_info_loc + size_t_size * 3],
        byteorder=byteorder,
    )
    trace(f"{hex(null_terminated)=} {hex(length_prefixed)=} ")

    emtrace = Emtrace(
        data,
        ptr_size,
        size_t_size,
        null_terminated,
        length_prefixed,
        debug_trace=trace,
    )

    magic_address = int.from_bytes(istream(ptr_size), byteorder=byteorder)
    trace(f"{hex(magic_address)=}")
    emtrace.set_offset(magic_offset - magic_address)

    cache: dict[int, FmtInfo] = {}
    min_path_length: int = 0
    new_line_missing = True
    while True:
        trace("")
        b = istream(ptr_size)
        if len(b) == 0:
            break
        if len(b) < ptr_size:
            error(
                "Stream ended in the middle of reading the bytes for the next format info location.",
            )
            error(f"Leftover bytes: {b}")
            sys.exit(1)
        trace(f"Next format info location bytes: {b}")
        address = int.from_bytes(b, byteorder="little")
        trace(f"as address: {hex(address)}")
        if address in cache:
            trace("Associated format info already parsed into cache.")
            info = cache[address]
        else:
            trace("Not cached yet.")
            info = emtrace.parse_fmt_info(int.from_bytes(b, byteorder="little"))
            cache[address] = info

        trace(hex(address))
        formatted = info.format(istream)
        match formatted:
            case tuple():
                error(
                    f"Stream ended in the middle of parsing bytes associated with format string {info.fmt_string}.",
                )
                error(f"from {info.file}:{info.line}")
                error("successfully parsed arguments: ", *formatted[0])
                error(f"Leftover bytes: {formatted[1]}")
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

        trace(args)
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


def get_input_stream(x: str) -> Callable[[int], bytes]:
    """Get a function that reads bytes from an input stream."""
    parts = x.split("://", 1)
    if not parts:
        raise ArgumentTypeError("Bad input stream: cannot be empty string.")

    if len(parts) == 1:
        # auto detect stream_type
        stream_id = parts[0]
        match stream_id.split(":"):
            case ["stdin"]:
                return sys.stdin.buffer.read
            case parts if len(parts) in [2, 9]:
                stream_type = "tcp"
            case _:
                stream_type = "file"
    else:
        stream_type = parts[0]
        if stream_type not in ["tcp", "unix", "file"]:
            raise ArgumentTypeError(
                f"Bad input stream type {stream_type}: has to either be file, tcp or unix."
            )
        stream_id = parts[1]

    match stream_type:
        case "file":
            with Path(stream_id).open("rb") as f:
                return f.read
        case "unix":
            family = socket.AF_UNIX
            address = stream_id
        case "tcp":
            parts = stream_id.split(":")
            if len(parts) == 2:
                family = socket.AF_INET
                address = (parts[0], int(parts[1]))
            elif len(parts) == 9:
                family = socket.AF_INET6
                address = (":".join(parts[:-1]), int(parts[-1]))
            else:
                raise ArgumentTypeError(
                    "Bad address for tcp type input stream: Needs to be <ip-address>:<port> (ip can be ipv4 or ipv6)"
                )
        case _:
            assert False

    s = socket.socket(family, socket.SOCK_STREAM)
    s.connect(address)

    return s.recv


def dump(x: str | None) -> tuple[Callable[[bytes], Any], Callable[[], None]]:
    """Get a function that writes to a file and a function to close it."""
    if x is None:
        return lambda _: None, lambda: None

    f = Path(x).open("wb")
    return f.write, f.close


if __name__ == "__main__":
    parser = ArgumentParser("emtrace")

    _ = parser.add_argument("elf")
    _ = parser.add_argument(
        "--input",
        "-i",
        nargs="?",
        default=get_input_stream("stdin"),
        type=get_input_stream,
    )
    _ = parser.add_argument(
        "--dump-input",
        nargs="?",
        default=dump(None),
        const="emtrace_input.bin",
        type=dump,
    )
    _ = parser.add_argument("--section-name", nargs="?", default=".emtrace", type=str)
    _ = parser.add_argument(
        "--with-src-loc",
        nargs="?",
        default="none",
        const="relative",
        choices=["none", "absolute", "relative"],
        type=str,
    )
    _ = parser.add_argument("--src-hyperlinks", action="store_true")
    _ = parser.add_argument("--debug-script", action="store_true")
    _ = parser.add_argument(
        "--test",
        nargs="?",
        const=".emtrace.test.expected",
        default=None,
        help="Run emtrace in test mode. This will read the expected output from the ELF section specified (default: .emtrace.test.expected), and will compare it against the actual output.",
    )

    args = parser.parse_args()

    # lazy evaluate the default option ('emtrace_input.bin') of the argument
    if type(args.dump_input) is str:
        args.dump_input = dump(args.dump_input)

    def patched_input(n: int) -> bytes:
        b = args.input(n)
        args.dump_input[0](b)
        return b

    main(
        Path(args.elf),
        patched_input,
        sys.stdout.buffer.write,
        args.section_name,
        args.with_src_loc,
        args.src_hyperlinks,
        args.debug_script,
        args.test,
    )
    # flush
    _ = args.dump_input[1]()
