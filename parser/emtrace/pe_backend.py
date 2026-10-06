"""PE backend for :mod:`emtrace.exemod` (LIEF-based).

Refusal rules:
- COFF object files (not executable images or DLLs) are refused.
- removal is refused when Authenticode signatures or overlay data are
  present (any byte shift would invalidate them), when a relocation block
  targets the section's address range, when a debug entry points into the
  section, or when COFF symbols exist (their section numbers cannot be
  fixed up safely here).
"""

from __future__ import annotations

from pathlib import Path
from typing import Any

import lief

from .exemod import ExeError

FORMAT = "pe"


def _linked_only(binary: Any) -> None:
    header = binary.header
    characteristics = lief.PE.Header.CHARACTERISTICS
    if not (
        header.has_characteristic(characteristics.EXECUTABLE_IMAGE)
        or header.has_characteristic(characteristics.DLL)
    ):
        raise ExeError(
            "input is a COFF object file, not a linked image; "
            "run this command on a linked executable or DLL instead"
        )


def try_parse(path: Path, *, experimental_be: bool = False) -> Any | None:
    """Parse *path* as PE, or return None when the magic does not match."""
    _ = experimental_be  # PE is little-endian by specification.
    with path.open("rb") as f:
        if f.read(2) != b"MZ":
            return None
    binary = lief.PE.parse(str(path))
    if binary is None:
        raise ExeError(f"unable to parse '{path}' as PE")
    _linked_only(binary)
    return binary


def section_names(binary: Any) -> list[str]:
    """Names of all sections in the image."""
    return [section.name for section in binary.sections]


def find_section(binary: Any, name: str) -> Any | None:
    """The section called *name*, or None when it does not exist."""
    return binary.get_section(name)


def check_strippable(binary: Any, section: Any) -> list[Any]:
    """Validate that *section* can be removed.

    Always returns an empty deletion list: any COFF symbol table presence is
    a refusal (see module docstring).  Raises ExeError when unsafe.
    """
    if len(binary.signatures) > 0 or len(bytes(binary.overlay)) > 0:
        raise ExeError(
            f"cannot remove '{section.name}': "
            f"the binary carries signatures or overlay data"
        )
    if len(binary.symbols) > 0:
        raise ExeError(
            f"cannot remove '{section.name}': "
            f"the binary has a COFF symbol table "
            f"(section numbers cannot be fixed up safely here)"
        )

    start = int(section.virtual_address)
    end = start + max(int(section.virtual_size), len(bytes(section.content)))
    for relocation in binary.relocations:
        if start <= int(relocation.virtual_address) < end:
            raise ExeError(
                f"cannot remove '{section.name}': "
                f"a relocation targets its address range"
            )
    for debug in binary.debug:
        raw_addr = getattr(debug, "addressof_rawdata", None)
        if raw_addr is not None and start <= int(raw_addr) < end:
            raise ExeError(
                f"cannot remove '{section.name}': a debug entry points into it"
            )
    return []


def delete_symbols(binary: Any, symbols: list[Any]) -> int:
    """Delete static *symbols*; the PE backend never has any (see above)."""
    _ = binary
    return 0


def remove_section(binary: Any, section: Any) -> None:
    """Remove *section*, clearing its content so no traces remain."""
    binary.remove(section, clear=True)


def apply_nonload(binary: Any, section: Any) -> tuple[int, int, bool]:
    """Nonload has no PE equivalent; always raise."""
    _ = binary, section
    raise ExeError(
        "nonload is only implemented for ELF; "
        "use strip to remove the section from PE binaries"
    )


def rebuild(binary: Any) -> bytes:
    """Rebuild the binary and return the new file content."""
    return bytes(binary.write_to_bytes())
