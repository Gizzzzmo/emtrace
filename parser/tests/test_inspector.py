"""Unit tests for emtrace.inspector (check, export helpers, inspect)."""

from __future__ import annotations

import struct
import pytest
from pathlib import Path

from emtrace import inspector
from emtrace.inspector import (
    _check,
    _detect_input_type,
    _parse_archive,
    _parse_member_elf,
    _summarize_records,
)
from emtrace.record_parser import RecordParser, RecordVersionError

from .test_record_parser import build_mgic_record, build_trce_record


# ─────────────────────────────────────────────────────────────────────────────
# _check
# ─────────────────────────────────────────────────────────────────────────────


def run_check(
    data: bytes, *, error_on_minor: bool = False
) -> tuple[list[str], list[str]]:
    warnings: list[str] = []
    errors: list[str] = []
    mgic = None
    try:
        mgic = RecordParser(memoryview(data)).find_and_parse_mgic()
    except RecordVersionError as e:
        errors.append(str(e))
    _check(
        data,
        mgic,
        error_on_minor=error_on_minor,
        warnings=warnings,
        errors=errors,
    )
    return warnings, errors


def aligned_section(alignment_power: int = 0) -> bytes:
    """A valid section: MGIC followed by a TRCE record at an aligned offset."""
    mgic = build_mgic_record(alignment_power=alignment_power)
    trce = build_trce_record(fmt_string="{0}", args=[("int", 4, 0, [])])
    if alignment_power == 0:
        return mgic + trce
    step = 2**alignment_power
    pad_len = (-len(mgic)) % step
    return mgic + b"\x00" * pad_len + trce


def test_check_valid_section():
    warnings, errors = run_check(aligned_section())
    assert errors == []
    assert warnings == []


def test_check_valid_section_with_alignment():
    warnings, errors = run_check(aligned_section(alignment_power=3))
    assert errors == []
    assert warnings == []


def test_check_no_mgic():
    warnings, errors = run_check(build_trce_record())
    assert any("No MGIC record found" in e for e in errors)


def test_check_duplicate_mgic():
    data = build_mgic_record() + build_mgic_record()
    warnings, errors = run_check(data)
    assert any("exactly one" in e for e in errors)


def test_check_trce_major_version_mismatch():
    data = build_mgic_record() + build_trce_record(trace_version=(2, 0))
    warnings, errors = run_check(data)
    assert any("unsupported major version 2.0" in e for e in errors)


def test_check_mgic_major_version_mismatch():
    data = build_mgic_record(magic_version=(2, 0))
    warnings, errors = run_check(data)
    assert any("unsupported major version 2.0" in e for e in errors)


def test_check_minor_version_ignored_by_default():
    data = build_mgic_record() + build_trce_record(trace_version=(1, 9))
    warnings, errors = run_check(data)
    assert errors == []


def test_check_minor_version_error_with_flag():
    data = build_mgic_record() + build_trce_record(trace_version=(1, 9))
    warnings, errors = run_check(data, error_on_minor=True)
    assert any("minor version 9 is higher" in e for e in errors)


def test_check_unknown_record_type_warns():
    unknown = bytearray(build_trce_record())
    unknown[10:14] = b"BLUB"
    data = build_mgic_record() + bytes(unknown)
    warnings, errors = run_check(data)
    assert errors == []
    assert any("Unknown record type b'BLUB'" in w for w in warnings)


def test_check_misaligned_record():
    # Force an unaligned record: MGIC declares alignment power 3, but the TRCE
    # record follows immediately (at a non-8-aligned offset).
    data = build_mgic_record(alignment_power=3) + build_trce_record()
    warnings, errors = run_check(data)
    assert any(
        "declares alignment power 3" in e
        and "only conform to an alignment power of 0" in e
        for e in errors
    )


def test_check_alignment_power_within_common_alignment():
    """A declared power equal to the records' common alignment is accepted."""
    # MGIC (75 bytes) + padding + TRCE at offset 80 → common power 4.
    mgic = build_mgic_record(alignment_power=4)
    data = mgic + b"\x00" * (80 - len(mgic)) + build_trce_record()
    warnings, errors = run_check(data)
    assert not any("alignment power" in e for e in errors)


def test_check_alignment_power_above_common_alignment():
    """A declared power above the records' common alignment is an error."""
    # MGIC (75 bytes) + padding + TRCE at offset 80 → common power 4,
    # but the MGIC declares power 5 (32-byte alignment).
    mgic = build_mgic_record(alignment_power=5)
    data = mgic + b"\x00" * (80 - len(mgic)) + build_trce_record()
    warnings, errors = run_check(data)
    assert any(
        "declares alignment power 5" in e
        and "only conform to an alignment power of 4" in e
        for e in errors
    )
    # The offending records are listed in the error.
    assert any("Records not 2^5-aligned" in e for e in errors)


