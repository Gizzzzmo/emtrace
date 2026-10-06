import contextlib
import json
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Any, cast

import pytest

# Add paths to test executables here.
# These executables have to have a .emt_exp section
# with the expected output of emtrace.py.
# If a binary specified here does not exist, the test is skipped.
# `just build_all` should build all binaries.
C_TEST_EXECUTABLES = [
    "examples/test_basic",
    "examples/test_basic_cobs",
    "examples/test_integers",
    "examples/test_integers_cobs",
    "examples/test_strings",
    "examples/test_strings_cobs",
    "examples/test_doubles",
    "examples/test_doubles_cobs",
    "examples/test_mixed",
    "examples/test_mixed_cobs",
    "examples/test_edge_cases",
    "examples/test_edge_cases_cobs",
    "examples/test_large_numbers",
    "examples/test_large_numbers_cobs",
    "examples/test_arr",
    "examples/test_arr_cobs",
]

# Executables that are expected to FAIL `emtrace check`: they deliberately
# declare more records than their emt_ptr_t can address (a 1-byte emt_ptr_t
# with alignment power 8 can only represent addresses below 2^16 bytes, but
# the .emtrace section is larger).
# These must not be added to C_TEST_EXECUTABLES: the generic tests expect
# check to pass, and the runtime tests need a .emt_exp section.
C_TEST_EXECUTABLES_EXPECT_CHECK_FAILURE = [
    "examples/test_too_many_records",
    "examples/test_too_many_records_cobs",
]

_DEFAULT_BUILD_DIRS = [
    "../c/build/clang/rel",
    "../c/build/clang/dbg",
    "../c/build/clang/opt-dbg",
]


def _load_build_dirs() -> list[str]:
    """Build dirs from build_dirs.json if present, else the default list."""
    build_dir_file = Path(__file__).parent / "build_dirs.json"
    if build_dir_file.is_file():
        with open(build_dir_file) as fp, contextlib.suppress(Exception):
            loaded: list[str] = json.load(fp)
            return loaded
    return _DEFAULT_BUILD_DIRS


C_BUILD_DIRS = _load_build_dirs()

# The strip/nonload tests edit the binaries themselves and are comparatively
# slow, so they only run against one build directory (the compiled code is
# identical across presets).
EDIT_BUILD_DIR = "../c/build/clang/rel"
EDIT_TEST_EXECUTABLES = [f"{EDIT_BUILD_DIR}/{exe}" for exe in C_TEST_EXECUTABLES]

RUST_TEST_EXECUTABLES: list[str] = [
    # "test_basic",
    # "test_bool",
    # "test_f32",
    # "test_integers",
    # "test_strings",
    # "test_doubles",
    # "test_mixed",
    # "test_edge_cases",
    # "test_large_numbers",
    # "test_vec",
]

RUST_BUILD_DIRS = [
    "../rust/target/debug/examples/",
    "../rust/target/release/examples/",
]

TEST_EXECUTABLES: list[Path] = []

for build_dir in RUST_BUILD_DIRS:
    for test_exe in RUST_TEST_EXECUTABLES:
        file = Path(__file__).parent.resolve() / build_dir / test_exe
        if os.name == "nt":
            file = file.with_suffix(".exe")

        TEST_EXECUTABLES.append(file)

for build_dir in C_BUILD_DIRS:
    for test_exe in C_TEST_EXECUTABLES:
        file: Path = Path(__file__).parent.resolve() / build_dir / test_exe
        if os.name == "nt":
            file = file.with_suffix(".exe")

        TEST_EXECUTABLES.append(file)

print(TEST_EXECUTABLES)


@pytest.mark.parametrize("executable_path_str", TEST_EXECUTABLES)
def test_check_on_executable(executable_path_str: str):
    """
    Runs emtrace check on the .emtrace section of each test executable and
    verifies it passes conformance without errors.
    """
    pytest_dir = Path(__file__).parent
    executable = pytest_dir / executable_path_str

    if not executable.exists():
        pytest.skip(
            f"Executable {executable_path_str} not found. Make sure it is built."
        )

    parser_dir = str(pytest_dir.parent / "parser")
    env = os.environ.copy()
    env["PYTHONPATH"] = parser_dir + os.pathsep + env.get("PYTHONPATH", "")
    try:
        result = subprocess.run(
            [sys.executable, "-m", "emtrace", "check", str(executable)],
            capture_output=True,
            check=True,
            timeout=5,
            env=env,
        )
    except subprocess.CalledProcessError as e:
        pytest.fail(
            "\n".join(
                [
                    f"emtrace check failed for {executable_path_str} with exit code {e.returncode}:",
                    f"STDERR:\n{e.stderr.decode()}",
                    f"STDOUT:\n{e.stdout.decode()}",
                ]
            ),
            pytrace=False,
        )
    except subprocess.TimeoutExpired:
        pytest.fail(f"Timeout running emtrace check for {executable_path_str}")

    if result.stderr:
        print(result.stderr.decode(), file=sys.stderr)


