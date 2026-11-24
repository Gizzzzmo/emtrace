"""Post-link editing operations for emtrace sections (library core).

``strip`` removes the section completely (ELF, PE, and Mach-O); ``nonload``
makes the section non-loadable on ELF, the same effect as an ``(INFO)``
region in a linker script (clear SHF_ALLOC and sh_addr, keep the section
bytes in the file so the traces remain parseable).

Both operations rebuild the binary with LIEF; the guarantees are behavioral
(the section entry is gone, its symbols are deleted, its content no longer
parses, and the binary still runs) rather than byte-exact.

All entry points return :class:`StripReport` / :class:`NonloadReport` with
``errors`` set on failure — this module never prints. File writing lives in
:mod:`emtrace.cli.edit_cmds`.
"""

from __future__ import annotations

from pathlib import Path

from .exemod import (
    ExeError,
    ExeImage,
    NonloadReport,
    NotSupportedError,
    SectionNotFoundError,
    StripReport,
)

__all__ = ["nonload_binary", "strip_binary"]


def _open_image(
    source: Path, command: str, experimental_be: bool
) -> tuple[ExeImage | None, str | None]:
    """Open *source* for *command*; return ``(image, None)`` or ``(None, error)``."""
    try:
        return ExeImage.open(source, experimental_be=experimental_be), None
    except (ExeError, NotSupportedError, SectionNotFoundError) as e:
        return None, f"{command}: {e}"


def strip_binary(
    path: str | Path,
    section_name: str = ".emtrace",
    *,
    experimental_be: bool = False,
) -> StripReport:
    """Remove the section *section_name* from the binary at *path* (Tier 2).

    On success, ``report.data`` holds the rebuilt binary bytes.
    """
    source = Path(path).resolve()
    report = StripReport(section=section_name)
    if not source.is_file():
        report.fail(f"{source} does not exist.")
        return report

    image, open_error = _open_image(source, "strip", experimental_be)
    if image is None:
        report.fail(open_error or f"strip: cannot open {source}.")
        return report

    try:
        inner = image.strip_section(section_name)
    except (SectionNotFoundError, NotSupportedError, ExeError) as e:
        report.fail(f"strip: {e}")
        return report

    inner.data = image.bytes()
    return inner


def nonload_binary(
    path: str | Path,
    section_name: str = ".emtrace",
    *,
    experimental_be: bool = False,
) -> NonloadReport:
    """Make *section_name* non-loadable in the binary at *path* (Tier 2).

    On success with changes, ``report.data`` holds the rebuilt binary bytes.
    """
    source = Path(path).resolve()
    report = NonloadReport(section=section_name)
    if not source.is_file():
        report.fail(f"{source} does not exist.")
        return report

    image, open_error = _open_image(source, "nonload", experimental_be)
    if image is None:
        report.fail(open_error or f"nonload: cannot open {source}.")
        return report

    try:
        inner = image.make_nonload(section_name)
    except (SectionNotFoundError, NotSupportedError, ExeError) as e:
        report.fail(f"nonload: {e}")
        return report

    if inner.changed:
        inner.data = image.bytes()
    return inner
