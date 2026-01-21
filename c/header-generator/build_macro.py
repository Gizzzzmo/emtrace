from io import TextIOWrapper
import subprocess
import sys
from pathlib import Path
from argparse import ArgumentParser


def build_recursive_macro(
    name: str,
    args: list[str],
    body: str,
    body_rec: str,
    body_empty: str,
    depth: int,
    global_args: list[str] | None = None,
) -> str:
    for arg in args:
        assert arg[0] != "_"

    assert depth > 0
    assert len(args) > 0

    if global_args == None:
        global_args = []

    for arg in global_args:
        assert arg[0] != "_"

    body = body.replace("\n", " ")
    body_rec = body_rec.replace("\n", " ")
    body_empty = body_empty.replace("\n", " ")

    global_arg_string = ", ".join(global_args)
    total_n_args = len(global_args)

    if total_n_args > 0:
        global_arg_string += ", "

    # to account for _dummy
    total_n_args += 1

    generated: str = ""

    def append_wrapped(s: str):
        nonlocal total_n_args
        nonlocal generated
        generated += f"#if EMT_MACRO_CAP >= {total_n_args}\n"
        generated += f"{s}\n"
        generated += "#endif\n\n"

    append_wrapped(f"#define {name}_0({global_arg_string}_dummy) {body_empty}")

    total_n_args += len(args)
    arg_string = ", ".join(args)
    
    append_wrapped(f"#define {name}_{len(args)}({global_arg_string}{arg_string}, _dummy) {body}")

    for i in range(2, depth + 1):
        postfix: int = i * len(args)
        passthrough_args = [f"_{j:x}" for j in range(postfix - len(args))]
        passthrough_string = ", ".join(passthrough_args)

        prev: str = (
            f"{name}_{postfix - len(args)}({global_arg_string}{passthrough_string}, 0)"
        )

        expanded_body = body_rec.format(prev=prev, i=i - 1)

        macro: str = f"#define {name}_{postfix}({global_arg_string}{passthrough_string}, {arg_string}, _dummy) {expanded_body}"

        total_n_args += len(args)
        append_wrapped(macro)

    generated += f"#define {name}({global_arg_string}x, ...) {name}_HELPER({global_arg_string}x, __VA_ARGS__)\n"
    generated += f"#define {name}_HELPER({global_arg_string}x, ...) {name}_##x({global_arg_string}__VA_ARGS__)\n"

    return generated


def build_dispatch_macro(name: str, args: list[str], choices: dict[str, str]) -> str:
    generated: str = ""

    arg_string = ", ".join(args)
    for tag, body in choices.items():
        body = str(body).replace("\n", " ")

        generated += f"#define {name}_{tag}({arg_string}) {body}\n\n"

    if len(args) > 0:
        param_string = f"{arg_string}, _tag"
    else:
        param_string = "_tag"

    generated += (
        f"#define {name}_HELPER({param_string}) {name}_##_tag({arg_string})\n\n"
    )
    generated += f"#define {name}({param_string}) {name}_HELPER({param_string})\n\n"

    return generated


