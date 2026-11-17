"""Formatters that combine a format string with the decoded arguments."""

from __future__ import annotations

import re
from collections.abc import Callable
from typing import Any

from .values import Placeholder

Formatter = Callable[[str, list[Any]], str]


def py_formatter(fmt: str, args: list[Any]) -> str:
    return fmt.format(*args)


# A printf-style conversion specification as understood by Python's `%`.
_PRINTF_SPEC = re.compile(
    r"%(?:\([^)]*\))?[-#0 +]*(?P<width>\*|\d+)?(?:\.(?P<precision>\*|\d*))?[hlL]?"
    r"(?P<conversion>[diouxXeEfFgGcrsab%])"
)


def _placeholders_as_strings(fmt: str, args: list[Any]) -> tuple[str, list[Any]]:
    """Rewrite the conversions of placeholder arguments to plain ``%s``.

    A placeholder can't be formatted with e.g. ``%d``; replacing the whole
    conversion (including any ``*`` width/precision arguments it consumes)
    lets it be shown in place of the value.
    """
    if not any(isinstance(arg, Placeholder) for arg in args):
        return fmt, args
    parts: list[str] = []
    new_args: list[Any] = []
    pos = 0
    arg_index = 0
    for match in _PRINTF_SPEC.finditer(fmt):
        parts.append(fmt[pos : match.start()])
        pos = match.end()
        if match["conversion"] == "%":
            parts.append(match.group())
            continue
        consumed = [arg_index + i for i in range(int(match["width"] == "*"))]
        consumed += [
            arg_index + len(consumed) + i for i in range(int(match["precision"] == "*"))
        ]
        value_index = arg_index + len(consumed)
        arg_index = value_index + 1
        if value_index < len(args) and isinstance(args[value_index], Placeholder):
            parts.append("%s")
            new_args.append(args[value_index])
        else:
            parts.append(match.group())
            new_args += [args[i] for i in [*consumed, value_index] if i < len(args)]
    parts.append(fmt[pos:])
    new_args += args[arg_index:]
    return "".join(parts), new_args


def c_formatter(fmt: str, args: list[Any]) -> str:
    fmt, args = _placeholders_as_strings(fmt, args)
    return fmt % tuple(args)


def raw_formatter(fmt: str, args: list[Any]) -> str:
    if args:
        raise ValueError(f"the raw formatter takes no arguments, got {len(args)}")
    return fmt


FORMATTERS: dict[int, Formatter] = {
    0: py_formatter,
    1: raw_formatter,
    2: c_formatter,
}
