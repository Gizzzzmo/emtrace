"""Version and build provenance of the emtrace package.

Resolution order for the commit: a git checkout around the package wins, since
there the code being run *is* that checkout's HEAD; otherwise the commit baked
into ``emtrace/_build_info.py`` by the build hook is used (that is the
provenance of an installed wheel); otherwise the commit is unknown.
"""

from __future__ import annotations

import importlib
import subprocess
from pathlib import Path

from ._version import __version__

__all__ = ["__version__", "baked_commit", "build_commit", "git_commit"]


def git_commit() -> tuple[str, bool] | None:
    """``(commit, dirty)`` of the git checkout containing this package, or None."""
    cwd = Path(__file__).resolve().parent  # the emtrace package directory
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


def baked_commit() -> tuple[str, bool] | None:
    """``(commit, dirty)`` recorded by the build hook, or None when unavailable."""
    try:
        module = importlib.import_module("emtrace._build_info")
    except ModuleNotFoundError:
        return None  # fresh checkout, no wheel build has run here yet
    commit = getattr(module, "COMMIT", None)
    if not isinstance(commit, str):
        return None  # built outside a git checkout without EMTRACE_BUILD_COMMIT
    return commit, bool(getattr(module, "DIRTY", False))


def build_commit() -> str:
    """The commit emtrace was built from, suffixed ``(dirty)``, or ``unknown``."""
    info = git_commit() or baked_commit()
    if info is None:
        return "unknown"
    rev, dirty = info
    return f"{rev} (dirty)" if dirty else rev
