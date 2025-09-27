#!/usr/bin/env python3

from __future__ import annotations
from typing import Callable, Literal, Any, override
from argparse import ArgumentParser, ArgumentTypeError
from pathlib import Path
from dataclasses import dataclass
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

    None is returned if the sequence is longer than 256, shorter than 2, or if there is a
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


class MyList[T]:
    def __init__(self, list_arg: list[T]) -> None:
        self.list: list[T] = list_arg

    @override
    def __format__(self, format_spec: str, /) -> str:
        parts = format_spec.split("*", 1)
        match parts:
            case [_]:
                return self.list.__format__(format_spec)
            case [sep, el_spec]:
                return sep.join([el.__format__(el_spec) for el in self.list])
            case _:
                assert False

    @override
    def __repr__(self) -> str:
        return self.list.__repr__()


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
        self.type_infos: list[tuple[str, TypeInfo]] = []
        self.file: str = ""
        self.line: int = -1

    def add_source_info(self, file: str, line: int) -> None:
        """Add source location information to the format info."""
        self.file = file
        self.line = line

    def add_param(self, id: str, type_info: TypeInfo) -> None:
        """Add a parameter to the format info."""
        self.type_infos.append((id, type_info))

    def format(self, parser: Parser) -> str | list[Any] | tuple[list[Any], bytes]:
        """Format the trace message from the stream."""
        args: list[Any] = []
        for id, type_info in self.type_infos:
            args.append(parser.parse(id, type_info))

        try:
            formatted = self.formatter(self.fmt_string, args)
        except (IndexError, ValueError) as err:
            return [*args, err]

        return formatted


@dataclass
class Size:
    min_size: int
    length_prefixed: bool
    null_terminated: bool


class TypeInfo:
    size: Size
    children: dict[int | str, tuple[str, TypeInfo]]

    def __init__(
        self, size: Size, children: dict[int | str, tuple[str, TypeInfo]] | None = None
    ) -> None:
        self.size = size
        if children is None:
            self.children = {}
        else:
            self.children = children


class EndOfStreamException(Exception):
    pass


class Parser:
    translation: dict[str, Callable[[Parser, TypeInfo], Any]]
    _istream: Callable[[int], bytes]
    debug_trace: Callable[[*tuple[Any, ...]], None] = lambda *args: None
    size_t_size: int
    ptr_size: int
    size_t_byteorder: Literal["big", "little"]
    ptr_byteorder: Literal["big", "little"]

    def __init__(
        self,
        translation: dict[str, Callable[[Parser, TypeInfo], Any]],
        istream: Callable[[int], bytes],
        debug_trace: Callable[[*tuple[Any, ...]], None] = lambda *args: None,
        size_t_size: int = 8,
        ptr_size: int = 8,
        size_t_byteorder: Literal["big", "little"] = "little",
        ptr_byteorder: Literal["big", "little"] = "little",
    ):
        self.translation = translation
        self._istream = istream
        self.debug_trace = debug_trace
        self.size_t_size = size_t_size
        self.ptr_size = ptr_size
        self.size_t_byteorder = size_t_byteorder
        self.ptr_byteorder = ptr_byteorder
        pass

    def parse(self, id: str, info: TypeInfo):
        return self.translation[id](self, info)

    def read(self, amount: int):
        b = self._istream(amount)
        if len(b) < amount:
            raise EndOfStreamException

        return b

    def read_ptr(self):
        return int.from_bytes(
            self.read(self.ptr_size), byteorder=self.ptr_byteorder, signed=False
        )

    def read_size_t(self):
        return int.from_bytes(
            self.read(self.size_t_size), byteorder=self.size_t_byteorder, signed=False
        )

    def read_until(self, b: bytes = b"\x00") -> bytes:
        bs = bytearray(self.read(len(b)))
        while bytes(bs[-len(b) :]) != b:
            bs.extend(self.read(1))

        return bytes(bs[: -len(b)])


