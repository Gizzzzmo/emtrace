"""emctl - management and inspection tool for emtrace sections."""

from __future__ import annotations

import json
import sys
from argparse import ArgumentParser, _SubParsersAction
from pathlib import Path
from typing import Any

from .emtrace import get_format_info
from .record_parser import RecordParser, MgicInfo

# ─────────────────────────────────────────────────────────────────────────────
# Parser version: the highest file-format version this tool understands.
# ─────────────────────────────────────────────────────────────────────────────
PARSER_VERSION = 0

# Known four-byte record type strings for version 0.
_KNOWN_RECORD_TYPES_V0: frozenset[bytes] = frozenset([b"MGIC", b"TRCE"])



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


def _check(
    data: bytes,
    mgic: MgicInfo,
    *,
    warnings: list[str],
    errors: list[str],
) -> None:
    """Scan all records in *data* and validate them against the spec."""

    def _int(buf: bytes) -> int:
        return int.from_bytes(buf, byteorder=mgic.byteorder)

    sz = mgic.size_t_size

    # We scan the entire section looking for 'EMT'-prefixed records.
    # Any byte sequence that starts with 'EMT\x06' and has a valid type at [6:10] is a record.
    i = 0
    found_mgic = False
    while i < len(data):
        if data[i : i + 3] != b"EMT":
            i += 1
            continue

        # framing[3] should be 6 for all current records (offset to type string).
        if i + 10 > len(data) or data[i + 3] != 6:
            i += 1
            continue

        rec_type: bytes = data[i + 6 : i + 10]
        if len(rec_type) < 4:
            i += 1
            continue

        rec_size_offset = int(data[i + 4])
        if i + rec_size_offset + sz > len(data):
            errors.append(
                f"Record at offset {i}: record_size field extends past end of section."
            )
            i += 1
            continue

        rec_size = _int(data[i + rec_size_offset : i + rec_size_offset + sz])
        if rec_size == 0:
            errors.append(f"Record at offset {i} type={rec_type!r}: record_size is 0.")
            i += 1
            continue
        if i + rec_size > len(data):
            errors.append(
                f"Record at offset {i} type={rec_type!r}: record_size={rec_size} "
                f"extends past end of section (len={len(data)})."
            )
            i += 1
            continue

        if rec_type not in _KNOWN_RECORD_TYPES_V0:
            msg = (
                f"Unknown record type {rec_type!r} at offset {i} "
                f"(file version={mgic.version}, parser version={PARSER_VERSION})."
            )
            if mgic.version <= PARSER_VERSION:
                errors.append(msg)
            else:
                warnings.append(f"[warning] {msg} Skipping.")
            i += rec_size
            continue

        if rec_type == b"MGIC":
            found_mgic = True
            _check_mgic_record(data, i, mgic, warnings=warnings, errors=errors)
        elif rec_type == b"TRCE":
            _check_trce_record(data, i, mgic, warnings=warnings, errors=errors)

        i += rec_size

    if not found_mgic:
        errors.append("No MGIC record found during full scan.")


def _check_mgic_record(
    data: bytes,
    offset: int,
    mgic: MgicInfo,
    *,
    warnings: list[str],
    errors: list[str],
) -> None:
    sz = mgic.size_t_size

    def _int(buf: bytes) -> int:
        return int.from_bytes(buf, byteorder=mgic.byteorder)

    # Verify framing fields.
    rec_size_offset = int(data[offset + 4])
    meta_offset = int(data[offset + 5])

    # meta[34] = size_t_meta offset from record start
    magic_off = offset + meta_offset
    if magic_off + 39 > len(data):
        errors.append(f"MGIC record at {offset}: meta[] extends past end of section.")
        return

    size_t_meta_offset = int(data[magic_off + 34])
    size_t_meta_loc = offset + size_t_meta_offset
    if size_t_meta_loc + 2 * sz > len(data):
        errors.append(
            f"MGIC record at {offset}: size_t_meta[] extends past end of section."
        )
        return

    encoding_id = _int(data[size_t_meta_loc + sz : size_t_meta_loc + 2 * sz])
    if encoding_id not in _VALID_ENCODING_IDS:
        errors.append(
            f"MGIC record at {offset}: unknown encoding_id={encoding_id} "
            f"(expected one of {sorted(_VALID_ENCODING_IDS)})."
        )

    # sizeof(emt_size_t) and sizeof(emt_ptr_t) must be > 0.
    emt_ptr = int(data[magic_off + 36])
    if emt_ptr == 0:
        errors.append(f"MGIC record at {offset}: sizeof(emt_ptr_t) is 0.")


