"""Check, export, and inspect logic for emtrace sections (library core).

All entry points return report objects carrying ``errors``/``warnings`` —
this module never prints. Rendering lives in :mod:`emtrace.cli.read_cmds`.
"""

from __future__ import annotations

import tempfile
from collections import Counter
from dataclasses import dataclass, field
from math import gcd
from pathlib import Path
from typing import Any, Callable

from . import _lief
from ._lief import lief_missing_error, parse_binary
from .engine import load_format_info
from .errors import Report
from .record_parser import (
    KNOWN_VERSIONS,
    MgicInfo,
    RecordInfo,
    RecordParser,
    RecordVersionError,
    _FRAMING_SIZE,
)

# ─────────────────────────────────────────────────────────────────────────────
# Parser versions: the highest per-record-type versions this tool understands.
# Record types are versioned independently (see TRACE_FORMAT.md).
# ─────────────────────────────────────────────────────────────────────────────
PARSER_VERSIONS: dict[bytes, tuple[int, int]] = dict(KNOWN_VERSIONS)


# ─────────────────────────────────────────────────────────────────────────────
# Helpers
# ─────────────────────────────────────────────────────────────────────────────


@dataclass
class LoadedSection(Report):
    """Outcome of loading raw section bytes (Tier 2)."""

    data: bytes | None = None


@dataclass
class CheckReport(Report):
    """Outcome of validating a section against the format spec (Tier 2)."""


@dataclass
class ExportReport(Report):
    """Outcome of exporting trace-point metadata as JSON data (Tier 2)."""

    trace_points: int = 0
    output: dict[str, Any] | None = None


def _load_section(
    path: Path,
    file_format: str,
    section_name: str,
) -> LoadedSection:
    """Load raw section bytes from *path* (Tier 2)."""
    result = LoadedSection()
    loaded = load_format_info(path, file_format, section_name)  # type: ignore[arg-type]
    result.errors.extend(loaded.errors)
    result.warnings.extend(loaded.warnings)
    payload = loaded.payload
    if payload is None:
        if result.ok:
            result.fail(f"Unable to read section '{section_name}' from '{path}'.")
        return result
    if isinstance(payload, dict):
        result.fail("JSON input is not supported for this command.")
        return result
    result.data = bytes(payload)
    return result


# ─────────────────────────────────────────────────────────────────────────────
# Conformance checker
# ─────────────────────────────────────────────────────────────────────────────

_VALID_ENCODING_IDS = {0, 1, 2}
_VALID_FLAG_VALUES = {0, 1, 2}
_VALID_FORMATTER_IDS = {0, 1, 2}
_MAX_ALIGNMENT_POWER = 63


def _common_alignment_power(offsets: list[int]) -> int | None:
    """Largest power of two that every offset in *offsets* is a multiple of.

    Returns ``None`` when there is no constraint (no offsets, or all offsets
    are zero — a record at offset 0 is aligned to anything).
    """
    if not offsets:
        return None
    g = 0
    for offset in offsets:
        g = gcd(g, offset)
    if g == 0:
        return None
    return (g & -g).bit_length() - 1


def _check_framing(
    data: bytes,
    info: RecordInfo,
    *,
    errors: list[str],
) -> None:
    """Check that the pointed-to pieces of a record do not overlap each other
    or the 14-byte framing header, and that the framing offsets are sane (see
    TRACE_FORMAT.md)."""
    offset = info.record_start
    sz = info.size_t_size

    # The type tag is part of the framing; the remaining pieces must start
    # outside of it.
    for name, start in (
        ("record_size", info.size_offset),
        ("payload", info.payload_offset),
        ("endianness probe", info.endianness_offset),
    ):
        if start < offset + _FRAMING_SIZE:
            errors.append(
                f"Record at offset {offset}: {name} piece starts at {start}, "
                f"inside the {_FRAMING_SIZE}-byte framing header."
            )

    # The pieces must not overlap each other (the payload piece is treated as
    # one byte long, since its length is unknown at this point).
    pieces: list[tuple[str, int, int]] = [
        ("type tag", info.type_offset, info.type_offset + 4),
        ("record_size", info.size_offset, info.size_offset + sz),
        ("payload", info.payload_offset, info.payload_offset + 1),
        ("endianness probe", info.endianness_offset, info.endianness_offset + sz),
    ]
    for i, (name_a, a_start, a_end) in enumerate(pieces):
        for name_b, b_start, b_end in pieces[i + 1 :]:
            if a_start < b_end and b_start < a_end:
                errors.append(
                    f"Record at offset {offset}: {name_a} and {name_b} pieces overlap."
                )