def generate_macros(fp: TextIOWrapper, max_args: int):
    _ = fp.write(
        build_dispatch_macro(
            "EMT_F_LAYOUT_SIZE_DISPATCH",
            [],
            {
                "EMT_TAG_VAL": "3",
                "EMT_TAG_STR": "3",
                "EMT_TAG_ARR": "7",
                "EMT_TAG_SLC": "7",
                "EMT_TAG_STS": "7",
            },
        )
    )

    _ = fp.write(
        build_recursive_macro(
            "EMT_F_LAYOUT_SIZE",
            ["type", "x", "len", "tag"],
            "EMT_F_LAYOUT_SIZE_DISPATCH(tag)",
            "{prev} + EMT_F_LAYOUT_SIZE_DISPATCH(tag)",
            "0",
            max_args,
        )
    )

    _ = fp.write(
        build_dispatch_macro(
            "EMT_F_INFO_MEMBER_DISPATCH",
            ["type", "name"],
            {
                "EMT_TAG_VAL": "char name[sizeof(#type)];",
                "EMT_TAG_STR": 'char name[sizeof("string")];',
                "EMT_TAG_ARR": 'char name[sizeof("list")]; char name##_child_name[sizeof("")]; char name##_child_type_id[sizeof(#type)];',
                "EMT_TAG_SLC": 'char name[sizeof("list")]; char name##_child_name[sizeof("")]; char name##_child_type_id[sizeof(#type)];',
                "EMT_TAG_STS": 'char name[sizeof("list")]; char name##_child_name[sizeof("")]; char name##_child_type_id[sizeof(#type)];',
            },
        )
    )

    _ = fp.write(
        build_recursive_macro(
            "EMT_F_INFO_MEMBER",
            ["type", "x", "len", "tag"],
            "EMT_F_INFO_MEMBER_DISPATCH(type, type0, tag)",
            "{prev} EMT_F_INFO_MEMBER_DISPATCH(type, type{i}, tag)",
            "",
            depth=max_args,
        )
    )

    _ = fp.write(
        build_dispatch_macro(
            "EMT_F_LAYOUT_DISPATCH",
            ["name", "len", "type"],
            {
                "EMT_TAG_VAL": "offsetof(struct emt_info_unlikely_to_shadow_t, name), sizeof(type), 0,",
                "EMT_TAG_STR": "offsetof(struct emt_info_unlikely_to_shadow_t, name), EMT_NULL_TERMINATED, 0,",
                "EMT_TAG_ARR": "offsetof(struct emt_info_unlikely_to_shadow_t, name), (emt_size_t)1*(len), 1, "
                + "offsetof(struct emt_info_unlikely_to_shadow_t, name##_child_name), "
                + "sizeof(type), 0, "
                + "offsetof(struct emt_info_unlikely_to_shadow_t, name##_child_type_id),",
                "EMT_TAG_STS": "offsetof(struct emt_info_unlikely_to_shadow_t, name), (emt_size_t)1*(len), 1, "
                + "offsetof(struct emt_info_unlikely_to_shadow_t, name##_child_name), "
                + "sizeof(type), 0, "
                + "offsetof(struct emt_info_unlikely_to_shadow_t, name##_child_type_id),",
                "EMT_TAG_SLC": "offsetof(struct emt_info_unlikely_to_shadow_t, name), EMT_LENGTH_PREFIXED, 1, "
                + "offsetof(struct emt_info_unlikely_to_shadow_t, name##_child_name), "
                + "sizeof(type), 0, "
                + "offsetof(struct emt_info_unlikely_to_shadow_t, name##_child_type_id), ",
                # + "sizeof(type), 0,"
            },
        )
    )

    _ = fp.write(
        build_recursive_macro(
            "EMT_F_LAYOUT",
            ["type", "x", "len", "tag"],
            "EMT_F_LAYOUT_DISPATCH(type0, len, type, tag)",
            "{prev} EMT_F_LAYOUT_DISPATCH(type{i}, len, type, tag)",
            "",
            depth=max_args,
        )
    )

    _ = fp.write(
        build_dispatch_macro(
            "EMT_F_INFO_DISPATCH",
            ["type"],
            {
                "EMT_TAG_VAL": "#type,",
                "EMT_TAG_STR": '"string",',
                "EMT_TAG_ARR": '"list", "", #type,',
                "EMT_TAG_SLC": '"list", "", #type,',
                "EMT_TAG_STS": '"list", "", #type,',
            },
        )
    )

    _ = fp.write(
        build_recursive_macro(
            "EMT_F_INFO",
            ["type", "x", "len", "tag"],
            "EMT_F_INFO_DISPATCH(type, tag)",
            "{prev} EMT_F_INFO_DISPATCH(type, tag)",
            "",
            max_args,
        )
    )

    _ = fp.write(
        build_recursive_macro(
            "EMT_F_TOTAL_SIZE",
            ["type", "x", "len", "tag"],
            body="(sizeof(emt_ptr_t) + (len))",
            body_rec="{prev} + (len)",
            body_empty="sizeof(emt_ptr_t)",
            depth=max_args,
        )
    )

    _ = fp.write(
        build_dispatch_macro(
            "EMT_F_DISPATCH",
            ["type", "x", "len", "out_fn", "extra_arg"],
            {
                "EMT_TAG_VAL": "{type temp = x; out_fn((const void*)&temp, sizeof(type), extra_arg);}",
                "EMT_TAG_STR": "{const char* temp = x; out_fn(temp, (emt_size_t)(strlen(temp) + 1), extra_arg);}",
                "EMT_TAG_ARR": "{const type (*temp)[] = &(x); out_fn(temp, sizeof(x), extra_arg);}",
                "EMT_TAG_SLC": "{const type* emt_temp_unlikely_to_shadow = x; emt_size_t size = len; out_fn(&size, sizeof(size), extra_arg); out_fn(emt_temp_unlikely_to_shadow, size * sizeof(type), extra_arg);}",
                "EMT_TAG_STS": "{const type* temp = x; out_fn(temp, (emt_size_t) sizeof(type)*(len), extra_arg);}",
            },
        )
    )

    _ = fp.write(
        build_recursive_macro(
            "EMT_F",
            ["type", "x", "len", "tag"],
            "EMT_F_DISPATCH(type, x, len, out_fn, extra_arg, tag)",
            "{prev} EMT_F_DISPATCH(type, x, len, out_fn, extra_arg, tag)",
            "",
            depth=max_args,
            global_args=["out_fn", "extra_arg"],
        )
    )


if __name__ == "__main__":
    parser = ArgumentParser("generate-emtrace-header")
    _ = parser.add_argument(
        "output",
        nargs="?",
        default=(
            Path(__file__).parent.parent / "include/c/include/emtrace/emtrace.h"
        ).resolve(),
        type=Path,
    )
    _ = parser.add_argument("--max-args", nargs="?", default=32, type=int)
    args = parser.parse_args()

    max_args: int = args.max_args
    output_path: Path = args.output
    template_path = (Path(__file__).parent / "emtrace_template.h").resolve()
    git_root = Path(__file__).parent.parent.parent.resolve()
    with open(output_path, "w") as output:
        relative_template_path = template_path.relative_to(git_root)
        relative_script_path = Path(__file__).resolve().relative_to(git_root)
        _ = output.write(
            f"// DO NOT MODIFY DIRECTLY! Header is auto-generated from {relative_template_path} using this python script: {relative_script_path}\n// Script arguments were: --max-args={max_args}\n"
        )
        _ = output.write("#ifndef EMTRACE_EMTRACE_H\n")
        _ = output.write("#define EMTRACE_EMTRACE_H\n")
        _ = output.write("// NOLINTBEGIN(modernize-avoid-c-arrays)\n\n")

        with open(template_path, "r") as template:
            _ = output.write(template.read())

        generate_macros(output, max_args)

        _ = output.write("// NOLINTEND(modernize-avoid-c-arrays)\n\n")
        _ = output.write("\n#endif // EMTRACE_EMTRACE_H\n")

    _ = subprocess.run(["clang-format", "-i", output_path])
