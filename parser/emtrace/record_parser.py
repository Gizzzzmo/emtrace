from __future__ import annotations

from .format_info import TypeInfo, FmtInfo, Size
from dataclasses import dataclass
from typing import Any, Literal, Callable

# Per-argument flag values in TRCE record layout entries.
_FLAG_STATIC = 0
_FLAG_NULL_TERMINATED = 1
_FLAG_LENGTH_PREFIXED = 2

# TRCE record framing: 'EMT'(3) + offset_bytes(3) + 'TRCE'(4) = 10 bytes.
_RECORD_FRAMING_PREFIX = b"EMT"

RECORD_TYPE = Literal[b"TRCE", b"MGIC"]

# 32-byte magic constant that identifies the MGIC record.
_MAGIC_CONSTANT: bytes = bytes.fromhex(
    "d197f522d9269fd1ad703392f659dfd0fbecbd60971325e89201b25a385d9ec7"
)


def detect_byteorder(b: bytes) -> Literal["little", "big", "unknown"] | None:
    """Detect endianness from a size_t-shaped byte-order probe value.

    The probe value is ``0x0706050403020100`` (or the low-order bytes thereof) stored in
    native byte order.  Returns ``"little"`` or ``"big"`` on success, ``"unknown"`` if the
    pattern is not recognised, or ``None`` if *b* is shorter than 1 byte or longer than 256
    bytes, or contains a duplicate byte.
    """
    if len(b) > 256 or len(b) < 1:
        return None
    found = [False for _ in range(len(b))]
    if int(b[0]) == 0:
        byteorder: Literal["little", "big", "unknown"] = "little"
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
    return byteorder


@dataclass
class MgicInfo:
    """Parsed contents of a MGIC record."""

    record_start: int
    magic_offset: int  # absolute offset of the 32-byte magic constant within the section data
    version: int
    size_t_size: int
    ptr_size: int
    alignment_power: int
    byteorder: Literal["little", "big"]
    encoding_id: int


