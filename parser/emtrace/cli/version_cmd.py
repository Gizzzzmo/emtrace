"""The ``version`` command: package version plus the commit it was built from."""

from __future__ import annotations

from typing import Any

from .. import provenance


def version_text() -> str:
    """The full ``version`` output; also shown by the top-level ``--version`` flag."""
    return f"emtrace {provenance.__version__}\ncommit:           {provenance.build_commit()}"


def cmd_version(args: Any) -> int:
    """Print the emtrace version and the commit it was built from."""
    _ = args  # no options; present for the uniform subcommand dispatch signature
    print(version_text())
    return 0