def test_check_alignment_unconstrained_single_record():
    """A single record at offset 0 conforms to any declared alignment power."""
    data = build_mgic_record(alignment_power=8)
    warnings, errors = run_check(data)
    assert not any("alignment power" in e for e in errors)


def test_check_address_representable():
    """A 1-byte emt_ptr_t with alignment power 0 can address 256 bytes; a
    record at offset 256 is not representable."""
    mgic = build_mgic_record(ptr_size=1, alignment_power=0)
    data = mgic + b"\x00" * (256 - len(mgic)) + build_trce_record()
    warnings, errors = run_check(data)
    assert any(
        "cannot be represented in a 1-byte emt_ptr_t with alignment power 0" in e
        for e in errors
    )


def test_check_address_representable_ok():
    """Records within the emt_ptr_t address space are accepted."""
    mgic = build_mgic_record(ptr_size=1, alignment_power=0)
    trce = build_trce_record()
    # Last record ends well below the 256-byte modulus.
    data = mgic + trce
    warnings, errors = run_check(data)
    assert not any("cannot be represented" in e for e in errors)


def test_check_address_uniqueness_includes_mgic():
    """The check is not triggered by a normal, representable section."""
    data = build_mgic_record(ptr_size=2, alignment_power=0) + build_trce_record()
    warnings, errors = run_check(data)
    assert not any("emt_ptr_t" in e for e in errors)


def test_check_framing_overlap():
    # Make the payload offset point into the record_size piece.
    data = bytearray(build_mgic_record() + build_trce_record(fmt_string="x"))
    trce_start = len(build_mgic_record())
    # framing bytes of the TRCE record: [3]=10 type, [4]=size, [5]=payload,
    # [6]=endianness. Point payload at the record_size piece (offset 14).
    data[trce_start + 5] = 14
    warnings, errors = run_check(bytes(data))
    assert any(
        "pieces overlap" in e or "inside the 14-byte framing" in e for e in errors
    )


def test_check_mgic_bad_encoding():
    data = bytearray(build_mgic_record(encoding_id=5))
    warnings, errors = run_check(bytes(data))
    assert any("unknown encoding_id=5" in e for e in errors)


def test_check_trce_invalid_flag():
    data = build_mgic_record() + build_trce_record(
        fmt_string="{0}", args=[("int", 4, 7, [])]
    )
    warnings, errors = run_check(data)
    assert any("invalid flag=7" in e for e in errors)


def test_check_trce_bad_formatter():
    data = build_mgic_record() + build_trce_record(formatter_id=9)
    warnings, errors = run_check(data)
    assert any("unknown formatter_id=9" in e for e in errors)


# ─────────────────────────────────────────────────────────────────────────────
# _summarize_records
# ─────────────────────────────────────────────────────────────────────────────


def test_summarize_valid_section():
    summary = _summarize_records(aligned_section())
    assert summary.record_counts == {"MGIC": 1, "TRCE": 1}
    assert summary.versions["MGIC"]["1.0"] == 1
    assert summary.versions["TRCE"]["1.0"] == 1
    assert summary.mgic is not None
    assert summary.num_trace_points == 1
    assert summary.size_t_sizes == {8}
    assert summary.byteorders == {"little"}


def test_summarize_common_alignment():
    # Records at offsets 0 and 256 → common alignment power 8.
    mgic = build_mgic_record()
    trce = build_trce_record()
    pad = b"\x00" * (256 - len(mgic))
    summary = _summarize_records(mgic + pad + trce)
    assert summary.common_alignment_power == 8


def test_summarize_common_alignment_single_record_at_zero():
    summary = _summarize_records(build_trce_record())
    assert summary.common_alignment_power is None


# ─────────────────────────────────────────────────────────────────────────────
# Archive / ELF parsing for `inspect`
# ─────────────────────────────────────────────────────────────────────────────