def _check_trce_record(
    data: bytes,
    offset: int,
    mgic: MgicInfo,
    *,
    warnings: list[str],
    errors: list[str],
) -> None:
    sz = mgic.size_t_size

    def _int(buf: bytes) -> int:
        return int.from_bytes(buf, byteorder=mgic.byteorder)

    def _sz_at(pos: int) -> int:
        return _int(data[pos : pos + sz])

    rec_size_offset = int(data[offset + 4])
    payload_offset = int(data[offset + 5])

    rec_size = _sz_at(offset + rec_size_offset)
    rec_end = offset + rec_size

    payload_start = offset + payload_offset
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
    fmt_abs = offset + fmt_offset
    if fmt_abs >= rec_end:
        errors.append(
            f"TRCE record at {offset}: fmt_offset={fmt_offset} points outside record."
        )
        return
    if b"\x00" not in data[fmt_abs:rec_end]:
        errors.append(
            f"TRCE record at {offset}: format string at offset {fmt_offset} is not null-terminated within the record."
        )
        return

    # Validate each argument descriptor.
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
        type_id_abs = offset + type_id_offset
        if type_id_abs >= rec_end or b"\x00" not in data[type_id_abs:rec_end]:
            errors.append(
                f"TRCE record at {offset}, arg {arg_idx}: type_id_offset={type_id_offset} "
                f"does not point to a null-terminated string within the record."
            )

        if flag not in _VALID_FLAG_VALUES:
            errors.append(
                f"TRCE record at {offset}, arg {arg_idx}: invalid flag={flag} "
                f"(expected one of {sorted(_VALID_FLAG_VALUES)})."
            )

        # Validate child descriptors.
        for child_idx in range(num_children):
            if pos + 4 * sz > rec_end:
                errors.append(
                    f"TRCE record at {offset}, arg {arg_idx}, child {child_idx}: "
                    f"descriptor extends past record end."
                )
                return

            child_name_offset = _sz_at(pos)
            pos += sz
            _child_size = _sz_at(pos)
            pos += sz
            child_flag = _sz_at(pos)
            pos += sz
            child_type_id_offset = _sz_at(pos)
            pos += sz

            child_name_abs = offset + child_name_offset
            if child_name_abs >= rec_end or b"\x00" not in data[child_name_abs:rec_end]:
                errors.append(
                    f"TRCE record at {offset}, arg {arg_idx}, child {child_idx}: "
                    f"child_name_offset={child_name_offset} does not point to a null-terminated "
                    f"string within the record."
                )

            child_type_abs = offset + child_type_id_offset
            if child_type_abs >= rec_end or b"\x00" not in data[child_type_abs:rec_end]:
                errors.append(
                    f"TRCE record at {offset}, arg {arg_idx}, child {child_idx}: "
                    f"child_type_id_offset={child_type_id_offset} does not point to a "
                    f"null-terminated string within the record."
                )

            if child_flag not in _VALID_FLAG_VALUES:
                errors.append(
                    f"TRCE record at {offset}, arg {arg_idx}, child {child_idx}: "
                    f"invalid child_flag={child_flag}."
                )

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

    mgic = RecordParser(memoryview(data)).find_and_parse_mgic()
    if mgic is None:
        print("[error] Failed to find or parse MGIC record in section data.", file=sys.stderr)
        return 1

    warnings: list[str] = []
    errors: list[str] = []

    _check(data, mgic, warnings=warnings, errors=errors)

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
    mgic = record_parser.find_and_parse_mgic()
    if mgic is None:
        print("[error] Failed to find or parse MGIC record in section data.", file=sys.stderr)
        return 1

    fmt_infos = record_parser.parse_all_trace_records()

    output: dict[str, Any] = {
        "version": mgic.version,
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
            "trace format specification. Unknown record types are a hard error when the "
            "file version is ≤ the parser version, and a warning otherwise."
        ),
    )
    _add_input_args(check_parser)
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

    args = parser.parse_args()
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
