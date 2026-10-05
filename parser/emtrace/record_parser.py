from __future__ import annotations

from .format_info import TypeInfo, FmtInfo, Size
from dataclasses import dataclass
from typing import Any, Literal, Callable

# Per-argument flag values in TRCE record layout entries.
_FLAG_STATIC = 0
_FLAG_NULL_TERMINATED = 1
_FLAG_LENGTH_PREFIXED = 2

# Record framing: 'EMT'(3) + metadata(7) + type tag(4) = 14 bytes.
#
#   [0..2]  'E','M','T'
#   [3]     type offset (single-byte, points at the 4-byte type tag)
#   [4]     size offset (points at the size_t record_size value)
#   [5]     payload offset
#   [6]     endianness probe offset (points at a size_t-shaped probe value)
#   [7]     size_t width in bytes
#   [8]     major version of the record type's layout
#   [9]     minor version of the record type's layout
#   [10..13] type tag
_RECORD_FRAMING_PREFIX = b"EMT"
_FRAMING_SIZE = 14
_FRAMING_METADATA_SIZE = 7  # between the 'EMT' prefix and the type tag

RECORD_TYPE = Literal[b"TRCE", b"MGIC"]

# Highest record-type versions this parser knows how to decode.  Record types
# are versioned independently (see TRACE_FORMAT.md).
KNOWN_VERSIONS: dict[bytes, tuple[int, int]] = {
    b"MGIC": (1, 0),
    b"TRCE": (1, 0),
}

# `size_t` widths the parser is able to decode (endianness probe detection
# supports arbitrary widths, but widths beyond 8 make no sense).
_MIN_SIZE_T_SIZE = 1
_MAX_SIZE_T_SIZE = 8

# 32-byte magic constant that identifies the MGIC record.
_MAGIC_CONSTANT: bytes = bytes.fromhex(
    "d197f522d9269fd1ad703392f659dfd0fbecbd60971325e89201b25a385d9ec7"
)

# MGIC payload layout (relative to the payload offset):
#   [0..31]  32-byte magic constant
#   [32]     sizeof(emt_ptr_t)
#   [33]     alignment power
#   [34]     encoding id
_MGIC_PTR_SIZE_OFFSET = 32
_MGIC_ALIGNMENT_POWER_OFFSET = 33
_MGIC_ENCODING_ID_OFFSET = 34


class RecordVersionError(Exception):
    """A record's major version is not supported by this parser.

    Minor version differences are deliberately *not* errors: parsers decode
    unknown minor versions with the rules of the highest minor version they
    know (see TRACE_FORMAT.md).
    """

    def __init__(
        self,
        record_type: bytes,
        major: int,
        minor: int,
        known: tuple[int, int],
    ) -> None:
        self.record_type = record_type
        self.major = major
        self.minor = minor
        self.known = known
        super().__init__(
            f"Unsupported {record_type.decode('ascii', 'replace')} record version "
            f"{major}.{minor} (parser knows {'.'.join(map(str, known))})."
        )


@dataclass
class RecordInfo:
    """Per-record decoding context parsed from the record framing."""

    record_start: int
    record_size: int
    type_offset: int  # absolute offset of the 4-byte type tag
    size_offset: int  # absolute offset of the size_t record_size value
    payload_offset: int  # absolute offset of the payload
    endianness_offset: int  # absolute offset of the size_t endianness probe
    size_t_size: int
    major: int
    minor: int
    byteorder: Literal["little", "big", "unknown"]
    record_type: bytes | None = None  # 4-byte tag; None when unknown

    @property
    def record_end(self) -> int:
        return self.record_start + self.record_size

    @property
    def version(self) -> tuple[int, int]:
        return (self.major, self.minor)