def signed_le(parser: Parser, info: TypeInfo) -> int:
    """Interpret bytes as a little-endian signed integer."""
    assert not info.size.null_terminated

    if info.size.length_prefixed:
        size = parser.read_size_t()
    else:
        size = info.size.min_size

    b = parser.read(size)
    return int.from_bytes(b, byteorder="little", signed=True)


def signed_be(parser: Parser, info: TypeInfo) -> int:
    """Interpret bytes as a big-endian signed integer."""
    assert not info.size.null_terminated

    if info.size.length_prefixed:
        size = parser.read_size_t()
    else:
        size = info.size.min_size

    b = parser.read(size)
    return int.from_bytes(b, byteorder="big", signed=True)


def unsigned_le(parser: Parser, info: TypeInfo) -> int:
    """Interpret bytes as a little-endian unsigned integer."""
    assert not info.size.null_terminated

    if info.size.length_prefixed:
        size = parser.read_size_t()
    else:
        size = info.size.min_size

    b = parser.read(size)
    return int.from_bytes(b, byteorder="little", signed=False)


def unsigned_be(parser: Parser, info: TypeInfo) -> int:
    """Interpret bytes as a big-endian unsigned integer."""
    assert not info.size.null_terminated

    if info.size.length_prefixed:
        size = parser.read_size_t()
    else:
        size = info.size.min_size

    b = parser.read(size)
    return int.from_bytes(b, byteorder="little", signed=True)


def string(parser: Parser, info: TypeInfo) -> str:
    if info.size.null_terminated:
        assert not info.size.length_prefixed
        b = parser.read_until()
        parser.debug_trace(b)
        return b.decode("utf-8")

    if info.size.length_prefixed:
        size = parser.read_size_t()
    else:
        size = info.size.min_size

    return parser.read(size).decode("utf-8")


def to_bool(parser: Parser, info: TypeInfo):
    x = unsigned_be(parser, info)
    return x != 0


def float_le(parser: Parser, info: TypeInfo) -> float:
    """Interpret bytes as a little-endian float."""
    assert not info.size.null_terminated

    if info.size.length_prefixed:
        size = parser.read_size_t()
    else:
        size = info.size.min_size

    assert size in [2, 4, 8]
    b = parser.read(size)
    match size:
        case 2:
            return struct.unpack("<e", b)[0]
        case 4:
            return struct.unpack("<f", b)[0]
        case 8:
            return struct.unpack("<d", b)[0]
        case _:
            assert False


def float_be(parser: Parser, info: TypeInfo) -> float:
    """Interpret bytes as a big-endian float."""
    assert not info.size.null_terminated

    if info.size.length_prefixed:
        size = parser.read_size_t()
    else:
        size = info.size.min_size

    assert size in [2, 4, 8]
    b = parser.read(size)
    match size:
        case 2:
            return struct.unpack(">e", b)[0]
        case 4:
            return struct.unpack(">f", b)[0]
        case 8:
            return struct.unpack(">d", b)[0]
        case _:
            assert False


def signed_char(parser: Parser, info: TypeInfo):
    assert not info.size.null_terminated
    assert not info.size.length_prefixed
    assert info.size.min_size == 1

    return SChar(parser.read(info.size.min_size))


def char(parser: Parser, info: TypeInfo):
    assert not info.size.null_terminated
    assert not info.size.length_prefixed
    assert info.size.min_size == 1

    return Char(parser.read(info.size.min_size))


def to_list(parser: Parser, info: TypeInfo) -> MyList[Any]:
    parser.debug_trace(info.size)
    if info.size.length_prefixed:
        size = parser.read_size_t()
    else:
        size = info.size.min_size

    parser.debug_trace(f"{size=}")

    parsed = []
    for _ in range(size):
        child_id, child_info = info.children[""]
        parsed.append(parser.parse(child_id, child_info))

    return MyList(parsed)


