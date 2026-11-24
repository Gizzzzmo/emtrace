import subprocess
from argparse import ArgumentParser
from io import TextIOWrapper
from pathlib import Path


def _wrap_define_header(name: str, params: list[str], width: int = 98) -> list[str]:
    one_line = f"#define {name}({', '.join(params)})"
    if len(one_line) <= width:
        return [one_line]
    lines = [f"#define {name}("]
    cur = "    "
    for idx, param in enumerate(params):
        piece = param if idx == 0 else ", " + param
        if cur.strip() and len(cur) + len(piece) > width:
            lines.append(cur + ",")
            cur = "    " + param
        else:
            cur += piece
    lines.append(cur)
    lines.append(")")
    return lines


def _format_macro(
    name: str,
    params: list[str],
    items: list[str],
    sep: str = " ",
    prefix: str = "",
    suffix: str = "",
    width: int = 98,
) -> str:
    hdr_lines = _wrap_define_header(name, params, width=width)
    body_lines: list[str] = []
    cur = "    " + prefix
    for idx, item in enumerate(items):
        piece = item if idx == 0 else sep + item
        if cur.strip() and len(cur) + len(piece) > width:
            body_lines.append(cur + ("," if sep == ", " else ""))
            cur = "    " + item
        else:
            cur += piece
    if suffix and len(cur) + len(suffix) > width:
        body_lines.append(cur)
        body_lines.append("    " + suffix.lstrip())
    else:
        body_lines.append(cur + suffix)

    all_lines = [*hdr_lines, *body_lines]
    padded = [f"{line:<99}\\" for line in all_lines[:-1]] + [all_lines[-1]]
    return "\n".join(padded)


def _emit_flat_macros(max_args: int) -> str:
    assert max_args > 0
    macros: list[str] = []

    if max_args == 1:
        macros.append("#define EMT_NORM_TAIL(overflow, ...) EMT_OVERFLOW_##overflow")
    elif max_args == 2:
        macros.append("#define EMT_NORM_REST(overflow, ...) EMT_OVERFLOW_##overflow")
        macros.append(
            "#define EMT_NORM_TAIL(a1, ...) , a1 EMT_NORM_REST(EMT_ARG_##__VA_ARGS__, EMT_END)"
        )
    else:
        rest_params = [*(f"a{i}" for i in range(2, max_args)), "overflow", "..."]
        rest_items = ["a2", *(f"EMT_ARG_##a{i}" for i in range(3, max_args))]
        macros.append(
            _format_macro(
                "EMT_NORM_REST",
                rest_params,
                rest_items,
                sep=", ",
                prefix=", ",
                suffix=" EMT_OVERFLOW_##overflow",
            )
        )
        macros.append(
            _format_macro(
                "EMT_NORM_TAIL",
                ["a1", "..."],
                ["EMT_ARG_##__VA_ARGS__", *(["EMT_END"] * (max_args - 1))],
                sep=", ",
                prefix=", a1 EMT_NORM_REST(",
                suffix=")",
            )
        )

    params = [f"a{i}" for i in range(max_args)]
    macros.extend(
        [
            _format_macro(
                "EMT_F_COUNT",
                params,
                [f"EMT_F_COUNT_##a{i}" for i in range(max_args)],
                prefix="(0 ",
                suffix=")",
            ),
            _format_macro(
                "EMT_F_LAYOUT_SIZE",
                params,
                [f"EMT_F_LAYOUT_SIZE_##a{i}" for i in range(max_args)],
                prefix="(0 ",
                suffix=")",
            ),
            _format_macro(
                "EMT_F_INFO_MEMBER",
                params,
                [f"EMT_F_INFO_MEMBER_DISPATCH(type{i}, a{i})" for i in range(max_args)],
            ),
            _format_macro(
                "EMT_F_LAYOUT",
                params,
                [f"EMT_F_LAYOUT_DISPATCH(type{i}, a{i})" for i in range(max_args)],
            ),
            _format_macro(
                "EMT_F_INFO",
                params,
                [f"EMT_F_INFO_##a{i}" for i in range(max_args)],
            ),
            _format_macro(
                "EMT_F_TOTAL_SIZE",
                params,
                [f"EMT_F_TOTAL_SIZE_##a{i}" for i in range(max_args)],
                prefix="(0 ",
                suffix=")",
            ),
            _format_macro(
                "EMT_F",
                ["out_fn", "extra_arg", *params],
                [f"EMT_F_DISPATCH(out_fn, extra_arg, a{i})" for i in range(max_args)],
            ),
        ]
    )

    return "// clang-format off\n" + "\n\n".join(macros) + "\n// clang-format on\n\n"


def generate_macros(fp: TextIOWrapper, max_args: int):
    assert max_args > 0
    # At `max_args` trace arguments, EMT_NORM_REST receives 2 * max_args - 1 arguments and
    # EMT_TRACE_F_IMPL receives max_args + 9 arguments.
    max_invocation_args = max(2 * max_args - 1, max_args + 9)
    if max_invocation_args > 127:
        # Cap at 64 under MSVC's 127-argument limit (2 * 64 - 1 = 127, 64 + 9 = 73).
        capped_args = min(max_args, 64)
        _ = fp.write(
            f"#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= {max_invocation_args}\n"
            f"{_emit_flat_macros(max_args)}"
            "#else\n"
            f"{_emit_flat_macros(capped_args)}"
            "#endif\n\n"
        )
    else:
        _ = fp.write(_emit_flat_macros(max_args))


