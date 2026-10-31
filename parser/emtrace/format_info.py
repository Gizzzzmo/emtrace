from __future__ import annotations

from dataclasses import dataclass
from typing import Any, Literal


@dataclass
class Size:
    kind: Literal["fixed", "length_prefixed", "null_terminated"]
    # when kind == "fixed", the fixed size in bytes
    # when kind == "length_prefixed", irrelevant
    # when kind == "null_terminated", the number of expected null bytes to terminate the sequence
    size: int


class TypeInfo:
    size: Size
    children: dict[int | str, tuple[str, TypeInfo]]
    runtime_data_version: tuple[int, int]

    def to_dict(self) -> dict[str, Any]:
        return {
            "size": {
                "kind": self.size.kind,
                "size": self.size.size,
            },
            "children": {
                k: {"id": v[0], "type_info": v[1].to_dict()}
                for k, v in self.children.items()
            },
        }

    @staticmethod
    def from_dict(
        data: dict[str, Any],
        runtime_data_version_major: int,
        runtime_data_version_minor: int,
    ) -> TypeInfo:
        size_data = data["size"]
        size = Size(
            size_data["kind"],
            size_data["size"],
        )
        children_data = data["children"]
        children = {
            k: (
                v["id"],
                TypeInfo.from_dict(
                    v["type_info"],
                    runtime_data_version_major,
                    runtime_data_version_minor,
                ),
            )
            for k, v in children_data.items()
        }
        return TypeInfo(
            size, runtime_data_version_major, runtime_data_version_minor, children
        )

    def __init__(
        self,
        size: Size,
        runtime_data_version_major: int,
        runtime_data_version_minor: int,
        children: dict[int | str, tuple[str, TypeInfo]] | None = None,
    ) -> None:
        self.size = size
        self.runtime_data_version = (
            runtime_data_version_major,
            runtime_data_version_minor,
        )
        if children is None:
            self.children = {}
        else:
            self.children = children


class FmtInfo:
    """Contains all information about a format string and its parameters."""

    def __init__(
        self,
        fmt_string: str,
        runtime_data_version_major: int,
        runtime_data_version_minor: int,
        formatter: int = 0,
    ) -> None:
        """Initialize the FmtInfo."""
        self.fmt_string: str = fmt_string
        self.runtime_data_version: tuple[int, int] = (
            runtime_data_version_major,
            runtime_data_version_minor,
        )
        self.formatter: int = formatter
        self.type_infos: list[tuple[str, TypeInfo]] = []
        self.file: str = ""
        self.line: int = -1
        self.trace_record_version: tuple[int, int] | None = None

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
            "trace_data_version": {
                "major": self.runtime_data_version[0],
                "minor": self.runtime_data_version[1],
            },
            "version": {
                "major": self.trace_record_version[0],
                "minor": self.trace_record_version[1],
            }
            if self.trace_record_version is not None
            else None,
        }

    @staticmethod
    def from_dict(data: dict[str, Any]) -> FmtInfo:
        trace_data_version = data.get("trace_data_version") or {}
        runtime_data_version_major = trace_data_version.get("major", 1)
        runtime_data_version_minor = trace_data_version.get("minor", 0)
        fmt_info = FmtInfo(
            data["fmt_string"],
            runtime_data_version_major,
            runtime_data_version_minor,
            data.get("formatter", 0),
        )
        fmt_info.type_infos = [
            (
                type_info["id"],
                TypeInfo.from_dict(
                    type_info["type_info"],
                    runtime_data_version_major,
                    runtime_data_version_minor,
                ),
            )
            for type_info in data["type_infos"]
        ]
        fmt_info.file = data["file"]
        fmt_info.line = data["line"]
        version = data.get("version")
        if version is not None:
            fmt_info.trace_record_version = (version["major"], version["minor"])
        return fmt_info

    def add_source_info(self, file: str, line: int) -> None:
        """Add source location information to the format info."""
        self.file = file
        self.line = line

    def add_version(self, major: int, minor: int) -> None:
        """Record the record-type version this trace point was parsed with."""
        self.trace_record_version = (major, minor)

    def add_param(self, id: str, type_info: TypeInfo) -> None:
        """Add a parameter to the format info."""
        self.type_infos.append((id, type_info))