def _check_version(
    info: RecordInfo,
    *,
    error_on_minor: bool,
    warnings: list[str],
    errors: list[str],
) -> bool:
    """Check a record's version against the versions this tool understands.

    Returns True when the record's layout is decodable by this tool.
    """
    assert info.record_type is not None
    known = PARSER_VERSIONS[info.record_type]
    rec_type = info.record_type.decode("ascii", "replace")
    if info.major != known[0]:
        errors.append(
            f"{rec_type} record at offset {info.record_start}: unsupported major "
            f"version {info.major}.{info.minor} (tool understands "
            f"{known[0]}.{known[1]})."
        )
        return False
    if info.minor > known[1]:
        msg = (
            f"{rec_type} record at offset {info.record_start}: minor version "
            f"{info.minor} is higher than the known {known[1]}; decoding with the "
            f"known rules."
        )
        if error_on_minor:
            errors.append(msg)
        return True
    return True


def _check(
    data: bytes,
    mgic: MgicInfo | None,
    *,
    error_on_minor: bool,
    warnings: list[str],
    errors: list[str],
) -> None:
    """Scan all records in *data* and validate them against the spec."""

    parser = RecordParser(memoryview(data))

    found_mgic_offsets: list[int] = []
    record_starts: list[int] = []

    i = 0
    while i < len(data):
        info = parser._read_framing(i)
        if info is None:
            i += 1
            continue

        record_starts.append(i)

        # The framing must be well-formed independently of the record type.
        _check_framing(data, info, errors=errors)

        rec_type = info.record_type
        assert rec_type is not None

        if rec_type not in PARSER_VERSIONS:
            warnings.append(
                f"Unknown record type {rec_type!r} at offset {i} "
                f"(version {info.major}.{info.minor}). Skipping."
            )
            i = info.record_end
            continue

        decodable = _check_version(
            info, error_on_minor=error_on_minor, warnings=warnings, errors=errors
        )
        _ = decodable

        if rec_type == b"MGIC":
            found_mgic_offsets.append(i)
            _check_mgic_record(data, info, errors=errors)
        elif rec_type == b"TRCE":
            _check_trce_record(data, info, warnings=warnings, errors=errors)

        i = info.record_end

    if not found_mgic_offsets:
        errors.append("No MGIC record found during full scan.")
    elif len(found_mgic_offsets) > 1:
        errors.append(
            f"Found {len(found_mgic_offsets)} MGIC records "
            f"(at offsets {found_mgic_offsets}), but the spec demands exactly one."
        )

    # Alignment conformance: the MGIC record's declared alignment power must
    # not exceed the alignment power that all records in the section — the
    # MGIC record itself and every TRCE record — actually conform to (see
    # TRACE_FORMAT.md).
    if mgic is not None:
        power = mgic.alignment_power
        if power > _MAX_ALIGNMENT_POWER:
            errors.append(
                f"MGIC record: alignment power {power} is unreasonably large."
            )
        else:
            common = _common_alignment_power(record_starts)
            if common is not None and power > common:
                misaligned = [s for s in record_starts if s % 2**power != 0]
                errors.append(
                    f"MGIC record declares alignment power {power} "
                    f"(2^{power} = {2**power} bytes), but the records in the "
                    f"section only conform to an alignment power of {common}. "
                    f"Records not 2^{power}-aligned: {misaligned}."
                )

        # Address uniqueness: the runtime stores each record's address as an
        # emt_ptr_t value right-shifted by the alignment power. With a
        # ptr_size-byte emt_ptr_t, only addresses below
        # 2^(8*ptr_size + alignment_power) are representable; records at or
        # above it lose bits when shifted and can no longer be told apart
        # from other records. Since the runtime address of a record is at
        # least its section offset, an offset at or above the modulus can
        # never be representable. (The load address must additionally keep
        # the runtime addresses below the modulus, which cannot be verified
        # from the section bytes alone.)
        if mgic.ptr_size > 0:
            address_modulus = 2 ** (8 * mgic.ptr_size + power)
            for start in record_starts:
                if start >= address_modulus:
                    errors.append(
                        f"Record at offset {start} cannot be represented in a "
                        f"{mgic.ptr_size}-byte emt_ptr_t with alignment power "
                        f"{power}: addresses of {address_modulus} bytes and "
                        f"above lose bits when shifted."
                    )