C_HEADER_TEMPLATE_PATH = (Path(__file__).parent / "c/emtrace_template.h").resolve()
COMMON_HEADER_PATH = (Path(__file__).parent / "c/emtrace_common.h").resolve()
CPP_HEADER_PATH = (Path(__file__).parent / "cxx/emtrace_header.hpp").resolve()
ROOT = Path(__file__).parent.parent.parent.resolve()


def write_common(output, version: tuple[int, int, int], nolint_errors: str):
    major, minor, patch = version
    _ = output.write("#ifdef EMTRACE_COMMON\n")
    _ = output.write(
        f"#if (defined(EMTRACE_VERSION_MAJOR) && EMTRACE_VERSION_MAJOR != {major})"
    )
    _ = output.write(
        f" || (defined(EMTRACE_VERSION_MINOR) && EMTRACE_VERSION_MINOR != {minor})"
    )
    _ = output.write(
        f" || (defined(EMTRACE_VERSION_PATCH) && EMTRACE_VERSION_PATCH != {patch})\n"
    )
    _ = output.write(
        '#error "version mismatch between this header and another previously included one (probably C++ variant emtrace.hpp)"\n'
    )
    _ = output.write("#endif\n")
    _ = output.write("#else // EMTRACE_COMMON\n")

    _ = output.write("#define EMTRACE_COMMON\n")
    _ = output.write(f"#define EMTRACE_VERSION_MAJOR {major}\n")
    _ = output.write(f"#define EMTRACE_VERSION_MINOR {minor}\n")
    _ = output.write(f"#define EMTRACE_VERSION_PATCH {patch}\n\n")

    _ = output.write(f"// NOLINTBEGIN({nolint_errors})\n\n")

    with open(COMMON_HEADER_PATH) as common:
        _ = output.write(common.read())

    _ = output.write(f"// NOLINTEND({nolint_errors})\n\n")
    _ = output.write("#endif // EMTRACE_COMMON\n")


def write_cpp_header(output_path: Path, version: tuple[int, int, int]):
    with open(output_path, "w") as output:
        relative_header_path = CPP_HEADER_PATH.relative_to(ROOT)
        relative_common_path = COMMON_HEADER_PATH.relative_to(ROOT)
        relative_script_path = Path(__file__).resolve().relative_to(ROOT)
        _ = output.write(
            f"// DO NOT MODIFY DIRECTLY! Header is auto-generated from {relative_header_path} and {relative_common_path} using this python script: {relative_script_path}\n"
        )
        header_guard = "EMTRACE_EMTRACE_HPP"
        _ = output.write(f"#ifndef {header_guard}\n")
        _ = output.write(f"#define {header_guard}\n")

        nolint_errors = "modernize-*,bugprone-casting-through-void"
        write_common(output, version, nolint_errors)

        with open(CPP_HEADER_PATH) as template:
            first_line = template.readline()
            assert first_line == '#include "../c/emtrace_common.h"\n'
            _ = output.write(template.read())

        _ = output.write(f"\n#endif // {header_guard}\n")


def write_c_header(output_path: Path, version: tuple[int, int, int], max_args: int):
    with open(output_path, "w") as output:
        relative_template_path = C_HEADER_TEMPLATE_PATH.relative_to(ROOT)
        relative_common_path = COMMON_HEADER_PATH.relative_to(ROOT)
        relative_script_path = Path(__file__).resolve().relative_to(ROOT)
        _ = output.write(
            f"// DO NOT MODIFY DIRECTLY! Header is auto-generated from {relative_template_path} and {relative_common_path} using this python script: {relative_script_path}\n// Script arguments were: max_args={max_args}\n"
        )
        _ = output.write("#ifndef EMTRACE_EMTRACE_H\n")
        _ = output.write("#define EMTRACE_EMTRACE_H\n")

        nolint_errors = "modernize-*,bugprone-casting-through-void"
        write_common(output, version, nolint_errors)

        _ = output.write(f"// NOLINTBEGIN({nolint_errors})\n\n")
        with open(C_HEADER_TEMPLATE_PATH) as template:
            first_line = template.readline()
            assert first_line == '#include "emtrace_common.h"\n'
            _ = output.write(template.read())

        generate_macros(output, max_args)

        _ = output.write(f"// NOLINTEND({nolint_errors})\n\n")
        _ = output.write("\n#endif // EMTRACE_EMTRACE_H\n")


if __name__ == "__main__":
    parser = ArgumentParser("generate-emtrace-header")
    _ = parser.add_argument(
        "output",
        nargs="?",
        default=None,
    )
    _ = parser.add_argument("--cpp", action="store_true")
    _ = parser.add_argument("--c-max-args", nargs="?", default=64, type=int)
    args = parser.parse_args()

    if args.output is None:
        if args.cpp:
            args.output = (
                Path(__file__).parent.parent / "include/cxx/include/emtrace/emtrace.hpp"
            ).resolve()
        else:
            args.output = (
                Path(__file__).parent.parent / "include/c/include/emtrace/emtrace.h"
            ).resolve()

    with open(Path(__file__).parent.parent / "VERSION") as version_file:
        version = version_file.read()

    version = version.strip().split(".")
    assert len(version) == 3
    version = int(version[0]), int(version[1]), int(version[2])

    max_args: int = args.c_max_args
    output_path: Path = args.output
    print(output_path)
    if args.cpp:
        write_cpp_header(output_path, version)
    else:
        write_c_header(output_path, version, max_args)

    _ = subprocess.run(["clang-format", "-i", output_path])
