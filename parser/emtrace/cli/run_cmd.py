"""The ``run`` command: process the trace output of a binary."""

from __future__ import annotations

import os
import shutil
import sys
from pathlib import Path
from typing import Any

from ..engine import emtrace as run_emtrace
from .output import print_issues
from .streams import dump_file, get_input_stream


def _run_test_comparison(captured_output: bytes, expected_output: bytes) -> int:
    """Compare captured output against expected output; 0 on match."""
    if captured_output == expected_output:
        print("Test passed!")
        return 0
    import difflib

    print("Test failed!", file=sys.stderr)
    diff = difflib.unified_diff(
        expected_output.decode("utf-8").splitlines(keepends=True),
        captured_output.decode("utf-8").splitlines(keepends=True),
        fromfile="expected",
        tofile="actual",
    )
    _ = sys.stdout.writelines(diff)
    return 1


def cmd_run(args: Any) -> int:
    """Process the trace output of a binary (the old `emtrace` script)."""
    if args.check_available:
        return 0

    if args.search_in_path:
        path = shutil.which(args.trace_info)
        if path is not None:
            args.trace_info = path

    args.trace_info = str(Path(args.trace_info).resolve())
    args.input = get_input_stream(args.input, [args.trace_info] + args.run_args)

    # lazy evaluate the default option ('emtrace_input.bin') of the argument
    if type(args.dump_input) is str:
        args.dump_input = dump_file(args.dump_input)

    def patched_input(n: int) -> bytes:
        b = args.input(n)
        args.dump_input[0](b)
        return b

    def ostream(b: bytes):
        _ = sys.stdout.buffer.write(b)
        if b"\n" in b:
            _ = sys.stdout.buffer.flush()

    if os.name == "posix":
        from signal import SIG_DFL, SIGPIPE, signal

        _ = signal(SIGPIPE, SIG_DFL)

    report = run_emtrace(
        Path(args.trace_info),
        args.format,
        patched_input,
        ostream,
        args.section_name,
        args.with_src_loc,
        args.src_hyperlinks,
        args.debug_script,
        args.test,
    )
    # flush
    _ = args.dump_input[1]()

    print_issues(report)
    if not report.ok:
        return 1

    if args.test is not None:
        assert report.captured is not None
        assert report.expected is not None
        return _run_test_comparison(report.captured, report.expected)
    return 0
