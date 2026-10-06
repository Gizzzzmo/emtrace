"""Mach-O backend for :mod:`emtrace.exemod` (LIEF-based).

Refusal rules:
- fat (multi-architecture) binaries are refused; only thin binaries are edited.
- unlinked object files (MH_OBJECT) are refused.
- removal is refused when a code signature is present (any byte change
  invalidates it; re-sign with ``codesign`` instead), when the section has
  relocations, or when an external (exported or imported) symbol points into
  the section.  Local symbols pointing into the section are deleted.
"""

from __future__ import annotations

from pathlib import Path
from typing import Any

import lief

from .exemod import ExeError

FORMAT = "macho"

# On-disk magic bytes. Thin little-endian files start with CE/DB FA ED FE
# (MH_MAGIC / MH_MAGIC_64 as read on a little-endian host); the byte-swapped
# FE ED FA CE/CF forms are big-endian (MH_CIGAM / MH_CIGAM_64). FAT_MAGIC is
# always stored big-endian.
_MACHO_MAGICS = {
    b"\xce\xfa\xed\xfe",
    b"\xcf\xfa\xed\xfe",
    b"\xca\xfe\xba\xbe",
}

_BE_MAGICS = {
    b"\xfe\xed\xfa\xce",
    b"\xfe\xed\xfa\xcf",
}


def try_parse(path: Path, *, experimental_be: bool = False) -> Any | None:
    """Parse *path* as thin Mach-O, or return None when the magic mismatches.

    Big-endian (CIGAM) inputs are always refused: writing them back is only
    supported by LIEF 1.0, so rebuilding them here may produce corrupt output.
    """
    _ = experimental_be
    with path.open("rb") as f:
        magic = f.read(4)
        if magic in _BE_MAGICS:
            raise ExeError(
                f"cannot edit '{path}': big-endian Mach-O binaries are not supported"
            )
        if magic not in _MACHO_MAGICS:
            return None
    fat = lief.MachO.parse(str(path))
    if fat is None:
        raise ExeError(f"unable to parse '{path}' as Mach-O")
    if fat.size != 1:
        raise ExeError(
            f"cannot edit '{path}': "
            f"fat binaries with {fat.size} architectures are not supported"
        )
    binary = fat.at(0)
    if binary.header.file_type == lief.MachO.Header.FILE_TYPE.OBJECT:
        raise ExeError(
            "input is an unlinked Mach-O object file; "
            "run this command on a linked executable instead"
        )
    return binary


def section_names(binary: Any) -> list[str]:
    """Names of all sections in the image."""
    return [section.name for section in binary.sections]


def find_section(binary: Any, name: str) -> Any | None:
    """The section called *name*, or None when it does not exist."""
    for section in binary.sections:
        if section.name == name:
            return section
    return None


def _resident_symbols(binary: Any, section: Any) -> tuple[list[Any], list[Any]]:
    """Split symbols pointing into *section* into (local, external) lists."""
    start = int(section.virtual_address)
    end = start + int(section.size)
    local: list[Any] = []
    external: list[Any] = []
    for symbol in binary.symbols:
        value = int(symbol.value)
        if start <= value < end:
            (external if symbol.is_external else local).append(symbol)
    return local, external


def check_strippable(binary: Any, section: Any) -> list[Any]:
    """Validate that *section* can be removed; return local symbols to delete.

    Raises ExeError when removal is unsafe.
    """
    if binary.has_code_signature:
        raise ExeError(
            f"cannot remove '{section.name}': "
            f"the binary is code signed (re-sign with 'codesign' after editing)"
        )
    if int(section.numberof_relocations) > 0:
        raise ExeError(f"cannot remove '{section.name}': the section has relocations")
    local, external = _resident_symbols(binary, section)
    if external:
        names = ", ".join(sorted(f"'{symbol.name}'" for symbol in external))
        raise ExeError(
            f"cannot remove '{section.name}': external symbol(s) {names} point into it"
        )
    for symbol in local:
        if not binary.can_remove_symbol(symbol.name):
            raise ExeError(
                f"cannot remove '{section.name}': "
                f"symbol '{symbol.name}' cannot be removed"
            )
    return local


def delete_symbols(binary: Any, symbols: list[Any]) -> int:
    """Delete static *symbols* from the symbol table; return the count."""
    for symbol in symbols:
        binary.remove_symbol(symbol.name)
    return len(symbols)


def remove_section(binary: Any, section: Any) -> None:
    """Remove *section*, clearing its content so no traces remain."""
    binary.remove_section(section.segment_name, section.name, clear=True)


def apply_nonload(binary: Any, section: Any) -> tuple[int, int, bool]:
    """Nonload has no Mach-O equivalent; always raise."""
    _ = binary, section
    raise ExeError(
        "nonload is only implemented for ELF; "
        "use strip to remove the section from Mach-O binaries"
    )


def rebuild(binary: Any) -> bytes:
    """Rebuild the binary and return the new file content."""
    return bytes(binary.write_to_bytes())
