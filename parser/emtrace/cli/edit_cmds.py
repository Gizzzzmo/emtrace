"""The ``strip`` and ``nonload`` commands (file writing only)."""

from __future__ import annotations

import os
import tempfile
from pathlib import Path
from typing import Any

from .. import edit
from .output import print_issues


def _write_result(output: str | None, source: Path, data: bytes) -> None:
    if output:
        _ = Path(output).write_bytes(data)
        _ = os.chmod(output, source.stat().st_mode & 0o7777)
        print(f"wrote modified binary to {output}")
        return

    # Modify in place, atomically: write to a temporary file in the same
    # directory (preserving the file mode) and rename it over the input.
    with tempfile.NamedTemporaryFile(
        dir=source.parent, prefix=source.name + ".", suffix=".tmp", delete=False
    ) as tmp:
        _ = tmp.write(data)
        tmp_name = tmp.name
    _ = os.chmod(tmp_name, source.stat().st_mode & 0o7777)
    os.replace(tmp_name, source)
    print(f"modified {source} in place")


def cmd_nonload(args: Any) -> int:
    """Make the emtrace section non-loadable ((INFO) linker-script effect)."""
    source = Path(args.input).resolve()
    report = edit.nonload_binary(
        source,
        args.section_name,
        experimental_be=bool(getattr(args, "experimental_be", False)),
    )
    if not report.ok:
        assert report.data is None
        print_issues(report)
        return 1

    if not report.changed:
        print(
            f"{report.section} is already non-loadable (loadable flag not set); "
            f"nothing to do."
        )
        return 0

    assert report.data is not None
    _write_result(args.output, source, report.data)
    addr_size = 8 if report.format == "elf" else 4
    print(
        f"{report.section}: loadable flag cleared "
        f"(flags {report.old_flags:#x} -> {report.old_flags & ~0x2:#x}), "
        f"address {report.old_addr:#0{2 + 2 * addr_size}x} -> 0x0."
    )
    return 0


def cmd_strip(args: Any) -> int:
    """Remove the emtrace section from the binary completely."""
    source = Path(args.input).resolve()
    old_size = source.stat().st_size if source.is_file() else 0
    report = edit.strip_binary(
        source,
        args.section_name,
        experimental_be=bool(getattr(args, "experimental_be", False)),
    )
    if not report.ok:
        assert report.data is None
        print_issues(report)
        return 1

    assert report.data is not None
    _write_result(args.output, source, report.data)
    new_size = len(report.data)
    details = f"removed the {report.section} section entry"
    if report.removed_symbols:
        details += f", deleted {report.removed_symbols} symbol(s)"
    details += f" (file changed from {old_size} to {new_size} bytes)."
    print(details)
    return 0
