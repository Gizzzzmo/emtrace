#!/usr/bin/env -S just --justfile

ECHO := 'cmake -E echo'
# echo no newline
ECHO_NNL := 'cmake -E echo_append'
CAT := 'cmake -E cat'
CP := 'cmake -E copy'
SWITCH_PROFILE := 'cmake -P c/scripts/switch-profile.cmake'
DEFAULT_PRESET := x'${DEFAULT_PRESET:-dbg}'

SAMPLE_PROFILES := if os() == "windows" {"c/profiles/CMakeProfilesSampleWindows.json"} else {"c/profiles/CMakeProfilesSample.json"}

_1 := shell(CAT + ' c/profiles/CMakeProfiles.json || ' + CP + ' ' + SAMPLE_PROFILES + ' c/profiles/CMakeProfiles.json')

current_preset := shell(CAT + ' c/profiles/.current_preset || ' + ECHO_NNL + ' ' + DEFAULT_PRESET)
current_build_dir := shell(CAT + ' c/build/.build_dir || ' + ECHO + ' build')

_2 := shell(ECHO_NNL + ' ' + current_preset + ' > c/profiles/.current_preset')

justfile_dir := if os() == "windows" { shell("cygpath -u '" + justfile_directory() + "'") } else { justfile_directory() }

[no-cd]
emtrace *ARGS:
    python {{justfile_dir}}/parser/emtrace.py {{ARGS}}

gen-header *ARGS:
    python ./c/header-generator/build_macro.py {{ARGS}}

alias genh := gen-header

check-header:
    python ./c/header-generator/build_macro.py ./c/build/emtrace.h
    diff ./c/include/c/include/emtrace/emtrace.h ./c/build/emtrace.h

alias checkh := check-header

hatch_build:
    cd parser && hatchling build

twine *ARGS:
    cd parser && python -m twine {{ARGS}}

alias run := emtrace

alias c := cargo

cargo *ARGS:
    cd rust && cargo {{ARGS}}

ruff *ARGS:
    ruff {{ARGS}}

basedpyright *ARGS:
    basedpyright {{ARGS}}

pytest *ARGS:
    pytest {{ARGS}}

test_parser *ARGS:
    cd parser && pytest {{ARGS}}

eetest: (pytest "-rs" "-n" "auto")

# Build and run the cross-language interop demos (rust/examples/interop_*,
# c/examples/interop_*): the executable owning the entry point always hosts the
# magic - demo 1: Rust executable + C guest library, demo 2: C executable +
# Rust guest staticlib, demo 3: Rust executable + BOTH guest libraries. Each
# demo's decoded trace stream is printed. Needs one prior `just conf` for the C
# build directory.
interop:
    #!/bin/sh
    set -eu
    just cb
    just cargo build --example interop_rust_host
    just cargo build --example interop_rust_guest
    just cargo build --example interop_combined
    just cb
    work="$(mktemp -d)"
    trap 'rm -rf "$work"' EXIT
    export PYTHONPATH=parser
    echo "== demo 1: Rust executable hosts the magic, C static library is the guest =="
    rust/target/debug/examples/interop_rust_host > "$work/host.bin"
    python3 parser/emtrace.py export rust/target/debug/examples/interop_rust_host --output "$work/host.json" > /dev/null
    python3 parser/emtrace.py run "$work/host.json" --input "file://$work/host.bin"
    echo "== demo 2: C executable hosts the magic, Rust static library is the guest =="
    "{{current_build_dir}}/examples/interop_c_host" > "$work/chost.bin"
    python3 parser/emtrace.py export "{{current_build_dir}}/examples/interop_c_host" --output "$work/chost.json" > /dev/null
    python3 parser/emtrace.py run "$work/chost.json" --input "file://$work/chost.bin"
    echo "== demo 3: one Rust executable hosts the magic for BOTH guest libraries =="
    rust/target/debug/examples/interop_combined > "$work/combined.bin"
    python3 parser/emtrace.py export rust/target/debug/examples/interop_combined --output "$work/combined.json" > /dev/null
    python3 parser/emtrace.py run "$work/combined.json" --input "file://$work/combined.bin"

cmake *ARGS:
    cd c && cmake {{ARGS}}

ctest *ARGS:
    cd c && ctest {{ARGS}}

clang-format *ARGS:
    clang-format $(git ls-files c | grep "\.\(h\|hpp\|c\|cpp\)$") {{ARGS}}

clang-tidy *ARGS:
    cd c && clang-tidy -p build/compile_commands.json $(git ls-files . | grep "\.\(c\|cpp\)$") {{ARGS}}

alias tidy := clang-tidy

cargo-clippy *ARGS: (cargo "clippy" "--" "--remap-path-prefix src/=$(pwd)/src/" ARGS)

alias clippy := cargo-clippy

configure PRESET=current_preset: (cmake "--preset" PRESET)
    @{{ECHO_NNL}} {{PRESET}} > c/profiles/.current_preset
    
alias conf := configure

cbuild PRESET=current_preset: (cmake "--build" "--preset" PRESET)
    @{{ECHO_NNL}} {{PRESET}} > c/profiles/.current_preset

alias cb := cbuild

alejandra *ARGS:
    alejandra $(git ls-files | grep "\\.nix$") {{ARGS}}

mdformat *ARGS:
    mdformat $(git ls-files | grep "\\.md$") {{ARGS}}

gersemi *ARGS:
    gersemi $(git ls-files | grep "\(\\.cmake\|CMakeLists.txt\)$") {{ARGS}}

[no-cd]
preproc FILE:
    clang -std=c23 -E -P -I {{justfile_dir}}/c/include/c/include {{FILE}} | clang-format | bat --language=c

alias pp := preproc

[parallel]
test: (cargo "test" "--" "--nocapture") (ctest "--preset" "dbg") (ctest "--preset" "rel") (ctest "--preset" "opt-dbg") eetest test_parser

[parallel]
format: (ruff "format" ".") (cargo "fmt") (clang-format "-i") alejandra mdformat (gersemi "-i")

alias fmt := format

[parallel]
lint: (ruff "check") (basedpyright "--warnings") cargo-clippy (clang-tidy "--use-color") (clang-format "--dry-run" "-Werror") 

[parallel]
conf_all: (configure "dbg") (configure "rel") (configure "opt-dbg")

[parallel]
build_all: (cargo "build" "--release" "--all-targets") (cargo "build" "--all-targets") (cbuild "dbg") (cbuild "rel") (cbuild "opt-dbg") hatch_build

[parallel]
build: (cargo "build" "--all-targets") cbuild

switch PROFILE TYPE='':
    @{{SWITCH_PROFILE}} {{PROFILE}} {{TYPE}} 

list:
    @{{SWITCH_PROFILE}}

publish_crates: (cargo "publish")

publish_pypi: (twine "upload" "parser/dist/*")

[parallel]
publish_all: publish_crates publish_pypi


