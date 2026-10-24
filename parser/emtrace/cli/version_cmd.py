"""The ``version`` command: package version plus the commit it was built from.

Resolution order for the commit: a git checkout around the package wins, since
there the code being run *is* that checkout's HEAD; otherwise the commit baked
into ``emtrace/_build_info.py`` by the build hook is used (that is the
provenance of an installed wheel); otherwise the commit is unknown.
"""

from __future__ import annotations

import importlib
import subprocess
from pathlib import Path
from typing import Any

from .. import __version__


def _git_commit() -> tuple[str, bool] | None:
    """``(commit, dirty)`` of the git checkout containing this package, or None."""
    cwd = Path(__file__).resolve().parent.parent  # the emtrace package directory
    try:
        proc = subprocess.run(
            ["git", "rev-parse", "HEAD"],
            cwd=cwd,
            capture_output=True,
            text=True,
            check=True,
        )
    except (OSError, subprocess.CalledProcessError):
        return None
    commit = proc.stdout.strip()
    try:
        status = subprocess.run(
            ["git", "status", "--porcelain", "-uno"],
            cwd=cwd,
            capture_output=True,
            text=True,
            check=True,
        )
    except (OSError, subprocess.CalledProcessError):
        return (commit, False)
    return commit, status.stdout.strip() != ""


def _baked_commit() -> tuple[str, bool] | None:
    """``(commit, dirty)`` recorded by the build hook, or None when unavailable."""
    try:
        module = importlib.import_module("emtrace._build_info")
    except ModuleNotFoundError:
        return None  # fresh checkout, no wheel build has run here yet
    commit = getattr(module, "COMMIT", None)
    if not isinstance(commit, str):
        return None  # built outside a git checkout without EMTRACE_BUILD_COMMIT
    return commit, bool(getattr(module, "DIRTY", False))


def version_text() -> str:
    """The full ``version`` output; also shown by the top-level ``--version`` flag."""
    info = _git_commit() or _baked_commit()
    if info is None:
        commit = "unknown"
    else:
        rev, dirty = info
        commit = f"{rev} (dirty)" if dirty else rev
    return f"emtrace {__version__}\ncommit:           {commit}"


def cmd_version(args: Any) -> int:
    """Print the emtrace version and the commit it was built from."""
    _ = args  # no options; present for the uniform subcommand dispatch signature
    print(version_text())
    return 0
