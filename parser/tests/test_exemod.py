"""Unit tests for emtrace.exemod (LIEF-backed strip / nonload surgery).

The guarantees are behavioral, not byte-exact (LIEF fully rebuilds the
binary): the section entry is gone, bound symbols are deleted, the content no
longer parses, neighbors survive, and refusal rules hold.
"""

from __future__ import annotations

import struct
from pathlib import Path
from types import SimpleNamespace
from typing import Any

import pytest
from emtrace import edit
from emtrace.exemod import ExeError, ExeImage, SectionNotFoundError
from emtrace.record_parser import RecordParser

from .test_record_parser import build_mgic_record, build_trce_record

lief = pytest.importorskip("lief")

_PARAMS = [(True, True), (False, True)]
_SYM_PARAMS = [(True, True), (False, True)]
# NOTE: only little-endian fixtures are exercised beyond the refusal tests.
# The previous hand-rolled
# surgery covered big-endian ELF as well, but LIEF 0.17 misparses big-endian
# headers (it reads e_type/e_machine byte-swapped: "768 is not a valid
# FILE_TYPE"), so BE binaries are refused explicitly (see
# test_open_refuses_big_endian).

ET_REL = 1
ET_DYN = 3
SHF_ALLOC = 0x2
SHT_NOBITS = 8


def _write(tmp_path: Path, name: str, data: bytes) -> Path:
    path = tmp_path / name
    _ = path.write_bytes(data)
    return path


# ── ELF fixtures (raw bytes, as before) ────────────────────────────────────


def build_test_exe(
    *,
    is64: bool = True,
    little: bool = True,
    e_type: int = ET_DYN,
    emt_type: int = 1,
) -> bytes:
    """Build a minimal but structurally complete linked ELF.

    Layout: ehdr | phdr table (PT_PHDR, LOAD R covering header+records,
    LOAD RW for .data with a bss-like memsz tail) | .emtrace records |
    padding | .data | .shstrtab | section header table.
    """
    end = "<" if little else ">"
    cls = 2 if is64 else 1
    ehsize = 64 if is64 else 52
    phentsize = 56 if is64 else 32
    shentsize = 64 if is64 else 40
    phnum = 3
    shnum = 4

    emt_off = ehsize + phnum * phentsize
    byteorder = "little" if little else "big"
    records = build_mgic_record(byteorder=byteorder) + build_trce_record(
        byteorder=byteorder
    )
    data_off = emt_off + len(records) + 8
    data = b"\x11" * 16
    strtab = b"\x00.emtrace\x00.data\x00.shstrtab\x00"
    strtab_off = data_off + len(data)
    shoff = strtab_off + len(strtab)
    emt_end = emt_off + len(records)

    buf = bytearray(shoff + shnum * shentsize)
    buf[emt_off:emt_end] = records
    ident = b"\x7fELF" + bytes([cls, 1 if little else 2, 1, 0]) + bytes(8)
    efmt = "16sHHIQQQIHHHHHH" if is64 else "16sHHIIIIIHHHHHH"
    struct.pack_into(
        end + efmt,
        buf,
        0,
        ident,
        e_type,
        0x3E if is64 else 0x03,
        1,
        0x1000,
        ehsize,
        shoff,
        0,
        ehsize,
        phentsize,
        phnum,
        shentsize,
        shnum,
        3,
    )

    phfmt = "IIQQQQQQ" if is64 else "IIIIIIII"

    def phdr(
        index: int,
        p_type: int,
        p_flags: int,
        off: int,
        vaddr: int,
        filesz: int,
        memsz: int,
        align: int,
    ) -> None:
        values: tuple[int, ...] = (
            (p_type, p_flags, off, vaddr, vaddr, filesz, memsz, align)
            if is64
            else (p_type, off, vaddr, vaddr, filesz, memsz, p_flags, align)
        )
        struct.pack_into(end + phfmt, buf, ehsize + index * phentsize, *values)

    phdr(0, 6, 4, ehsize, ehsize, phnum * phentsize, phnum * phentsize, 8)  # PT_PHDR
    phdr(1, 1, 4, 0, 0, emt_end, emt_end, 0x1000)  # LOAD covering header + records
    phdr(2, 1, 6, data_off, data_off + 0x1000, len(data), len(data) + 0x30, 0x1000)

    _ = buf[data_off : data_off + len(data)]
    buf[data_off : data_off + len(data)] = data
    _ = buf[strtab_off : strtab_off + len(strtab)]
    buf[strtab_off : strtab_off + len(strtab)] = strtab

    shfmt = "IIQQQQIIQQ" if is64 else "IIIIIIIIII"

    def shdr(
        index: int,
        name: int,
        typ: int,
        flags: int,
        addr: int,
        off: int,
        size: int,
        addralign: int,
    ):
        struct.pack_into(
            end + shfmt,
            buf,
            shoff + index * shentsize,
            *(name, typ, flags, addr, off, size, 0, 0, addralign, 0),
        )

    shdr(0, 0, 0, 0, 0, 0, 0, 0)  # NULL
    shdr(1, 1, emt_type, SHF_ALLOC, emt_off, emt_off, len(records), 256)  # .emtrace
    shdr(2, 10, 1, 3, data_off + 0x1000, data_off, len(data), 8)  # .data (W | A)
    shdr(3, 16, 3, 0, 0, strtab_off, len(strtab), 1)  # .shstrtab
    return bytes(buf)