translation_le: dict[str, Callable[[Parser, TypeInfo], Any]] = {
    # signed
    "signed": signed_le,
    "int": signed_le,
    "signed int": signed_le,
    "int32_t": signed_le,
    "long": signed_le,
    "signed long": signed_le,
    "long long": signed_le,
    "signed long long": signed_le,
    "int64_t": signed_le,
    "int128_t": signed_le,
    "short": signed_le,
    "signed short": signed_le,
    "int16_t": signed_le,
    "ssize_t": signed_le,
    "ptrdiff_t": signed_le,
    "intptr_t": signed_le,
    # char
    "signed char": signed_char,
    "int8_t": signed_char,
    "unsigned char": char,
    "char": char,
    "uint8_t": char,
    # unsigned
    "unsigned": unsigned_le,
    "unsigned int": unsigned_le,
    "uint32_t": unsigned_le,
    "unsigned long": unsigned_le,
    "unsigned long long": unsigned_le,
    "uint64_t": unsigned_le,
    "uint128_t": unsigned_le,
    "uint16_t": unsigned_le,
    "size_t": unsigned_le,
    "uintptr_t": unsigned_le,
    "*": unsigned_le,
    # string
    "string": string,
    # other
    "bool": to_bool,
    "_Bool": to_bool,
    "float": float_le,
    "double": float_le,
    # list
    "list": to_list,
}

