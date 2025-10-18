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

[no-cd]
emtrace *ARGS:
    python3 {{justfile_directory()}}/parser/emtrace.py {{ARGS}} 

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

pytest *ARGS:
    pytest {{ARGS}}

eetest: (pytest "-rs" "-n" "auto")
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


[parallel]
test: (cargo "test" "--" "--nocapture") (ctest "--preset" "dbg") (ctest "--preset" "rel") (ctest "--preset" "opt-dbg") eetest

[parallel]
format: (ruff "format" ".") (cargo "fmt") (clang-format "-i") alejandra mdformat (gersemi "-i")

alias fmt := format

[parallel]
lint: (ruff "check") cargo-clippy (clang-tidy "--use-color") (clang-format "--dry-run" "-Werror")

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