def build_minimal_elf_o(emtrace_content: bytes, sh_addralign: int) -> bytes:
    """Build a minimal 64-bit little-endian ET_REL ELF with one .emtrace section."""
    shstrtab = b"\x00.emtrace\x00.shstrtab\x00"
    name_emtrace = 1
    name_shstrtab = 1 + len(b".emtrace\x00")

    ehdr_size = 64
    shentsize = 64
    emtrace_off = ehdr_size
    padded = emtrace_content + b"\x00" * ((8 - len(emtrace_content) % 8) % 8)
    shstrtab_off = emtrace_off + len(padded)
    shoff = (shstrtab_off + len(shstrtab) + 7) & ~7
    shnum = 3  # null, .emtrace, .shstrtab

    ehdr = struct.pack(
        "<16sHHIQQQIHHHHHH",
        b"\x7fELF\x02\x01\x01\x00" + b"\x00" * 8,
        1,  # e_type = ET_REL
        0x3E,  # e_machine = EM_X86_64
        1,  # e_version
        0,  # e_entry
        0,  # e_phoff
        shoff,  # e_shoff
        0,  # e_flags
        ehdr_size,  # e_ehsize
        0,  # e_phentsize
        0,  # e_phnum
        shentsize,  # e_shentsize
        shnum,  # e_shnum
        2,  # e_shstrndx
    )

    def shdr(
        name: int, stype: int, flags: int, off: int, size: int, align: int
    ) -> bytes:
        return struct.pack(
            "<IIQQQQIIQQ", name, stype, flags, 0, off, size, 0, 0, align, 0
        )

    shdrs = (
        shdr(0, 0, 0, 0, 0, 0)
        + shdr(name_emtrace, 1, 0x2, emtrace_off, len(emtrace_content), sh_addralign)
        + shdr(name_shstrtab, 3, 0, shstrtab_off, len(shstrtab), 1)
    )

    assert shoff >= shstrtab_off + len(shstrtab)
    return (
        ehdr
        + padded
        + shstrtab
        + b"\x00" * (shoff - shstrtab_off - len(shstrtab))
        + shdrs
    )


def ar_member(name: str, content: bytes) -> bytes:
    """Wrap *content* in a GNU-style ar member header."""
    header = bytearray(60)
    header[0:16] = (name + "/").ljust(16).encode("ascii")
    header[48:58] = str(len(content)).ljust(10).encode("ascii")
    header[58:60] = b"`\n"
    pad = b"\n" if len(content) % 2 else b""
    return bytes(header) + content + pad


def build_archive(members: list[tuple[str, bytes]]) -> bytes:
    return b"!<arch>\n" + b"".join(
        ar_member(name, content) for name, content in members
    )


def test_detect_input_type(tmp_path: Path):
    archive = tmp_path / "lib.a"
    _ = archive.write_bytes(build_archive([("a.o", b"\x7fELFgarbage")]))
    elf = tmp_path / "b.o"
    _ = elf.write_bytes(build_minimal_elf_o(b"", 8))
    raw = tmp_path / "c.bin"
    _ = raw.write_bytes(b"\x00" * 32)
    assert _detect_input_type(archive) == "archive"
    assert _detect_input_type(elf) == "elf"
    assert _detect_input_type(raw) == "raw"


def test_parse_archive_gnu(tmp_path: Path):
    archive = tmp_path / "lib.a"
    _ = archive.write_bytes(
        build_archive([("one.o", b"\x01\x02"), ("two.o", b"\x03\x04\x05")])
    )
    members = _parse_archive(archive)
    assert [name for name, _ in members] == ["one.o", "two.o"]
    assert members[0][1] == b"\x01\x02"
    assert members[1][1] == b"\x03\x04\x05"


def test_parse_archive_long_names(tmp_path: Path):
    long_name = "a-very-long-object-file-name.o"
    assert len(long_name) > 15
    names_blob = (long_name + "/\n").encode("ascii")

    # The long-name table member's raw name is exactly "//" (no trailing slash).
    names_header = bytearray(60)
    names_header[0:16] = b"//".ljust(16)
    names_header[48:58] = str(len(names_blob)).ljust(10).encode("ascii")
    names_header[58:60] = b"`\n"

    ref_header = bytearray(60)
    ref_header[0:16] = b"/0".ljust(16)
    ref_header[48:58] = str(len(b"data")).ljust(10).encode("ascii")
    ref_header[58:60] = b"`\n"

    data = (
        b"!<arch>\n"
        + bytes(names_header)
        + names_blob
        + (b"\n" if len(names_blob) % 2 else b"")
        + bytes(ref_header)
        + b"data"
        + b"\n"  # odd-size padding
    )
    archive = tmp_path / "lib.a"
    _ = archive.write_bytes(data)
    members = _parse_archive(archive)
    assert members == [(long_name, b"data")]


def test_parse_archive_symbol_table_skipped(tmp_path: Path):
    data = (
        b"!<arch>\n"
        + ar_member("/", b"\x00\x00\x00\x00symdata")
        + ar_member("real.o", b"\x01")
    )
    archive = tmp_path / "lib.a"
    _ = archive.write_bytes(data)
    members = _parse_archive(archive)
    assert members == [("real.o", b"\x01")]


lief = pytest.importorskip("lief", reason="member ELF parsing needs lief")


def test_parse_member_elf_minimal():
    elf = build_minimal_elf_o(b"section-bytes", sh_addralign=256)
    binary = _parse_member_elf(elf)
    assert binary is not None
    sections = {s.name: s for s in binary.sections}
    assert ".emtrace" in sections
    section = sections[".emtrace"]
    assert bytes(section.content) == b"section-bytes"
    assert int(section.alignment) == 256


