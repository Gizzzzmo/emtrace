"""Loading trace information (section bytes or a JSON export) from files."""

from __future__ import annotations

import json
from dataclasses import dataclass
from functools import partial
from pathlib import Path
from typing import Any, Literal

from ..errors import Report
from ..exe._lief import HAS_LIEF, find_section_bytes, lief_missing_error, parse_binary

__all__ = [
    "FormatInfoResult",
    "LoadedSection",
    "TraceInfoFormat",
    "load_format_info",
    "load_section",
]

#: Accepted values for the ``file_format`` parameter of the entry points.
TraceInfoFormat = Literal["auto", "exe", "elf", "pe", "macho", "oat", "json", "binary"]

_EXECUTABLE_FORMATS: frozenset[str] = frozenset(["exe", "elf", "pe", "macho", "oat"])


@dataclass
class FormatInfoResult(Report):
    """Outcome of loading format information (Tier 2).

    ``payload`` is the loaded data: a JSON-derived dict, or raw section
    bytes. ``None`` with errors set means loading failed.
    """

    payload: dict[str, Any] | bytes | None = None


@dataclass
class LoadedSection(Report):
    """Outcome of loading raw section bytes (Tier 2)."""

    data: bytes | None = None


def _read_json(path: Path) -> dict[str, Any] | None:
    try:
        with open(path, encoding="utf-8") as f:
            loaded = json.load(f)
    except (json.JSONDecodeError, UnicodeDecodeError, RecursionError):
        return None
    return loaded if isinstance(loaded, dict) else None


def load_format_info(
    trace_info_file: Path,
    file_format: TraceInfoFormat,
    section_name: str,
) -> FormatInfoResult:
    """Load format information from *trace_info_file* (Tier 2).

    ``auto`` tries JSON, then the embedded executable section, then raw
    binary — degrading gracefully when lief is not installed.
    """
    result = FormatInfoResult()

    def unable() -> FormatInfoResult:
        result.fail(
            f"Unable to get section {section_name} from file {trace_info_file} "
            f"as {file_format}."
        )
        return result

    try:
        if file_format == "auto":
            loaded = _read_json(trace_info_file)
            if loaded is not None:
                result.payload = loaded
                return result
            section_bytes = find_section_bytes(
                trace_info_file, section_name, partial(parse_binary, "exe")
            )
            if section_bytes is not None:
                result.payload = section_bytes
                return result
            result.payload = trace_info_file.read_bytes()
            return result

        if file_format == "json":
            loaded = _read_json(trace_info_file)
            if loaded is None:
                return unable()
            result.payload = loaded
            return result

        if file_format == "binary":
            result.payload = trace_info_file.read_bytes()
            return result
    except OSError:
        return unable()

    if str(file_format) not in _EXECUTABLE_FORMATS:
        result.fail(f"Unknown format: {file_format}.")
        return result
    if not HAS_LIEF:
        result.fail(lief_missing_error(f"reading {file_format} executables"))
        return result
    section_bytes = find_section_bytes(
        trace_info_file, section_name, partial(parse_binary, file_format)
    )
    if section_bytes is None:
        return unable()
    result.payload = section_bytes
    return result


def load_section(
    path: Path,
    file_format: TraceInfoFormat,
    section_name: str,
) -> LoadedSection:
    """Load raw section bytes from *path* (Tier 2)."""
    result = LoadedSection()
    loaded = load_format_info(path, file_format, section_name)
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
