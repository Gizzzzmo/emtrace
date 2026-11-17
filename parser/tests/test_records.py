"""Unit tests for the unified section record parser."""

from __future__ import annotations

from typing import Literal

import pytest
from emtrace.section.records import (
    FRAMING_SIZE,
    MAGIC_CONSTANT,
    MAX_NESTING_DEPTH,
    MgicRecord,
    TraceRecord,
    detect_byteorder,
    dump_record,
    find_mgic,
    iter_records,
    parse_record,
    supported_version,
)
from emtrace.section.type_tree import Size

from .builders import (
    Entry,
    Layout,
    build_mgic_record,
    build_trce_record,
    composite,
    leaf,
)


def _resize_record(data: bytes, size: int) -> bytes:
    """Change the framed size and truncate the record to that size."""
    result = bytearray(data[:size])
    size_offset = result[4]
    size_width = result[7]
    byteorder: Literal["little", "big"] = (
        "little"
        if bytes(result[result[6] : result[6] + size_width]) == bytes(range(size_width))
        else "big"
    )
    result[size_offset : size_offset + size_width] = size.to_bytes(
        size_width, byteorder
    )
    return bytes(result)


@pytest.mark.parametrize(
    ("probe", "expected"),
    [
        (b"\x00", "little"),
        (b"\x00\x01\x02\x03", "little"),
        (b"\x03\x02\x01\x00", "big"),
        (b"\x02\x01\x00\x03", "unknown"),
        (b"", None),
        (b"\x00\x00", None),
        (bytes(257), None),
    ],
)
def test_detect_byteorder(probe: bytes, expected: str | None) -> None:
    assert detect_byteorder(probe) == expected


def test_framing_without_emt_prefix_reports_the_offset() -> None:
    parsed = parse_record(b"not a record", 0)
    assert parsed.framing is None
    assert parsed.errors[0].offset == 0
    assert "no 'EMT' prefix" in parsed.errors[0].message


def test_supported_version_accepts_higher_minor_only() -> None:
    known = [(1, 0), (2, 1)]
    assert supported_version(known, 2, 3) == (2, 1)
    assert supported_version(known, 2, 0) is None
    assert supported_version(known, 3, 0) is None


@pytest.mark.parametrize("size_t_size", [0, 4, 8])
def test_framing_rejects_zero_width_and_truncation(size_t_size: int) -> None:
    data = bytearray(build_trce_record())
    if size_t_size == 0:
        data[7] = 0
        parsed = parse_record(bytes(data), 0)
        assert parsed.framing is None
        assert parsed.errors[0].offset == 7
        assert "size_t width is 0" in parsed.errors[0].message
    else:
        parsed = parse_record(b"EMT\x0a", 0)
        assert parsed.framing is None
        assert parsed.errors[0].offset == 0
        assert "framing truncated" in parsed.errors[0].message


@pytest.mark.parametrize(
    ("mutation", "expected"),
    [
        ("probe", "endianness probe"),
        ("oversized", "extends past the end of the section"),
        ("undersized", "smaller than the 14-byte framing"),
        ("size_pointer", "record size at"),
    ],
)
def test_framing_rejects_invalid_probe_and_sizes(mutation: str, expected: str) -> None:
    data = bytearray(build_trce_record())
    if mutation == "probe":
        probe_start = data[6]
        data[probe_start : probe_start + data[7]] = b"\x02\x01\x00\x03\x00\x00\x00\x00"
    elif mutation == "oversized":
        size_start, width = data[4], data[7]
        data[size_start : size_start + width] = (len(data) + 10).to_bytes(
            width, "little"
        )
    elif mutation == "undersized":
        size_start, width = data[4], data[7]
        data[size_start : size_start + width] = (FRAMING_SIZE - 1).to_bytes(
            width, "little"
        )
    else:
        data[4] = len(data) + 1
    parsed = parse_record(bytes(data), 0)
    assert parsed.framing is None
    assert expected in parsed.errors[0].message
    assert parsed.errors[0].offset is not None


