"""Check and inspect logic for emtrace sections (library core).

All entry points return report objects carrying ``errors``/``warnings`` —
this module never prints. Rendering lives in :mod:`emtrace.cli.read_cmds`.

Rules about a single record are enforced by the record parser
(:mod:`emtrace.section.records`); this module adds the rules that concern the
section as a whole (exactly one MGIC record, alignment, representable
addresses) and those that need knowledge beyond the spec (known formatters).
"""

from __future__ import annotations

import tempfile
from collections import Counter
from collections.abc import Callable
from dataclasses import dataclass, field
from math import gcd
from pathlib import Path
from typing import Any

from ..errors import Issue, Report
from ..exe._lief import HAS_LIEF, lief_missing_error, parse_binary
from ..stream.formatters import FORMATTERS
from .records import (
    MAX_ALIGNMENT_POWER,
    MgicRecord,
    ParsedRecord,
    TraceRecord,
    dump_record,
    iter_records,
)

DebugTrace = Callable[[str], None]


@dataclass
class CheckReport(Report):
    """Outcome of validating a section against the format spec (Tier 2)."""


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


def _parse_all(data: bytes, debug_trace: DebugTrace | None) -> list[ParsedRecord]:
    records: list[ParsedRecord] = []
    for record in iter_records(data):
        if debug_trace is not None:
            for line in dump_record(data, record):
                debug_trace(line)
        records.append(record)
    return records


def _first_mgic(records: list[ParsedRecord]) -> MgicRecord | None:
    for record in records:
        if isinstance(record.value, MgicRecord):
            return record.value
    return None


def _span_offset(record: ParsedRecord, label: str) -> tuple[int | None, int]:
    for span in record.spans:
        if span.label == label:
            return span.start, span.end - span.start
    return None, 0


# ─────────────────────────────────────────────────────────────────────────────
# Command: check
# ─────────────────────────────────────────────────────────────────────────────


def check_section(
    data: bytes,
    *,
    error_on_minor: bool = False,
    debug_trace: DebugTrace | None = None,
) -> CheckReport:
    """Validate section bytes against the trace format spec (Tier 2)."""
    report = CheckReport()
    data = bytes(data)
    records = _parse_all(data, debug_trace)

    for record in records:
        for issue in record.issues:
            report.add(issue)
        framing = record.framing
        if framing is None:
            continue
        if error_on_minor and framing.newer_minor_than_known:
            report.add(
                Issue.error(
                    f"record version {framing.major}.{framing.minor} is newer than "
                    f"the versions known to this version of emtrace",
                    record.start + 8,
                    2,
                    (record.label,),
                )
            )
        value = record.value
        if isinstance(value, TraceRecord) and value.formatter not in FORMATTERS:
            offset, length = _span_offset(record, "formatter id")
            report.add(
                Issue.error(
                    f"unknown formatter_id={value.formatter} (expected one of "
                    f"{sorted(FORMATTERS)})",
                    offset,
                    length,
                    (record.label,),
                )
            )

    mgic_records = [
        r for r in records if r.framing is not None and r.framing.record_type == b"MGIC"
    ]
    if not mgic_records:
        report.fail("No MGIC record found.")
    elif len(mgic_records) > 1:
        report.fail(
            f"Found {len(mgic_records)} MGIC records (at offsets "
            f"{[hex(r.start) for r in mgic_records]}), but the spec demands exactly one."
        )

    mgic = _first_mgic(records)
    if mgic is None or mgic.alignment_power > MAX_ALIGNMENT_POWER:
        return report

    # Alignment conformance: the MGIC record's declared alignment power must
    # not exceed the alignment power that all records in the section actually
    # conform to (see TRACE_FORMAT.md).
    power = mgic.alignment_power
    record_starts = [r.start for r in records if r.framing is not None]
    common = _common_alignment_power(record_starts)
    if common is not None and power > common:
        misaligned = [s for s in record_starts if s % 2**power != 0]
        report.fail(
            f"MGIC record declares alignment power {power} (2^{power} = {2**power} "
            f"bytes), but the records in the section only conform to an alignment "
            f"power of {common}. Records not 2^{power}-aligned: "
            f"{[hex(s) for s in misaligned]}."
        )

    # Address uniqueness: the runtime stores each record's address as an
    # emt_ptr_t value right-shifted by the alignment power. With a
    # ptr_size-byte emt_ptr_t, only addresses below
    # 2^(8*ptr_size + alignment_power) are representable; records at or above
    # it lose bits when shifted and can no longer be told apart from other
    # records. Since the runtime address of a record is at least its section
    # offset, an offset at or above the modulus can never be representable.
    # (The load address must additionally keep the runtime addresses below the
    # modulus, which cannot be verified from the section bytes alone.)
    if mgic.ptr_size > 0:
        address_modulus = 2 ** (8 * mgic.ptr_size + power)
        for start in record_starts:
            if start >= address_modulus:
                report.fail(
                    f"Record at offset {start:#x} cannot be represented in a "
                    f"{mgic.ptr_size}-byte emt_ptr_t with alignment power {power}: "
                    f"addresses of {address_modulus} bytes and above lose bits when "
                    f"shifted."
                )
    return report