@pytest.mark.parametrize(
    "executable_path_str",
    [
        f"{build_dir}/{exe}"
        for build_dir in C_BUILD_DIRS
        for exe in C_TEST_EXECUTABLES_EXPECT_CHECK_FAILURE
    ],
)
def test_check_address_overflow(executable_path_str: str):
    """
    Runs emtrace check on executables that deliberately declare more records
    than their emt_ptr_t can address, and verifies that check fails with the
    address-representability error.
    """
    pytest_dir = Path(__file__).parent
    executable = pytest_dir / executable_path_str

    if not executable.exists():
        pytest.skip(
            f"Executable {executable_path_str} not found. Make sure it is built."
        )

    parser_dir = str(pytest_dir.parent / "parser")
    env = os.environ.copy()
    env["PYTHONPATH"] = parser_dir + os.pathsep + env.get("PYTHONPATH", "")

    result = subprocess.run(
        [sys.executable, "-m", "emtrace", "check", str(executable)],
        capture_output=True,
        timeout=5,
        env=env,
    )

    stderr = result.stderr.decode()
    assert result.returncode == 1, (
        f"emtrace check unexpectedly returned {result.returncode} for "
        f"{executable_path_str}:\nSTDOUT:\n{result.stdout.decode()}\nSTDERR:\n{stderr}"
    )
    assert "cannot be represented" in stderr
    assert "1-byte emt_ptr_t" in stderr


@pytest.mark.parametrize("executable_path_str", TEST_EXECUTABLES)
def test_emtrace_json_roundtrip(executable_path_str: str):
    """
    Exports the .emtrace section of an executable to JSON via emtrace export, runs the
    executable to capture its raw trace output, extracts the .emt_exp expected output
    from the ELF, then feeds the JSON and raw input into emtrace run in --test mode,
    comparing against the .emt_exp section (the same reference used by test_emtrace_on_executable).
    """
    pytest_dir = Path(__file__).parent
    executable = pytest_dir / executable_path_str

    if not executable.exists():
        pytest.skip(
            f"Executable {executable_path_str} not found. Make sure it is built."
        )

    parser_dir = str(pytest_dir.parent / "parser")
    emtrace_script = str(pytest_dir.parent / "parser/emtrace.py")
    env = os.environ.copy()
    env["PYTHONPATH"] = parser_dir + os.pathsep + env.get("PYTHONPATH", "")

    # Import lief via the parser's own import path so we reuse the same lief
    # that emtrace.py uses, without depending on pyelftools.
    sys.path.insert(0, parser_dir)
    try:
        import lief as _lief
    except ImportError:
        pytest.skip("lief not available")
    finally:
        _ = sys.path.pop(0)

    with tempfile.TemporaryDirectory() as tmpdir:
        json_path = os.path.join(tmpdir, "format.json")
        raw_path = os.path.join(tmpdir, "raw.bin")
        expected_path = os.path.join(tmpdir, "expected.bin")

        # Step 1: export .emtrace section to JSON.
        try:
            _ = subprocess.run(
                [
                    sys.executable,
                    "-m",
                    "emtrace",
                    "export",
                    str(executable),
                    "--output",
                    json_path,
                ],
                capture_output=True,
                check=True,
                timeout=5,
                env=env,
            )
        except subprocess.CalledProcessError as e:
            pytest.fail(
                "\n".join(
                    [
                        f"emtrace export failed for {executable_path_str} with exit code {e.returncode}:",
                        f"STDERR:\n{e.stderr.decode()}",
                        f"STDOUT:\n{e.stdout.decode()}",
                    ]
                ),
                pytrace=False,
            )

        # Step 2: run the executable to capture its raw trace output.
        try:
            run_result = subprocess.run(
                [str(executable)],
                capture_output=True,
                check=True,
                timeout=5,
            )
        except subprocess.CalledProcessError as e:
            pytest.fail(
                f"Executable {executable_path_str} exited with {e.returncode}.",
                pytrace=False,
            )
        except subprocess.TimeoutExpired:
            pytest.fail(f"Timeout running {executable_path_str}")

        with open(raw_path, "wb") as f:
            _ = f.write(run_result.stdout)

        # Step 3: extract .emt_exp section from the executable using lief.
        # Cast to Any: lief's stubs type the generic parse result as the base
        # Binary, which does not declare get_section.
        exe = cast("Any", _lief.parse(str(executable)))
        if exe is None:
            pytest.skip(f"lief could not parse {executable_path_str}")
        exp_section = exe.get_section(".emt_exp")
        if exp_section is None:
            pytest.skip(f"No .emt_exp section in {executable_path_str}")
        expected_bytes = bytes(exp_section.content).rstrip(b"\x00")

        with open(expected_path, "wb") as f:
            _ = f.write(expected_bytes)

        # Step 4: decode using emtrace run with JSON format info, raw input, and --test.
        try:
            _ = subprocess.run(
                [
                    sys.executable,
                    emtrace_script,
                    "run",
                    json_path,
                    "--input",
                    "file://" + raw_path,
                    "--test",
                    expected_path,
                ],
                capture_output=True,
                check=True,
                timeout=5,
            )
        except subprocess.CalledProcessError as e:
            pytest.fail(
                "\n".join(
                    [
                        f"emtrace (JSON --test) failed for {executable_path_str} with exit code {e.returncode}:",
                        f"STDERR:\n{e.stderr.decode()}",
                        f"STDOUT:\n{e.stdout.decode()}",
                    ]
                ),
                pytrace=False,
            )