@pytest.mark.parametrize(
    ("field", "expected"),
    [
        ("probe", "endianness probe at"),
        ("size", "record size at"),
        ("type", "type tag at"),
        ("payload", "payload at"),
    ],
)
def test_framing_rejects_piece_offsets_outside_record_or_section(
    field: str, expected: str
) -> None:
    data = bytearray(build_trce_record())
    if field == "probe":
        data[6] = len(data) - 1
    elif field == "size":
        data[4] = len(data) - 1
    elif field == "type":
        data[3] = 250
    else:
        data[5] = 250
    parsed = parse_record(bytes(data), 0)
    assert parsed.framing is None
    assert any(expected in issue.message for issue in parsed.errors)
    assert parsed.errors[0].offset is not None


@pytest.mark.parametrize("payload_offset", [13, 14])
def test_framing_rejects_overlap_or_piece_inside_framing(payload_offset: int) -> None:
    data = bytearray(build_trce_record())
    data[5] = payload_offset
    parsed = parse_record(bytes(data), 0)
    assert parsed.framing is not None
    assert parsed.errors
    assert any(
        "overlap" in issue.message or "inside the 14-byte framing" in issue.message
        for issue in parsed.errors
    )
    assert all(issue.offset is not None for issue in parsed.errors)


def test_framing_accepts_big_endian_records() -> None:
    data = build_trce_record(size_t_size=4, byteorder="big")
    parsed = parse_record(data, 0)
    assert parsed.ok
    assert parsed.framing is not None
    assert parsed.framing.byteorder == "big"
    assert parsed.framing.size_t_size == 4
    assert parsed.next_offset == len(data)


@pytest.mark.parametrize("corruption", ["magic", "truncated", "invalid_metadata"])
def test_mgic_payload_failures_are_reported(corruption: str) -> None:
    data = bytearray(build_mgic_record())
    payload = data[5]
    if corruption == "magic":
        data[payload] ^= 0xFF
    elif corruption == "truncated":
        data = bytearray(_resize_record(bytes(data), payload + 32))
    else:
        data[payload + 32] = 0
    parsed = parse_record(bytes(data), 0)
    assert parsed.errors
    assert parsed.errors[0].offset is not None
    assert isinstance(parsed.value, MgicRecord) is (corruption == "invalid_metadata")
    if corruption == "magic":
        assert "magic constant mismatch" in parsed.errors[0].message
        assert parsed.errors[0].offset == payload
    if corruption == "truncated":
        assert "payload truncated" in parsed.errors[0].message
    if corruption == "invalid_metadata":
        assert not parsed.ok
        assert "sizeof(emt_ptr_t) is 0" in parsed.errors[0].message


def test_mgic_fields_and_round_trip() -> None:
    data = build_mgic_record(
        size_t_size=4,
        byteorder="big",
        ptr_size=4,
        alignment_power=2,
        encoding_id=2,
        magic_version=(1, 7),
    )
    parsed = parse_record(data, 0)
    assert parsed.ok
    assert isinstance(parsed.value, MgicRecord)
    assert parsed.value.version == (1, 7)
    assert parsed.value.magic_offset == data[5]
    assert parsed.value.size_t_size == 4
    assert parsed.value.byteorder == "big"
    assert (
        parsed.value.to_dict() == MgicRecord.from_dict(parsed.value.to_dict()).to_dict()
    )


def test_mgic_invalid_alignment_and_encoding_are_both_reported() -> None:
    data = bytearray(build_mgic_record(alignment_power=0, encoding_id=0))
    payload = data[5]
    data[payload + 33] = 64
    data[payload + 34] = 99
    parsed = parse_record(bytes(data), 0)
    assert isinstance(parsed.value, MgicRecord)
    assert not parsed.ok
    assert any("unreasonably large" in issue.message for issue in parsed.errors)
    assert any("unknown encoding_id=99" in issue.message for issue in parsed.errors)
    assert [issue.offset for issue in parsed.errors] == [payload + 33, payload + 34]