# ─────────────────────────────────────────────────────────────────────────────
# Command: inspect
# ─────────────────────────────────────────────────────────────────────────────


@dataclass
class SectionSummary:
    """Aggregate information about all records in a section."""

    record_counts: Counter[str]
    versions: dict[str, Counter[str]]
    size_t_sizes: set[int]
    byteorders: set[str]
    mgic: MgicRecord | None
    record_starts: list[int]
    num_trace_points: int
    # Highest power of two that all record start offsets conform to
    # (None when there is no constraint, e.g. a single record at offset 0).
    common_alignment_power: int | None
    #: Number of errors found in the records (details via ``emtrace check``).
    num_errors: int = 0


def summarize_records(
    data: bytes, debug_trace: DebugTrace | None = None
) -> SectionSummary:
    """Walk all records in *data* and aggregate summary information."""
    data = bytes(data)
    records = _parse_all(data, debug_trace)

    record_counts: Counter[str] = Counter()
    versions: dict[str, Counter[str]] = {}
    size_t_sizes: set[int] = set()
    byteorders: set[str] = set()
    record_starts: list[int] = []
    num_trace_points = 0
    num_errors = 0

    for record in records:
        num_errors += len(record.errors)
        framing = record.framing
        if framing is None:
            continue
        name = framing.type_name
        if framing.record_type not in (b"MGIC", b"TRCE"):
            name = f"unknown({name})"
        record_counts[name] += 1
        versions.setdefault(name, Counter())[f"{framing.major}.{framing.minor}"] += 1
        size_t_sizes.add(framing.size_t_size)
        byteorders.add(framing.byteorder)
        record_starts.append(record.start)
        if framing.record_type == b"TRCE":
            num_trace_points += 1

    return SectionSummary(
        record_counts=record_counts,
        versions=versions,
        size_t_sizes=size_t_sizes,
        byteorders=byteorders,
        mgic=_first_mgic(records),
        record_starts=record_starts,
        num_trace_points=num_trace_points,
        common_alignment_power=_common_alignment_power(record_starts),
        num_errors=num_errors,
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
    if not HAS_LIEF:
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


@dataclass
class ArchiveMemberEntry:
    """One archive member containing the inspected section."""

    name: str
    alignment: int | None
    summary: SectionSummary
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
    summary: SectionSummary | None = None
    notes: list[str] = field(default_factory=list)


@dataclass
class RawDetails:
    """Summary data for inspecting raw section bytes."""

    summary: SectionSummary | None = None
    notes: list[str] = field(default_factory=list)


@dataclass
class InspectReport(Report):
    """Outcome of inspecting an archive, binary, or raw bytes (Tier 2)."""

    path: Path | None = None
    input_type: str = ""
    archive: ArchiveDetails | None = None
    binary: BinaryDetails | None = None
    raw: RawDetails | None = None


def inspect_path(
    path: Path,
    section_name: str,
    debug_trace: DebugTrace | None = None,
) -> InspectReport:
    """Summarize the emtrace data in *path* (Tier 2).

    Auto-detects archives, executables/object files, and raw section bytes.
    Executable inputs require the ``lief`` package.
    """
    report = InspectReport(path=path)
    resolved = path.resolve()
    report.path = resolved
    if not resolved.is_file():
        report.fail(f"{resolved} does not exist.")
        return report

    input_type = _detect_input_type(resolved)
    report.input_type = input_type

    if input_type == "archive":
        report.archive = _inspect_archive_data(
            resolved, section_name, report, debug_trace
        )
    elif input_type in ("elf", "pe", "macho"):
        report.binary = _inspect_binary_data(
            resolved, input_type, section_name, report, debug_trace
        )
    else:
        report.raw = _inspect_raw_data(resolved, section_name, report, debug_trace)
    return report


def _inspect_archive_data(
    path: Path,
    section_name: str,
    report: InspectReport,
    debug_trace: DebugTrace | None = None,
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
            if debug_trace is not None:
                debug_trace(f"archive member {name}:")
            summary = summarize_records(content_bytes, debug_trace)
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
    path: Path,
    input_type: str,
    section_name: str,
    report: InspectReport,
    debug_trace: DebugTrace | None = None,
) -> BinaryDetails | None:
    if not HAS_LIEF:
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
        summary=summarize_records(content, debug_trace),
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
    path: Path,
    section_name: str,
    report: InspectReport,
    debug_trace: DebugTrace | None = None,
) -> RawDetails:
    """Inspect a raw binary file that is assumed to contain section bytes."""
    report.warn(
        f"input is neither an archive nor a known executable format; "
        f"interpreting it as raw '{section_name}' section bytes."
    )
    data = path.read_bytes()
    summary = summarize_records(data, debug_trace)
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
