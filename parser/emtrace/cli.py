from . import emtrace
from argparse import ArgumentParser, ArgumentTypeError
from typing import Callable, Any
import sys
import socket
from pathlib import Path


def get_input_stream(x: str) -> Callable[[int], bytes]:
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
            with Path(stream_id).open("rb") as f:
                return f.read
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
        "elf",
        help="Path to either an elf executable, or a raw binary that contains the .emtrace section bytes of the program whose output to process.",
    )
    _ = parser.add_argument(
        "--input",
        "-i",
        nargs="?",
        default=get_input_stream("stdin"),
        type=get_input_stream,
        help="Where to get the bytes from that the traced binary produced. If not supplied it reads from stdin, otherwise it defaults to interpreting the argument as a file path from which the data will be read. It can also read from either an IP- or a unix-socket by specifying either tcp://<ip>:<port> or unix://path/to/unix/socket",
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
        const=".emtrace.test.expected",
        default=None,
        help="Run emtrace in test mode. This will read the expected output from the ELF section specified (default: .emtrace.test.expected), and will compare it against the actual output. A non-zero exit code is returned, and a diff is written to stdout in case of failure.",
    )

    args = parser.parse_args()

    # lazy evaluate the default option ('emtrace_input.bin') of the argument
    if type(args.dump_input) is str:
        args.dump_input = dump(args.dump_input)

    def patched_input(n: int) -> bytes:
        b = args.input(n)
        args.dump_input[0](b)
        return b

    emtrace(
        Path(args.elf),
        patched_input,
        sys.stdout.buffer.write,
        args.section_name,
        args.with_src_loc,
        args.src_hyperlinks,
        args.debug_script,
        args.test,
    )
    # flush
    _ = args.dump_input[1]()


if __name__ == "__main__":
    main()
