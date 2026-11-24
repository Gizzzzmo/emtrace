"""The type tree of a trace point, as described by a ``TRCE`` record.

The type tree fully determines how the runtime bytes of a trace event are
split up (see :mod:`emtrace.stream.framing`); type ids are opaque labels that
only become meaningful when decoding those bytes into Python values (see
:mod:`emtrace.stream.decoding`).
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any, Literal

SizeKind = Literal["fixed", "null_terminated", "length_prefixed"]

#: Size flag values as stored in ``TRCE`` records.
FLAG_STATIC = 0
FLAG_NULL_TERMINATED = 1
FLAG_LENGTH_PREFIXED = 2

FLAG_KINDS: dict[int, SizeKind] = {
    FLAG_STATIC: "fixed",
    FLAG_NULL_TERMINATED: "null_terminated",
    FLAG_LENGTH_PREFIXED: "length_prefixed",
}


@dataclass(frozen=True)
class Size:
    """How the runtime size of a value is determined.

    - ``fixed``: ``size`` bytes (leaves) or ``size`` elements (composites).
    - ``null_terminated``: content followed by a terminator of ``size``
      null bytes.
    - ``length_prefixed``: an element (or byte) count precedes the content;
      ``size`` is only used by runtime-data version 1.0, where it is the
      width of the count.
    """

    kind: SizeKind
    size: int

    def to_dict(self) -> dict[str, Any]:
        return {"kind": self.kind, "size": self.size}

    @staticmethod
    def from_dict(data: dict[str, Any]) -> Size:
        kind = data["kind"]
        if kind not in ("fixed", "null_terminated", "length_prefixed"):
            raise ValueError(f"unknown size kind {kind!r}")
        return Size(kind, int(data["size"]))


@dataclass
class TypeNode:
    """A node of a trace point's type tree.

    ``name`` is the name of the node within its parent (empty for top-level
    arguments and unnamed elements). ``padding`` is the number of bytes that
    follow the node's content in the runtime stream (and are discarded).
    Nodes with ``children`` are composites: every runtime element of a
    composite is the concatenation of one value per child, in order.
    """

    type_id: str
    size: Size
    padding: int = 0
    name: str = ""
    children: list[TypeNode] = field(default_factory=list)

    @property
    def is_leaf(self) -> bool:
        return not self.children

    def to_dict(self) -> dict[str, Any]:
        return {
            "type_id": self.type_id,
            "name": self.name,
            "size": self.size.to_dict(),
            "padding": self.padding,
            "children": [child.to_dict() for child in self.children],
        }

    @staticmethod
    def from_dict(data: dict[str, Any]) -> TypeNode:
        return TypeNode(
            type_id=str(data["type_id"]),
            size=Size.from_dict(data["size"]),
            padding=int(data["padding"]),
            name=str(data["name"]),
            children=[TypeNode.from_dict(child) for child in data["children"]],
        )

    def describe(self) -> str:
        """A compact one-line description, e.g. ``list[fixed 3] +2``."""
        text = f"{self.type_id!r} {self.size.kind} {self.size.size}"
        if self.padding:
            text += f" +{self.padding} padding"
        if self.children:
            text += f", {len(self.children)} child(ren)"
        return text
