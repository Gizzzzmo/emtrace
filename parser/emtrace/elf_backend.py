"""ELF backend for :mod:`emtrace.exemod` (LIEF-based).

Refusal rules (ported from the previous hand-rolled surgery):
- unlinked object files (ET_REL) are refused; only ET_EXEC/ET_DYN are edited.
- only data sections (PROGBITS/NOBITS) can be removed: symbol tables, string
  tables, relocations, and other metadata sections are refused, since their
  linkages cannot be expressed safely here.
- removal is refused when a dynamic symbol references the section (dynamic
  hash tables cannot be rebuilt here) or when a relocation targets the
  section's address range or a deleted symbol.
"""

from __future__ import annotations

from pathlib import Path
from typing import Any

import lief

from .exemod import ExeError

FORMAT = "elf"

_ET_REL = 1
_ET_EXEC = 2
_ET_DYN = 3


def _linked_only(binary: Any) -> None:
    file_type = binary.header.file_type
    if file_type not in (
        lief.ELF.Header.FILE_TYPE.EXEC,
        lief.ELF.Header.FILE_TYPE.DYN,
    ):
        raise ExeError(
            "input is an unlinked ELF object file; "
            "run this command on a linked executable or shared object instead"
        )


def try_parse(path: Path, *, experimental_be: bool = False) -> Any | None:
    """Parse *path* as ELF, or return None when the magic does not match.

    Big-endian inputs are refused unless *experimental_be* is set (their
    rebuild is unverified and may produce corrupt output).
    """
    with path.open("rb") as f:
        ident = f.read(6)
        if ident[:4] != b"\x7fELF":
            return None
        if ident[5] == 2 and not experimental_be:
            raise ExeError(
                f"cannot edit '{path}': "
                f"big-endian ELF binaries are not supported "
                f"(use --experimental-be to try anyway; "
                f"rebuilding them is unverified)"
            )
    binary = lief.ELF.parse(str(path))
    if binary is None:
        raise ExeError(f"unable to parse '{path}' as ELF")
    _linked_only(binary)
    return binary


def section_names(binary: Any) -> list[str]:
    """Names of all sections in the image."""
    return [section.name for section in binary.sections]


def find_section(binary: Any, name: str) -> Any | None:
    """The section called *name*, or None when it does not exist."""
    return binary.get_section(name)


def _section_index(binary: Any, section: Any) -> int:
    return int(binary.get_section_idx(section))


def check_strippable(binary: Any, section: Any) -> list[Any]:
    """Validate that *section* can be removed; return bound static symbols.

    Raises ExeError when removal is unsafe.
    """
    sec_type = section.type
    if sec_type not in (
        lief.ELF.Section.TYPE.PROGBITS,
        lief.ELF.Section.TYPE.NOBITS,
    ):
        raise ExeError(
            f"cannot remove '{section.name}': "
            f"only data sections can be removed (section type {sec_type})"
        )

    index = _section_index(binary, section)

    bound = [symbol for symbol in binary.symbols if int(symbol.shndx) == index]
    bound_names = {symbol.name for symbol in bound}

    for symbol in binary.dynamic_symbols:
        if int(symbol.shndx) == index:
            raise ExeError(
                f"cannot remove '{section.name}': dynamic symbol '{symbol.name}' "
                f"references it (dynamic hash tables cannot be rebuilt here)"
            )

    start = int(section.virtual_address)
    end = start + int(section.size)
    for relocation in binary.relocations:
        address = int(relocation.address)
        if start <= address < end:
            raise ExeError(
                f"cannot remove '{section.name}': "
                f"a relocation targets its address range"
            )
        symbol = relocation.symbol if hasattr(relocation, "symbol") else None
        if symbol is not None and symbol.name in bound_names:
            raise ExeError(
                f"cannot remove '{section.name}': "
                f"a relocation references deleted symbol '{symbol.name}'"
            )
    return bound


def delete_symbols(binary: Any, symbols: list[Any]) -> int:
    """Delete static *symbols* from the symbol table; return the count."""
    for symbol in symbols:
        binary.remove_symtab_symbol(symbol)
    return len(symbols)


def remove_section(binary: Any, section: Any) -> None:
    """Remove *section*, clearing its content so no traces remain."""
    binary.remove(section, clear=True)


def apply_nonload(binary: Any, section: Any) -> tuple[int, int, bool]:
    """Clear SHF_ALLOC and the section address; return (old_flags, old_addr, changed)."""
    old_flags = int(section.flags)
    old_addr = int(section.virtual_address)
    if not section.has(lief.ELF.Section.FLAGS.ALLOC):
        return old_flags, old_addr, False
    section.flags = lief.ELF.Section.FLAGS(
        old_flags & ~int(lief.ELF.Section.FLAGS.ALLOC)
    )
    section.virtual_address = 0
    return old_flags, old_addr, True


def rebuild(binary: Any) -> bytes:
    """Rebuild the binary and return the new file content."""
    return bytes(binary.write_to_bytes())
