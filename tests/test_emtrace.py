import pytest
import subprocess
import sys
import json
import os
import tempfile
from pathlib import Path

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

C_BUILD_DIRS = [
    "../c/build/clang/rel",
    "../c/build/clang/dbg",
    "../c/build/clang/opt-dbg",
]

build_dir_file = Path(__file__).parent / "build_dirs.json"
if build_dir_file.is_file():
    with open(build_dir_file, "r") as fp:
        try:
            C_BUILD_DIRS = json.load(fp)
        except:
            pass

RUST_TEST_EXECUTABLES = [
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
        file: Path = Path(__file__).parent.resolve() / build_dir / test_exe
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
def test_emctl_check_on_executable(executable_path_str: str):
    """
    Runs emctl check on the .emtrace section of each test executable and
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
            [sys.executable, "-m", "emtrace.emctl", "check", str(executable)],
            capture_output=True,
            check=True,
            timeout=5,
            env=env,
        )
    except subprocess.CalledProcessError as e:
        pytest.fail(
            "\n".join(
                [
                    f"emctl check failed for {executable_path_str} with exit code {e.returncode}:",
                    f"STDERR:\n{e.stderr.decode()}",
                    f"STDOUT:\n{e.stdout.decode()}",
                ]
            ),
            pytrace=False,
        )
    except subprocess.TimeoutExpired:
        pytest.fail(f"Timeout running emctl check for {executable_path_str}")

    if result.stderr:
        print(result.stderr.decode(), file=sys.stderr)


@pytest.mark.parametrize("executable_path_str", TEST_EXECUTABLES)
def test_emtrace_json_roundtrip(executable_path_str: str):
    """
    Dumps the .emtrace section of an executable to JSON via emctl dump, runs the
    executable to capture its raw trace output, extracts the .emt_exp expected output
    from the ELF, then feeds the JSON and raw input into emtrace in --test mode,
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
        return
    finally:
        sys.path.pop(0)

    with tempfile.TemporaryDirectory() as tmpdir:
        json_path = os.path.join(tmpdir, "format.json")
        raw_path = os.path.join(tmpdir, "raw.bin")
        expected_path = os.path.join(tmpdir, "expected.bin")

        # Step 1: dump .emtrace section to JSON.
        try:
            subprocess.run(
                [
                    sys.executable,
                    "-m",
                    "emtrace.emctl",
                    "dump",
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
                        f"emctl dump failed for {executable_path_str} with exit code {e.returncode}:",
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
            f.write(run_result.stdout)

        # Step 3: extract .emt_exp section from the executable using lief.
        exe = _lief.parse(str(executable))
        if exe is None:
            pytest.skip(f"lief could not parse {executable_path_str}")
            return
        exp_section = exe.get_section(".emt_exp")
        if exp_section is None:
            pytest.skip(f"No .emt_exp section in {executable_path_str}")
            return
        expected_bytes = bytes(exp_section.content).rstrip(b"\x00")

        with open(expected_path, "wb") as f:
            f.write(expected_bytes)

        # Step 4: decode using emtrace with JSON format info, raw input, and --test.
        try:
            subprocess.run(
                [
                    sys.executable,
                    emtrace_script,
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
    Runs an executable, captures its trace output, and then runs emtrace.py
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
