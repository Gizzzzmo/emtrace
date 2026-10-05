"""emctl - management and inspection tool for emtrace sections."""

from __future__ import annotations

import json
import sys
import tempfile
from argparse import ArgumentParser, _SubParsersAction
from collections import Counter
from dataclasses import dataclass
from math import gcd
from pathlib import Path
from typing import Any

from .emtrace import get_format_info
from .record_parser import (
    KNOWN_VERSIONS,
    MgicInfo,
    RecordInfo,
    RecordParser,
    RecordVersionError,
    _FRAMING_SIZE,
)

try:
    import lief
except ImportError:
    from . import lief_stub as lief  # type: ignore[no-redef]

# ─────────────────────────────────────────────────────────────────────────────
# Parser versions: the highest per-record-type versions this tool understands.
# Record types are versioned independently (see TRACE_FORMAT.md).
# ─────────────────────────────────────────────────────────────────────────────
PARSER_VERSIONS: dict[bytes, tuple[int, int]] = dict(KNOWN_VERSIONS)


# ─────────────────────────────────────────────────────────────────────────────
# Helpers
# ─────────────────────────────────────────────────────────────────────────────


def _load_section(
    path: Path,
    file_format: str,
    section_name: str,
) -> bytes | str:
    """Load raw section bytes from *path*.  Returns bytes or an error string."""
    result = get_format_info(path, file_format, section_name)  # type: ignore[arg-type]
    if result is None:
        return f"Unable to read section '{section_name}' from '{path}'."
    if isinstance(result, dict):
        return "JSON input is not supported for this command."
    return bytes(result)


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
# Subcommand: check
# ─────────────────────────────────────────────────────────────────────────────


def cmd_check(args: Any) -> int:
    path = Path(args.input).resolve()
    data = _load_section(path, args.format, args.section_name)
    if isinstance(data, str):
        print(f"[error] {data}", file=sys.stderr)
        return 1

    try:
        mgic = RecordParser(memoryview(data)).find_and_parse_mgic()
    except RecordVersionError as e:
        mgic = None
        print(f"[error] {e}", file=sys.stderr)

    warnings: list[str] = []
    errors: list[str] = []

    _check(
        data,
        mgic,
        error_on_minor=args.error_on_minor,
        warnings=warnings,
        errors=errors,
    )

    for w in warnings:
        print(w, file=sys.stderr)
    for e in errors:
        print(f"[error] {e}", file=sys.stderr)

    if errors:
        print(
            f"check failed: {len(errors)} error(s), {len(warnings)} warning(s).",
            file=sys.stderr,
        )
        return 1

    if warnings:
        print(
            f"check passed with {len(warnings)} warning(s).",
            file=sys.stderr,
        )
    else:
        print("check passed.", file=sys.stderr)
    return 0


# ─────────────────────────────────────────────────────────────────────────────
# Subcommand: dump
# ─────────────────────────────────────────────────────────────────────────────


def cmd_dump(args: Any) -> int:
    debug_script = args.debug_script

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

    path = Path(args.input).resolve()
    data = _load_section(path, args.format, args.section_name)
    if isinstance(data, str):
        print(f"[error] {data}", file=sys.stderr)
        return 1

    record_parser = RecordParser(memoryview(data), debug_trace=trace)
    try:
        mgic = record_parser.find_and_parse_mgic()
    except RecordVersionError as e:
        print(f"[error] {e}", file=sys.stderr)
        return 1
    if mgic is None:
        print(
            "[error] Failed to find or parse MGIC record in section data.",
            file=sys.stderr,
        )
        return 1

    # Walk all trace records, tolerating records with unsupported versions.
    fmt_infos: dict[int, Any] = {}
    i = 0
    while i < len(data):
        try:
            info, next_offset = record_parser.parse_trace_record(i)
        except RecordVersionError as e:
            print(f"[warning] skipping record at offset {i}: {e}", file=sys.stderr)
            _structural, _framing, next_offset = record_parser.parse_record_header(i)
            info = None
        if info is not None:
            fmt_infos[i] = info
        i = next_offset

    output: dict[str, Any] = {
        "mgic_version": {"major": mgic.version_major, "minor": mgic.version_minor},
        "mgic_record_start": mgic.record_start,
        "size_t_size": mgic.size_t_size,
        "ptr_size": mgic.ptr_size,
        "alignment_power": mgic.alignment_power,
        "byteorder": mgic.byteorder,
        "encoding_id": mgic.encoding_id,
        "trace_points": {i: fmt_info.to_dict() for i, fmt_info in fmt_infos.items()},
    }

    out_text = json.dumps(output, indent=2)

    if args.output == "-":
        print(out_text)
    else:
        out_path = Path(args.output)
        _ = out_path.write_text(out_text, encoding="utf-8")
        print(
            f"Wrote {len(fmt_infos)} trace point(s) to '{out_path}'.", file=sys.stderr
        )

    return 0


