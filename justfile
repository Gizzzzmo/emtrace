#!/usr/bin/env -S just --justfile

CARGO := x'${CARGO:-cargo}'
CMAKE := x'${CMAKE:-cmake}'
CTEST := x'${CTEST:-ctest}'
CLANG_TIDY := x'${CLANG_TIDY:-clang-tidy}'
CLANG_FORMAT := x'${CLANG_FORMAT:-clang-format}'
GIT := x'${GIT:-git}'
RUFF := x'${RUFF:-ruff}'
PYTEST := x'${PYTEST:-pytest}'
GERSEMI := x'${GERSEMI:-gersemi}'
ALEJANDRA := x'${ALEJANDRA:-alejandra}'
MDFORMAT := x'${MDFORMAT:-mdformat}'
ECHO := CMAKE + ' -E echo'
ECHO_NNN := CMAKE + ' -E echo_append'
CAT := CMAKE + ' -E cat'
CP := CMAKE + ' -E copy'
SWITCH_PROFILE := CMAKE + ' -P c/scripts/switch-profile.cmake'
DEFAULT_PRESET := x'${DEFAULT_PRESET:-dbg}'

_1 := shell(CAT + ' c/profiles/CMakeProfiles.json || ' + CP + ' c/profiles/CMakeProfilesSample.json c/profiles/CMakeProfiles.json')

current_preset := shell(CAT + ' c/profiles/.current_preset || ' + ECHO_NNN + ' ' + DEFAULT_PRESET)
current_build_dir := shell(CAT + ' c/build/.build_dir || ' + ECHO + ' build')

_2 := shell(ECHO_NNN + ' ' + current_preset + ' > c/profiles/.current_preset')

alias c := cargo

cargo *ARGS:
    cd rust && {{CARGO}} {{ARGS}}

ruff *ARGS:
    {{RUFF}} {{ARGS}}

pytest *ARGS:
    {{PYTEST}} {{ARGS}}

eetest: (pytest "-rs" "-n" "auto")
cmake *ARGS:
    cd c && {{CMAKE}} {{ARGS}}

ctest *ARGS:
    cd c && ctest {{ARGS}}

clang-format *ARGS:
    {{CLANG_FORMAT}} $({{GIT}} ls-files c | grep "\.\(h\|hpp\|c\|cpp\)$") {{ARGS}}

clang-tidy *ARGS:
    cd c && {{CLANG_TIDY}} -p build/compile_commands.json $({{GIT}} ls-files . | grep "\.\(c\|cpp\)$") {{ARGS}}

alias tidy := clang-tidy

cargo-clippy *ARGS: (cargo "clippy" "--" "--remap-path-prefix src/=$(pwd)/src/" ARGS)

alias clippy := cargo-clippy

configure PRESET=current_preset: (cmake "--preset" PRESET)
    @{{ECHO_NNN}} {{PRESET}} > c/profiles/.current_preset
    
alias conf := configure

cbuild PRESET=current_preset: (cmake "--build" "--preset" PRESET)
    @{{ECHO_NNN}} {{PRESET}} > c/profiles/.current_preset

alias cb := cbuild

alejandra *ARGS:
    alejandra $({{GIT}} ls-files | grep "\\.nix$") {{ARGS}}

mdformat *ARGS:
    mdformat $({{GIT}} ls-files | grep "\\.md$") {{ARGS}}

gersemi *ARGS:
    {{GERSEMI}} $({{GIT}} ls-files | grep "\(\\.cmake\|CMakeLists.txt\)$") {{ARGS}}


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
build_all: (cargo "build" "--release" "--all-targets") (cargo "build" "--all-targets") (cbuild "dbg") (cbuild "rel") (cbuild "opt-dbg")

[parallel]
build: (cargo "build" "--all-targets") cbuild

switch PROFILE TYPE='':
    @{{SWITCH_PROFILE}} {{PROFILE}} {{TYPE}} 

list:
    @{{SWITCH_PROFILE}}

publish: (cargo "publish") 


