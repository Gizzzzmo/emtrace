"""Input/output stream helpers for the ``run`` command."""

from __future__ import annotations

import socket
from argparse import ArgumentTypeError
from collections.abc import Callable
from pathlib import Path
from subprocess import PIPE, Popen
from typing import Any


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
                import sys

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
            assert False  # noqa: B011  # unreachable

    s = socket.socket(family, socket.SOCK_STREAM)
    s.connect(address)

    return s.recv


def dump_file(x: str | None) -> tuple[Callable[[bytes], Any], Callable[[], None]]:
    """Get a function that writes to a file and a function to close it."""
    if x is None:
        return lambda _: None, lambda: None

    f = Path(x).open("wb")  # noqa: SIM115  # handle intentionally returned
    return f.write, f.close
