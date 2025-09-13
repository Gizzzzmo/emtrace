#!/usr/bin/env -S just --justfile

current_preset := "rel"

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

test: (cargo "test" "--" "--nocapture") (ctest ) 

configure PRESET=current_preset: (cmake "--preset" PRESET)
    
alias conf := configure

cbuild PRESET=current_preset: (cmake "--build" "--preset" PRESET)

alias cb := cbuild

[parallel]
format: (cargo "fmt") clang-format

alias fmt := format

[parallel]
lint: cargo-clippy (clang-tidy "--use-color")

[parallel]
build: (cargo "build") cbuild



