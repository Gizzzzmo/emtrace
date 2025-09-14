#!/usr/bin/env -S just --justfile

current_preset := "rel"

ruff *ARGS:
    ruff {{ARGS}}

pytest *ARGS:
    pytest {{ARGS}}

alias c := cargo

cargo *ARGS:
    cd rust && cargo {{ARGS}}

cmake *ARGS:
    cd c && cmake {{ARGS}}

ctest *ARGS:
    cd c && ctest {{ARGS}}

clang-format *ARGS:
    clang-format $(git ls-files c | grep "\.\(h\|hpp\|c\|.cpp\)$") {{ARGS}}

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
test: (cargo "test" "--" "--nocapture") (ctest "--preset" current_preset) (pytest "-rs")

[parallel]
format: (ruff "format" ".") (cargo "fmt") (clang-format "-i") alejandra mdformat

alias fmt := format

[parallel]
lint: (ruff "check") cargo-clippy (clang-tidy "--use-color") (clang-format "--dry-run" "-Werror")

[parallel]
build: (cargo "build") cbuild



