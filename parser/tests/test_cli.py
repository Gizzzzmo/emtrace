"""Unit tests for emtrace.cli (rendering, exit codes, lief-missing errors).

Tests that do not need lief run in every environment; tests that build or
parse real executables skip themselves when lief is unavailable.
"""

from __future__ import annotations

import json
from pathlib import Path
from types import SimpleNamespace

import pytest

from emtrace.cli import edit_cmds, read_cmds, run_cmd

from .test_inspector import aligned_section, build_archive, build_minimal_elf_o


def _check_args(input: str, file_format: str = "binary") -> SimpleNamespace:
    return SimpleNamespace(
        input=input,
        format=file_format,
        section_name=".emtrace",
        error_on_minor=False,
    )


def test_check_raw_bytes_passes(
    tmp_path: Path, capsys: pytest.CaptureFixture[str]
) -> None:
    section = tmp_path / "section.bin"
    _ = section.write_bytes(aligned_section())
    assert read_cmds.cmd_check(_check_args(str(section))) == 0
    assert "check passed." in capsys.readouterr().err


def test_check_raw_bytes_fails(
    tmp_path: Path, capsys: pytest.CaptureFixture[str]
) -> None:
    section = tmp_path / "section.bin"
    _ = section.write_bytes(b"garbage" * 64)
    assert read_cmds.cmd_check(_check_args(str(section))) == 1
    assert "error" in capsys.readouterr().err


def test_export_stdout(tmp_path: Path, capsys: pytest.CaptureFixture[str]) -> None:
    section = tmp_path / "section.bin"
    _ = section.write_bytes(aligned_section())
    args = SimpleNamespace(
        input=str(section),
        format="binary",
        section_name=".emtrace",
        output="-",
        debug_script=False,
    )
    assert read_cmds.cmd_export(args) == 0
    out = capsys.readouterr().out
    assert json.loads(out)["trace_points"]


def test_inspect_raw_bytes(tmp_path: Path, capsys: pytest.CaptureFixture[str]) -> None:
    section = tmp_path / "section.bin"
    _ = section.write_bytes(aligned_section())
    args = SimpleNamespace(input=str(section), section_name=".emtrace")
    assert read_cmds.cmd_inspect(args) == 0
    out = capsys.readouterr().out
    assert "type:             raw" in out
    assert "note:             input is neither an archive" in out


def test_run_check_available() -> None:
    assert run_cmd.cmd_run(SimpleNamespace(check_available=True)) == 0


def test_check_exe_format_requires_lief(
    tmp_path: Path,
    capsys: pytest.CaptureFixture[str],
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    monkeypatch.setattr("emtrace._lief.HAS_LIEF", False)
    section = tmp_path / "section.bin"
    _ = section.write_bytes(aligned_section())
    assert read_cmds.cmd_check(_check_args(str(section), "exe")) == 1
    assert "'lief' package" in capsys.readouterr().err


def test_inspect_binary_requires_lief(
    tmp_path: Path,
    capsys: pytest.CaptureFixture[str],
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    monkeypatch.setattr("emtrace._lief.HAS_LIEF", False)
    obj = tmp_path / "test.o"
    _ = obj.write_bytes(build_minimal_elf_o(aligned_section(), sh_addralign=8))
    args = SimpleNamespace(input=str(obj), section_name=".emtrace")
    assert read_cmds.cmd_inspect(args) == 1
    assert "'lief' package" in capsys.readouterr().err


def test_strip_requires_lief(
    tmp_path: Path,
    capsys: pytest.CaptureFixture[str],
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    monkeypatch.setattr("emtrace._lief.HAS_LIEF", False)
    blob = tmp_path / "blob.bin"
    _ = blob.write_bytes(b"not an executable" * 32)
    args = SimpleNamespace(
        input=str(blob), output=None, section_name=".emtrace", experimental_be=False
    )
    assert edit_cmds.cmd_strip(args) == 1
    assert "'lief' package" in capsys.readouterr().err
    assert edit_cmds.cmd_nonload(args) == 1
    assert "'lief' package" in capsys.readouterr().err


def test_inspect_archive_renders_table(
    tmp_path: Path, capsys: pytest.CaptureFixture[str]
) -> None:
    pytest.importorskip("lief")
    archive = tmp_path / "lib.a"
    _ = archive.write_bytes(
        build_archive(
            [("test.o", build_minimal_elf_o(aligned_section(), sh_addralign=8))]
        )
    )
    args = SimpleNamespace(input=str(archive), section_name=".emtrace")
    assert read_cmds.cmd_inspect(args) == 0
    out = capsys.readouterr().out
    assert "type:             archive" in out
    assert "test.o" in out


def test_nonload_in_place_preserves_mode(tmp_path: Path) -> None:
    pytest.importorskip("lief")
    from emtrace import ExeImage

    from .test_exemod import build_test_exe

    exe = tmp_path / "exe"
    _ = exe.write_bytes(build_test_exe())
    _ = exe.chmod(0o755)

    args = SimpleNamespace(input=str(exe), output=None, section_name=".emtrace")
    assert edit_cmds.cmd_nonload(args) == 0

    assert ".emtrace" in ExeImage.open(exe).section_names()
    assert exe.stat().st_mode & 0o7777 == 0o755  # mode survives the atomic replace


def test_strip_output_flag_leaves_input_untouched(tmp_path: Path) -> None:
    pytest.importorskip("lief")
    from emtrace import ExeImage

    from .test_exemod import build_test_exe

    exe = tmp_path / "exe"
    _ = exe.write_bytes(build_test_exe())
    original = exe.read_bytes()

    out = tmp_path / "out"
    args = SimpleNamespace(input=str(exe), output=str(out), section_name=".emtrace")
    assert edit_cmds.cmd_strip(args) == 0

    assert exe.read_bytes() == original  # original untouched when -o is used
    assert not ExeImage.open(out).has_section(".emtrace")