def _pack_sym(end: str, is64: bool, name: int, info: int, shndx: int, value: int):
    if is64:
        return struct.pack(end + "IBBHQQ", name, info, 0, shndx, value, 8)
    return struct.pack(end + "IIIBBH", name, value, 8, info, 0, shndx)


def build_symtab_exe(
    *,
    is64: bool = True,
    little: bool = True,
    bad_rela: bool = False,
) -> bytes:
    """Linked ELF with a symbol table and relocations for strip tests.

    Sections: NULL, .emtrace (records, ALLOC), .data, .rela.test (two
    entries referencing kept symbols, or one referencing a deleted symbol
    when *bad_rela*), .shstrtab, .symtab (NULL + local->.emtrace +
    local->.data + global->.data), .strtab.
    """
    end = "<" if little else ">"
    cls = 2 if is64 else 1
    ehsize = 64 if is64 else 52
    phentsize = 56 if is64 else 32
    shentsize = 64 if is64 else 40
    symsize = 24 if is64 else 16
    relasize = 24 if is64 else 12

    emt_off = ehsize + 3 * phentsize
    byteorder = "little" if little else "big"
    records = build_mgic_record(byteorder=byteorder) + build_trce_record(
        byteorder=byteorder
    )
    data_off = emt_off + len(records) + 8
    data = b"\x22" * 16
    rela_off = data_off + len(data)
    rela_entries = [(2, 1), (3, 1)] if not bad_rela else [(1, 1)]
    rela = b"".join(
        struct.pack(
            end + ("QQ" if is64 else "II") + ("Q" if is64 else "I"),
            0x5000 + 8 * k,
            (sym << (32 if is64 else 8)) | r_type,
            *([0] if True else []),
        )
        for k, (sym, r_type) in enumerate(rela_entries)
    )
    strtab = b"\x00emt_rec\x00data_var\x00global_fn\x00"
    shstrtab = (
        b"\x00.emtrace\x00.data\x00.rela.test\x00.shstrtab\x00.symtab\x00.strtab\x00"
    )
    shstrtab_off = rela_off + len(rela)
    symtab_off = shstrtab_off + len(shstrtab)
    symtab = b"".join(
        _pack_sym(end, is64, name, info, shndx, value)
        for name, info, shndx, value in [
            (0, 0x00, 0, 0),
            (1, 0x01, 1, emt_off),
            (9, 0x01, 2, data_off + 0x1000),
            (18, 0x12, 2, data_off + 0x1000),
        ]
    )
    strtab_off = symtab_off + len(symtab)
    shoff = strtab_off + len(strtab)
    emt_end = emt_off + len(records)

    buf = bytearray(shoff + 7 * shentsize)
    buf[emt_off:emt_end] = records
    buf[data_off : data_off + len(data)] = data
    buf[rela_off : rela_off + len(rela)] = rela
    buf[shstrtab_off : shstrtab_off + len(shstrtab)] = shstrtab
    buf[symtab_off : symtab_off + len(symtab)] = symtab
    buf[strtab_off : strtab_off + len(strtab)] = strtab

    ident = b"\x7fELF" + bytes([cls, 1 if little else 2, 1, 0]) + bytes(8)
    efmt = "16sHHIQQQIHHHHHH" if is64 else "16sHHIIIIIHHHHHH"
    struct.pack_into(
        end + efmt,
        buf,
        0,
        ident,
        ET_DYN,
        0x3E if is64 else 0x03,
        1,
        0x1000,
        ehsize,
        shoff,
        0,
        ehsize,
        phentsize,
        3,
        shentsize,
        7,
        4,
    )

    phfmt = "IIQQQQQQ" if is64 else "IIIIIIII"

    def phdr(
        index: int,
        p_type: int,
        p_flags: int,
        off: int,
        vaddr: int,
        filesz: int,
        memsz: int,
    ) -> None:
        values = (
            (p_type, p_flags, off, vaddr, vaddr, filesz, memsz, 0x1000)
            if is64
            else (p_type, off, vaddr, vaddr, filesz, memsz, p_flags, 0x1000)
        )
        struct.pack_into(end + phfmt, buf, ehsize + index * phentsize, *values)

    phdr(0, 6, 4, ehsize, ehsize, 3 * phentsize, 3 * phentsize)
    phdr(1, 1, 4, 0, 0, emt_end, emt_end)
    phdr(
        2,
        1,
        6,
        data_off,
        data_off + 0x1000,
        len(rela) + len(data),
        len(rela) + len(data),
    )

    shfmt = "IIQQQQIIQQ" if is64 else "IIIIIIIIII"

    def shdr(
        index: int,
        name: int,
        typ: int,
        flags: int,
        addr: int,
        off: int,
        size: int,
        link: int,
        info: int,
        addralign: int,
        entsize: int = 0,
    ) -> None:
        struct.pack_into(
            end + shfmt,
            buf,
            shoff + index * shentsize,
            *(name, typ, flags, addr, off, size, link, info, addralign, entsize),
        )

    shdr(0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
    shdr(1, 1, 1, SHF_ALLOC, emt_off, emt_off, len(records), 0, 0, 256)
    shdr(2, 10, 1, 3, data_off + 0x1000, data_off, len(data), 0, 0, 8)
    shdr(3, 16, 4, 0, 0, rela_off, len(rela), 5, 2, 8, relasize)
    shdr(4, 27, 3, 0, 0, shstrtab_off, len(shstrtab), 0, 0, 1)
    shdr(5, 37, 2, 0, 0, symtab_off, len(symtab), 6, 3, 8, symsize)
    shdr(6, 45, 3, 0, 0, strtab_off, len(strtab), 0, 0, 1)
    return bytes(buf)


# ── Big-endian ELF fixture ───────────────────────────────────────────────


def build_be_s390_exe() -> bytes:
    """Minimal big-endian ELF64 with a consistent machine/data pair (EM_S390).

    Unlike a big-endian x86-64 file (which is self-contradictory, since
    x86-64 is little-endian-only), this is shaped like binaries found in the
    wild, so LIEF's architecture-based endianness detection handles it.
    Layout: ehdr | .emtrace records | .shstrtab | section header table.
    """
    end = ">"
    emt_off = 64
    records = build_mgic_record(byteorder="big") + build_trce_record(byteorder="big")
    strtab = b"\x00.emtrace\x00.shstrtab\x00"
    strtab_off = emt_off + len(records)
    shoff = strtab_off + len(strtab)

    buf = bytearray(shoff + 3 * 64)
    buf[emt_off : emt_off + len(records)] = records
    buf[strtab_off : strtab_off + len(strtab)] = strtab

    ident = b"\x7fELF" + bytes([2, 2, 1, 0]) + bytes(8)
    struct.pack_into(
        end + "16sHHIQQQIHHHHHH",
        buf,
        0,
        ident,
        ET_DYN,
        0x16,  # EM_S390: big-endian-native, unlike EM_X86_64
        1,
        0,
        0,
        shoff,
        0,
        64,
        0,
        0,
        64,
        3,
        2,
    )

    def shdr(index: int, name: int, typ: int, flags: int, size: int, off: int):
        struct.pack_into(
            end + "IIQQQQIIQQ",
            buf,
            shoff + index * 64,
            *(name, typ, flags, off, off, size, 0, 0, 1, 0),
        )

    shdr(0, 0, 0, 0, 0, 0)  # NULL
    shdr(1, 1, 1, SHF_ALLOC, len(records), emt_off)  # .emtrace
    shdr(2, 10, 3, 0, len(strtab), strtab_off)  # .shstrtab
    return bytes(buf)


# ── PE fixture ─────────────────────────────────────────────────────────────


def build_pe_exe() -> bytes:
    """Minimal PE32+ image (AMD64) with .text and .emtrace sections."""
    """Minimal PE32+ image (AMD64) with .text and .emtrace sections."""
    file_align = 0x200
    marker = b"TRACE_MARKER_" * 32
    eraw = (len(marker) + file_align - 1) // file_align * file_align

    dos = bytearray(0x80)
    dos[0:2] = b"MZ"
    struct.pack_into("<I", dos, 0x3C, 0x80)
    out = bytearray(dos) + b"PE\0\0"
    out += struct.pack("<HHIIIHH", 0x8664, 2, 0, 0, 0, 240, 0x0022)
    opt = bytearray(240)
    struct.pack_into(
        "<HBBIIIIIQIIHHHHHHIIIHHQQQQII",
        opt,
        0,
        0x20B,
        14,
        0,
        0x200,
        eraw,
        0,
        0x1000,
        0x1000,
        0x140000000,
        0x1000,
        file_align,
        6,
        0,
        0,
        6,
        0,
        0,
        0x3000,
        0x200,
        0,
        3,
        0,
        0x100000,
        0x1000,
        0x100000,
        0x1000,
        0,
        16,
    )
    out += opt

    def sh(
        name: bytes, vsz: int, va: int, rawsz: int, rawptr: int, chars: int
    ) -> bytes:
        return struct.pack(
            "<8sIIIIIIHHI", name, vsz, va, rawsz, rawptr, 0, 0, 0, 0, chars
        )

    out += sh(b".text", 0x10, 0x1000, 0x200, 0x200, 0x60000020)
    out += sh(b".emtrace", len(marker), 0x2000, eraw, 0x400, 0x40000040)
    out += bytes(0x200 - len(out))
    out += b"\xc3" + bytes(0x200 - 1)
    out += marker + bytes(eraw - len(marker))
    return bytes(out)


# ── Mach-O fixture ─────────────────────────────────────────────────────────


def build_macho_exe(*, file_type: int = 2) -> bytes:
    """Minimal thin Mach-O 64-bit executable with a __DATA/.emtrace section
    and one local symbol pointing into it."""
    sec_addr = 0x100001000
    marker = b"MACHO_MARKER_" * 32
    symname = b"_local_emtrace_sym"
    strtab = b"\x00" + symname + b"\x00"
    seg_cmdsize = 72 + 80
    sizeofcmds = seg_cmdsize + 24
    # On-disk magic CF FA ED FE reads as MH_MAGIC_64 on a little-endian
    # host (little-endian file); the byte-swapped FE ED FA CF form is
    # MH_CIGAM_64 (big-endian). The remaining header fields are little-endian.
    hdr = struct.pack("<8I", 0xFEEDFACF, 0x01000007, 3, file_type, 2, sizeofcmds, 0, 0)
    seg = struct.pack("<II", 0x19, seg_cmdsize)
    seg += struct.pack(
        "<16sQQQQiiII", b"__DATA", sec_addr, len(marker), 0, 0, 3, 3, 1, 0
    )
    secoff = 32 + sizeofcmds
    seg += struct.pack(
        "<16s16sQQIIIIIIII",
        b".emtrace",
        b"__DATA",
        sec_addr,
        len(marker),
        secoff,
        3,
        0,
        0,
        0,
        0,
        0,
        0,
    )
    symoff = secoff + len(marker)
    stroff = symoff + 16
    symtab = struct.pack("<IIIIII", 0x2, 24, symoff, 1, stroff, len(strtab))
    out = bytearray(hdr + seg + symtab)
    assert len(out) == secoff
    out += marker
    out += struct.pack("<IBBHQ", 1, 0x0E, 1, 0, sec_addr + 8)
    out += strtab
    return bytes(out)


# ── open / detection ───────────────────────────────────────────────────────


def test_open_detects_format(tmp_path: Path) -> None:
    elf = ExeImage.open(_write(tmp_path, "a.so", build_test_exe()))
    assert elf.format == "elf"
    assert ".emtrace" in elf.section_names()

    pe = ExeImage.open(_write(tmp_path, "b.exe", build_pe_exe()))
    assert pe.format == "pe"
    assert ".emtrace" in pe.section_names()

    macho = ExeImage.open(_write(tmp_path, "c.macho", build_macho_exe()))
    assert macho.format == "macho"
    assert ".emtrace" in macho.section_names()


def test_open_refuses_garbage_and_missing(tmp_path: Path) -> None:
    blob = _write(tmp_path, "blob.bin", b"not an executable" * 32)
    with pytest.raises(ExeError):
        _ = ExeImage.open(blob)
    with pytest.raises(ExeError):
        _ = ExeImage.open(tmp_path / "does-not-exist")


def test_open_refuses_unlinked(tmp_path: Path) -> None:
    obj = _write(tmp_path, "obj.o", build_test_exe(e_type=ET_REL))
    with pytest.raises(ExeError, match="[Oo]bject"):
        _ = ExeImage.open(obj)
    macho_obj = _write(tmp_path, "obj.macho", build_macho_exe(file_type=1))
    with pytest.raises(ExeError, match="[Oo]bject"):
        _ = ExeImage.open(macho_obj)


def test_open_refuses_big_endian(tmp_path: Path) -> None:
    be_elf = _write(tmp_path, "be.so", build_test_exe(little=False))
    with pytest.raises(ExeError, match="[Bb]ig-endian"):
        _ = ExeImage.open(be_elf)

    be_macho = bytearray(build_macho_exe())
    be_macho[:4] = b"\xfe\xed\xfa\xcf"  # MH_CIGAM_64: byte-swapped magic
    with pytest.raises(ExeError, match="[Bb]ig-endian"):
        _ = ExeImage.open(_write(tmp_path, "be.macho", bytes(be_macho)))


def test_experimental_be_allows_genuine_be_elf(tmp_path: Path) -> None:
    data = build_be_s390_exe()
    path = _write(tmp_path, "be-s390.so", data)
    with pytest.raises(ExeError, match="[Bb]ig-endian"):
        _ = ExeImage.open(path)

    image = ExeImage.open(path, experimental_be=True)
    assert image.format == "elf"
    assert ".emtrace" in image.section_names()

    report = image.strip_section(".emtrace")
    assert report.removed_symbols == 0
    out = image.bytes()
    assert b"MGIC" not in out
    assert _reparse(image).get_section(".emtrace") is None


def test_open_requires_lief(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    blob = _write(tmp_path, "a.so", build_test_exe())
    monkeypatch.setattr("emtrace.exemod.HAS_LIEF", False)
    with pytest.raises(ExeError, match="lief"):
        _ = ExeImage.open(blob)


def test_missing_section(tmp_path: Path) -> None:
    image = ExeImage.open(_write(tmp_path, "a.so", build_test_exe()))
    with pytest.raises(SectionNotFoundError):
        _ = image.strip_section(".nope")
    with pytest.raises(SectionNotFoundError):
        _ = image.make_nonload(".nope")


# ── nonload (ELF) ──────────────────────────────────────────────────────────


@pytest.mark.parametrize(("is64", "little"), _PARAMS)
def test_nonload_clears_alloc_and_addr(
    tmp_path: Path, is64: bool, little: bool
) -> None:
    image = ExeImage.open(
        _write(tmp_path, "a.so", build_test_exe(is64=is64, little=little))
    )
    report = image.make_nonload(".emtrace")
    assert report.changed
    assert report.old_flags & SHF_ALLOC
    assert report.old_addr != 0

    reparsed_image = _reparse(image)
    sec = reparsed_image.get_section(".emtrace")
    assert not sec.has(lief.ELF.Section.FLAGS.ALLOC)
    assert int(sec.virtual_address) == 0

    content = bytes(sec.content)
    assert RecordParser(memoryview(content)).find_and_parse_mgic() is not None

    assert not image.make_nonload(".emtrace").changed


def _reparse(image: ExeImage) -> Any:
    """Re-parse the current bytes of *image* with LIEF (ELF only)."""
    import tempfile

    with tempfile.NamedTemporaryFile(suffix=".so", delete=False) as f:
        _ = f.write(image.bytes())
        tmp = f.name
    try:
        parsed = lief.ELF.parse(tmp)
        assert parsed is not None
        return parsed
    finally:
        Path(tmp).unlink()


def test_nonload_unsupported_on_pe_and_macho(tmp_path: Path) -> None:
    pe = ExeImage.open(_write(tmp_path, "b.exe", build_pe_exe()))
    with pytest.raises(ExeError, match="only implemented for ELF"):
        _ = pe.make_nonload(".emtrace")
    macho = ExeImage.open(_write(tmp_path, "c.macho", build_macho_exe()))
    with pytest.raises(ExeError, match="only implemented for ELF"):
        _ = macho.make_nonload(".emtrace")


# ── strip (ELF) ────────────────────────────────────────────────────────────


@pytest.mark.parametrize(("is64", "little"), _PARAMS)
def test_strip_removes_section_and_content(
    tmp_path: Path, is64: bool, little: bool
) -> None:
    image = ExeImage.open(
        _write(tmp_path, "a.so", build_test_exe(is64=is64, little=little))
    )
    report = image.strip_section(".emtrace")
    assert report.removed_symbols == 0

    out = image.bytes()
    assert b"MGIC" not in out
    reparsed = _reparse(image)
    assert reparsed.get_section(".emtrace") is None

    # Neighbors survive with their content.
    data = reparsed.get_section(".data")
    assert bytes(data.content) == b"\x11" * 16


def test_strip_nobits_keeps_working(tmp_path: Path) -> None:
    image = ExeImage.open(_write(tmp_path, "a.so", build_test_exe(emt_type=SHT_NOBITS)))
    _ = image.strip_section(".emtrace")
    assert _reparse(image).get_section(".emtrace") is None


@pytest.mark.parametrize(("is64", "little"), _SYM_PARAMS)
def test_strip_deletes_bound_symbols(tmp_path: Path, is64: bool, little: bool) -> None:
    image = ExeImage.open(
        _write(tmp_path, "a.so", build_symtab_exe(is64=is64, little=little))
    )
    report = image.strip_section(".emtrace")
    assert report.removed_symbols == 1

    reparsed = _reparse(image)
    assert reparsed.get_section(".emtrace") is None
    names = sorted(symbol.name for symbol in reparsed.symbols)
    assert names == ["", "data_var", "global_fn"]

    data = reparsed.get_section(".data")
    assert bytes(data.content) == b"\x22" * 16


@pytest.mark.parametrize(("is64", "little"), _SYM_PARAMS)
def test_strip_refuses_reloc_to_deleted_symbol(
    tmp_path: Path, is64: bool, little: bool
) -> None:
    image = ExeImage.open(
        _write(
            tmp_path, "a.so", build_symtab_exe(is64=is64, little=little, bad_rela=True)
        )
    )
    with pytest.raises(ExeError, match="relocation"):
        _ = image.strip_section(".emtrace")


def test_strip_refuses_dynsym_reference() -> None:
    from emtrace import elf_backend

    target = _mock_section("target", index=3)
    binary = _mock_binary(
        symbols=[_mock_sym("local", 3)],
        dynamic_symbols=[_mock_sym("dyn_ref", 3)],
    )
    with pytest.raises(ExeError, match="[Dd]ynamic symbol"):
        _ = elf_backend.check_strippable(binary, target)


def test_strip_refuses_reloc_in_range() -> None:
    from emtrace import elf_backend

    target = _mock_section("target", index=3, va=0x1000, size=64)
    binary = _mock_binary(relocs=[SimpleNamespace(address=0x1020)])
    with pytest.raises(ExeError, match="relocation"):
        _ = elf_backend.check_strippable(binary, target)


def test_strip_refuses_reloc_to_deleted_symbol_mock() -> None:
    from emtrace import elf_backend

    target = _mock_section("target", index=3)
    bound = _mock_sym("doomed", 3)
    reloc = SimpleNamespace(address=0x9999, symbol=_mock_sym("doomed", 3))
    binary = _mock_binary(symbols=[bound], relocs=[reloc])
    with pytest.raises(ExeError, match="relocation"):
        _ = elf_backend.check_strippable(binary, target)


def test_check_strippable_returns_bound_symbols() -> None:
    from emtrace import elf_backend

    target = _mock_section("target", index=3)
    bound = _mock_sym("local", 3)
    other = _mock_sym("elsewhere", 5)
    binary = _mock_binary(symbols=[bound, other])
    assert elf_backend.check_strippable(binary, target) == [bound]


def _mock_section(
    name: str, index: int = 3, va: int = 0x1000, size: int = 64
) -> SimpleNamespace:
    section = SimpleNamespace(
        name=name,
        type=lief.ELF.Section.TYPE.PROGBITS,
        virtual_address=va,
        size=size,
    )
    _ = index
    return section


def _mock_sym(name: str, shndx: int) -> SimpleNamespace:
    return SimpleNamespace(name=name, shndx=shndx)


def _mock_binary(
    symbols: list[SimpleNamespace] | None = None,
    dynamic_symbols: list[SimpleNamespace] | None = None,
    relocs: list[SimpleNamespace] | None = None,
) -> SimpleNamespace:
    binary = SimpleNamespace(
        symbols=symbols or [],
        dynamic_symbols=dynamic_symbols or [],
        relocations=relocs or [],
    )
    binary.get_section_idx = lambda section: 3  # pyright: ignore[reportUnknownLambdaType]
    return binary


def test_strip_refuses_metadata_sections(tmp_path: Path) -> None:
    image = ExeImage.open(_write(tmp_path, "a.so", build_symtab_exe()))
    with pytest.raises(ExeError):
        _ = image.strip_section(".shstrtab")
    with pytest.raises(ExeError):
        _ = image.strip_section(".symtab")


# ── strip (PE / Mach-O) ────────────────────────────────────────────────────


def test_strip_pe(tmp_path: Path) -> None:
    image = ExeImage.open(_write(tmp_path, "b.exe", build_pe_exe()))
    report = image.strip_section(".emtrace")
    assert report.removed_symbols == 0

    out = image.bytes()
    assert b"TRACE_MARKER_" not in out
    reparsed = lief.PE.parse(str(_write(tmp_path, "b2.exe", out)))
    assert reparsed is not None
    assert reparsed.get_section(".emtrace") is None
    assert reparsed.get_section(".text") is not None


def test_strip_macho(tmp_path: Path) -> None:
    image = ExeImage.open(_write(tmp_path, "c.macho", build_macho_exe()))
    report = image.strip_section(".emtrace")
    assert report.removed_symbols == 1

    out = image.bytes()
    assert b"MACHO_MARKER_" not in out
    reparsed = lief.MachO.parse(str(_write(tmp_path, "c2.macho", out)))
    assert reparsed is not None
    macho = reparsed.at(0)
    assert [section.name for section in macho.sections] == []
    assert [symbol.name for symbol in macho.symbols] == []


# ── edit wiring ────────────────────────────────────────────────────────


def test_edit_refuses_object_files(tmp_path: Path) -> None:
    obj = _write(tmp_path, "obj.o", build_test_exe(e_type=ET_REL))
    nonload = edit.nonload_binary(obj, ".emtrace")
    assert not nonload.ok
    assert nonload.data is None
    strip = edit.strip_binary(obj, ".emtrace")
    assert not strip.ok
    assert strip.data is None


def test_edit_refuses_non_exe(tmp_path: Path) -> None:
    plain = _write(tmp_path, "blob.bin", b"not an executable" * 32)
    assert not edit.nonload_binary(plain, ".emtrace").ok
    assert not edit.strip_binary(plain, ".emtrace").ok


def test_edit_refuses_missing_file(tmp_path: Path) -> None:
    missing = tmp_path / "nope"
    nonload = edit.nonload_binary(missing, ".emtrace")
    assert not nonload.ok
    assert "does not exist" in nonload.errors[0].message


def test_edit_missing_section(tmp_path: Path) -> None:
    exe = _write(tmp_path, "exe", build_test_exe())
    nonload = edit.nonload_binary(exe, ".nope")
    assert not nonload.ok
    strip = edit.strip_binary(exe, ".nope")
    assert not strip.ok


def test_edit_nonload_roundtrip(tmp_path: Path) -> None:
    exe = _write(tmp_path, "exe", build_test_exe())
    report = edit.nonload_binary(exe, ".emtrace")
    assert report.ok and report.changed
    assert report.data is not None

    out = _write(tmp_path, "out", report.data)
    image = ExeImage.open(out)
    assert ".emtrace" in image.section_names()

    again = edit.nonload_binary(out, ".emtrace")
    assert again.ok and not again.changed
    assert again.data is None


def test_edit_strip_roundtrip(tmp_path: Path) -> None:
    exe = _write(tmp_path, "exe", build_test_exe())

    report = edit.strip_binary(exe, ".emtrace")
    assert report.ok
    assert report.data is not None
    assert b"MGIC" not in report.data

    out = _write(tmp_path, "out", report.data)
    assert not ExeImage.open(out).has_section(".emtrace")
    assert exe.read_bytes() != report.data  # the input file itself is untouched
