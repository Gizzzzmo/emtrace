"""Rendering of library reports for CLI users."""

from __future__ import annotations

import sys
from collections.abc import Callable
from typing import Any

from ..errors import Issue, Report
from ..hexdump import context_hexdump


def error(*args: Any, **kwargs: Any):
    """Print error-prefixed lines to stderr (one ``[error]`` per line)."""
    print(
        " ".join(
            [
                "\n".join("[error] " + line for line in str(arg).split("\n"))
                for arg in args
            ]
        ),
        file=sys.stderr,
        **kwargs,
    )


def print_issue(issue: Issue, data: bytes | None = None) -> None:
    """Print a single issue to stderr.

    When the bytes the issue refers to are given in *data*, a hexdump of the
    affected bytes is printed below the message.
    """
    if issue.severity == "error":
        error(issue.describe())
    else:
        print(f"[warning] {issue.describe()}", file=sys.stderr)
    if data is not None and issue.offset is not None:
        for line in context_hexdump(data, issue.offset, issue.length, indent="    "):
            print(line, file=sys.stderr)


def print_issues(report: Report, data: bytes | None = None) -> None:
    """Print all issues of *report* to stderr, errors first."""
    for issue in report.errors:
        print_issue(issue, data)
    for issue in report.warnings:
        print_issue(issue, data)


def exit_code(report: Report) -> int:
    """Map a report to a process exit code (0 on success, 1 on failure)."""
    return 0 if report.ok else 1


def debug_trace(enabled: bool) -> Callable[[str], None] | None:
    """A debug trace sink printing ``[trace]``-prefixed lines to stderr."""
    if not enabled:
        return None

    def trace(text: str) -> None:
        for line in text.split("\n"):
            print(f"[trace] {line}", file=sys.stderr)

    return trace
