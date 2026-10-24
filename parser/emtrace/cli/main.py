"""emtrace command-line interface.

One entry point with subcommands:
  run      process the trace output of a binary (the old `emtrace` script)
  check    validate an emtrace section against the trace format specification
  export   export trace-point metadata as JSON
  inspect  summarize the emtrace data of an executable, object file, or archive
  strip    remove the emtrace section from a linked ELF binary completely
  nonload  make the emtrace section non-loadable ((INFO) linker-script effect)
  version  print the emtrace version and the commit it was built from
"""

from __future__ import annotations

import sys
from argparse import ArgumentParser, _SubParsersAction

from . import edit_cmds, read_cmds, run_cmd, version_cmd
from .streams import dump_file


def _add_input_args(sub: ArgumentParser) -> None:
    """Add --format and --section-name to a subcommand parser."""
    _ = sub.add_argument(
        "input",
        help=(
            "Path to an executable, or a raw binary file containing the .emtrace section bytes."
        ),
    )
    _ = sub.add_argument(
        "--format",
        nargs="?",
        default="auto",
        choices=["auto", "exe", "elf", "pe", "macho", "binary"],
        help=(
            "Format of the input file. 'auto' (default) tries to detect automatically: "
            "first as an executable (ELF/PE/MachO), then as raw binary."
        ),
    )
    _ = sub.add_argument(
        "--section-name",
        default=".emtrace",
        metavar="NAME",
        help="ELF/PE/MachO section name to read (default: .emtrace).",
    )


def _add_edit_args(sub: ArgumentParser) -> None:
    """Add the shared arguments of the strip and nonload commands."""
    _ = sub.add_argument(
        "input",
        help="Path to a linked executable or shared object (ELF, PE, or Mach-O).",
    )
    _ = sub.add_argument(
        "--section-name",
        default=".emtrace",
        metavar="NAME",
        help="Section to operate on (default: .emtrace).",
    )
    _ = sub.add_argument(
        "--output",
        "-o",
        default=None,
        metavar="FILE",
        help="Write the modified binary to FILE instead of modifying INPUT in place.",
    )
    _ = sub.add_argument(
        "--experimental-be",
        action="store_true",
        help=(
            "Allow big-endian inputs. Rebuilding big-endian binaries is "
            "unverified and may produce corrupt output — verify before use."
        ),
    )


# ─────────────────────────────────────────────────────────────────────────────
# Parser construction
# ─────────────────────────────────────────────────────────────────────────────