def _check_mgic_record(
    data: bytes,
    info: RecordInfo,
    *,
    errors: list[str],
) -> None:
    try:
        mgic = RecordParser(memoryview(data)).parse_mgic_payload(info)
    except AssertionError as e:
        errors.append(f"MGIC record at {info.record_start}: {e}")
        return

    if mgic.ptr_size == 0:
        errors.append(f"MGIC record at {info.record_start}: sizeof(emt_ptr_t) is 0.")

    if mgic.alignment_power > _MAX_ALIGNMENT_POWER:
        errors.append(
            f"MGIC record at {info.record_start}: alignment power "
            f"{mgic.alignment_power} is unreasonably large."
        )

    if mgic.encoding_id not in _VALID_ENCODING_IDS:
        errors.append(
            f"MGIC record at {info.record_start}: unknown encoding_id={mgic.encoding_id} "
            f"(expected one of {sorted(_VALID_ENCODING_IDS)})."
        )


def _check_trce_record(
    data: bytes,
    info: RecordInfo,
    *,
    warnings: list[str],
    errors: list[str],
) -> None:
    offset = info.record_start
    sz = info.size_t_size
    byteorder = info.byteorder
    assert byteorder in ("little", "big")
    rec_end = info.record_end

    def _int(buf: bytes) -> int:
        return int.from_bytes(buf, byteorder=byteorder)

    def _sz_at(pos: int) -> int:
        return _int(data[pos : pos + sz])

    def _string_ok(abs_pos: int, what: str, label: str) -> None:
        if abs_pos >= rec_end or b"\x00" not in data[abs_pos:rec_end]:
            errors.append(
                f"TRCE record at {offset}, {label}: {what} does not point to a "
                f"null-terminated string within the record."
            )

    payload_start = info.payload_offset
    if payload_start + 2 * sz > rec_end:
        errors.append(
            f"TRCE record at {offset}: payload (layout[0..1]) extends past record end."
        )
        return

    pos = payload_start
    num_args = _sz_at(pos)
    pos += sz
    fmt_offset = _sz_at(pos)
    pos += sz

    # Validate format string offset.
    _string_ok(offset + fmt_offset, f"fmt_offset={fmt_offset}", "payload")

    # Validate each argument descriptor and its children.
    # Children are serialized in pre-order: each 5-entry child block
    # [name_offset, type_id_offset, size, flag, num_children] is followed
    # immediately by the blocks of its own children, and so on.
    for arg_idx in range(num_args):
        if pos + 4 * sz > rec_end:
            errors.append(
                f"TRCE record at {offset}: argument {arg_idx} descriptor extends past record end."
            )
            return

        type_id_offset = _sz_at(pos)
        pos += sz
        _size = _sz_at(pos)
        pos += sz
        flag = _sz_at(pos)
        pos += sz
        num_children = _sz_at(pos)
        pos += sz

        # Validate type_id string.
        _string_ok(
            offset + type_id_offset,
            f"type_id_offset={type_id_offset}",
            f"arg {arg_idx}",
        )

        if flag not in _VALID_FLAG_VALUES:
            errors.append(
                f"TRCE record at {offset}, arg {arg_idx}: invalid flag={flag} "
                f"(expected one of {sorted(_VALID_FLAG_VALUES)})."
            )

        # Validate child descriptors, mirroring the parser's pre-order walk.
        stack: list[tuple[str, int]] = [(f"arg {arg_idx}", num_children)]
        while stack:
            parent_label, remaining = stack[-1]
            if remaining == 0:
                _ = stack.pop()
                continue
            if pos + 5 * sz > rec_end:
                errors.append(
                    f"TRCE record at {offset}, {parent_label}: child descriptor "
                    f"extends past record end."
                )
                return

            child_name_offset = _sz_at(pos)
            pos += sz
            child_type_id_offset = _sz_at(pos)
            pos += sz
            _child_size = _sz_at(pos)
            pos += sz
            child_flag = _sz_at(pos)
            pos += sz
            child_num_children = _sz_at(pos)
            pos += sz

            _string_ok(
                offset + child_name_offset,
                f"child_name_offset={child_name_offset}",
                parent_label,
            )
            _string_ok(
                offset + child_type_id_offset,
                f"child_type_id_offset={child_type_id_offset}",
                parent_label,
            )

            if child_flag not in _VALID_FLAG_VALUES:
                errors.append(
                    f"TRCE record at {offset}, {parent_label}: "
                    f"invalid child_flag={child_flag}."
                )

            stack[-1] = (parent_label, remaining - 1)
            if child_num_children > 0:
                stack.append((f"{parent_label}, child", child_num_children))

    # Validate trailing layout entries: formatter_id, file_offset, line.
    if pos + 3 * sz > rec_end:
        errors.append(
            f"TRCE record at {offset}: trailing layout entries (formatter_id/file_offset/line) "
            f"extend past record end."
        )
        return

    formatter_id = _sz_at(pos)
    pos += sz
    file_offset = _sz_at(pos)
    pos += sz
    _line = _sz_at(pos)
    pos += sz

    if formatter_id not in _VALID_FORMATTER_IDS:
        errors.append(
            f"TRCE record at {offset}: unknown formatter_id={formatter_id} "
            f"(expected one of {sorted(_VALID_FORMATTER_IDS)})."
        )

    file_abs = offset + file_offset
    if file_abs >= rec_end or b"\x00" not in data[file_abs:rec_end]:
        errors.append(
            f"TRCE record at {offset}: file_offset={file_offset} does not point to a "
            f"null-terminated string within the record."
        )