def test_mgic_unsupported_major_is_rejected() -> None:
    parsed = parse_record(build_mgic_record(magic_version=(255, 0)), 0)
    assert parsed.framing is not None
    assert parsed.value is None
    assert "unsupported record version 255.0" in parsed.errors[0].message


def test_unknown_type_and_unsupported_major_are_errors() -> None:
    unknown = bytearray(build_trce_record())
    unknown[10:14] = b"UNKN"
    unknown_parsed = parse_record(bytes(unknown), 0)
    assert unknown_parsed.framing is not None
    assert unknown_parsed.value is None
    assert "unknown record type" in unknown_parsed.errors[0].message
    assert unknown_parsed.errors[0].offset == 10

    unsupported = parse_record(build_trce_record(trace_version=(4, 0)), 0)
    assert unsupported.framing is not None
    assert unsupported.value is None
    assert "unsupported record version 4.0" in unsupported.errors[0].message
    assert unsupported.errors[0].offset == 8


def test_higher_minor_version_is_accepted() -> None:
    parsed = parse_record(build_trce_record(trace_version=(3, 9)), 0)
    assert parsed.ok
    assert isinstance(parsed.value, TraceRecord)
    assert parsed.value.version == (3, 9)
    assert parsed.framing is not None and parsed.framing.newer_minor_than_known


def test_unsupported_runtime_version_is_reported_without_losing_trace() -> None:
    parsed = parse_record(
        build_trce_record(trace_version=(2, 0), runtime_version=(4, 0)), 0
    )
    assert isinstance(parsed.value, TraceRecord)
    assert not parsed.ok
    assert any(
        "unsupported runtime data version 4.0" in e.message for e in parsed.errors
    )


@pytest.mark.parametrize(
    ("trace_version", "runtime_version", "padding"),
    [((1, 0), (1, 0), 0), ((2, 0), (2, 0), 0), ((3, 0), (3, 0), 7)],
)
def test_supported_trace_versions_parse(
    trace_version: tuple[int, int], runtime_version: tuple[int, int], padding: int
) -> None:
    node = leaf("int", 4, padding=padding)
    parsed = parse_record(
        build_trce_record(
            args=[node], trace_version=trace_version, runtime_version=runtime_version
        ),
        0,
    )
    assert parsed.ok
    assert isinstance(parsed.value, TraceRecord)
    assert parsed.value.args[0].padding == padding


def test_padding_is_invalid_before_runtime_version_3() -> None:
    parsed = parse_record(
        build_trce_record(
            args=[leaf("int", 4, padding=3)],
            trace_version=(3, 0),
            runtime_version=(2, 0),
        ),
        0,
    )
    assert isinstance(parsed.value, TraceRecord)
    issue = next(
        issue
        for issue in parsed.errors
        if "requires runtime data version" in issue.message
    )
    assert issue.offset is not None
    assert issue.path[-2:] == ("arg 0", "padding")
    assert not parsed.ok


def test_invalid_flag_reports_error_but_preserves_parsed_value() -> None:
    parsed = parse_record(build_trce_record(args=[("int", 4, 9, [])]), 0)
    assert isinstance(parsed.value, TraceRecord)
    assert parsed.value.args[0].size == Size("fixed", 4)
    assert not parsed.ok
    issue = next(issue for issue in parsed.errors if "invalid flag=9" in issue.message)
    assert issue.offset is not None
    assert issue.path[-2:] == ("arg 0", "size flag")


def test_zero_size_null_terminated_entry_is_an_error() -> None:
    parsed = parse_record(build_trce_record(args=[("char*", 0, 1, [])]), 0)
    assert isinstance(parsed.value, TraceRecord)
    issue = next(
        issue for issue in parsed.errors if "0-byte terminator" in issue.message
    )
    assert issue.offset is not None
    assert issue.path[-2:] == ("arg 0", "size")


