"""Rendering of library reports for CLI users."""

from __future__ import annotations

import sys
from typing import Any

from ..errors import Issue, Report


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


def print_issue(issue: Issue) -> None:
    """Print a single issue to stderr (errors prefixed, warnings plain)."""
    if issue.severity == "error":
        error(issue.message)
    else:
        print(issue.message, file=sys.stderr)


def print_issues(report: Report) -> None:
    """Print all issues of *report* to stderr, errors first."""
    for issue in report.errors:
        print_issue(issue)
    for issue in report.warnings:
        print_issue(issue)


def exit_code(report: Report) -> int:
    """Map a report to a process exit code (0 on success, 1 on failure)."""
    return 0 if report.ok else 1