# ─────────────────────────────────────────────────────────────────────────────
# Command: check
# ─────────────────────────────────────────────────────────────────────────────


def check_section(data: bytes, *, error_on_minor: bool = False) -> CheckReport:
    """Validate section bytes against the trace format spec (Tier 2)."""
    report = CheckReport()

    try:
        mgic = RecordParser(memoryview(data)).find_and_parse_mgic()
    except RecordVersionError as e:
        mgic = None
        report.fail(str(e))

    warnings: list[str] = []
    errors: list[str] = []

    _check(
        data,
        mgic,
        error_on_minor=error_on_minor,
        warnings=warnings,
        errors=errors,
    )

    for w in warnings:
        report.warn(w)
    for e in errors:
        report.fail(e)
    return report


# ─────────────────────────────────────────────────────────────────────────────
# Command: export
# ─────────────────────────────────────────────────────────────────────────────


def export_section(
    data: bytes, *, debug_trace: Callable[[*tuple[Any, ...]], None] | None = None
) -> ExportReport:
    """Export all trace-point metadata from section bytes (Tier 2)."""
    report = ExportReport()

    record_parser = RecordParser(
        memoryview(data), debug_trace=debug_trace or (lambda *args: None)
    )
    try:
        mgic = record_parser.find_and_parse_mgic()
    except RecordVersionError as e:
        report.fail(str(e))
        return report
    if mgic is None:
        report.fail("Failed to find or parse MGIC record in section data.")
        return report

    # Walk all trace records, tolerating records with unsupported versions.
    fmt_infos: dict[int, Any] = {}
    i = 0
    while i < len(data):
        try:
            info, next_offset = record_parser.parse_trace_record(i)
        except RecordVersionError as e:
            report.warn(f"skipping record at offset {i}: {e}")
            _structural, _framing, next_offset = record_parser.parse_record_header(i)
            info = None
        if info is not None:
            fmt_infos[i] = info
        i = next_offset

    report.trace_points = len(fmt_infos)
    report.output = {
        "mgic_version": {"major": mgic.version_major, "minor": mgic.version_minor},
        "mgic_record_start": mgic.record_start,
        "size_t_size": mgic.size_t_size,
        "ptr_size": mgic.ptr_size,
        "alignment_power": mgic.alignment_power,
        "byteorder": mgic.byteorder,
        "encoding_id": mgic.encoding_id,
        "trace_points": {i: fmt_info.to_dict() for i, fmt_info in fmt_infos.items()},
    }
    return report


# ─────────────────────────────────────────────────────────────────────────────
# Command: inspect
# ─────────────────────────────────────────────────────────────────────────────


