#!/usr/bin/env python3

from typing import Callable, Literal, Any, override
from elftools.elf.elffile import ELFFile
from argparse import ArgumentParser, ArgumentDefaultsHelpFormatter, ArgumentTypeError
from pathlib import Path
import sys
import re
import os
import socket


def signed_le(b: bytes):
    return int.from_bytes(b, byteorder="little", signed=True)


def signed_be(b: bytes):
    return int.from_bytes(b, byteorder="big", signed=True)


def unsigned_le(b: bytes):
    return int.from_bytes(b, byteorder="little", signed=False)


def unsigned_be(b: bytes):
    return int.from_bytes(b, byteorder="big", signed=False)


def string(b: bytes):
    return b.decode("utf-8")


class Char:
    def __init__(self, value: bytes) -> None:
        self.value: int = value[0]

    @override
    def __format__(self, format_spec: str, /) -> str:
        if len(format_spec) == 0 or not format_spec[-1].isalpha():
            return ("{:" + format_spec + "c}").format(self.value)
        else:
            return ("{:" + format_spec + "}").format(self.value)

    @override
    def __repr__(self) -> str:
        return f"char({hex(self.value)})"


class FmtInfo:
    def __init__(
        self,
        fmt_string: str,
        size_t_size: int = 8,
        size_t_byteorder: Literal["little", "big"] = "little",
        no_format: bool = False,
    ) -> None:
        self.fmt_string: str = fmt_string
        self.size_t_size: int = size_t_size
        self.size_t_byteorder: Literal["little", "big"] = size_t_byteorder
        self.no_format: bool = no_format
        self.parsers: list[Callable[[bytes], Any]] = []
        self.param_sizes: list[int | Literal["null_terminated", "length_prefixed"]] = []
        self.file: str = ""
        self.line: int = -1

    def add_source_info(self, file: str, line: int):
        self.file = file
        self.line = line

    def add_param(
        self,
        parser: Callable[[bytes], Any],
        size: int | Literal["null_terminated", "length_prefixed"],
    ):
        self.parsers.append(parser)
        self.param_sizes.append(size)

    def format(self, stream: Callable[[int], bytes]):
        if self.no_format:
            return self.fmt_string
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
            formatted = self.fmt_string.format(*args)
        except (IndexError, ValueError) as err:
            return args + [err]

        return formatted


class Emtrace:
    remove_qualifiers: re.Pattern[str] = re.compile(r"\b(const|volatile)\b")

    def __init__(
        self,
        data: bytes,
        ptr_size: int = 8,
        size_t_size: int = 8,
        byteorder: Literal["little", "big"] = "little",
        debug_trace: Callable[[*tuple[Any, ...]], None] = lambda *args: None,
    ) -> None:
        unsigned = unsigned_le if byteorder == "little" else unsigned_be
        signed = signed_le if byteorder == "little" else signed_be
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
            "ptrdiff_t": signed,
            "intptr_t": signed,
            # char
            "char": Char,
            "int8_t": Char,
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
        }
        self.ptr_size: int = ptr_size
        self.size_t_size: int = size_t_size
        self.data: bytes = data
        self.offset: int = 0
        self.byteorder: Literal["little", "big"] = byteorder
        self.debug_trace: Callable[[*tuple[Any, ...]], None] = debug_trace

    def set_offset(self, offset: int):
        self.offset = offset

    def parse_fmt_info(
        self, ptr: int, with_src_loc: bool = True, offset: int | None = None
    ):
        self.debug_trace(f"parse_fmt_info:")
        if offset is None:
            offset = self.offset

        pos: int = ptr + offset
        self.debug_trace(f"  {pos=}")

        def consume(n: int):
            nonlocal pos
            slice = self.data[pos : pos + n]
            pos += n
            return slice

        def consume_size_t():
            return int.from_bytes(consume(self.size_t_size), byteorder=self.byteorder)

        def consume_until(b: bytes = b"\x00"):
            nonlocal pos
            slice = self.data[pos : pos + len(b)]
            start = pos
            while slice != b and len(slice) == len(b):
                slice = self.data[pos : pos + len(b)]
                pos += 1

            return self.data[start:pos]

        num_args = consume_size_t()
        self.debug_trace(f"  {num_args=}")
        no_format: bool = num_args >= 2**self.size_t_size - 2 and num_args % 2 == 1
        if no_format:
            num_args = 0

        format_offset = consume_size_t()
        type_info_offsets: list[tuple[int, int]] = []
        for i in range(num_args):
            self.debug_trace(f"  {i + 1}:")
            offset_type_desc = consume_size_t()
            self.debug_trace(f"    {offset_type_desc=}")
            type_size = consume_size_t()
            self.debug_trace(f"    {type_size=}")
            type_info_offsets.append((offset_type_desc, type_size))

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
        info: FmtInfo = FmtInfo(fmt_string, self.size_t_size, self.byteorder, no_format)
        info.add_source_info(file, line)

        for offset_type_desc, type_size in type_info_offsets:
            pos = ptr + offset + offset_type_desc
            type_desc = consume_until()[:-1]
            type_desc = Emtrace.remove_qualifiers.sub(
                "", type_desc.decode("utf-8")
            ).strip()
            if type_desc[-1] == "*":
                type_desc = "*"
            self.debug_trace(f"    {type_desc=}")
            if type_size >= 2**self.size_t_size - 2:
                if type_size % 2 == 1:
                    type_size = "null_terminated"
                else:
                    type_size = "length_prefixed"
            info.add_param(self.type_mapping[type_desc], type_size)

        self.debug_trace()

        return info


