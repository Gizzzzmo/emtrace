from __future__ import annotations

from typing import Any
from dataclasses import dataclass


@dataclass
class Size:
    min_size: int
    length_prefixed: bool
    null_terminated: bool


class TypeInfo:
    size: Size
    children: dict[int | str, tuple[str, TypeInfo]]

    def to_dict(self) -> dict[str, Any]:
        return {
            "size": {
                "min_size": self.size.min_size,
                "length_prefixed": self.size.length_prefixed,
                "null_terminated": self.size.null_terminated,
            },
            "children": {
                k: {"id": v[0], "type_info": v[1].to_dict()}
                for k, v in self.children.items()
            },
        }

    @staticmethod
    def from_dict(data: dict[str, Any]) -> TypeInfo:
        size_data = data["size"]
        size = Size(
            size_data["min_size"],
            size_data["length_prefixed"],
            size_data["null_terminated"],
        )
        children_data = data["children"]
        children = {
            k: (v["id"], TypeInfo.from_dict(v["type_info"]))
            for k, v in children_data.items()
        }
        return TypeInfo(size, children)

    def __init__(
        self, size: Size, children: dict[int | str, tuple[str, TypeInfo]] | None = None
    ) -> None:
        self.size = size
        if children is None:
            self.children = {}
        else:
            self.children = children


class FmtInfo:
    """Contains all information about a format string and its parameters."""

    def __init__(
        self,
        fmt_string: str,
        formatter: int = 0,
    ) -> None:
        """Initialize the FmtInfo."""
        self.fmt_string: str = fmt_string
        self.formatter: int = formatter
        self.type_infos: list[tuple[str, TypeInfo]] = []
        self.file: str = ""
        self.line: int = -1

    def to_dict(self) -> dict[str, Any]:
        return {
            "fmt_string": self.fmt_string,
            "formatter": self.formatter,
            "type_infos": [
                {"id": id, "type_info": type_info.to_dict()}
                for id, type_info in self.type_infos
            ],
            "file": self.file,
            "line": self.line,
        }

    @staticmethod
    def from_dict(data: dict[str, Any]) -> FmtInfo:
        fmt_info = FmtInfo(data["fmt_string"], data["formatter"])
        fmt_info.type_infos = [
            (type_info["id"], TypeInfo.from_dict(type_info["type_info"]))
            for type_info in data["type_infos"]
        ]
        fmt_info.file = data["file"]
        fmt_info.line = data["line"]
        return fmt_info

    def add_source_info(self, file: str, line: int) -> None:
        """Add source location information to the format info."""
        self.file = file
        self.line = line

    def add_param(self, id: str, type_info: TypeInfo) -> None:
        """Add a parameter to the format info."""
        self.type_infos.append((id, type_info))
