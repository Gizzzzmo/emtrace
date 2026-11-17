"""Hexdump rendering for diagnostics and debug output."""

from __future__ import annotations

_BYTES_PER_LINE = 16


def _printable(b: int) -> str:
    return chr(b) if 0x20 <= b < 0x7F else "."


def hexdump(
    data: bytes | bytearray | memoryview,
    start: int,
    end: int,
    highlight: tuple[int, int] | None = None,
    indent: str = "  ",
) -> list[str]:
    """Render ``data[start:end]`` as hexdump lines with absolute offsets.

    Lines are aligned to 16-byte boundaries. Bytes inside the half-open
    *highlight* range are marked with ``^^`` on a line below the bytes.
    """
    start = max(0, start)
    end = min(len(data), end)
    if start >= end:
        return []
    lines: list[str] = []
    width = max(4, len(f"{end:x}"))
    line_start = start - start % _BYTES_PER_LINE
    while line_start < end:
        hex_cells: list[str] = []
        marks: list[str] = []
        text: list[str] = []
        any_mark = False
        for i in range(line_start, line_start + _BYTES_PER_LINE):
            sep = " " if i % 8 != 0 or i == line_start else "  "
            if start <= i < end:
                hex_cells.append(f"{sep}{data[i]:02x}")
                text.append(_printable(data[i]))
            else:
                hex_cells.append(f"{sep}  ")
                text.append(" ")
            marked = highlight is not None and highlight[0] <= i < highlight[1]
            any_mark |= marked
            marks.append(f"{' ' * len(sep)}{'^^' if marked else '  '}")
        prefix = f"{indent}{line_start:0{width}x}:"
        lines.append(f"{prefix}{''.join(hex_cells)}  |{''.join(text)}|")
        if any_mark:
            lines.append(f"{' ' * len(prefix)}{''.join(marks).rstrip()}")
        line_start += _BYTES_PER_LINE
    return lines


def context_hexdump(
    data: bytes | bytearray | memoryview,
    offset: int,
    length: int,
    context: int = 16,
    max_bytes: int = 64,
    indent: str = "  ",
) -> list[str]:
    """Hexdump around ``data[offset:offset+length]`` with the range highlighted.

    At most *max_bytes* of the highlighted range are shown (plus *context*
    bytes on either side), so that huge ranges don't flood the output.
    """
    length = max(length, 1)
    shown_end = offset + min(length, max_bytes)
    lines = hexdump(
        data,
        offset - context,
        shown_end + context,
        highlight=(offset, offset + length),
        indent=indent,
    )
    if length > max_bytes:
        lines.append(f"{indent}… ({length - max_bytes} more highlighted bytes)")
    return lines