def test_parse_member_elf_non_elf():
    assert _parse_member_elf(b"garbage") is None


def test_inspect_archive_summary(tmp_path: Path, capsys: pytest.CaptureFixture[str]):
    """Full inspect flow over an archive with differently aligned members."""
    mgic_rec = build_mgic_record()
    trce_rec = build_trce_record(fmt_string="{0}", args=[("int", 4, 0, [])])
    # Records at offsets {0, 256} → common record alignment power 8,
    # section alignment 256 (power 8) → effective power 8.
    section_a = mgic_rec + b"\x00" * ((256 - len(mgic_rec) % 256) % 256) + trce_rec
    # Records at offsets {0, 80} → common record alignment power 4,
    # section alignment 8 (power 3) → effective power 3 (section binds).
    section_b = mgic_rec + b"\x00" * (80 - len(mgic_rec)) + trce_rec

    archive = tmp_path / "libmix.a"
    _ = archive.write_bytes(
        build_archive(
            [
                ("aligned.o", build_minimal_elf_o(section_a, sh_addralign=256)),
                ("loose.o", build_minimal_elf_o(section_b, sh_addralign=8)),
            ]
        )
    )

    assert (
        inspector.cmd_inspect(
            type("Args", (), {"input": str(archive), "section_name": ".emtrace"})()
        )
        == 0
    )
    out = capsys.readouterr().out
    assert "type:             archive" in out
    assert "aligned.o" in out
    assert "loose.o" in out
    # The record-level alignment of loose.o (power 4) and its section
    # alignment (power 3) cap the overall achievable alignment.
    assert "highest possible alignment power: 3 (constrained by: loose.o)" in out


def test_inspect_archive_record_constrained(
    tmp_path: Path, capsys: pytest.CaptureFixture[str]
):
    """A member's *records* can cap the alignment despite a well-aligned section."""
    mgic_rec = build_mgic_record()
    trce_rec = build_trce_record(fmt_string="{0}", args=[("int", 4, 0, [])])
    # Records at offsets {0, 84}: gcd 84 = 4·21 → common record alignment
    # power 2, while the section itself is 256-aligned (power 8) → effective 2.
    section_wide = mgic_rec + b"\x00" * (84 - len(mgic_rec)) + trce_rec
    # Records at offsets {0, 80} → power 4, section alignment 8 → effective 3.
    section_loose = mgic_rec + b"\x00" * (80 - len(mgic_rec)) + trce_rec

    archive = tmp_path / "lib.a"
    _ = archive.write_bytes(
        build_archive(
            [
                ("wide_records.o", build_minimal_elf_o(section_wide, sh_addralign=256)),
                ("loose.o", build_minimal_elf_o(section_loose, sh_addralign=8)),
            ]
        )
    )

    assert (
        inspector.cmd_inspect(
            type("Args", (), {"input": str(archive), "section_name": ".emtrace"})()
        )
        == 0
    )
    out = capsys.readouterr().out
    # Despite the 256-aligned section, wide_records.o's records only conform
    # to power 2 — and that is what caps the archive.
    assert "highest possible alignment power: 2 (constrained by: wide_records.o)" in out


def test_inspect_archive_unconstrained_member(
    tmp_path: Path, capsys: pytest.CaptureFixture[str]
):
    """A member with a single record at offset 0 does not constrain alignment."""
    mgic_rec = build_mgic_record()
    trce_rec = build_trce_record(fmt_string="{0}", args=[("int", 4, 0, [])])
    # Single record at offset 0 → unconstrained → effective = section power 8.
    section_magic_only = mgic_rec
    # Records at offsets {0, 80} → power 4, section alignment 8 → effective 3.
    section_loose = mgic_rec + b"\x00" * (80 - len(mgic_rec)) + trce_rec

    archive = tmp_path / "lib.a"
    _ = archive.write_bytes(
        build_archive(
            [
                (
                    "magic_only.o",
                    build_minimal_elf_o(section_magic_only, sh_addralign=256),
                ),
                ("loose.o", build_minimal_elf_o(section_loose, sh_addralign=8)),
            ]
        )
    )

    assert (
        inspector.cmd_inspect(
            type("Args", (), {"input": str(archive), "section_name": ".emtrace"})()
        )
        == 0
    )
    out = capsys.readouterr().out
    assert "highest possible alignment power: 3 (constrained by: loose.o)" in out
    # The unconstrained member's record alignment is reported as "-".
    lines = [
        line for line in out.splitlines() if line.strip().startswith("magic_only.o")
    ]
    assert len(lines) == 1
    assert " - " in lines[0]