@pytest.mark.parametrize("executable_path_str", TEST_EXECUTABLES)
def test_emtrace_on_executable(executable_path_str: str):
    """
    Runs an executable, captures its trace output, and then runs emtrace run
    in test mode to verify its output against the expected output embedded
    in the executable's .emt_exp section.
    """
    pytest_dir = Path(__file__).parent
    executable = pytest_dir / executable_path_str

    if not executable.exists():
        pytest.skip(
            f"Executable {executable_path_str} not found. Make sure it is built."
        )

    emtrace_script = pytest_dir.parent / "parser/emtrace.py"
    try:
        emtrace_process = subprocess.run(
            [
                sys.executable,
                str(emtrace_script),
                "run",
                str(executable),
                "--test",
                "--debug-script",
            ],
            capture_output=True,
            check=True,
            timeout=5,
        )
    except subprocess.CalledProcessError as e:
        pytest.fail(
            "\n".join(
                [
                    f"emtrace.py failed for {executable_path_str} with exit code {e.returncode}:",
                    f"STDERR:\n{e.stderr.decode()}",
                    f"STDOUT:\n{e.stdout.decode()}",
                ]
            ),
            pytrace=False,
        )
    except subprocess.TimeoutExpired:
        pytest.fail(f"Timeout running emtrace.py for {executable_path_str}")

    # The test passes if emtrace.py exits with 0.
    # The output of the test is printed by emtrace.py itself.
    print(emtrace_process.stdout.decode())
    if emtrace_process.stderr:
        print(emtrace_process.stderr.decode(), file=sys.stderr)


def _parser_env() -> tuple[str, dict[str, str]]:
    """Parser directory and a subprocess environment with it on PYTHONPATH."""
    pytest_dir = Path(__file__).parent
    parser_dir = str(pytest_dir.parent / "parser")
    env = os.environ.copy()
    env["PYTHONPATH"] = parser_dir + os.pathsep + env.get("PYTHONPATH", "")
    return parser_dir, env