# ─────────────────────────────────────────────────────────────────────────────
# Subcommand: inspect
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
    if not content.startswith(b"\x7fELF"):
        return None
    with tempfile.NamedTemporaryFile(suffix=".o", delete=False) as f:
        _ = f.write(content)
        tmp_path = f.name
    try:
        return lief.ELF.parse(Path(tmp_path))
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


def _fmt_count(counter: Counter[str]) -> str:
    return ", ".join(f"{name}×{count}" for name, count in sorted(counter.items()))


def _print_section_summary(summary: _SectionSummary, indent: str = "  ") -> None:
    counts = summary.record_counts
    print(f"{indent}records:          {_fmt_count(counts) or 'none'}")
    version_parts = [
        f"{rec} ({', '.join(f'{v}x{c}' for v, c in sorted(vers.items()))})"
        for rec, vers in sorted(summary.versions.items())
    ]
    print(f"{indent}versions:         {'; '.join(version_parts) or 'none'}")
    if summary.size_t_sizes:
        print(f"{indent}size_t widths:    {sorted(summary.size_t_sizes)}")
    if summary.byteorders:
        print(f"{indent}endianness:       {sorted(summary.byteorders)}")
    if summary.mgic is not None:
        m = summary.mgic
        print(
            f"{indent}mgic:             version {m.version_major}.{m.version_minor}, "
            f"size_t width {m.size_t_size} ({m.byteorder}), ptr width {m.ptr_size}, "
            f"alignment power {m.alignment_power}, encoding id {m.encoding_id}"
        )
    else:
        print(f"{indent}mgic:             none")


def cmd_inspect(args: Any) -> int:
    path = Path(args.input).resolve()
    if not path.is_file():
        print(f"[error] {path} does not exist.", file=sys.stderr)
        return 1

    input_type = _detect_input_type(path)
    print(f"input:            {path}")
    print(f"type:             {input_type}")

    if input_type == "archive":
        return _inspect_archive(path, args.section_name)
    if input_type in ("elf", "pe", "macho"):
        return _inspect_binary(path, input_type, args.section_name)
    return _inspect_raw(path, args.section_name)


def _inspect_archive(path: Path, section_name: str) -> int:
    try:
        members = _parse_archive(path)
    except ValueError as e:
        print(f"[error] {e}", file=sys.stderr)
        return 1

    with_emtrace: list[tuple[str, int | None, _SectionSummary]] = []
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
            with_emtrace.append((name, _section_alignment(section), summary))

    print(
        f"members:          {len(members)} total, {len(with_emtrace)} with '{section_name}'"
    )
    if with_emtrace:
        print()
        print(
            f"  {'member':<20} {'sh_addralign':>12} {'rec align':>9} "
            f"{'effective':>9} {'mgic':>4}  records"
        )
        effective: list[tuple[int, str]] = []
        unknown_alignment: list[str] = []
        for name, alignment, summary in with_emtrace:
            align_str = str(alignment) if alignment is not None else "?"
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
            rec_str = str(rec_power) if rec_power is not None else "-"
            eff_str = str(member_power) if member_power is not None else "?"
            print(
                f"  {name:<20} {align_str:>12} {rec_str:>9} "
                f"{eff_str:>9} {'yes' if summary.mgic is not None else 'no':>4}  "
                f"{_fmt_count(summary.record_counts) or 'none'}"
            )
            if sec_power is None:
                unknown_alignment.append(name)
            elif member_power is not None:
                effective.append((member_power, name))

        print()
        if effective:
            best_power, _ = min(effective)
            constrainers = sorted(n for p, n in effective if p == best_power)
            print(
                f"highest possible alignment power: {best_power} "
                f"(constrained by: {', '.join(constrainers)})"
            )
            if unknown_alignment:
                print(
                    f"excluded from the minimum (unknown section alignment): "
                    f"{', '.join(unknown_alignment)}"
                )
        else:
            print("highest possible alignment power: unknown (no alignment info)")
    return 0