@dataclass
class _SectionSummary:
    """Aggregate information about all records in a section."""

    record_counts: Counter[str]
    versions: dict[str, Counter[str]]
    size_t_sizes: set[int]
    byteorders: set[str]
    mgic: MgicInfo | None
    record_starts: list[int]
    num_trace_points: int
    # Highest power of two that all record start offsets conform to
    # (None when there is no constraint, e.g. a single record at offset 0).
    common_alignment_power: int | None


def _summarize_records(data: bytes) -> _SectionSummary:
    """Walk all records in *data* and aggregate summary information."""
    parser = RecordParser(memoryview(data))

    mgic: MgicInfo | None = None
    try:
        mgic = parser.find_and_parse_mgic()
    except RecordVersionError:
        mgic = None

    record_counts: Counter[str] = Counter()
    versions: dict[str, Counter[str]] = {}
    size_t_sizes: set[int] = set()
    byteorders: set[str] = set()
    record_starts: list[int] = []
    num_trace_points = 0

    i = 0
    while i < len(data):
        info = parser._read_framing(i)
        if info is None:
            i += 1
            continue

        assert info.record_type is not None
        rec_type = info.record_type.decode("ascii", "replace")
        if info.record_type not in KNOWN_VERSIONS:
            rec_type = f"unknown({info.record_type.decode('ascii', 'replace')})"
        record_counts[rec_type] += 1
        versions.setdefault(rec_type, Counter())[f"{info.major}.{info.minor}"] += 1
        size_t_sizes.add(info.size_t_size)
        byteorders.add(str(info.byteorder))
        record_starts.append(i)
        if info.record_type == b"TRCE":
            num_trace_points += 1
        i = info.record_end

    common_alignment_power: int | None = _common_alignment_power(record_starts)

    return _SectionSummary(
        record_counts=record_counts,
        versions=versions,
        size_t_sizes=size_t_sizes,
        byteorders=byteorders,
        mgic=mgic,
        record_starts=record_starts,
        num_trace_points=num_trace_points,
        common_alignment_power=common_alignment_power,
    )


def _detect_input_type(path: Path) -> str:
    """Detect the input file type from its magic bytes."""
    with path.open("rb") as f:
        magic = f.read(8)
    if magic.startswith(b"!<arch>\n"):
        return "archive"
    if magic.startswith(b"\x7fELF"):
        return "elf"
    if magic.startswith(b"MZ"):
        return "pe"
    if magic[:4] in (
        b"\xfe\xed\xfa\xce",
        b"\xfe\xed\xfa\xcf",
        b"\xce\xfa\xed\xfe",
        b"\xcf\xfa\xed\xfe",
        b"\xca\xfe\xba\xbe",
    ):
        return "macho"
    return "raw"


def _parse_archive(path: Path) -> list[tuple[str, bytes]]:
    """Parse a unix ar archive (GNU and BSD style) into (name, content) pairs.

    Symbol table members (``/``, ``__.SYMDEF``) and the long-name table
    (``//``) are resolved or skipped.
    """
    data = path.read_bytes()
    if data[:8] != b"!<arch>\n":
        raise ValueError(f"{path} is not an ar archive.")

    members: list[tuple[str, bytes]] = []
    long_names = b""
    pos = 8
    while pos + 60 <= len(data):
        header = data[pos : pos + 60]
        raw_name = header[0:16].decode("ascii", "replace").rstrip()
        fmag = header[58:60]
        if fmag != b"`\n":
            break
        try:
            size = int(header[48:58].decode("ascii", "replace").strip())
        except ValueError:
            break
        content = data[pos + 60 : pos + 60 + size]
        pos += 60 + size
        if size % 2 == 1:
            pos += 1

        if raw_name == "/":
            continue  # GNU symbol index
        if raw_name == "//":
            long_names = content  # GNU long-name table
            continue
        if raw_name.startswith("__.SYMDEF"):
            continue  # BSD symbol index
        if raw_name.startswith("/") and raw_name[1:].isdigit():
            idx = int(raw_name[1:])
            end = long_names.find(b"\n", idx)
            if end == -1:
                end = len(long_names)
            name = long_names[idx:end].decode("utf-8", "replace").rstrip("/")
        elif raw_name.startswith("#1/") and raw_name[3:].isdigit():
            # BSD: the name is embedded at the start of the content.
            name_len = int(raw_name[3:])
            name = content[:name_len].rstrip(b"\x00").decode("utf-8", "replace")
            content = content[name_len:]
        else:
            name = raw_name.rstrip("/")
        members.append((name, content))
    return members