def build_parser() -> ArgumentParser:
    parser = ArgumentParser(
        "emtrace",
        description="Parse and format emtrace data: run, check, export, inspect, strip, nonload.",
    )
    _ = parser.add_argument(
        "--version",
        action="version",
        version=version_cmd.version_text(),
        help="Print the emtrace version and the commit it was built from, then exit.",
    )
    subs: _SubParsersAction[ArgumentParser] = parser.add_subparsers(
        dest="command", metavar="<command>"
    )
    _ = subs.required = True

    # ── run ────────────────────────────────────────────────────────────────
    run_parser = subs.add_parser(
        "run",
        help="Process the trace output of a binary.",
        description=(
            "Process the trace output of a binary and print the formatted traces. "
            "Everything after '--' is passed as arguments to the traced binary."
        ),
    )
    _ = run_parser.add_argument(
        "trace_info",
        help="Path to either an executable, an emtrace json file, or a raw binary that contains the .emtrace section bytes of the program whose output to process.",
    )
    _ = run_parser.add_argument(
        "--format",
        nargs="?",
        default="auto",
        choices=["auto", "exe", "elf", "pe", "macho", "json", "binary"],
        help="Format that the main trace is in. By default (or when explicitly set to 'auto') try to auto-detect, which means first try to parse it as an executable file (meaning either ELF, PE, or MachO based on the magic bytes). If that is unsuccessful then try to parse it as json (with schema validation). If none of that worked the file is interpreted as raw binary.",
    )
    _ = run_parser.add_argument(
        "--input",
        "-i",
        nargs="?",
        default="run",
        help="Where to get the bytes from that the traced binary produced. If not supplied it executes the provided binary file, and processes its output. This can also be made explicit by setting it to 'run'. If set to 'stdin' it reads from stdin. Otherwise, if the argument looks like an IP address and port, it is interpreted as such, and an attempt is made to open a tcp connection to that socket and read the input from there. In all other cases it defaults to interpreting the argument as a file path from which the data will be read. To read from a file named 'run' or 'stdin' or one that looks like an IP address you can specify files with a prefix file://path/to/file. Tcp-, and also unix-, sockets can also be forced with either tcp://<ip>:<port> or unix://path/to/unix/socket",
    )
    _ = run_parser.add_argument(
        "--search-in-path",
        action="store_true",
        help="First check if there is a program in the PATH with the supplied name, and if there is, resolve its location and use that file to get the emtrace trace data (and also run that file if -i is set to run).",
    )
    _ = run_parser.add_argument(
        "--dump-input",
        nargs="?",
        default=dump_file(None),
        const="emtrace_input.bin",
        type=dump_file,
        help="Separately dump the bytes being processed to the given file (by default to emtrace_input.bin).",
    )
    _ = run_parser.add_argument(
        "--section-name",
        nargs="?",
        default=".emtrace",
        type=str,
        help="Specify which section of the elf file to read the format information from.",
    )
    _ = run_parser.add_argument(
        "--with-src-loc",
        nargs="?",
        default="none",
        const="relative",
        choices=["none", "absolute", "relative"],
        type=str,
        help="Prepend, to every line of trace output, the source location where it originated.",
    )
    _ = run_parser.add_argument(
        "--src-hyperlinks", action="store_true", help="Doesn't work yet."
    )
    _ = run_parser.add_argument(
        "--debug-script",
        action="store_true",
        help="Make the script output trace information for debugging.",
    )
    _ = run_parser.add_argument(
        "--test",
        nargs="?",
        const=".emt_exp",
        default=None,
        help="Run emtrace in test mode. This will read the expected output from the ELF section specified (default: .emt_exp), and will compare it against the actual output. A non-zero exit code is returned, and a diff is written to stdout in case of failure.",
    )
    _ = run_parser.add_argument(
        "--check-available",
        action="store_true",
        help="Exit with exit code zero and don't do anything else.",
    )
    run_parser.set_defaults(func=run_cmd.cmd_run)

    # ── check ──────────────────────────────────────────────────────────────
    check_parser = subs.add_parser(
        "check",
        help="Check that an emtrace section conforms to the trace format specification.",
        description=(
            "Scans all records in the emtrace section and validates them against the "
            "trace format specification: framing layout, per-record versions, record "
            "payloads, MGIC uniqueness, that the MGIC record's alignment power "
            "does not exceed the alignment power all records actually conform to, "
            "and that every record's address is representable and unique in a "
            "emt_ptr_t. Major version incompatibilities are a hard error; minor "
            "version differences are ignored unless --error-on-minor is given."
        ),
    )
    _add_input_args(check_parser)
    _ = check_parser.add_argument(
        "--error-on-minor",
        action="store_true",
        help="Treat records with a higher minor version than known as errors instead of ignoring them.",
    )
    check_parser.set_defaults(func=read_cmds.cmd_check)

    # ── export ─────────────────────────────────────────────────────────────
    export_parser = subs.add_parser(
        "export",
        help="Export all trace-point metadata from an emtrace section as JSON.",
        description=(
            "Parses every TRCE record in the emtrace section and writes a JSON "
            "representation to stdout (or to a file with --output)."
        ),
    )
    _add_input_args(export_parser)
    _ = export_parser.add_argument(
        "--output",
        "-o",
        default="-",
        metavar="FILE",
        help="Write JSON output to FILE instead of stdout. Use '-' for stdout (default).",
    )
    _ = export_parser.add_argument(
        "--debug-script",
        action="store_true",
        help="Debug the script by printing internal trace information to stderr (for development use only).",
    )
    export_parser.set_defaults(func=read_cmds.cmd_export)

    # ── inspect ────────────────────────────────────────────────────────────
    inspect_parser = subs.add_parser(
        "inspect",
        help="Summarize the emtrace data of an executable, object file, or static archive.",
        description=(
            "Auto-detects whether the input is a static archive, an unlinked "
            "object file, or a linked executable, and prints summary information: "
            "record counts per type, record-type versions, size_t widths, "
            "detected endianness, emt_ptr_t size, the alignment power reported by "
            "the MGIC record, the highest alignment all records actually conform "
            "to, and — for archives — the highest possible alignment power "
            "achievable when linking."
        ),
    )
    _ = inspect_parser.add_argument(
        "input",
        help="Path to a static archive (.a), object file (.o), executable, or raw section bytes.",
    )
    _ = inspect_parser.add_argument(
        "--section-name",
        default=".emtrace",
        metavar="NAME",
        help="ELF/PE/MachO section name to inspect (default: .emtrace).",
    )
    inspect_parser.set_defaults(func=read_cmds.cmd_inspect)

    # ── strip ──────────────────────────────────────────────────────────────
    strip_parser = subs.add_parser(
        "strip",
        help="Remove an emtrace section from a linked binary completely.",
        description=(
            "Removes the section entry, deletes symbols bound to it, and "
            "clears its content (rebuilding the binary with LIEF) — the same "
            "outcome as objcopy --remove-section. The binary keeps running, "
            "but its traces can no longer be parsed."
        ),
    )
    _add_edit_args(strip_parser)
    strip_parser.set_defaults(func=edit_cmds.cmd_strip)

    # ── nonload ────────────────────────────────────────────────────────────
    nonload_parser = subs.add_parser(
        "nonload",
        help=(
            "Make an emtrace section non-loadable (ELF only), the same effect "
            "as an (INFO) region in a linker script."
        ),
        description=(
            "Clears the SHF_ALLOC flag and the section address of the "
            "section, the same effect as placing it in an (INFO) region of a "
            "linker script. The section bytes stay in the file, so the traces "
            "remain parseable, but tools that load or flash the binary (the "
            "OS loader, gdb/lldb, objcopy to raw binary or ihex) ignore "
            "non-ALLOC sections. Only implemented for ELF; use strip for "
            "PE and Mach-O binaries."
        ),
    )
    _add_edit_args(nonload_parser)
    nonload_parser.set_defaults(func=edit_cmds.cmd_nonload)

    # ── version ────────────────────────────────────────────────────────────
    version_parser = subs.add_parser(
        "version",
        help="Print the emtrace version and the commit it was built from.",
        description=(
            "Print the emtrace version and the git commit it was built from. "
            "For a wheel this is the commit baked in at build time; when "
            "running from a git checkout it is the checkout's current HEAD, "
            "suffixed ' (dirty)' when the tree has uncommitted changes."
        ),
    )
    version_parser.set_defaults(func=version_cmd.cmd_version)

    return parser


def main() -> int:
    # Everything after the first '--' is not meant for this script but for the
    # binary that `run` will execute.
    argv = sys.argv[1:]
    if "--" in argv:
        sep = argv.index("--")
        parser_args, run_args = argv[:sep], argv[sep + 1 :]
    else:
        parser_args, run_args = argv, []

    args = build_parser().parse_args(parser_args)
    args.run_args = run_args
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