def _inspect_binary(path: Path, input_type: str, section_name: str) -> int:
    binary = None
    if input_type == "elf":
        binary = lief.ELF.parse(path)
    elif input_type == "pe":
        binary = lief.PE.parse(path)
    elif input_type == "macho":
        binary = (
            lief.parse(path)
            if getattr(lief, "is_macho", lambda _: False)(path)
            else None
        )

    if binary is None:
        print(
            f"[error] Unable to parse '{path}' as {input_type} (is lief installed?).",
            file=sys.stderr,
        )
        return 1

    section = None
    for sec_name, sec in _binary_sections(binary):
        if sec_name == section_name:
            section = sec
            break
    if section is None:
        print(f"[error] No '{section_name}' section found in {path}.", file=sys.stderr)
        return 1

    content = _section_content(section)
    if content is None:
        print(
            f"[error] Unable to read section content of '{section_name}'.",
            file=sys.stderr,
        )
        return 1

    summary = _summarize_records(content)
    alignment = _section_alignment(section)

    linked = True
    if input_type == "elf":
        try:
            file_type = getattr(binary.header, "file_type", None)
            type_name = str(file_type).split(".")[-1]
            linked = type_name in ("EXEC", "DYN")
            print(
                f"elf type:         {type_name} ({'linked' if linked else 'unlinked object'})"
            )
        except AttributeError:
            pass

    align_str = (
        f"sh_addralign {alignment}" if alignment is not None else "alignment unknown"
    )
    print(f"section:          {section_name}, size {len(content)} bytes, {align_str}")
    if alignment is not None and alignment > 0:
        print(f"                  (sh_addralign power {alignment.bit_length() - 1})")
    _print_section_summary(summary)

    mgic = summary.mgic
    if mgic is None:
        return 0

    power = mgic.alignment_power
    step = 2**power
    notes: list[str] = []
    if alignment is not None:
        if alignment >= step:
            notes.append(
                f"section alignment OK (sh_addralign {alignment} >= 2^{power})"
            )
        else:
            notes.append(
                f"SECTION ALIGNMENT TOO LOW (sh_addralign {alignment} < 2^{power})"
            )
    misaligned = [s for s in summary.record_starts if s % step != 0]
    if not misaligned:
        notes.append(
            f"all {len(summary.record_starts)} records start at 2^{power}-aligned offsets"
        )
    else:
        notes.append(f"MISALIGNED RECORDS (not 2^{power}-aligned): {misaligned}")
    if summary.common_alignment_power is not None:
        notes.append(
            f"all records conform to an alignment power of "
            f"{summary.common_alignment_power}"
        )
    else:
        notes.append("record alignment unconstrained (no non-trivial common alignment)")

    print("alignment:        " + "; ".join(notes))
    return 0


def _inspect_raw(path: Path, section_name: str) -> int:
    """Inspect a raw binary file that is assumed to contain section bytes."""
    print(
        f"note:             input is neither an archive nor a known executable format; "
        f"interpreting it as raw '{section_name}' section bytes."
    )
    data = path.read_bytes()
    summary = _summarize_records(data)
    _print_section_summary(summary)
    mgic = summary.mgic
    if mgic is not None:
        power = mgic.alignment_power
        misaligned = [s for s in summary.record_starts if s % 2**power != 0]
        if summary.common_alignment_power is not None:
            print(
                f"alignment:        all records conform to an alignment power of "
                f"{summary.common_alignment_power}; "
                + (
                    f"all {len(summary.record_starts)} records start at "
                    f"2^{power}-aligned offsets"
                    if not misaligned
                    else f"MISALIGNED RECORDS (not 2^{power}-aligned): {misaligned}"
                )
            )
    return 0