@pytest.mark.parametrize("executable_path_str", EDIT_TEST_EXECUTABLES)
def test_nonload_on_executable(executable_path_str: str):
    """
    Writes a copy of each test executable with `emtrace nonload` (the (INFO)
    linker-script effect: the .emtrace section stays in the file but is
    marked non-loadable) and verifies that the copy still runs, that
    `emtrace run --test` still decodes its output against .emt_exp, that
    `emtrace check` still passes, and that the section really lost its
    loadable flag and its address.
    """
    pytest_dir = Path(__file__).parent
    executable = pytest_dir / executable_path_str

    if not executable.exists():
        pytest.skip(
            f"Executable {executable_path_str} not found. Make sure it is built."
        )

    _parser_dir, env = _parser_env()
    with tempfile.TemporaryDirectory() as tmpdir:
        copy_path = os.path.join(tmpdir, "nonloaded")

        result = subprocess.run(
            [
                sys.executable,
                "-m",
                "emtrace",
                "nonload",
                str(executable),
                "--output",
                copy_path,
            ],
            capture_output=True,
            timeout=5,
            env=env,
        )
        assert result.returncode == 0, (
            f"emtrace nonload failed for {executable_path_str}:\n"
            f"STDOUT:\n{result.stdout.decode()}\nSTDERR:\n{result.stderr.decode()}"
        )
        assert "loadable flag cleared" in result.stdout.decode()

        result = subprocess.run(
            [sys.executable, "-m", "emtrace", "run", copy_path, "--test"],
            capture_output=True,
            timeout=5,
            env=env,
        )
        assert result.returncode == 0, (
            f"emtrace run --test failed on the nonloaded copy of "
            f"{executable_path_str}:\nSTDOUT:\n{result.stdout.decode()}\n"
            f"STDERR:\n{result.stderr.decode()}"
        )

        result = subprocess.run(
            [sys.executable, "-m", "emtrace", "check", copy_path],
            capture_output=True,
            timeout=5,
            env=env,
        )
        assert result.returncode == 0, (
            f"emtrace check failed on the nonloaded copy of {executable_path_str}:\n"
            f"STDOUT:\n{result.stdout.decode()}\nSTDERR:\n{result.stderr.decode()}"
        )

        import lief

        parsed = lief.ELF.parse(copy_path)
        assert parsed is not None
        sec = parsed.get_section(".emtrace")
        assert not sec.has(lief.ELF.Section.FLAGS.ALLOC)
        assert int(sec.virtual_address) == 0


@pytest.mark.parametrize("executable_path_str", EDIT_TEST_EXECUTABLES)
def test_strip_on_executable(executable_path_str: str):
    """
    Writes a copy of each test executable with `emtrace strip` (the .emtrace
    section entry removed and its content cleared) and verifies that the copy
    still runs, that `emtrace check` fails because the section is gone, and
    that no trace data remains in the file.
    """
    pytest_dir = Path(__file__).parent
    executable = pytest_dir / executable_path_str

    if not executable.exists():
        pytest.skip(
            f"Executable {executable_path_str} not found. Make sure it is built."
        )

    _, env = _parser_env()

    with tempfile.TemporaryDirectory() as tmpdir:
        copy_path = os.path.join(tmpdir, "stripped")

        result = subprocess.run(
            [
                sys.executable,
                "-m",
                "emtrace",
                "strip",
                str(executable),
                "--output",
                copy_path,
            ],
            capture_output=True,
            timeout=5,
            env=env,
        )
        assert result.returncode == 0, (
            f"emtrace strip failed for {executable_path_str}:\n"
            f"STDOUT:\n{result.stdout.decode()}\nSTDERR:\n{result.stderr.decode()}"
        )
        assert "removed the .emtrace section entry" in result.stdout.decode()

        result = subprocess.run([copy_path], capture_output=True, timeout=5)
        assert result.returncode == 0, (
            f"The stripped copy of {executable_path_str} does not run anymore:\n"
            f"STDOUT:\n{result.stdout.decode()}\nSTDERR:\n{result.stderr.decode()}"
        )

        result = subprocess.run(
            [sys.executable, "-m", "emtrace", "check", copy_path],
            capture_output=True,
            timeout=5,
            env=env,
        )
        stderr = result.stderr.decode()
        assert result.returncode == 1, (
            f"emtrace check unexpectedly passed on the stripped copy of "
            f"{executable_path_str}:\nSTDOUT:\n{result.stdout.decode()}\nSTDERR:\n{stderr}"
        )
        assert "error" in stderr

        import lief

        parsed = lief.ELF.parse(copy_path)
        assert parsed is not None
        assert parsed.get_section(".emtrace") is None
        assert b"MGIC" not in Path(copy_path).read_bytes()


