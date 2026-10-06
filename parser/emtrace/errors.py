"""Error-as-value types for the emtrace library.

Calling convention (two tiers):

- Tier 1: leaf lookups with a single obvious failure meaning return ``T | None``.
  ``None`` means only "not found / not applicable" — never "an error occurred
  but the details were discarded".
- Tier 2: command boundaries return report dataclasses that carry ``errors``
  and ``warnings`` (lists of :class:`Issue`) plus an :attr:`ok` property.
  Rule of thumb: if the CLI prints a specific message for a failure, the
  library must return that message as an :class:`Issue`.

If a Tier 1 helper grows a second failure mode with a user-facing message,
promote it to a tiny Tier 2 report. Internal layers may still raise
exceptions for control flow; public API boundaries fold them into issues.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Literal

Severity = Literal["error", "warning"]


@dataclass(frozen=True)
class Issue:
    """A single diagnostic message produced by a library call."""

    severity: Severity
    message: str

    @staticmethod
    def error(message: str) -> Issue:
        """Create an error-severity issue."""
        return Issue(severity="error", message=message)

    @staticmethod
    def warning(message: str) -> Issue:
        """Create a warning-severity issue."""
        return Issue(severity="warning", message=message)


@dataclass
class Report:
    """Base class for Tier 2 command reports.

    Subclasses add their result payload (counts, summaries, rebuilt bytes).
    ``errors`` being empty means success; ``warnings`` never affect success.
    """

    errors: list[Issue] = field(default_factory=list)
    warnings: list[Issue] = field(default_factory=list)

    @property
    def ok(self) -> bool:
        """Whether the command succeeded (no errors)."""
        return not self.errors

    def fail(self, message: str) -> None:
        """Append an error issue."""
        self.errors.append(Issue.error(message))

    def warn(self, message: str) -> None:
        """Append a warning issue."""
        self.warnings.append(Issue.warning(message))


class ExeError(Exception):
    """Raised when a file cannot be edited as a supported executable."""


class SectionNotFoundError(ExeError):
    """Raised when the requested section does not exist in the image."""


class NotSupportedError(ExeError):
    """Raised when an operation is not supported for the image or format."""
