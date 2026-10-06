"""The ``check``, ``export``, and ``inspect`` commands (rendering only)."""

from __future__ import annotations

import json
import sys
from collections import Counter
from pathlib import Path
from typing import Any

from .. import inspector
from ..inspector import _SectionSummary
from .output import error, print_issue


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


def cmd_check(args: Any) -> int:
    """Check that an emtrace section conforms to the trace format spec."""
    path = Path(args.input).resolve()
    loaded = inspector.load_section(path, args.format, args.section_name)
    if loaded.data is None:
        for issue in loaded.errors:
            print_issue(issue)
        for issue in loaded.warnings:
            print_issue(issue)
        return 1

    report = inspector.check_section(loaded.data, error_on_minor=args.error_on_minor)
    for issue in loaded.warnings:
        print_issue(issue)
    for w in report.warnings:
        print(w.message, file=sys.stderr)
    for e in report.errors:
        error(e.message)

    if report.errors:
        print(
            f"check failed: {len(report.errors)} error(s), "
            f"{len(report.warnings)} warning(s).",
            file=sys.stderr,
        )
        return 1

    if report.warnings:
        print(
            f"check passed with {len(report.warnings)} warning(s).",
            file=sys.stderr,
        )
    else:
        print("check passed.", file=sys.stderr)
    return 0


def cmd_export(args: Any) -> int:
    """Export all trace-point metadata from an emtrace section as JSON."""
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
    loaded = inspector.load_section(path, args.format, args.section_name)
    if loaded.data is None:
        for issue in loaded.errors:
            print_issue(issue)
        for issue in loaded.warnings:
            print_issue(issue)
        return 1

    report = inspector.export_section(loaded.data, debug_trace=trace)
    if not report.ok:
        for issue in report.errors:
            print_issue(issue)
        return 1

    assert report.output is not None
    for w in report.warnings:
        print(f"[warning] {w.message}", file=sys.stderr)
    out_text = json.dumps(report.output, indent=2)

    if args.output == "-":
        print(out_text)
    else:
        out_path = Path(args.output)
        _ = out_path.write_text(out_text, encoding="utf-8")
        print(
            f"Wrote {report.trace_points} trace point(s) to '{out_path}'.",
            file=sys.stderr,
        )

    return 0


def cmd_inspect(args: Any) -> int:
    """Summarize the emtrace data of an archive, object file, or executable."""
    report = inspector.inspect_path(Path(args.input), args.section_name)
    for issue in report.errors:
        print_issue(issue)
    for issue in report.warnings:
        # A raw-bytes fallback is informational, not a failure.
        print(f"note:             {issue.message}")
    if not report.ok:
        return 1

    assert report.path is not None
    print(f"input:            {report.path}")
    print(f"type:             {report.input_type}")

    if report.archive is not None:
        _render_archive(report.archive)
    elif report.binary is not None:
        _render_binary(report.binary, args.section_name)
    elif report.raw is not None:
        _render_raw(report.raw)
    return 0


def _render_archive(details: inspector.ArchiveDetails) -> None:
    with_emtrace = details.with_emtrace
    print(
        f"members:          {details.members_total} total, "
        f"{len(with_emtrace)} with '{details.section_name}'"
    )
    if not with_emtrace:
        return
    print()
    print(
        f"  {'member':<20} {'sh_addralign':>12} {'rec align':>9} "
        f"{'effective':>9} {'mgic':>4}  records"
    )
    for entry in with_emtrace:
        align_str = str(entry.alignment) if entry.alignment is not None else "?"
        rec_str = str(entry.rec_power) if entry.rec_power is not None else "-"
        eff_str = str(entry.member_power) if entry.member_power is not None else "?"
        summary = entry.summary
        print(
            f"  {entry.name:<20} {align_str:>12} {rec_str:>9} "
            f"{eff_str:>9} {'yes' if summary.mgic is not None else 'no':>4}  "
            f"{_fmt_count(summary.record_counts) or 'none'}"
        )

    print()
    if details.best_power is not None:
        print(
            f"highest possible alignment power: {details.best_power} "
            f"(constrained by: {', '.join(details.constrainers)})"
        )
        if details.unknown_alignment:
            print(
                f"excluded from the minimum (unknown section alignment): "
                f"{', '.join(details.unknown_alignment)}"
            )
    else:
        print("highest possible alignment power: unknown (no alignment info)")


def _render_binary(details: inspector.BinaryDetails, section_name: str) -> None:
    if details.elf_type is not None:
        linked = details.linked if details.linked is not None else False
        print(
            f"elf type:         {details.elf_type} "
            f"({'linked' if linked else 'unlinked object'})"
        )

    alignment = details.alignment
    align_str = (
        f"sh_addralign {alignment}" if alignment is not None else "alignment unknown"
    )
    print(
        f"section:          {section_name}, size {details.content_len} bytes, {align_str}"
    )
    if alignment is not None and alignment > 0:
        print(f"                  (sh_addralign power {alignment.bit_length() - 1})")
    assert details.summary is not None
    _print_section_summary(details.summary)

    if details.summary.mgic is None:
        return
    print("alignment:        " + "; ".join(details.notes))


def _render_raw(details: inspector.RawDetails) -> None:
    assert details.summary is not None
    _print_section_summary(details.summary)
    if details.summary.mgic is not None and details.notes:
        print("alignment:        " + "; ".join(details.notes))
