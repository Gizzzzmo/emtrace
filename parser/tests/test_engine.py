"""Tests of the run engine with synthetic sections and streams."""

from __future__ import annotations

import json
import struct
from collections.abc import Callable
from io import BytesIO
from pathlib import Path
from typing import Any

import pytest
from emtrace import __version__
from emtrace.errors import Issue
from emtrace.section.export import export_section
from emtrace.section.loading import TraceInfoFormat
from emtrace.stream.engine import RunReport, emtrace

from .builders import (
    Entry,
    build_mgic_record,
    build_section,
    build_trce_record,
    cobs_encode,
    composite,
    encode_pointer,
    leaf,
    leb128,
    record_offsets,
)

PTR = 4


class Section:
    """A section with an MGIC record followed by the given TRCE records."""

    def __init__(
        self, *records: bytes, encoding_id: int = 0, alignment_power: int = 2
    ) -> None:
        mgic = build_mgic_record(
            ptr_size=PTR, encoding_id=encoding_id, alignment_power=alignment_power
        )
        self.records: list[bytes] = [mgic, *records]
        self.alignment_power: int = alignment_power
        self.data: bytes = build_section(self.records, alignment_power)
        self.offsets: list[int] = record_offsets(self.records, alignment_power)

    def pointer(self, index: int) -> bytes:
        """The record pointer of record *index* (0 is the MGIC record)."""
        return encode_pointer(self.offsets[index], PTR, self.alignment_power)


def run(
    tmp_path: Path,
    section: Section,
    stream: bytes,
    reader: Callable[[BytesIO], Callable[[int], bytes]] = lambda s: s.read,
    file_format: TraceInfoFormat = "binary",
    trace_info: Path | None = None,
    **kwargs: Any,
) -> tuple[RunReport, str, list[Issue]]:
    if trace_info is None:
        trace_info = tmp_path / "section.bin"
        _ = trace_info.write_bytes(section.data)
    out = bytearray()
    sunk: list[Issue] = []
    report = emtrace(
        trace_info,
        file_format,
        istream=reader(BytesIO(stream)),
        ostream=out.extend,
        issue_sink=sunk.append,
        **kwargs,
    )
    return report, out.decode(), sunk


def raw_stream(section: Section, *events: bytes) -> bytes:
    return section.pointer(0) + b"".join(events)


def cobs_stream(section: Section, *events: bytes) -> bytes:
    return b"".join(cobs_encode(e) for e in (section.pointer(0), *events))


INT_RECORD = build_trce_record("{0} and {1}\n", args=[leaf("int", 4), leaf("int", 4)])
STRING_RECORD = build_trce_record(
    "<{0}>\n", args=[leaf("string", 1, "null_terminated")]
)


def test_raw_stream(tmp_path: Path):
    section = Section(INT_RECORD, STRING_RECORD)
    stream = raw_stream(
        section,
        section.pointer(1) + struct.pack("<ii", 1, -2),
        section.pointer(2) + b"hey\x00",
        section.pointer(1) + struct.pack("<ii", 3, 4),
    )
    report, out, issues = run(tmp_path, section, stream)
    assert report.ok, report.errors
    assert issues == []
    assert out == "1 and -2\n<hey>\n3 and 4\n"
    assert report.records == 3


def test_partial_reads_like_sockets(tmp_path: Path):
    section = Section(INT_RECORD)
    stream = raw_stream(section, section.pointer(1) + struct.pack("<ii", 5, 6))
    report, out, _ = run(
        tmp_path, section, stream, reader=lambda s: lambda n: s.read(1)
    )
    assert report.ok
    assert out == "5 and 6\n"


def test_cobs_stream(tmp_path: Path):
    section = Section(INT_RECORD, STRING_RECORD, encoding_id=1)
    stream = cobs_stream(
        section,
        section.pointer(1) + struct.pack("<ii", 0, 256),
        section.pointer(2) + b"\x00",
    )
    report, out, _ = run(tmp_path, section, stream)
    assert report.ok, report.errors
    assert out == "0 and 256\n<>\n"