def test_nested_type_tree_and_trace_round_trip() -> None:
    tree = composite(
        "vec2",
        2,
        [
            leaf("float", 4, name="x"),
            composite("list", 3, [leaf("int", 4, name="")], name="y"),
        ],
    )
    parsed = parse_record(build_trce_record(args=[tree], fmt_string="{0}"), 0)
    assert parsed.ok
    assert isinstance(parsed.value, TraceRecord)
    root = parsed.value.args[0]
    assert root.type_id == "vec2"
    assert [(child.name, child.type_id) for child in root.children] == [
        ("x", "float"),
        ("y", "list"),
    ]
    nested = TraceRecord.from_dict(parsed.value.to_dict(), parsed.value.record_start)
    assert nested == parsed.value


def test_multiple_arguments_and_nested_siblings_preserve_preorder() -> None:
    first = composite(
        "pair",
        1,
        [leaf("int", 4, name="x"), leaf("double", 8, name="y")],
    )
    parsed = parse_record(
        build_trce_record(args=[first, leaf("uint16_t", 2)], fmt_string="{0} {1}"),
        0,
    )
    assert parsed.ok
    assert isinstance(parsed.value, TraceRecord)
    assert len(parsed.value.args) == 2
    assert [child.name for child in parsed.value.args[0].children] == ["x", "y"]
    assert parsed.value.args[1].type_id == "uint16_t"


def test_mixed_size_widths_and_byteorders_in_one_section() -> None:
    little = build_trce_record(fmt_string="four", size_t_size=4, byteorder="little")
    big = build_trce_record(fmt_string="eight", size_t_size=8, byteorder="big")
    records = list(iter_records(b"garbage" + little + b"\xaa\xbb" + big))
    assert len(records) == 2
    assert all(
        record.ok and isinstance(record.value, TraceRecord) for record in records
    )
    assert [
        record.value.fmt_string
        for record in records
        if isinstance(record.value, TraceRecord)
    ] == [
        "four",
        "eight",
    ]
    assert [record.framing.byteorder for record in records if record.framing] == [
        "little",
        "big",
    ]


def test_layout_orders_gaps_and_string_gap_parse() -> None:
    layout = Layout(order=("payload", "probe", "size"), gaps=(3, 1, 2), gap_byte=0xA5)
    mgic = parse_record(build_mgic_record(layout=layout), 0)
    trce = parse_record(
        build_trce_record(
            fmt_string="custom layout",
            args=[composite("pair", 1, [leaf("int", 4, name="left")])],
            layout=layout,
            string_gap=11,
        ),
        0,
    )
    assert mgic.ok
    assert trce.ok
    assert isinstance(trce.value, TraceRecord)
    assert trce.value.fmt_string == "custom layout"


def test_string_pointer_outside_record_is_an_error() -> None:
    data = bytearray(build_trce_record())
    payload = data[5]
    pointer_offset = payload + 3 * data[7]
    data[pointer_offset : pointer_offset + data[7]] = (len(data) + 1).to_bytes(
        data[7], "little"
    )
    parsed = parse_record(bytes(data), 0)
    issue = next(
        issue for issue in parsed.errors if "points outside the record" in issue.message
    )
    assert issue.offset == pointer_offset
    assert issue.path[-1] == "format string"
    assert isinstance(parsed.value, TraceRecord)


def test_string_without_terminator_is_an_error() -> None:
    data = bytearray(build_trce_record())
    data[-1] = 0x41
    parsed = parse_record(bytes(data), 0)
    issue = next(
        issue
        for issue in parsed.errors
        if "not null-terminated within the record" in issue.message
    )
    assert issue.offset is not None and issue.offset < len(data)
    assert issue.path[-1] == "source file"


