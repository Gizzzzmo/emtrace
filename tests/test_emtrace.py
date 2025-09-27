import pytest
import subprocess
import sys
from pathlib import Path

# Add paths to test executables here.
# These executables have to have a .emtrace.test.expected section
# with the expected output of emtrace.py.
# If a binary specified here does not exist, the test is skipped.
# `just build_all` should build all binaries.
C_TEST_EXECUTABLES = [
    "examples/test_basic",
    "examples/test_integers",
    "examples/test_strings",
    "examples/test_doubles",
    "examples/test_mixed",
    "examples/test_edge_cases",
    "examples/test_large_numbers",
]

C_BUILD_DIRS = [
    "../c/build/main/gcc/rel",
    "../c/build/main/gcc/dbg",
    "../c/build/main/gcc/opt-dbg",
    "../c/build/main/clang/rel",
    "../c/build/main/clang/dbg",
    "../c/build/main/clang/opt-dbg",
]

RUST_TEST_EXECUTABLES = [
    "test_basic",
    "test_bool",
    "test_f32",
    "test_integers",
    "test_strings",
    "test_doubles",
    "test_mixed",
    "test_edge_cases",
    "test_large_numbers",
    "test_vec",
]

RUST_BUILD_DIRS = [
    "../rust/target/debug/examples/",
    "../rust/target/release/examples/",
]

TEST_EXECUTABLES: list[Path] = []

for build_dir in RUST_BUILD_DIRS:
    for test_exe in RUST_TEST_EXECUTABLES:
        TEST_EXECUTABLES.append(Path(__file__).parent.resolve() / build_dir / test_exe)

for build_dir in C_BUILD_DIRS:
    for test_exe in C_TEST_EXECUTABLES:
        TEST_EXECUTABLES.append(Path(__file__).parent.resolve() / build_dir / test_exe)

print(TEST_EXECUTABLES)


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