def test_big_endian_record_with_little_endian_pointers(tmp_path: Path):
    be_record = build_trce_record(
        "{0}\n", args=[leaf("int", 4)], size_t_size=4, byteorder="big"
    )
    mgic = build_mgic_record(
        ptr_size=PTR, byteorder="big", size_t_size=4, alignment_power=2
    )
    data = build_section([mgic, be_record], 2)
    offsets = record_offsets([mgic, be_record], 2)
    section = Section()
    section.data, section.offsets, section.alignment_power = data, offsets, 2
    stream = raw_stream(section, section.pointer(1) + struct.pack(">i", -7))
    report, out, _ = run(tmp_path, section, stream)
    assert report.ok, report.errors
    assert out == "-7\n"


def test_unknown_type_id_placeholder_and_deduplicated_warning(tmp_path: Path):
    rec = build_trce_record("{0}|{1:08x}\n", args=[leaf("int", 4), leaf("frob", 2)])
    section = Section(rec)
    event = section.pointer(1) + struct.pack("<i", 1) + b"\xab\xcd"
    report, out, issues = run(
        tmp_path, section, raw_stream(section, event, event, event)
    )
    assert report.ok
    assert out == "1|<frob: unknown type id: ab cd>\n" * 3
    warnings = [i.message for i in issues if i.severity == "warning"]
    assert len(warnings) == 2
    assert (
        "arg 1: unknown type id (type id 'frob') (shown as placeholder)" in warnings[0]
    )
    assert "2 repetition(s)" in warnings[1]


def test_drop_undecodable(tmp_path: Path):
    rec = build_trce_record("{0}\n", args=[leaf("frob", 2)])
    section = Section(rec, INT_RECORD)
    stream = raw_stream(
        section,
        section.pointer(1) + b"\x01\x02",
        section.pointer(2) + struct.pack("<ii", 1, 2),
    )
    report, out, issues = run(tmp_path, section, stream, drop_undecodable=True)
    assert report.ok
    assert out == "1 and 2\n"
    assert report.dropped == 1
    assert report.records == 1
    assert any(i.severity == "warning" for i in issues)


def test_c_formatter_with_placeholder(tmp_path: Path):
    rec = build_trce_record(
        "%d %x\n", formatter_id=2, args=[leaf("int", 4), leaf("frob", 1)]
    )
    section = Section(rec)
    report, out, _ = run(
        tmp_path, section, raw_stream(section, section.pointer(1) + bytes(4) + b"\x05")
    )
    assert report.ok
    assert out == "0 <frob: unknown type id: 05>\n"


def test_format_error_is_reported_and_decoding_continues(tmp_path: Path):
    bad = build_trce_record("{5}\n", args=[leaf("int", 4)])
    section = Section(bad, INT_RECORD)
    stream = raw_stream(
        section,
        section.pointer(1) + bytes(4),
        section.pointer(2) + struct.pack("<ii", 1, 2),
    )
    report, out, _ = run(tmp_path, section, stream)
    assert out == "1 and 2\n"
    assert any("Failed to format" in e.message for e in report.errors)


def test_truncated_raw_stream(tmp_path: Path):
    section = Section(INT_RECORD)
    stream = raw_stream(
        section,
        section.pointer(1) + struct.pack("<ii", 1, 2),
        section.pointer(1) + b"\x01",
    )
    report, out, _ = run(tmp_path, section, stream)
    assert out == "1 and 2\n"
    assert len(report.errors) == 1
    message = report.errors[0].message
    assert "event 2" in message and "arg 0" in message
    assert "^^" in message  # hexdump of the event


def test_truncated_cobs_frame_continues_with_next_frame(tmp_path: Path):
    section = Section(INT_RECORD, encoding_id=1)
    stream = cobs_stream(
        section,
        section.pointer(1) + b"\x01",
        section.pointer(1) + struct.pack("<ii", 3, 4),
    )
    report, out, _ = run(tmp_path, section, stream)
    assert out == "3 and 4\n"
    assert len(report.errors) == 1


def test_leftover_bytes_in_cobs_frame(tmp_path: Path):
    section = Section(INT_RECORD, encoding_id=1)
    stream = cobs_stream(
        section, section.pointer(1) + struct.pack("<ii", 3, 4) + b"\x09"
    )
    report, out, _ = run(tmp_path, section, stream)
    assert out == "3 and 4\n"
    assert any("leftover" in e.message for e in report.errors)