class RecordParser:
    def __init__(
        self,
        data: memoryview,
        ptr_size: int = 8,
        size_t_size: int = 8,
        byteorder: Literal["little", "big"] = "little",
        debug_trace: Callable[[*tuple[Any, ...]], None] = lambda *args: None,
    ) -> None:
        """Initialize the Format parser."""
        self.ptr_size: int = ptr_size
        self.size_t_size: int = size_t_size
        self.data: memoryview = data
        self.byteorder: Literal["little", "big"] = byteorder
        self.debug_trace: Callable[[*tuple[Any, ...]], None] = debug_trace

    def _size_from_flag(self, size: int, flag: int) -> Size:
        """Convert a layout size+flag pair to a Size descriptor."""
        kind = (
            "fixed"
            if flag == _FLAG_STATIC
            else "length_prefixed"
            if flag == _FLAG_LENGTH_PREFIXED
            else "null_terminated"
            if flag == _FLAG_NULL_TERMINATED
            else None
        )
        assert kind is not None, f"Unknown flag value: {flag}"
        return Size(
            kind=kind,
            size=size,
        )

    def parse_record_header(self, offset: int) -> tuple[RECORD_TYPE | None, int, int]:
        if self.data[offset : offset + 3].tobytes() != _RECORD_FRAMING_PREFIX:
            return None, offset + 1, offset + 1
        if offset + 5 >= len(self.data):
            self.debug_trace(
                f"Reached end of data while looking for record metadata {offset}"
            )
            return None, len(self.data), len(self.data)

        record_type_offset = offset + self.data[offset + 3]
        record_size_offset = offset + self.data[offset + 4]
        record_payload_offset = offset + self.data[offset + 5]

        if record_type_offset + 4 >= len(
            self.data
        ) or record_size_offset + self.size_t_size >= len(self.data):
            self.debug_trace(
                "Reached end of data while looking for record type and size"
            )
            return None, len(self.data), len(self.data)

        record_size = int.from_bytes(
            self.data[record_size_offset : record_size_offset + self.size_t_size],
            byteorder=self.byteorder,
        )
        record_end = offset + record_size
        if record_end > len(self.data):
            self.debug_trace(
                f"Record at offset {offset} claims size {record_size} which extends past end of data."
            )
            return None, len(self.data), len(self.data)

        record_type = self.data[record_type_offset : record_type_offset + 4].tobytes()

        match record_type:
            case b"TRCE":
                return b"TRCE", record_payload_offset, record_end
            case b"MGIC":
                return b"MGIC", record_payload_offset, record_end
            case _:
                self.debug_trace(
                    f"Found EMT framing prefix at offset {offset} but type was not TRCE or MGIC ({record_type} instead)"
                )
                return None, record_end, record_end

    def parse_trace_record(self, offset: int) -> tuple[FmtInfo | None, int]:
        """Parse a TRCE record format info from the data starting at *offset* (record start)."""
        self.debug_trace("parse_fmt_info:")

        record_type, payload_start, record_end = self.parse_record_header(offset)

        if record_type != b"TRCE":
            return None, record_end

        return self.parse_trace_payload(offset, payload_start), record_end

    def parse_trace_payload(self, offset: int, payload_offset: int) -> FmtInfo:
        """Parse a TRCE record payload from the data starting at *offset* (record start)."""
        pos = payload_offset
        self.debug_trace(f"  record starts at {offset}, payload starts at {pos}")

        def consume(n: int) -> memoryview:
            nonlocal pos
            s = self.data[pos : pos + n]
            pos += n
            return s

        def consume_size_t() -> int:
            return int.from_bytes(consume(self.size_t_size), byteorder=self.byteorder)

        def get_string_at(str_pos: int, delimiter: bytes = b"\x00") -> str:
            s = self.data[str_pos : str_pos + len(delimiter)].tobytes()
            start = str_pos
            while s != delimiter and len(s) == len(delimiter):
                s = self.data[str_pos : str_pos + len(delimiter)].tobytes()
                str_pos += 1

            return str(self.data[start : str_pos - len(delimiter)], "utf-8")

        num_args = consume_size_t()
        self.debug_trace(f"  {num_args=}")

        format_offset = consume_size_t()
        fmt_string = get_string_at(offset + format_offset)
        self.debug_trace(f"  {fmt_string=}")

        type_infos: list[tuple[str, TypeInfo]] = []
        for i in range(num_args):
            self.debug_trace(f"  {i + 1}:")
            # Each layout entry: [type_id_offset, size, flag, num_children] (4 size_t values)
            offset_type_desc = consume_size_t()
            self.debug_trace(f"    {offset_type_desc=}")
            type_id = get_string_at(offset + offset_type_desc)
            self.debug_trace(f"    {type_id=}")
            raw_size = consume_size_t()
            raw_flag = consume_size_t()
            type_size = self._size_from_flag(raw_size, raw_flag)
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
                # Child layout entry: [name_offset, size, flag, type_id_offset] (4 size_t values)
                child_offset_name = consume_size_t()
                child_raw_size = consume_size_t()
                child_raw_flag = consume_size_t()
                child_offset_type_id = consume_size_t()
                child_name = get_string_at(offset + child_offset_name)
                child_type_id = get_string_at(offset + child_offset_type_id)
                child_size = self._size_from_flag(child_raw_size, child_raw_flag)
                child_num_children = (
                    0  # children of children not supported in current format
                )
                self.debug_trace(f"      {child_name=} {child_type_id=} {child_size=}")
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

        return info

    def parse_all_trace_records(self) -> dict[int, FmtInfo]:
        """Parse all TRCE record FmtInfo objects from the given data."""

        infos: dict[int, FmtInfo] = {}

        i = 0
        while i < len(self.data):
            info, next_offset = self.parse_trace_record(i)
            if info is not None:
                self.debug_trace(
                    f"Parsed record at offset {i} with formatter id {info.formatter}, and format string: {info.fmt_string}"
                )
                infos[i] = info
            self.debug_trace(
                f"Advanced to offset {next_offset} after parsing record at offset {i}"
            )
            i = next_offset

        return infos

    def parse_mgic_payload(self, record_start: int, payload_offset: int) -> MgicInfo:
        """Parse a MGIC record payload.

        *record_start* is the absolute offset of the ``EMT`` framing prefix.
        *payload_offset* is the absolute offset of the payload (i.e. the first byte of the
        32-byte magic constant).

        Sizes and byte order are derived entirely from the payload itself — this method does
        **not** read ``self.size_t_size`` or ``self.byteorder``.

        Raises ``AssertionError`` on malformed data.
        """
        magic_offset = payload_offset
        assert (
            magic_offset + 32 + 7 <= len(self.data)
        ), f"MGIC payload at {magic_offset} truncated: need {32 + 7} bytes, only {len(self.data) - magic_offset} available."

        assert self.data[magic_offset : magic_offset + 32].tobytes() == _MAGIC_CONSTANT, (
            f"Magic constant mismatch at offset {magic_offset}."
        )

        # Seven single-byte metadata fields immediately follow the 32-byte magic constant.
        version_lo = int(self.data[magic_offset + 32])
        version_hi = int(self.data[magic_offset + 33])
        size_t_meta_record_offset = int(self.data[magic_offset + 34])
        size_t_size = int(self.data[magic_offset + 35])
        ptr_size = int(self.data[magic_offset + 36])
        alignment_power = int(self.data[magic_offset + 37])

        assert size_t_size > 0, "sizeof(size_t) reported as 0 in MGIC payload."

        # size_t_meta[] is at record_start + size_t_meta_record_offset and holds two size_t values:
        #   [0]  byte-order probe (0x0706050403020100 in native endianness, truncated to size_t)
        #   [1]  encoding_id
        size_t_meta_loc = record_start + size_t_meta_record_offset
        assert size_t_meta_loc + 2 * size_t_size <= len(self.data), (
            f"MGIC size_t_meta[] at {size_t_meta_loc} truncated: need {2 * size_t_size} bytes."
        )

        byteorder_id = self.data[size_t_meta_loc : size_t_meta_loc + size_t_size].tobytes()
        byteorder = detect_byteorder(byteorder_id)
        assert byteorder is not None and byteorder != "unknown", (
            f"Unable to detect byte order from probe value {byteorder_id.hex()!r} (size_t_size={size_t_size})."
        )

        encoding_id = int.from_bytes(
            self.data[size_t_meta_loc + size_t_size : size_t_meta_loc + 2 * size_t_size].tobytes(),
            byteorder=byteorder,
        )

        return MgicInfo(
            record_start=record_start,
            magic_offset=magic_offset,
            version=version_lo | (version_hi << 8),
            size_t_size=size_t_size,
            ptr_size=ptr_size,
            alignment_power=alignment_power,
            byteorder=byteorder,
            encoding_id=encoding_id,
        )

    def parse_mgic_record(self, offset: int) -> tuple[MgicInfo | None, int]:
        """Parse a MGIC record starting at *offset*.

        Requires ``self.size_t_size`` to already be set correctly (used by
        ``parse_record_header`` to read the record-size field).

        Returns ``(MgicInfo, record_end)`` on success or ``(None, record_end)`` on failure.
        No internal state is mutated.
        """
        record_type, payload_start, record_end = self.parse_record_header(offset)

        if record_type != b"MGIC":
            return None, record_end

        return self.parse_mgic_payload(offset, payload_start), record_end

    def find_and_parse_mgic(self) -> MgicInfo | None:
        """Search for the MGIC record in ``self.data`` and parse it.

        Locates the 32-byte magic constant, then scans backward for the ``EMT`` framing
        header whose payload offset points exactly to that constant.  Delegates to
        ``parse_mgic_payload`` so no prior knowledge of ``size_t_size`` or ``byteorder``
        is required.

        On success updates ``self.size_t_size``, ``self.ptr_size``, and ``self.byteorder``
        from the parsed ``MgicInfo``.

        Returns the ``MgicInfo`` on success, or ``None`` if the record cannot be found or
        parsed.
        """
        data_bytes = self.data.tobytes()
        magic_offset = data_bytes.find(_MAGIC_CONSTANT)
        if magic_offset == -1:
            self.debug_trace("MGIC magic constant not found in section data.")
            return None

        self.debug_trace(f"Found magic constant at offset {magic_offset}")

        # Scan backward from the magic constant for the EMT framing header.
        # The framing header is: b"EMT" + type_offset_byte + size_offset_byte + payload_offset_byte
        # We verify that:
        #   data[candidate : candidate+3] == b"EMT"
        #   data[candidate + type_offset]  : candidate + type_offset + 4] == b"MGIC"
        #   candidate + payload_offset_byte == magic_offset  (payload starts at magic constant)
        record_start = -1
        for candidate in range(magic_offset - 1, max(magic_offset - 256, -1), -1):
            if self.data[candidate : candidate + 3].tobytes() != b"EMT":
                continue
            if candidate + 6 > len(self.data):
                continue
            type_offset = int(self.data[candidate + 3])
            payload_offset_byte = int(self.data[candidate + 5])
            if candidate + type_offset + 4 > len(self.data):
                continue
            if self.data[candidate + type_offset : candidate + type_offset + 4].tobytes() != b"MGIC":
                continue
            if candidate + payload_offset_byte != magic_offset:
                continue
            record_start = candidate
            break

        if record_start == -1:
            self.debug_trace(
                "MGIC record framing (EMT...MGIC) not found before magic constant."
            )
            return None

        self.debug_trace(f"Found MGIC record framing at offset {record_start}")

        try:
            mgic = self.parse_mgic_payload(record_start, magic_offset)
        except AssertionError as e:
            self.debug_trace(f"Failed to parse MGIC payload: {e}")
            return None

        self.size_t_size = mgic.size_t_size
        self.ptr_size = mgic.ptr_size
        self.byteorder = mgic.byteorder

        return mgic