translation_be: dict[str, Callable[[Parser, TypeInfo], Any]] = {
    # signed
    "signed": signed_be,
    "int": signed_be,
    "signed int": signed_be,
    "int32_t": signed_be,
    "long": signed_be,
    "signed long": signed_be,
    "long long": signed_be,
    "signed long long": signed_be,
    "int64_t": signed_be,
    "int128_t": signed_be,
    "short": signed_be,
    "signed short": signed_be,
    "int16_t": signed_be,
    "ssize_t": signed_be,
    "ptrdiff_t": signed_be,
    "intptr_t": signed_be,
    # char
    "signed char": signed_char,
    "int8_t": signed_char,
    "unsigned char": char,
    "char": char,
    "uint8_t": char,
    # unsigned
    "unsigned": unsigned_be,
    "unsigned int": unsigned_be,
    "uint32_t": unsigned_be,
    "unsigned long": unsigned_be,
    "unsigned long long": unsigned_be,
    "uint64_t": unsigned_be,
    "uint128_t": unsigned_be,
    "uint16_t": unsigned_be,
    "size_t": unsigned_be,
    "uintptr_t": unsigned_be,
    "*": unsigned_be,
    # string
    "string": string,
    # other
    "bool": to_bool,
    "_Bool": to_bool,
    "float": float_be,
    "double": float_be,
    # list
    "list": to_list,
}


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

    def size_from_raw_size(self, raw_size: int):
        return Size(
            raw_size & ~(self.null_terminated | self.length_prefixed),
            (raw_size & self.length_prefixed) == self.length_prefixed,
            (raw_size & self.null_terminated) == self.null_terminated,
        )

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

        def get_string_at(pos: int, delimiter: bytes = b"\x00") -> str:
            s = self.data[pos : pos + len(delimiter)]
            start = pos
            while s != delimiter and len(s) == len(delimiter):
                s = self.data[pos : pos + len(delimiter)]
                pos += 1

            return self.data[start : pos - len(delimiter)].decode("utf-8")

        num_args = consume_size_t()
        self.debug_trace(f"  {num_args=}")

        format_offset = consume_size_t()
        fmt_string = get_string_at(ptr + offset + format_offset)
        self.debug_trace(f"  {fmt_string=}")

        type_infos: list[tuple[str, TypeInfo]] = []
        for i in range(num_args):
            self.debug_trace(f"  {i + 1}:")
            offset_type_desc = consume_size_t()
            self.debug_trace(f"    {offset_type_desc=}")
            type_id = get_string_at(ptr + offset + offset_type_desc)
            self.debug_trace(f"    {type_id=}")
            type_size = self.size_from_raw_size(consume_size_t())
            self.debug_trace(f"    {type_size=}")
            type_info = TypeInfo(type_size)
            num_children = consume_size_t()
            self.debug_trace(f"    {num_children=}")

            if num_children > 0:
                stack: tuple[list[TypeInfo], list[int]] = [type_info], [num_children]
            else:
                stack = [], []

            while len(stack[0]) > 0:
                assert len(stack[0]) == len(stack[1])
                assert stack[1][-1] > 0
                self.debug_trace(f"      {stack[1]=}")
                child_offset_name = consume_size_t()
                child_size = self.size_from_raw_size(consume_size_t())
                child_num_children = consume_size_t()
                child_offset_type_id = consume_size_t()
                child_name = get_string_at(ptr + offset + child_offset_name)
                child_type_id = get_string_at(ptr + offset + child_offset_type_id)
                self.debug_trace(
                    f"      {child_name=} {child_type_id=} {child_size=} {child_num_children=}"
                )
                child_type_info = TypeInfo(child_size)

                stack[0][-1].children[child_name] = (child_type_id, child_type_info)
                stack[1][-1] -= 1

                if stack[1][-1] == 0:
                    _ = stack[0].pop()
                    _ = stack[1].pop()

                if child_num_children > 0:
                    stack[0].append(child_type_info)
                    stack[1].append(child_num_children)

            type_infos.append((type_id, type_info))

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
            file = get_string_at(ptr + offset + file_offset)
        else:
            file = ""
            line = -1

        info: FmtInfo = FmtInfo(fmt_string, self.size_t_size, self.byteorder, formatter)
        info.add_source_info(file, line)

        for type_id, type_info in type_infos:
            info.add_param(type_id, type_info)

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
    magic_address *= 2**alignment_power
    emtrace.set_offset(magic_offset - magic_address)

    cache: dict[int, FmtInfo] = {}
    min_path_length: int = 0
    new_line_missing = True

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
        address *= 2**alignment_power
        trace(f"adjusted address: {hex(address)}")
        if address in cache:
            trace("Associated format info already parsed into cache.")
            info = cache[address]
        else:
            trace("Not cached yet.")
            info = emtrace.parse_fmt_info(address)
            cache[address] = info

        trace(hex(address))
        formatted = info.format(parser)
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

    _ = parser.add_argument(
        "elf",
        help="Path to either an elf executable, or a raw binary that contains the .emtrace section bytes of the program whose output you are processing.",
    )
    _ = parser.add_argument(
        "--input",
        "-i",
        nargs="?",
        default=get_input_stream("stdin"),
        type=get_input_stream,
        help="Where to get read the bytes from that the traced binary produces. If not supplied it reads from stdin, otherwise it defaults to interpreting the argument as a file path from which the data will be read. It can also read from either a unix- or IP-socket by specifying either tcp://<ip>:<port> or unix://<path/to/unix/socket>",
    )
    _ = parser.add_argument(
        "--dump-input",
        nargs="?",
        default=dump(None),
        const="emtrace_input.bin",
        type=dump,
        help="Separately dump the bytes being processed to a file (by default to emtrace_input.bin).",
    )
    _ = parser.add_argument(
        "--section-name",
        nargs="?",
        default=".emtrace",
        type=str,
        help="Specify which section of the elf file to read the format information from.",
    )
    _ = parser.add_argument(
        "--with-src-loc",
        nargs="?",
        default="none",
        const="relative",
        choices=["none", "absolute", "relative"],
        type=str,
        help="Prepend, to every line of trace output, the source location from which it originated.",
    )
    _ = parser.add_argument(
        "--src-hyperlinks", action="store_true", help="Doesn't work yet."
    )
    _ = parser.add_argument(
        "--debug-script",
        action="store_true",
        help="Make the script output trace information for debugging.",
    )
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
