from . import emtrace
from argparse import ArgumentParser, ArgumentTypeError
from typing import Callable, Any
import sys
import os
import socket
import shutil
from pathlib import Path
from subprocess import Popen, PIPE


def get_input_stream(x: str, run_args: list[str]) -> Callable[[int], bytes]:
    """Get a function that reads bytes from an input stream."""
    parts = x.split("://", 1)
    if not parts:
        raise ArgumentTypeError("Bad input stream: cannot be empty string.")

    if len(parts) == 1:
        # auto detect stream_type
        stream_id = parts[0]
        match stream_id.split(":"):
            case ["stdin"]:
                return sys.stdin.buffer.read
            case ["run"]:
                p = Popen(run_args, stdout=PIPE)
                if p.stdout is None:
                    raise ArgumentTypeError(
                        "Failed to start the provided binary for reading its output."
                    )

                return p.stdout.read
            case parts if len(parts) in [2, 9]:
                stream_type = "tcp"
            case _:
                stream_type = "file"
    else:
        stream_type = parts[0]
        if stream_type not in ["tcp", "unix", "file"]:
            raise ArgumentTypeError(
                f"Bad input stream type {stream_type}: has to either be file, tcp or unix."
            )
        stream_id = parts[1]

    match stream_type:
        case "file":
            return Path(stream_id).open("rb").read
        case "unix":
            family = socket.AF_UNIX
            address = stream_id
        case "tcp":
            parts = stream_id.split(":")
            if len(parts) == 2:
                family = socket.AF_INET
                address = (parts[0], int(parts[1]))
            elif len(parts) == 9:
                family = socket.AF_INET6
                address = (":".join(parts[:-1]), int(parts[-1]))
            else:
                raise ArgumentTypeError(
                    "Bad address for tcp type input stream: Needs to be <ip-address>:<port> (ip can be ipv4 or ipv6)"
                )
        case _:
            assert False

    s = socket.socket(family, socket.SOCK_STREAM)
    s.connect(address)

    return s.recv


def dump(x: str | None) -> tuple[Callable[[bytes], Any], Callable[[], None]]:
    """Get a function that writes to a file and a function to close it."""
    if x is None:
        return lambda _: None, lambda: None

    f = Path(x).open("wb")
    return f.write, f.close


def main():
    parser = ArgumentParser("emtrace")

    _ = parser.add_argument(
        "trace_info",
        help="Path to either an executable, an emtrace json file, or a raw binary that contains the .emtrace section bytes of the program whose output to process.",
    )
    _ = parser.add_argument(
        "--format",
        nargs="?",
        default="auto",
        choices=["auto", "exe", "elf", "pe", "macho", "json", "binary"],
        help="Format that the main trace is in. By default (or when explicitly set to 'auto') try to auto-detect, which means first try to parse it as an executable file (meaning either ELF, PE, or MachO based on the magic bytes). If that is unsuccessful then try to parse it as json (with schema validation). If none of that worked the file is interpreted as raw binary.",
    )
    _ = parser.add_argument(
        "--input",
        "-i",
        nargs="?",
        default="run",
        help="Where to get the bytes from that the traced binary produced. If not supplied it executes the provided binary file, and processes its output. This can also be made explicit by setting it to 'run'. If set to 'stdin' it reads from stdin. Otherwise, if the argument looks like an IP address and port, it is interpreted as such, and an attempt is made to open a tcp connection to that socket and read the input from there. In all other cases it defaults to interpreting the argument as a file path from which the data will be read. To read from a file named 'run' or 'stdin' or one that looks like an IP address you can specify files with a prefix file://path/to/file. Tcp-, and also unix-, sockets can also be forced with either tcp://<ip>:<port> or unix://path/to/unix/socket",
    )
    _ = parser.add_argument(
        "--search-in-path",
        action="store_true",
        help="First check if there is a program in the PATH with the supplied name, and if there is, resolve its location and use that file to get the emtrace trace data (and also run that file if -i is set to run).",
    )
    _ = parser.add_argument(
        "--dump-input",
        nargs="?",
        default=dump(None),
        const="emtrace_input.bin",
        type=dump,
        help="Separately dump the bytes being processed to the given file (by default to emtrace_input.bin).",
    )
    _ = parser.add_argument(
        "--section-name",
        nargs="?",
        default=".emtrace",
        type=str,
        help="Specify which section of the elf file to read the format information from.",
    )
    _ = parser.add_argument(
        "--with-src-loc",
        nargs="?",
        default="none",
        const="relative",
        choices=["none", "absolute", "relative"],
        type=str,
        help="Prepend, to every line of trace output, the source location where it originated.",
    )
    _ = parser.add_argument(
        "--src-hyperlinks", action="store_true", help="Doesn't work yet."
    )
    _ = parser.add_argument(
        "--debug-script",
        action="store_true",
        help="Make the script output trace information for debugging.",
    )
    _ = parser.add_argument(
        "--test",
        nargs="?",
        const=".emt_exp",
        default=None,
        help="Run emtrace in test mode. This will read the expected output from the ELF section specified (default: .emt_exp), and will compare it against the actual output. A non-zero exit code is returned, and a diff is written to stdout in case of failure.",
    )
    _ = parser.add_argument(
        "--check-available",
        action="store_true",
        help="Exit with exit code zero and don't do anything else.",
    )

    argv = sys.argv
    parser_args = []
    for a in argv[1:]:
        if a == "--":
            break
        parser_args.append(a)

    run_args = argv[1 + len(parser_args) + 1 :]

    args = parser.parse_args(parser_args)

    if args.check_available:
        return 0

    if args.search_in_path:
        path = shutil.which(args.elf)
        if path is not None:
            args.trace_info = path

    args.trace_info = str(Path(args.trace_info).resolve())
    args.input = get_input_stream(args.input, [args.trace_info] + run_args)

    # lazy evaluate the default option ('emtrace_input.bin') of the argument
    if type(args.dump_input) is str:
        args.dump_input = dump(args.dump_input)

    def patched_input(n: int) -> bytes:
        b = args.input(n)
        args.dump_input[0](b)
        return b

    def ostream(b: bytes):
        _ = sys.stdout.buffer.write(b)
        if b"\n" in b:
            _ = sys.stdout.buffer.flush()

    if os.name == "posix":
        from signal import signal, SIGPIPE, SIG_DFL

        _ = signal(SIGPIPE, SIG_DFL)

    emtrace(
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


if __name__ == "__main__":
    sys.exit(main())
