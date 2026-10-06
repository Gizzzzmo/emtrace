"""Sole adapter for the optional ``lief`` dependency.

``lief`` is a default install dependency of the ``emtrace`` package, but the
library also works without it: anything that does not need to parse an
executable (raw ``binary`` and ``json`` inputs, record-level helpers) keeps
working, while executable-based operations report a clear "lief is not
installed" issue instead of failing at import time.

This module is the only place that touches ``lief`` availability:

- ``HAS_LIEF`` tells callers whether the dependency is present.
- ``parse_binary()`` / ``find_section_bytes()`` are defined exactly once
  (typing-only imports under ``TYPE_CHECKING`` keep type checkers happy
  without requiring the package at runtime).
- :func:`lief_missing_error` builds the standard issue message so every
  command refuses identically.
"""

from __future__ import annotations

from pathlib import Path
from typing import Any, Callable

try:
    import lief as _lief
except ImportError:
    _lief = None  # type: ignore[assignment]

#: Whether the optional ``lief`` dependency is installed.
HAS_LIEF: bool = _lief is not None


def lief_missing_error(operation: str) -> str:
    """The standard message for refusing *operation* without lief installed."""
    return (
        f"{operation} requires the 'lief' package, which is not installed "
        f"(install the default 'emtrace' dependencies to enable it)"
    )


def parse_binary(kind: str, path: Path) -> Any | None:
    """Parse *path* as a binary of *kind* (Tier 1: ``None`` when unavailable).

    *kind* is one of ``"exe"``, ``"elf"``, ``"pe"``, ``"macho"`` or ``"oat"``.
    Returns ``None`` when lief is not installed, when the file is not of the
    requested kind, or when parsing fails.
    """
    if _lief is None:
        return None
    if kind == "exe":
        return _lief.parse(path)
    if kind == "elf":
        return _lief.ELF.parse(path)
    if kind == "pe":
        return _lief.PE.parse(path)
    if kind == "macho":
        return _lief.parse(path) if _lief.is_macho(path) else None
    if kind == "oat":
        return _lief.OAT.parse(path)
    raise ValueError(f"unknown binary kind: {kind!r}")


def find_section_bytes(
    trace_info_file: Path, section_name: str, parse: Callable[[Path], Any | None]
) -> bytes | None:
    """Extract section bytes via *parse* (Tier 1: ``None`` when absent)."""
    exe = parse(trace_info_file)
    if exe is None:
        return None
    section = exe.get_section(section_name)
    if section is None:
        return None
    return bytes(section.content)
