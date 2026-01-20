from __future__ import annotations

from typing import Any, Literal, Callable
import struct

from .format_types import SChar, Char, MyList
from .format_info import TypeInfo, FmtInfo


class EndOfStreamException(Exception):
    pass


class Parser:
    translation: dict[str, Callable[[Parser, TypeInfo], Any]]
    istream: Callable[[int], bytes]
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
        self.istream = istream
        self.debug_trace = debug_trace
        self.size_t_size = size_t_size
        self.ptr_size = ptr_size
        self.size_t_byteorder = size_t_byteorder
        self.ptr_byteorder = ptr_byteorder
        pass

    def parse(self, id: str, info: TypeInfo):
        return self.translation[id](self, info)

    def read(self, amount: int):
        b = self.istream(amount)
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

    def format(
        self, fmt_info: FmtInfo, formatters: dict[int, Callable[[str, list[Any]], str]]
    ) -> str | list[Any] | tuple[list[Any], bytes]:
        """Format the trace message from the stream."""
        args: list[Any] = []
        for id, type_info in fmt_info.type_infos:
            args.append(self.parse(id, type_info))

        try:
            formatted = formatters[fmt_info.formatter](fmt_info.fmt_string, args)
        except (IndexError, ValueError) as err:
            return [*args, err]

        return formatted


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