def test_strip_experimental_be_on_real_binary():
    """
    Strips a real big-endian binary with --experimental-be and verifies the
    section is gone, the traces no longer parse, and the binary still runs
    (under qemu when available).

    Fixture: tests/data/mips-be-emtrace, a static big-endian MIPS32
    executable with a .emtrace section.  Built with:
      clang --target=mips-linux-gnu -nostdlib -static \\
        -fuse-ld=<ld.lld> -Wl,-e,_start mips.c -o mips-be-emtrace
    from a freestanding _start (MIPS exit syscall, code 42) plus a used
    .emtrace section holding marker bytes.
    """
    pytest_dir = Path(__file__).parent
    executable = pytest_dir / "data" / "mips-be-emtrace"
    if not executable.exists():
        pytest.skip("BE fixture binary not found.")

    _, env = _parser_env()

    with tempfile.TemporaryDirectory() as tmpdir:
        copy_path = os.path.join(tmpdir, "stripped")

        result = subprocess.run(
            [sys.executable, "-m", "emtrace", "strip", str(executable)],
            capture_output=True,
            timeout=5,
            env=env,
        )
        assert result.returncode == 1, (
            f"emtrace strip unexpectedly accepted a BE binary without the flag:\n"
            f"STDOUT:\n{result.stdout.decode()}\nSTDERR:\n{result.stderr.decode()}"
        )
        assert "big-endian" in result.stderr.decode()

        result = subprocess.run(
            [
                sys.executable,
                "-m",
                "emtrace",
                "strip",
                "--experimental-be",
                str(executable),
                "--output",
                copy_path,
            ],
            capture_output=True,
            timeout=30,
            env=env,
        )
        assert result.returncode == 0, (
            f"emtrace strip --experimental-be failed:\n"
            f"STDOUT:\n{result.stdout.decode()}\nSTDERR:\n{result.stderr.decode()}"
        )

        import lief

        parsed = lief.ELF.parse(copy_path)
        assert parsed is not None
        assert parsed.get_section(".emtrace") is None
        assert b"TRACE_MARKER_" not in Path(copy_path).read_bytes()

        result = subprocess.run(
            [sys.executable, "-m", "emtrace", "check", copy_path],
            capture_output=True,
            timeout=5,
            env=env,
        )
        assert result.returncode == 1, (
            f"emtrace check unexpectedly passed on the stripped BE binary:\n"
            f"STDOUT:\n{result.stdout.decode()}\nSTDERR:\n{result.stderr.decode()}"
        )

        if shutil.which("qemu-mips") is None:
            pytest.skip("qemu-mips not installed; skipping execution checks.")
        for path, label in ((str(executable), "original"), (copy_path, "stripped")):
            result = subprocess.run(
                ["qemu-mips", path], capture_output=True, timeout=30
            )
            assert result.returncode == 42, (
                f"The {label} BE binary does not run anymore under qemu-mips "
                f"(exit {result.returncode})."
            )


@pytest.mark.skipif(shutil.which("objcopy") is None, reason="objcopy not installed")
@pytest.mark.parametrize("executable_path_str", EDIT_TEST_EXECUTABLES[:1])
def test_strip_matches_objcopy(executable_path_str: str):
    """
    Differential test against `objcopy --remove-section .emtrace`: both tools
    must produce a binary with no .emtrace section that still runs and whose
    traces no longer parse.  (Only behavioral parity is asserted; the rebuilt
    bytes legitimately differ.)
    """
    pytest_dir = Path(__file__).parent
    executable = pytest_dir / executable_path_str

    if not executable.exists():
        pytest.skip(
            f"Executable {executable_path_str} not found. Make sure it is built."
        )

    _, env = _parser_env()

    with tempfile.TemporaryDirectory() as tmpdir:
        oc_path = os.path.join(tmpdir, "oc-stripped")
        result = subprocess.run(
            ["objcopy", "--remove-section", ".emtrace", str(executable), oc_path],
            capture_output=True,
            timeout=30,
        )
        assert result.returncode == 0, (
            f"objcopy --remove-section failed:\nSTDERR:\n{result.stderr.decode()}"
        )

        em_path = os.path.join(tmpdir, "em-stripped")
        result = subprocess.run(
            [
                sys.executable,
                "-m",
                "emtrace",
                "strip",
                str(executable),
                "--output",
                em_path,
            ],
            capture_output=True,
            timeout=5,
            env=env,
        )
        assert result.returncode == 0, (
            f"emtrace strip failed:\nSTDOUT:\n{result.stdout.decode()}\n"
            f"STDERR:\n{result.stderr.decode()}"
        )

        import lief

        for path in (oc_path, em_path):
            parsed = lief.ELF.parse(path)
            assert parsed is not None
            assert parsed.get_section(".emtrace") is None
            assert b"MGIC" not in Path(path).read_bytes()
            run = subprocess.run([path], capture_output=True, timeout=5)
            assert run.returncode == 0, f"{path} does not run anymore"
            check = subprocess.run(
                [sys.executable, "-m", "emtrace", "check", path],
                capture_output=True,
                timeout=5,
                env=env,
            )
            assert check.returncode == 1, f"check passed on {path}"