def test_pointer_to_invalid_record(tmp_path: Path):
    broken = build_trce_record("{0}\n", args=[Entry("int", 4, 7)])
    section = Section(broken, INT_RECORD, encoding_id=1)
    stream = cobs_stream(
        section,
        section.pointer(1) + bytes(4),
        encode_pointer(len(section.data) + 64 & ~3, PTR, 2),
        section.pointer(2) + struct.pack("<ii", 1, 2),
    )
    report, out, _ = run(tmp_path, section, stream)
    assert out == "1 and 2\n"
    messages = [e.message for e in report.errors]
    assert any("invalid flag=7" in m for m in messages)
    assert any("outside of the section" in m for m in messages)


def test_raw_stream_stops_at_undecodable_record(tmp_path: Path):
    section = Section(INT_RECORD)
    stream = raw_stream(
        section,
        encode_pointer(len(section.data) + 64 & ~3, PTR, 2),
        section.pointer(1) + struct.pack("<ii", 1, 2),
    )
    report, out, _ = run(tmp_path, section, stream)
    assert out == ""
    assert len(report.errors) == 1


def test_length_prefixed_and_nested_values(tmp_path: Path):
    rec = build_trce_record(
        "{0:,*d} {1}\n",
        args=[
            composite("list", 0, [leaf("int", 2)], "length_prefixed"),
            composite(
                "list", 2, [composite("list", 0, [leaf("char", 1)], "length_prefixed")]
            ),
        ],
    )
    section = Section(rec)
    event = (
        section.pointer(1)
        + leb128(3)
        + struct.pack("<3h", 1, -2, 3)
        + leb128(1)
        + b"a"
        + leb128(0)
    )
    report, out, _ = run(tmp_path, section, raw_stream(section, event))
    assert report.ok, report.errors
    assert out == "1,-2,3 [[char(0x61)], []]\n"


def test_structs_and_dicts(tmp_path: Path):
    point = composite("struct", 1, [leaf("int", 4, name="x"), leaf("int", 4, name="y")])
    counts = composite(
        "dict",
        0,
        [leaf("string", 1, "null_terminated"), leaf("unsigned", 2)],
        "length_prefixed",
    )
    rec = build_trce_record("{0.x}/{0[y]} {0} {1:, *=*}\n", args=[point, counts])
    section = Section(rec)
    event = (
        section.pointer(1)
        + struct.pack("<ii", 3, -4)
        + leb128(2)
        + b"a\x00"
        + struct.pack("<H", 1)
        + b"b\x00"
        + struct.pack("<H", 2)
    )
    report, out, _ = run(tmp_path, section, raw_stream(section, event))
    assert report.ok, report.errors
    assert out == "3/-4 {'x': 3, 'y': -4} a=1, b=2\n"


def test_json_export_round_trip(tmp_path: Path):
    section = Section(INT_RECORD, STRING_RECORD)
    exported = export_section(section.data)
    assert exported.ok and exported.output is not None
    json_path = tmp_path / "trace.json"
    _ = json_path.write_text(json.dumps(exported.output))
    stream = raw_stream(section, section.pointer(2) + b"json\x00")
    report, out, _ = run(
        tmp_path, section, stream, file_format="json", trace_info=json_path
    )
    assert report.ok, report.errors
    assert out == "<json>\n"


def test_json_export_of_other_version_is_rejected(tmp_path: Path):
    section = Section(INT_RECORD)
    exported = export_section(section.data)
    assert exported.output is not None
    exported.output["emtrace_version"] = __version__ + ".other"
    json_path = tmp_path / "trace.json"
    _ = json_path.write_text(json.dumps(exported.output))
    report, out, _ = run(
        tmp_path, section, raw_stream(section), file_format="json", trace_info=json_path
    )
    assert out == ""
    assert any("Re-export" in e.message for e in report.errors)


@pytest.mark.parametrize("encoding_id", [0, 1])
def test_empty_stream(tmp_path: Path, encoding_id: int):
    report, out, _ = run(tmp_path, Section(encoding_id=encoding_id), b"")
    assert out == ""
    assert any("MGIC" in e.message for e in report.errors)


def test_debug_script_dumps_byte_trees(tmp_path: Path):
    section = Section(INT_RECORD)
    lines: list[str] = []
    report, _, _ = run(
        tmp_path,
        section,
        raw_stream(section, section.pointer(1) + struct.pack("<ii", 1, 2)),
        debug_script=True,
        trace_sink=lines.append,
    )
    assert report.ok
    text = "\n".join(lines)
    assert "arg 1: 'int'" in text
    assert "values [1, 2]" in text
