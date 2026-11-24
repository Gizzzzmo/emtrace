"""Exporting the trace information of a section as JSON, and loading it back.

Exports are tied to the emtrace version that created them: loading an export
of a different version is an error (the format is not stable before 1.0).
"""

from __future__ import annotations

from collections.abc import Callable
from dataclasses import dataclass, field
from typing import Any

from .._version import __version__
from ..errors import Report
from ..provenance import build_commit
from .records import MgicRecord, TraceRecord, dump_record, find_mgic, iter_records

__all__ = ["ExportReport", "LoadedExport", "export_section", "load_export"]


@dataclass
class ExportReport(Report):
    """Outcome of exporting trace-point metadata as JSON data (Tier 2)."""

    trace_points: int = 0
    output: dict[str, Any] | None = None


@dataclass
class LoadedExport(Report):
    """Outcome of loading a JSON export (Tier 2)."""

    mgic: MgicRecord | None = None
    records: dict[int, TraceRecord] = field(default_factory=dict)


def export_section(
    data: bytes, *, debug_trace: Callable[[str], None] | None = None
) -> ExportReport:
    """Export all trace-point metadata from section bytes (Tier 2).

    Records with errors are skipped with a warning.
    """
    report = ExportReport()
    data = bytes(data)

    mgic = find_mgic(data)
    if mgic is None or not isinstance(mgic.value, MgicRecord):
        report.fail("Failed to find or parse MGIC record in section data.")
        if mgic is not None:
            report.errors.extend(mgic.errors)
        return report
    if not mgic.ok:
        report.errors.extend(mgic.errors)
        return report

    traces: dict[int, TraceRecord] = {}
    for record in iter_records(data):
        if debug_trace is not None:
            for line in dump_record(data, record):
                debug_trace(line)
        if record.ok:
            if isinstance(record.value, TraceRecord):
                traces[record.start] = record.value
            continue
        if record.framing is not None and record.framing.record_type == b"MGIC":
            continue
        first = record.errors[0].describe() if record.errors else "unknown problem"
        report.warn(
            f"skipping {record.label}: {first} (run `emtrace check` for all details)"
        )

    report.trace_points = len(traces)
    report.output = {
        "emtrace_version": __version__,
        "commit": build_commit(),
        "mgic": mgic.value.to_dict(),
        "trace_points": {
            str(offset): record.to_dict() for offset, record in traces.items()
        },
    }
    return report


def load_export(document: dict[str, Any]) -> LoadedExport:
    """Load a JSON export created by :func:`export_section` (Tier 2)."""
    result = LoadedExport()
    version = document.get("emtrace_version")
    if version is None:
        result.fail(
            f"The JSON file has no 'emtrace_version'; it was exported by an older "
            f"version of emtrace. Re-export it with this version ({__version__})."
        )
        return result
    if version != __version__:
        result.fail(
            f"The JSON file was exported by emtrace {version}, but this is emtrace "
            f"{__version__}. Re-export it with this version."
        )
        return result
    try:
        result.mgic = MgicRecord.from_dict(document["mgic"])
        result.records = {
            int(offset): TraceRecord.from_dict(record, int(offset))
            for offset, record in document["trace_points"].items()
        }
    except (KeyError, TypeError, ValueError, AttributeError, RecursionError) as e:
        result.mgic = None
        result.records = {}
        result.fail(f"Malformed JSON export: {type(e).__name__}: {e}")
    return result
