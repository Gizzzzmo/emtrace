import pytest
import subprocess
import sys
from pathlib import Path

# Add paths to your test executables here.
# These executables should have a .emtrace.test.expected section
# with the expected output of emtrace.py.
# The executables should be built before running the tests.
# For C/C++ demos, you can build them with `just cb clang-dbg`.
# For Rust demos, you can build them with `just cargo build`.
C_TEST_EXECUTABLES = [
    "test/test_basic",
]

C_BUILD_DIRS = [
    "../c/build/clang/rel",
    "../c/build/clang/dbg",
    "../c/build/clang/opt-dbg",
]

RUST_TEST_EXECUTABLES = []

RUST_BUILD_DIRS = [
    "../rust/target/debug",
]

TEST_EXECUTABLES: list[Path] = []

for build_dir in RUST_BUILD_DIRS:
    for test_exe in RUST_TEST_EXECUTABLES:
        TEST_EXECUTABLES.append(Path(build_dir) / test_exe)

for build_dir in C_BUILD_DIRS:
    for test_exe in C_TEST_EXECUTABLES:
        TEST_EXECUTABLES.append(Path(build_dir) / test_exe)


@pytest.mark.parametrize("executable_path_str", TEST_EXECUTABLES)
def test_emtrace_on_executable(executable_path_str: str):
    """
    Runs an executable, captures its trace output, and then runs emtrace.py
    in test mode to verify its output against the expected output embedded
    in the executable's .emtrace.test.expected section.
    """
    pytest_dir = Path(__file__).parent
    executable = pytest_dir / executable_path_str

    if not executable.exists():
        pytest.skip(
            f"Executable {executable_path_str} not found. Make sure it is built."
        )

    # 1. Run the executable and capture its output (the raw trace).
    try:
        process = subprocess.run(
            [str(executable)],
            capture_output=True,
            check=True,
            timeout=5,
        )
    except subprocess.CalledProcessError as e:
        pytest.fail(
            f"Failed to run executable {executable_path_str}: {e.stderr.decode()}"
        )
    except subprocess.TimeoutExpired:
        pytest.fail(f"Timeout running executable {executable_path_str}")

    trace_output = process.stdout

    # 2. Run emtrace.py in test mode with the trace output as stdin.
    emtrace_script = pytest_dir.parent / "emtrace.py"
    try:
        emtrace_process = subprocess.run(
            [
                "python3",
                str(emtrace_script),
                str(executable),
                "--test",
                "--debug-script",
            ],
            input=trace_output,
            capture_output=True,
            check=True,
            timeout=5,
        )
    except subprocess.CalledProcessError as e:
        pytest.fail(
            "\n".join(
                [
                    f"emtrace.py failed for {executable_path_str} with exit code {e.returncode}:",
                    f"STDOUT:\n{e.stdout.decode()}",
                    f"STDERR:\n{e.stderr.decode()}",
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
