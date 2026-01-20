from .format_info import TypeInfo, FmtInfo, Size
from typing import Any, Literal, Callable


class FormatParser:
    FMT_INFO_HEAD: bytes = b"TRACE"

    def __init__(
        self,
        data: memoryview,
        ptr_size: int = 8,
        size_t_size: int = 8,
        null_terminated: int | None = None,
        length_prefixed: int | None = None,
        byteorder: Literal["little", "big"] = "little",
        debug_trace: Callable[[*tuple[Any, ...]], None] = lambda *args: None,
    ) -> None:
        """Initialize the Format parser."""
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

        self.data: memoryview = data
        self.byteorder: Literal["little", "big"] = byteorder
        self.debug_trace: Callable[[*tuple[Any, ...]], None] = debug_trace

    def size_from_raw_size(self, raw_size: int):
        return Size(
            raw_size & ~(self.null_terminated | self.length_prefixed),
            (raw_size & self.length_prefixed) == self.length_prefixed,
            (raw_size & self.null_terminated) == self.null_terminated,
        )

    def parse_fmt_info(self, offset: int) -> tuple[FmtInfo, int]:
        """Parse the format info from the data."""
        self.debug_trace("parse_fmt_info:")

        pos = offset
        self.debug_trace(f"  {pos=}")

        def consume(n: int) -> memoryview:
            nonlocal pos
            s = self.data[pos : pos + n]
            pos += n
            return s

        assert (
            consume(len(FormatParser.FMT_INFO_HEAD)).tobytes()
            == FormatParser.FMT_INFO_HEAD
        )

        pos += consume(1)[0] - len(FormatParser.FMT_INFO_HEAD)
        self.debug_trace(f"  adjusted offset after 'TRACE' header: {pos}")

        def consume_size_t() -> int:
            return int.from_bytes(consume(self.size_t_size), byteorder=self.byteorder)

        max_offset = 0

        def get_string_at(pos: int, delimiter: bytes = b"\x00") -> str:
            s = self.data[pos : pos + len(delimiter)].tobytes()
            start = pos
            while s != delimiter and len(s) == len(delimiter):
                s = self.data[pos : pos + len(delimiter)].tobytes()
                pos += 1

            nonlocal max_offset
            max_offset = max(max_offset, pos + len(delimiter))

            return str(self.data[start : pos - len(delimiter)], "utf-8")

        num_args = consume_size_t()
        self.debug_trace(f"  {num_args=}")

        format_offset = consume_size_t()
        fmt_string = get_string_at(offset + format_offset)
        self.debug_trace(f"  {fmt_string=}")

        type_infos: list[tuple[str, TypeInfo]] = []
        for i in range(num_args):
            self.debug_trace(f"  {i + 1}:")
            offset_type_desc = consume_size_t()
            self.debug_trace(f"    {offset_type_desc=}")
            type_id = get_string_at(offset + offset_type_desc)
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
                child_name = get_string_at(offset + child_offset_name)
                child_type_id = get_string_at(offset + child_offset_type_id)
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

        file_offset = consume_size_t()
        line = consume_size_t()
        file = get_string_at(offset + file_offset)

        info: FmtInfo = FmtInfo(fmt_string, formatter_id)
        info.add_source_info(file, line)

        for type_id, type_info in type_infos:
            info.add_param(type_id, type_info)

        self.debug_trace()

        max_offset = max(max_offset, pos)

        return info, max_offset

    def parse_all_fmt_infos(self) -> dict[int, FmtInfo]:
        """Parse all FmtInfo objects from the given data."""

        infos: dict[int, FmtInfo] = {}

        i = 0
        while i < len(self.data):
            id = self.data[i : i + len(FormatParser.FMT_INFO_HEAD)].tobytes()
            if id == FormatParser.FMT_INFO_HEAD:
                self.debug_trace(f"Found FmtInfo at offset {i}")
                self.debug_trace(f"First few bytes: {self.data[i : i + 16].tobytes()}")
                fmt_info, offset = self.parse_fmt_info(i)
                infos[i] = fmt_info
                i = offset
                continue
            i += 1

        return infos


# _ = parse_all_fmt_infos(memoryview(b""))
