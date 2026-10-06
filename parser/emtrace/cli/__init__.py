"""Command-line interface for emtrace (thin wrapper around the library).

Each subcommand calls into :mod:`emtrace.engine`, :mod:`emtrace.inspector`,
or :mod:`emtrace.edit`, renders the returned report's issues, and maps the
result to a process exit code. All user-facing output lives here; the
library modules never print.
"""