def test_invalid_utf8_string_warns_and_decodes_with_replacement() -> None:
    data = bytearray(build_trce_record(fmt_string="hello"))
    payload = data[5]
    pointer_offset = payload + 3 * data[7]
    string_offset = int.from_bytes(
        data[pointer_offset : pointer_offset + data[7]], "little"
    )
    data[string_offset] = 0xFF
    parsed = parse_record(bytes(data), 0)
    assert parsed.ok
    assert not parsed.errors
    assert parsed.warnings
    assert isinstance(parsed.value, TraceRecord)
    assert "�" in parsed.value.fmt_string
    assert parsed.warnings[0].offset == string_offset


def test_layout_table_running_past_record_end_is_fatal_with_offset_inside() -> None:
    full = build_trce_record()
    payload = full[5]
    width = full[7]
    # Keep the six preceding size_t fields and half the source-line size_t.
    short_size = payload + 6 * width + width // 2
    parsed = parse_record(_resize_record(full, short_size), 0)
    assert parsed.value is None
    assert parsed.errors
    issue = next(
        issue
        for issue in parsed.errors
        if "extends past the end of the record" in issue.message
    )
    assert issue.offset is not None
    assert issue.offset < short_size


def test_type_tree_depth_limit_is_reported() -> None:
    def nested_child(depth: int) -> Entry:
        children = [nested_child(depth - 1)] if depth else []
        return Entry("node", 1, 0, name="child", children=children)

    parsed = parse_record(
        build_trce_record(
            args=[
                Entry(
                    "root",
                    1,
                    0,
                    children=[nested_child(MAX_NESTING_DEPTH + 1)],
                )
            ]
        ),
        0,
    )
    assert parsed.value is None
    issue = next(
        issue for issue in parsed.errors if "nested deeper than" in issue.message
    )
    assert issue.offset is not None
    assert issue.path


def test_iter_records_reports_invalid_emt_and_continues() -> None:
    valid_before = build_trce_record(fmt_string="before")
    junk_emt = b"EMT\x0a"
    valid_after = build_trce_record(fmt_string="after")
    records = list(iter_records(valid_before + junk_emt + b"garbage" + valid_after))
    assert len(records) == 3
    assert records[0].ok
    assert records[1].framing is None
    assert records[1].errors
    assert records[1].start == len(valid_before)
    assert records[2].ok
    assert isinstance(records[2].value, TraceRecord)
    assert records[2].value.fmt_string == "after"


def test_iter_records_empty_section_and_unknown_type_continue() -> None:
    assert list(iter_records(b"\x00garbage")) == []
    unknown = bytearray(build_trce_record(fmt_string="unknown"))
    unknown[10:14] = b"UNKN"
    mgic = build_mgic_record()
    trace = build_trce_record(fmt_string="last")
    records = list(iter_records(mgic + bytes(unknown) + trace))
    assert len(records) == 3
    assert isinstance(records[0].value, MgicRecord)
    assert records[1].value is None
    assert "unknown record type" in records[1].errors[0].message
    assert isinstance(records[2].value, TraceRecord)
    assert records[2].value.fmt_string == "last"


def test_find_mgic_with_leading_garbage_and_missing_framing() -> None:
    prefix = b"\xde\xad\xbe\xef" * 8
    data = build_mgic_record(prefix=prefix)
    parsed = find_mgic(data)
    assert parsed is not None and parsed.ok
    assert parsed.start == len(prefix)
    assert find_mgic(b"junk" + MAGIC_CONSTANT + bytes(20)) is None
    assert find_mgic(b"no record") is None
    assert find_mgic(build_trce_record()) is None


def test_dump_record_includes_annotated_field_labels() -> None:
    data = build_trce_record(args=[composite("pair", 1, [leaf("int", 4, name="x")])])
    parsed = parse_record(data, 0)
    lines = dump_record(data, parsed)
    text = "\n".join(lines)
    assert "format string" in text
    assert "arg 0 › type id" in text