def main(
    elf: Path,
    istream: Callable[[int], bytes] = sys.stdin.buffer.read,
    ptr_size: int = 8,
    size_t_size: int = 8,
    section_name: str = ".emtrace",
    with_src_loc: Literal["none", "absolute", "relative"] = "none",
    src_hyperlinks: bool = False,
    debug_script: bool = False,
):
    def error(*args, **kwargs):
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

    def trace(*args, **kwargs):
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
        f"Main args: {elf=} {istream=} {ptr_size=} {size_t_size=} {section_name=} {with_src_loc=} {src_hyperlinks=} {debug_script=}"
    )

    magic_constant = bytes.fromhex(
        "d197f522d9269fd1ad703392f659dfd0fbecbd60971325e89201b25a385d9ec7"
    )
    elffile = ELFFile(open(elf, "rb"))
    section = elffile.get_section_by_name(section_name)
    assert section is not None
    data: bytes = section.data()
    magic_offset = data.find(magic_constant)
    trace(magic_offset)
    assert magic_offset >= 0

    emtrace = Emtrace(data, ptr_size, size_t_size, debug_trace=trace)
    elffile.close()

    # info = emtrace.parse_fmt_info(0)

    magic_address = int.from_bytes(istream(ptr_size), byteorder="little")
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
                f"Stream ended in the middle of reading the bytes for the next format info location.",
            )
            error(f"Leftover bytes: {b}")
            exit(1)
        trace(f"Next format info location bytes: {b}")
        address = int.from_bytes(b, byteorder="little")
        trace(f"as address: {hex(address)}")
        if address in cache.keys():
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
                error(f"successfully parsed arguments: ", *formatted[0])
                error(f"Leftover bytes: {formatted[1]}")
                exit(1)
            case list():
                error(f"Failed to format")
                error(f"```\n{info.fmt_string}\n```")
                error(f"from {info.file}:{info.line}")
                error(f"with arguments")
                error(f"    ", *formatted[:-1])
                error(formatted[-1])
                continue
            case str():
                pass

        assert type(formatted) is str

        trace(args)
        path = None
        if with_src_loc == "absolute":
            path = info.file
        elif with_src_loc == "relative":
            path = os.path.relpath(info.file, os.getcwd())

        if path is None:
            print(formatted, end="")
            continue

        location_string = f"{path}:{info.line}"
        if len(location_string) > min_path_length:
            min_path_length = len(location_string)

        location_string = location_string + " " * (
            min_path_length - len(location_string)
        )
        if new_line_missing:
            print(f"{location_string}: ", end="")
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
                print(f"\n{location_string}: ", end="")
            else:
                print("\n" + " " * (2 + min_path_length), end="")

            print(line, end="")

        if new_line_missing:
            print()


def input(x: str) -> Callable[[int], bytes]:
    parts = x.split("://", 1)
    if len(parts) == 0:
        raise ArgumentTypeError("Bad input stream: cannot be empty string.")

    if len(parts) == 1:
        # auto detect stream_type
        id = parts[0]
        match id.split(":"):
            case ["stdin"]:
                return sys.stdin.buffer.read
            case parts:
                if len(parts) in [2, 9]:
                    stream_type = "tcp"
                else:
                    stream_type = "file"
    else:
        stream_type = parts[0]
        if stream_type not in ["tcp", "unix", "file"]:
            raise ArgumentTypeError(
                f"Bad input stream type {stream_type}: has to either be file, tcp or unix."
            )
        id = parts[1]

    match stream_type:
        case "file":
            return open(id, "rb").read
        case "unix":
            family = socket.AF_UNIX
            address = id
        case "tcp":
            parts = id.split(":")
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

    print(family, address)
    s = socket.socket(family, socket.SOCK_STREAM)
    s.connect(address)

    return s.recv


def dump(x: str | None) -> tuple[Callable[[bytes], Any], Callable[[], None]]:
    if x is None:
        return lambda _: None, lambda: None

    file = open(x, "wb")
    return file.write, file.flush


if __name__ == "__main__":
    parser = ArgumentParser("emtrace")

    _ = parser.add_argument("elf")
    _ = parser.add_argument(
        "--input", "-i", nargs="?", default=input("stdin"), type=input
    )
    _ = parser.add_argument(
        "--dump-input",
        nargs="?",
        default=dump(None),
        const="emtrace_input.bin",
        type=dump,
    )
    _ = parser.add_argument("--ptr-size", nargs="?", default=8, type=int)
    _ = parser.add_argument("--size_t-size", nargs="?", default=8, type=int)
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

    args = parser.parse_args()

    # lazy evaluate the default option ('emtrace_input.bin') of the argument
    if type(args.dump_input) is str:
        args.dump_input = dump(args.dump_input)

    def patched_input(n: int) -> bytes:
        b = args.input(n)
        args.dump_input[0](b)
        return b

    main(
        args.elf,
        patched_input,
        args.ptr_size,
        args.size_t_size,
        args.section_name,
        args.with_src_loc,
        args.src_hyperlinks,
        args.debug_script,
    )
    # flush
    args.dump_input[1]()
