from typing import override


class SChar:
    """A wrapper for a single byte that can be formatted as a character or an integer."""

    def __init__(self, value: bytes) -> None:
        """Initialize the Char with a single byte."""
        self.value: int = value[0]
        if self.value >= 128:
            self.value -= 256

    @override
    def __format__(self, format_spec: str, /) -> str:
        if len(format_spec) == 0 or not format_spec[-1].isalpha():
            return ("{:" + format_spec + "c}").format(self.value)
        return ("{:" + format_spec + "}").format(self.value)

    @override
    def __repr__(self) -> str:
        return f"char({hex(self.value)})"


class Char:
    """A wrapper for a single byte that can be formatted as a character or an integer."""

    def __init__(self, value: bytes) -> None:
        """Initialize the Char with a single byte."""
        self.value: int = value[0]

    @override
    def __format__(self, format_spec: str, /) -> str:
        if len(format_spec) == 0 or not format_spec[-1].isalpha():
            return ("{:" + format_spec + "c}").format(self.value)
        return ("{:" + format_spec + "}").format(self.value)

    @override
    def __repr__(self) -> str:
        return f"char({hex(self.value)})"


class MyList[T]:
    def __init__(self, list_arg: list[T]) -> None:
        self.list: list[T] = list_arg

    @override
    def __format__(self, format_spec: str, /) -> str:
        parts = format_spec.split("*", 1)
        match parts:
            case [_]:
                return self.list.__format__(format_spec)
            case [sep, el_spec]:
                return sep.join([el.__format__(el_spec) for el in self.list])
            case _:
                assert False

    @override
    def __repr__(self) -> str:
        return self.list.__repr__()