def _parse_member_elf(content: bytes) -> Any | None:
    """Parse an ELF object file from its raw bytes (via a temp file)."""
    if not _lief.HAS_LIEF:
        return None
    if not content.startswith(b"\x7fELF"):
        return None
    with tempfile.NamedTemporaryFile(suffix=".o", delete=False) as f:
        _ = f.write(content)
        tmp_path = f.name
    try:
        return parse_binary("elf", Path(tmp_path))
    except (AttributeError, TypeError, ValueError):
        return None
    finally:
        tmp = Path(tmp_path)
        _ = tmp.unlink(missing_ok=True)


def _binary_sections(binary: Any) -> list[tuple[str, Any]]:
    """Best-effort section list from a lief binary."""
    try:
        return [(s.name, s) for s in binary.sections]
    except AttributeError:
        return []


def _section_alignment(section: Any) -> int | None:
    """Best-effort section alignment (sh_addralign) from a lief section."""
    alignment = getattr(section, "alignment", None)
    if alignment is None:
        return None
    try:
        return int(alignment)
    except (TypeError, ValueError):
        return None


def _section_content(section: Any) -> bytes | None:
    try:
        return bytes(section.content)
    except (AttributeError, TypeError):
        return None


def summarize_records(data: bytes) -> _SectionSummary:
    """Walk all records in *data* and aggregate summary information."""
    return _summarize_records(data)


@dataclass
class ArchiveMemberEntry:
    """One archive member containing the inspected section."""

    name: str
    alignment: int | None
    summary: _SectionSummary
    section_power: int | None
    rec_power: int | None
    member_power: int | None


@dataclass
class ArchiveDetails:
    """Summary data for inspecting a static archive."""

    members_total: int
    section_name: str
    with_emtrace: list[ArchiveMemberEntry] = field(default_factory=list)
    best_power: int | None = None
    constrainers: list[str] = field(default_factory=list)
    unknown_alignment: list[str] = field(default_factory=list)


@dataclass
class BinaryDetails:
    """Summary data for inspecting a linked/unlinked binary."""

    elf_type: str | None = None
    linked: bool | None = None
    alignment: int | None = None
    content_len: int = 0
    summary: _SectionSummary | None = None
    notes: list[str] = field(default_factory=list)


@dataclass
class RawDetails:
    """Summary data for inspecting raw section bytes."""

    summary: _SectionSummary | None = None
    notes: list[str] = field(default_factory=list)


@dataclass
class InspectReport(Report):
    """Outcome of inspecting an archive, binary, or raw bytes (Tier 2)."""

    path: Path | None = None
    input_type: str = ""
    archive: ArchiveDetails | None = None
    binary: BinaryDetails | None = None
    raw: RawDetails | None = None


def inspect_path(path: Path, section_name: str) -> InspectReport:
    """Summarize the emtrace data in *path* (Tier 2).

    Auto-detects archives, executables/object files, and raw section bytes.
    Executable inputs require the ``lief`` package.
    """
    report = InspectReport(path=path)
    resolved = path.resolve() if isinstance(path, Path) else Path(path).resolve()
    report.path = resolved
    if not resolved.is_file():
        report.fail(f"{resolved} does not exist.")
        return report

    input_type = _detect_input_type(resolved)
    report.input_type = input_type

    if input_type == "archive":
        report.archive = _inspect_archive_data(resolved, section_name, report)
    elif input_type in ("elf", "pe", "macho"):
        report.binary = _inspect_binary_data(resolved, input_type, section_name, report)
    else:
        report.raw = _inspect_raw_data(resolved, section_name, report)
    return report


