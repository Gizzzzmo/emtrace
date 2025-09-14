#!/usr/bin/env -S just --justfile

current_preset := "rel"

ruff *ARGS:
    ruff {{ARGS}}

alias c := cargo

cargo *ARGS:
    cd rust && cargo {{ARGS}}

cmake *ARGS:
    cd c && cmake {{ARGS}}

ctest *ARGS:
    cd c && ctest {{ARGS}}

clang-format:
    clang-format -i $(git ls-files c | grep "\.\(h\|hpp\|c\|.cpp\)$")

clang-tidy *ARGS:
    cd c && clang-tidy -p build/compile_commands.json $(git ls-files . | grep "\.\(c\|.cpp\)$") {{ARGS}}

alias tidy := clang-tidy

cargo-clippy *ARGS: (cargo "clippy" "--" "--remap-path-prefix src/=$(pwd)/src/" ARGS)

alias clippy := cargo-clippy

configure PRESET=current_preset: (cmake "--preset" PRESET)
    
alias conf := configure

cbuild PRESET=current_preset: (cmake "--build" "--preset" PRESET)

alias cb := cbuild

alejandra *ARGS:
    alejandra $(git ls-files | grep "\\.nix$") {{ARGS}}

mdformat *ARGS:
    mdformat $(git ls-files | grep "\\.md$") {{ARGS}}

[parallel]
test: (cargo "test" "--" "--nocapture") (ctest "--preset" current_preset)
    pytest

[parallel]
format: (ruff "format" ".") (cargo "fmt") clang-format alejandra mdformat

alias fmt := format

[parallel]
lint: (ruff "check") cargo-clippy (clang-tidy "--use-color")

[parallel]
build: (cargo "build") cbuild



