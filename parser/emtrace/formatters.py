from typing import Any, Callable


def py_formatter(fmt: str, args: list[Any]) -> str:
    return fmt.format(*args)


def c_formatter(fmt: str, args: list[Any]) -> str:
    return fmt % tuple(args)


def raw_formatter(fmt: str, args: list[Any]) -> str:
    assert len(args) == 0
    return fmt


FORMATTERS: dict[int, Callable[[str, list[Any]], str]] = {
    0: py_formatter,
    1: raw_formatter,
    2: c_formatter,
}