def _inspect_archive_data(
    path: Path, section_name: str, report: InspectReport
) -> ArchiveDetails | None:
    try:
        members = _parse_archive(path)
    except ValueError as e:
        report.fail(str(e))
        return None

    details = ArchiveDetails(members_total=len(members), section_name=section_name)
    for name, content in members:
        elf = _parse_member_elf(content)
        if elf is None:
            continue
        for sec_name, section in _binary_sections(elf):
            if sec_name != section_name:
                continue
            content_bytes = _section_content(section)
            if content_bytes is None:
                continue
            summary = _summarize_records(content_bytes)
            alignment = _section_alignment(section)
            sec_power = (
                alignment.bit_length() - 1
                if alignment is not None and alignment > 0
                else None
            )
            rec_power = summary.common_alignment_power
            if sec_power is not None and rec_power is not None:
                member_power = min(sec_power, rec_power)
            else:
                member_power = sec_power if sec_power is not None else rec_power
            details.with_emtrace.append(
                ArchiveMemberEntry(
                    name=name,
                    alignment=alignment,
                    summary=summary,
                    section_power=sec_power,
                    rec_power=rec_power,
                    member_power=member_power,
                )
            )

    effective = [
        (e.member_power, e.name)
        for e in details.with_emtrace
        if e.member_power is not None and e.section_power is not None
    ]
    details.unknown_alignment = [
        e.name for e in details.with_emtrace if e.section_power is None
    ]
    if effective:
        best_power, _ = min(effective)
        details.best_power = best_power
        details.constrainers = sorted(n for p, n in effective if p == best_power)
    return details


def _inspect_binary_data(
    path: Path, input_type: str, section_name: str, report: InspectReport
) -> BinaryDetails | None:
    if not _lief.HAS_LIEF:
        report.fail(lief_missing_error(f"parsing {input_type} executables"))
        return None
    binary = parse_binary(input_type, path)

    if binary is None:
        report.fail(f"Unable to parse '{path}' as {input_type} (is lief installed?).")
        return None

    section = None
    for sec_name, sec in _binary_sections(binary):
        if sec_name == section_name:
            section = sec
            break
    if section is None:
        report.fail(f"No '{section_name}' section found in {path}.")
        return None

    content = _section_content(section)
    if content is None:
        report.fail(f"Unable to read section content of '{section_name}'.")
        return None

    details = BinaryDetails(
        summary=_summarize_records(content),
        alignment=_section_alignment(section),
        content_len=len(content),
    )

    if input_type == "elf":
        try:
            file_type = getattr(binary.header, "file_type", None)
            type_name = str(file_type).split(".")[-1]
            details.elf_type = type_name
            details.linked = type_name in ("EXEC", "DYN")
        except AttributeError:
            pass

    assert details.summary is not None
    mgic = details.summary.mgic
    if mgic is None:
        return details

    power = mgic.alignment_power
    step = 2**power
    alignment = details.alignment
    if alignment is not None:
        if alignment >= step:
            details.notes.append(
                f"section alignment OK (sh_addralign {alignment} >= 2^{power})"
            )
        else:
            details.notes.append(
                f"SECTION ALIGNMENT TOO LOW (sh_addralign {alignment} < 2^{power})"
            )
    misaligned = [s for s in details.summary.record_starts if s % step != 0]
    if not misaligned:
        details.notes.append(
            f"all {len(details.summary.record_starts)} records start at "
            f"2^{power}-aligned offsets"
        )
    else:
        details.notes.append(
            f"MISALIGNED RECORDS (not 2^{power}-aligned): {misaligned}"
        )
    if details.summary.common_alignment_power is not None:
        details.notes.append(
            f"all records conform to an alignment power of "
            f"{details.summary.common_alignment_power}"
        )
    else:
        details.notes.append(
            "record alignment unconstrained (no non-trivial common alignment)"
        )
    return details


def _inspect_raw_data(
    path: Path, section_name: str, report: InspectReport
) -> RawDetails:
    """Inspect a raw binary file that is assumed to contain section bytes."""
    report.warn(
        f"input is neither an archive nor a known executable format; "
        f"interpreting it as raw '{section_name}' section bytes."
    )
    data = path.read_bytes()
    summary = _summarize_records(data)
    details = RawDetails(summary=summary)
    mgic = summary.mgic
    if mgic is not None:
        power = mgic.alignment_power
        misaligned = [s for s in summary.record_starts if s % 2**power != 0]
        if summary.common_alignment_power is not None:
            details.notes.append(
                f"all records conform to an alignment power of "
                f"{summary.common_alignment_power}; "
                + (
                    f"all {len(summary.record_starts)} records start at "
                    f"2^{power}-aligned offsets"
                    if not misaligned
                    else f"MISALIGNED RECORDS (not 2^{power}-aligned): {misaligned}"
                )
            )
    return details
