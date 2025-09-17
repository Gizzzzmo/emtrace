# Agent Guidelines for emtrace

## Build/Test Commands

- **Run commands in dev environment**: `nix develop --command <command>`
- **CMake build**: `just conf <preset>`
- **CMake build**: `just cb <preset>`
- **Rust build**: `just cargo build`
- **Build all**: `just build`
- **Rust run tests**: `just cargo test`
- **Rust run binary**: `just cargo run --bin <binary>`
- **Lint C code**: `just tidy`
- **Lint Rust code**: `just clippy`
- **Lint Python code**: `just ruff check`
- **Lint all**: `just lint`
- **Format C code**: `just clang-format`
- **Format Rust code**: `just cargo fmt`
- **Format Python code**: `just ruff format`
- **Format Nix code**: `just alejandra`
- **Format Markdown code**: `just mdformat`
- **Format all**: `just fmt`

## Code Style Guidelines

### C and C++ Code (c/)

- Follow `c/.clang-format` configuration (100 column limit, 4-space indent, pointer alignment left)
- C code should be C99 compliant, except for how the section placement is specified
- Use header guards: `#ifndef EMTRACE_EMTRACE_H` format is enforced by clang-tidy's
  llvm-header-guard check
- C file endings: `.c` and `.h`
- C++ file endings : `.cpp` and `.hpp`
- Include `extern "C"` blocks for C++ compatibility in C headers
- Follow `c/.clang-tidy` `readability-identifier-naming` settings for naming conventions, although
  these are relaxed in tests, and examples with their own `.clang-tidy`
- Use `static inline` for header-only functions
- private headers of a target are in an `include/` subdirectory of the respective source directory
- In C always use typedefs for enums, structs and unions

### CMake code (c/)

- Follow `.gersemirc` configuration for style (100 column limit, 4-space indent)

### Rust Code (rust/)

- Edition 2024, standard Rust formatting conventions
- Use `unsafe` blocks sparingly and document safety
- Module structure: `mod name { }` with pub exports
- Trait implementations for type extensions
- Use const generics for compile-time computations
- Test modules with `#[cfg(test)]` and `#[test]` attributes

### Python code

- Use type annotations pervasively
- use ruff for formatting

### Build System and Configuration files:

- Keep `flake.nix` up-to-date with all necessary packages for development
- Follow `alejandra.toml` for configuration for nix code style (2-space indent)
- Keep `rust/Cargo.toml` and `pyproject.toml` up-to-date with relevant dependencies

### Documentation

- Follow `.mdformat.toml` for markdown style (100 column limit)
- Keep documentation up-to-date with all changes:
  - Remove documentation that is out-of-date
  - Document all new features
