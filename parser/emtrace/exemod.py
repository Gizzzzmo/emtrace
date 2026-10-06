"""Multi-format binary surgery for the emtrace post-link commands.

This module is the single entry point for editing linked binaries, backed by
LIEF: ``ExeImage.open`` detects the format (ELF, PE, or Mach-O) and dispatches
to the per-format backend (``elf_backend``, ``pe_backend``, ``macho_backend``)
that implements the format-specific validation, symbol fixups, and rebuild
configuration.  Only ``strip`` is supported on all formats; ``nonload`` (clear
the loadable flag, the ``(INFO)`` linker-script effect) is an ELF-only concept.

Unlike the previous byte-exact header patching, LIEF always fully rebuilds
the binary, so no byte-offset or exact-size guarantees are made.  The
guarantees are behavioral: the section entry is gone, symbols bound to it are
deleted from static symbol tables, the section content no longer parses, and
the rebuilt binary still runs.
"""

from __future__ import annotations

import importlib
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from ._lief import HAS_LIEF
from .errors import (
    ExeError,
    NotSupportedError,
    Report,
    SectionNotFoundError,
)

# Re-exported for backward compatibility (import from .errors for new code).
__all__ = [
    "ExeError",
    "ExeImage",
    "NonloadReport",
    "NotSupportedError",
    "SectionNotFoundError",
    "StripReport",
]


@dataclass
class NonloadReport(Report):
    """Result of making a section non-loadable (Tier 2).

    ``data`` holds the rebuilt binary when the operation succeeded.
    """

    section: str = ""
    changed: bool = False
    old_flags: int = 0
    old_addr: int = 0
    format: str = ""
    data: bytes | None = None


@dataclass
class StripReport(Report):
    """Result of stripping a section (Tier 2).

    ``data`` holds the rebuilt binary when the operation succeeded.
    """

    section: str = ""
    # Number of static symbol table entries deleted.
    removed_symbols: int = 0
    # Change in file size after the LIEF rebuild (<= 0).
    size_delta: int = 0
    format: str = ""
    data: bytes | None = None


class ExeImage:
    """A linked executable image editable via its per-format backend."""

    def __init__(self, path: Path, format: str, binary: Any, backend: Any) -> None:
        self._path: Path = path
        self._format: str = format
        self._binary: Any = binary
        self._backend: Any = backend
        self._data: bytes = path.read_bytes()

    @staticmethod
    def open(path: str | Path, *, experimental_be: bool = False) -> ExeImage:
        """Open *path* as an editable linked executable.

        Big-endian inputs are refused unless *experimental_be* is set;
        rebuilding them is unverified and may produce corrupt output.

        Raises ExeError when LIEF is unavailable, the file is not a supported
        executable, or it is not linked (object files cannot be edited).
        """
        if not HAS_LIEF:
            raise ExeError(
                "editing binaries requires the 'lief' package, which is not installed"
            )
        resolved = Path(path)
        if not resolved.is_file():
            raise ExeError(f"{resolved} does not exist")
        # Imported dynamically (and lazily, so importing emtrace stays
        # lief-free): a static `from . import ...` here would form an import
        # cycle with the package __init__.
        backends = tuple(
            importlib.import_module(f".{name}", __package__)
            for name in ("elf_backend", "pe_backend", "macho_backend")
        )

        for backend in backends:
            binary = backend.try_parse(resolved, experimental_be=experimental_be)
            if binary is not None:
                return ExeImage(resolved, backend.FORMAT, binary, backend)
        raise ExeError(
            f"{resolved} is not a supported linked executable (ELF, PE, or Mach-O)"
        )

    @property
    def format(self) -> str:
        """The detected format: 'elf', 'pe', or 'macho'."""
        return self._format

    def section_names(self) -> list[str]:
        """Names of all sections in the image."""
        return self._backend.section_names(self._binary)

    def has_section(self, name: str) -> bool:
        """Whether a section called *name* exists in the image."""
        return name in self.section_names()

    def strip_section(self, name: str) -> StripReport:
        """Remove the section *name* completely.

        The section entry is removed, symbols bound to it are deleted from
        static symbol tables, and its content is cleared so no traces remain
        parseable.  Refuses when other structures reference the section (see
        the per-format backends for the exact refusal rules).
        """
        section = self._backend.find_section(self._binary, name)
        if section is None:
            raise SectionNotFoundError(
                f"no section named '{name}' in the {self._format} image"
            )
        to_delete = self._backend.check_strippable(self._binary, section)
        removed = self._backend.delete_symbols(self._binary, to_delete)
        self._backend.remove_section(self._binary, section)
        rebuilt = self._backend.rebuild(self._binary)
        size_delta = len(rebuilt) - len(self._data)
        self._data = rebuilt
        return StripReport(
            section=name,
            removed_symbols=removed,
            size_delta=size_delta,
            format=self._format,
        )

    def make_nonload(self, name: str) -> NonloadReport:
        """Make the section *name* non-loadable, keeping its bytes in the file.

        Only supported where the format has a loadable flag (ELF); the PE and
        Mach-O backends raise NotSupportedError.  Returns a report; ``changed``
        is False when the section is already non-loadable.
        """
        section = self._backend.find_section(self._binary, name)
        if section is None:
            raise SectionNotFoundError(
                f"no section named '{name}' in the {self._format} image"
            )
        old_flags, old_addr, changed = self._backend.apply_nonload(
            self._binary, section
        )
        if changed:
            self._data = self._backend.rebuild(self._binary)
        return NonloadReport(
            section=name,
            changed=changed,
            old_flags=old_flags,
            old_addr=old_addr,
            format=self._format,
        )

    def bytes(self) -> bytes:
        """The current (possibly rebuilt) file content."""
        return bytes(self._data)