# ─────────────────────────────────────────────────────────────────────────────
# Shared input-source arguments helper
# ─────────────────────────────────────────────────────────────────────────────


def _add_input_args(sub: ArgumentParser) -> None:
    """Add --format and --section-name to a subcommand parser."""
    _ = sub.add_argument(
        "input",
        help=(
            "Path to an executable, or a raw binary file containing the .emtrace section bytes."
        ),
    )
    _ = sub.add_argument(
        "--format",
        nargs="?",
        default="auto",
        choices=["auto", "exe", "elf", "pe", "macho", "binary"],
        help=(
            "Format of the input file. 'auto' (default) tries to detect automatically: "
            "first as an executable (ELF/PE/MachO), then as raw binary."
        ),
    )
    _ = sub.add_argument(
        "--section-name",
        default=".emtrace",
        metavar="NAME",
        help="ELF/PE/MachO section name to read (default: .emtrace).",
    )


# ─────────────────────────────────────────────────────────────────────────────
# Entry point
# ─────────────────────────────────────────────────────────────────────────────


def main() -> int:
    parser = ArgumentParser(
        "emctl",
        description="Management and inspection tool for emtrace sections.",
    )
    _ = parser.add_argument(
        "--debug-script",
        action="store_true",
        help="Debug the script by printing internal trace information to stderr (for development use only).",
    )

    subs: _SubParsersAction[ArgumentParser] = parser.add_subparsers(
        dest="command", metavar="<command>"
    )
    _ = subs.required = True

    # ── check ──────────────────────────────────────────────────────────────
    check_parser = subs.add_parser(
        "check",
        help="Check that an emtrace section conforms to the trace format specification.",
        description=(
            "Scans all records in the emtrace section and validates them against the "
            "trace format specification: framing layout, per-record versions, record "
            "payloads, MGIC uniqueness, that the MGIC record's alignment power "
            "does not exceed the alignment power all records actually conform to, "
            "and that every record's address is representable and unique in a "
            "emt_ptr_t. Major version incompatibilities are a hard error; minor "
            "version differences are ignored unless --error-on-minor is given."
        ),
    )
    _add_input_args(check_parser)
    _ = check_parser.add_argument(
        "--error-on-minor",
        action="store_true",
        help="Treat records with a higher minor version than known as errors instead of ignoring them.",
    )
    check_parser.set_defaults(func=cmd_check)

    # ── dump ───────────────────────────────────────────────────────────────
    dump_parser = subs.add_parser(
        "dump",
        help="Dump all trace-point metadata from an emtrace section as JSON.",
        description=(
            "Parses every TRCE record in the emtrace section and writes a JSON "
            "representation to stdout (or to a file with --output)."
        ),
    )
    _add_input_args(dump_parser)
    _ = dump_parser.add_argument(
        "--output",
        "-o",
        default="-",
        metavar="FILE",
        help="Write JSON output to FILE instead of stdout. Use '-' for stdout (default).",
    )
    dump_parser.set_defaults(func=cmd_dump)

    # ── inspect ────────────────────────────────────────────────────────────
    inspect_parser = subs.add_parser(
        "inspect",
        help="Summarize the emtrace data of an executable, object file, or static archive.",
        description=(
            "Auto-detects whether the input is a static archive, an unlinked "
            "object file, or a linked executable, and prints summary information: "
            "record counts per type, record-type versions, size_t widths, "
            "detected endianness, emt_ptr_t size, the alignment power reported by "
            "the MGIC record, the highest alignment all records actually conform "
            "to, and — for archives — the highest possible alignment power "
            "achievable when linking."
        ),
    )
    _ = inspect_parser.add_argument(
        "input",
        help="Path to a static archive (.a), object file (.o), executable, or raw section bytes.",
    )
    _ = inspect_parser.add_argument(
        "--section-name",
        default=".emtrace",
        metavar="NAME",
        help="ELF/PE/MachO section name to inspect (default: .emtrace).",
    )
    inspect_parser.set_defaults(func=cmd_inspect)

    args = parser.parse_args()
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