@dataclass
class MgicInfo:
    """Parsed contents of a MGIC record."""

    record_start: int
    magic_offset: int  # absolute offset of the 32-byte magic constant
    size_t_size: int  # from the MGIC record framing
    byteorder: Literal["little", "big"]  # from the MGIC record framing
    version_major: int
    version_minor: int
    ptr_size: int
    alignment_power: int
    encoding_id: int

    @property
    def version(self) -> tuple[int, int]:
        return (self.version_major, self.version_minor)


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

    def _read_framing(self, offset: int) -> RecordInfo | None:
        """Parse the 14-byte record framing at *offset*.

        Returns ``None`` when there is no structurally valid record framing at
        *offset* (no ``EMT`` prefix, truncated framing, undetectable
        endianness, or a record that extends past the end of the data).  No
        assumption is made about the record type.
        """
        data = self.data
        if offset + _FRAMING_SIZE > len(data):
            return None
        if data[offset : offset + 3].tobytes() != _RECORD_FRAMING_PREFIX:
            return None

        type_off = int(data[offset + 3])
        size_off = int(data[offset + 4])
        payload_off = int(data[offset + 5])
        endi_off = int(data[offset + 6])
        size_t_size = int(data[offset + 7])
        major = int(data[offset + 8])
        minor = int(data[offset + 9])

        if not (_MIN_SIZE_T_SIZE <= size_t_size <= _MAX_SIZE_T_SIZE):
            self.debug_trace(
                f"Record framing at {offset}: unsupported size_t width {size_t_size}."
            )
            return None

        if offset + type_off + 4 > len(data):
            return None
        record_type = data[offset + type_off : offset + type_off + 4].tobytes()

        probe_start = offset + endi_off
        if probe_start + size_t_size > len(data):
            return None
        probe = data[probe_start : probe_start + size_t_size].tobytes()
        byteorder = detect_byteorder(probe)
        if byteorder is None or byteorder == "unknown":
            self.debug_trace(
                f"Record framing at {offset}: unable to detect byte order from probe "
                f"{probe.hex()!r}."
            )
            return None
        assert byteorder in ("little", "big")

        size_start = offset + size_off
        if size_start + size_t_size > len(data):
            return None
        record_size = int.from_bytes(
            data[size_start : size_start + size_t_size], byteorder=byteorder
        )
        if record_size < _FRAMING_SIZE:
            self.debug_trace(
                f"Record framing at {offset}: record_size {record_size} is smaller "
                f"than the {_FRAMING_SIZE}-byte framing."
            )
            return None
        if offset + record_size > len(data):
            self.debug_trace(
                f"Record framing at {offset}: record_size {record_size} extends past "
                f"end of data ({len(data)})."
            )
            return None

        if offset + payload_off > len(data):
            return None

        return RecordInfo(
            record_start=offset,
            record_size=record_size,
            type_offset=offset + type_off,
            size_offset=size_start,
            payload_offset=offset + payload_off,
            endianness_offset=probe_start,
            size_t_size=size_t_size,
            major=major,
            minor=minor,
            byteorder=byteorder,
            record_type=record_type,
        )

    def parse_record_header(
        self, offset: int
    ) -> tuple[RECORD_TYPE | None, RecordInfo | None, int]:
        """Parse a record framing at *offset*.

        Returns ``(record_type, info, next_offset)``:

        - ``record_type`` is the 4-byte type tag when it belongs to a known
          record type, otherwise ``None``.
        - ``info`` carries the per-record decoding context.  It is ``None``
          only when there is no structurally valid framing at *offset*; for
          records with an unknown type tag ``info`` is still returned so that
          callers can skip the whole record.
        - ``next_offset`` is the offset where a scanning caller should continue.
        """
        if self.data[offset : offset + 3].tobytes() != _RECORD_FRAMING_PREFIX:
            return None, None, offset + 1
        if offset + _FRAMING_SIZE > len(self.data):
            return None, None, len(self.data)

        info = self._read_framing(offset)
        if info is None:
            # An 'EMT' prefix is present but the framing is invalid — scanning
            # byte by byte would produce spurious results, so bail out.
            return None, None, len(self.data)

        assert info.record_type is not None
        if info.record_type in KNOWN_VERSIONS:
            return info.record_type, info, info.record_end  # type: ignore[return-value]

        self.debug_trace(
            f"Found record framing at offset {offset} with unknown type "
            f"{info.record_type!r} (version {info.major}.{info.minor})."
        )
        return None, info, info.record_end

    def parse_trace_record(self, offset: int) -> tuple[FmtInfo | None, int]:
        """Parse a TRCE record format info from the data starting at *offset* (record start).

        Raises ``RecordVersionError`` when the record's major version is not
        supported by this parser.
        """
        self.debug_trace("parse_fmt_info:")

        record_type, info, next_offset = self.parse_record_header(offset)

        if record_type != b"TRCE" or info is None:
            return None, next_offset

        known_major, known_minor = KNOWN_VERSIONS[b"TRCE"]
        if info.major != known_major:
            raise RecordVersionError(
                b"TRCE", info.major, info.minor, (known_major, known_minor)
            )

        record_info = self.parse_trace_payload(
            offset,
            info.payload_offset,
            info.record_end,
            size_t_size=info.size_t_size,
            byteorder=info.byteorder,  # type: ignore[arg-type]
        )
        record_info.add_version(info.major, info.minor)
        return record_info, info.record_end

    def parse_trace_payload(
        self,
        offset: int,
        payload_offset: int,
        record_end: int | None = None,
        *,
        size_t_size: int | None = None,
        byteorder: Literal["little", "big"] | None = None,
    ) -> FmtInfo:
        """Parse a TRCE record payload from the data starting at *offset* (record start).

        If *record_end* is given, all layout consumption is bounds-checked against it
        (layout entries must not extend past the end of the record).

        *size_t_size* and *byteorder* describe the record's own decoding context; when
        not given, the parser's global context (as established by ``find_and_parse_mgic``)
        is used.
        """
        sz = self.size_t_size if size_t_size is None else size_t_size
        bo = self.byteorder if byteorder is None else byteorder

        pos = payload_offset
        self.debug_trace(f"  record starts at {offset}, payload starts at {pos}")

        limit = len(self.data) if record_end is None else record_end

        def consume(n: int) -> memoryview:
            nonlocal pos
            assert pos + n <= limit, (
                f"TRCE record at {offset}: layout consumption at {pos} + {n} extends "
                f"past the record end at {limit}."
            )
            s = self.data[pos : pos + n]
            pos += n
            return s

        def consume_size_t() -> int:
            return int.from_bytes(consume(sz), byteorder=bo)

        def get_string_at(str_pos: int, delimiter: bytes = b"\x00") -> str:
            start = str_pos
            s = self.data[str_pos : str_pos + len(delimiter)].tobytes()
            while s != delimiter and len(s) == len(delimiter):
                str_pos += 1
                s = self.data[str_pos : str_pos + len(delimiter)].tobytes()
            assert s == delimiter and len(s) == len(delimiter), (
                f"String at {start} is not terminated by {delimiter!r} before end of data."
            )
            try:
                return str(self.data[start:str_pos], "utf-8")
            except UnicodeDecodeError as e:
                raise AssertionError(
                    f"String at {start} is not valid UTF-8: {e}"
                ) from e

        num_args = consume_size_t()
        self.debug_trace(f"  {num_args=}")

        format_offset = consume_size_t()
        fmt_string = get_string_at(offset + format_offset)
        self.debug_trace(f"  {fmt_string=}")

        type_infos: list[tuple[str, TypeInfo]] = []
        for i in range(num_args):
            self.debug_trace(f"  {i + 1}:")
            # Top-level type entry: [type_id_offset, size, flag, num_children]
            # (4 size_t values; top-level types have no parent, hence no name pointer).
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

            # Children are serialized in pre-order: a child entry is followed
            # immediately by the entries of its own children, and so on.  The
            # stack holds one (parent, remaining-count) pair per open subtree.
            # Child entry: [name_offset, type_id_offset, size, flag, num_children]
            # (5 size_t values; children have a parent, hence the name pointer).
            if num_children > 0:
                stack: tuple[list[TypeInfo], list[int]] = [type_info], [num_children]
            else:
                stack = [], []

            while len(stack[0]) > 0:
                assert len(stack[0]) == len(stack[1])
                assert stack[1][-1] > 0
                self.debug_trace(f"      {stack[1]=}")
                child_offset_name = consume_size_t()
                child_offset_type_id = consume_size_t()
                child_raw_size = consume_size_t()
                child_raw_flag = consume_size_t()
                child_num_children = consume_size_t()
                child_name = get_string_at(offset + child_offset_name)
                child_type_id = get_string_at(offset + child_offset_type_id)
                child_size = self._size_from_flag(child_raw_size, child_raw_flag)
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

        return info

    def parse_all_trace_records(self) -> dict[int, FmtInfo]:
        """Parse all TRCE record FmtInfo objects from the given data.

        Records with an unsupported major version raise ``RecordVersionError``.
        """
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

    def parse_mgic_payload(self, info: RecordInfo) -> MgicInfo:
        """Parse a MGIC record payload from an already-parsed framing.

        Sizes and byte order come from the framing context *info* — this method
        does **not** read ``self.size_t_size`` or ``self.byteorder``.

        Raises ``AssertionError`` on malformed data.
        """
        assert info.record_type == b"MGIC", (
            f"parse_mgic_payload called on a {info.record_type!r} record."
        )

        magic_offset = info.payload_offset
        assert magic_offset + _MGIC_ENCODING_ID_OFFSET + 1 <= info.record_end, (
            f"MGIC payload at {magic_offset} truncated."
        )

        assert (
            self.data[magic_offset : magic_offset + 32].tobytes() == _MAGIC_CONSTANT
        ), f"Magic constant mismatch at offset {magic_offset}."

        ptr_size = int(self.data[magic_offset + _MGIC_PTR_SIZE_OFFSET])
        alignment_power = int(self.data[magic_offset + _MGIC_ALIGNMENT_POWER_OFFSET])

        assert ptr_size > 0, "sizeof(emt_ptr_t) reported as 0 in MGIC payload."

        encoding_id = int(self.data[magic_offset + _MGIC_ENCODING_ID_OFFSET])

        return MgicInfo(
            record_start=info.record_start,
            magic_offset=magic_offset,
            size_t_size=info.size_t_size,
            byteorder=info.byteorder,  # type: ignore[arg-type]
            version_major=info.major,
            version_minor=info.minor,
            ptr_size=ptr_size,
            alignment_power=alignment_power,
            encoding_id=encoding_id,
        )

    def parse_mgic_record(self, offset: int) -> tuple[MgicInfo | None, int]:
        """Parse a MGIC record starting at *offset*.

        Returns ``(MgicInfo, record_end)`` on success or ``(None, next_offset)``
        on failure.  No internal state is mutated.  Raises
        ``RecordVersionError`` when the MGIC record's major version is not
        supported by this parser.
        """
        record_type, info, next_offset = self.parse_record_header(offset)

        if record_type != b"MGIC" or info is None:
            return None, next_offset

        known_major, known_minor = KNOWN_VERSIONS[b"MGIC"]
        if info.major != known_major:
            raise RecordVersionError(
                b"MGIC", info.major, info.minor, (known_major, known_minor)
            )

        return self.parse_mgic_payload(info), info.record_end

    def find_and_parse_mgic(self) -> MgicInfo | None:
        """Search for the MGIC record in ``self.data`` and parse it.

        Locates the 32-byte magic constant, then scans backward for the ``EMT``
        framing header whose payload offset points exactly to that constant.
        Delegates to ``parse_mgic_payload`` so no prior knowledge of
        ``size_t_size`` or ``byteorder`` is required.

        On success updates ``self.size_t_size``, ``self.ptr_size``, and
        ``self.byteorder`` from the parsed ``MgicInfo``.

        Returns the ``MgicInfo`` on success, or ``None`` if the record cannot be
        found or parsed.  Raises ``RecordVersionError`` when the MGIC record's
        major version is not supported by this parser.
        """
        data_bytes = self.data.tobytes()
        magic_offset = data_bytes.find(_MAGIC_CONSTANT)
        if magic_offset == -1:
            self.debug_trace("MGIC magic constant not found in section data.")
            return None

        self.debug_trace(f"Found magic constant at offset {magic_offset}")

        # Scan backward from the magic constant for the EMT framing header
        # whose payload offset points exactly at the magic constant, and whose
        # type tag is b"MGIC".
        record_start = -1
        record_info: RecordInfo | None = None
        for candidate in range(magic_offset - 1, max(magic_offset - 256, -1), -1):
            info = self._read_framing(candidate)
            if info is None:
                continue
            if info.record_type != b"MGIC":
                continue
            if info.payload_offset != magic_offset:
                continue
            record_start = candidate
            record_info = info
            break

        if record_start == -1 or record_info is None:
            self.debug_trace(
                "MGIC record framing (EMT...MGIC) not found before magic constant."
            )
            return None

        self.debug_trace(f"Found MGIC record framing at offset {record_start}")

        mgic = self.parse_mgic_payload(record_info)

        known_major, known_minor = KNOWN_VERSIONS[b"MGIC"]
        if mgic.version_major != known_major:
            raise RecordVersionError(
                b"MGIC",
                mgic.version_major,
                mgic.version_minor,
                (known_major, known_minor),
            )

        self.size_t_size = mgic.size_t_size
        self.ptr_size = mgic.ptr_size
        self.byteorder = mgic.byteorder

        return mgic
