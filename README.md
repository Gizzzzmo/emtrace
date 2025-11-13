# Emtrace

Super fast (benchmarks WIP), minimal overhead, tracing / logging library for multiple languages
(currently C/C++ and Rust). All processing of logged information (like formatting) is deferred; the
application only sends out those bytes that are relevant to what you are trying to log and that
aren't known at compile time. By default this data is written to stdout, but in general the output
channel is completely customizable.

> [!Note]
>
> I don't know if it works with binaries that run on Windows or Mac OS (although it might). More
> generally it only explicitly supports the ELF binary format right now.

## Usage

You will always need the emtrace python parser to process the output your binary produces. Either
directly use [the script from this repository](./parser/emtrace.py) or install it from pypi:

```bash
pip install emtrace
```

Now you can choose the language you want to use below. Once you have a compiled binary (regardless
of source language) you can run it, and pipe its output into `emtrace`, to which you also have to
supply the path to the same binary. E.g. something like this (assuming your executable is a.out):

```bash
./a.out | emtrace a.out
```

### In C

The library currently consists of [a single header](./c/include/c/include/emtrace/emtrace.h). Simply
download, and include it (works out of the gate with gcc, and clang).

```c
#include <emtrace/emtrace.h>

int main(void) {
    EMTRACE_INIT();
    EMTRACELN("Hello World!");

    int a = 1;
    int b = 2;
    EMTRACELN_F("{} + {} = {}", int, a, int, b, int, a + b);
}
```

### In Rust

> [!Note]
>
> If you *only* need this to work in Rust, and aren't planning on incorporating any other language,
> you should probably be using [demft](https://github.com/knurling-rs/defmt). It is more mature, and
> featureful. For more details see [below](#comparison).

Use `cargo add emtrace` in your project to get the latest version from [crates.io](crates.io).

```rust
use emtrace::{init, traceln};

fn main() {
    init(&mut std::io::stdout().lock());
    traceln!("Hello World!");

    let a = 1;
    let b = 2;
    traceln!("{} + {} = {}", i32: a, i32: b, i32: a + b);
}
```

## Format string syntax

By default the format string syntax uses (and also extends in some ways) python's
[`str.format`](https://docs.python.org/3/library/stdtypes.html#str.format) function which is similar
to rust's `print` and `println` functions.

## How it works

All information needed to display the trace is written, at compile time, to a dedicated section of
the output binary (`.emtrace` section by default, currently only supports ELF executables). Only
data that can change at runtime is actually (barely) processed then, and sent out along with a
pointer to associate it to the right piece of format information in this dedicated section. The
section can be completely removed (or made no-load) from the final binary (e.g. with objcopy), and
the program will still run.

The [post-processing script](./parser/emtrace/__init__.py) takes the data output by the program
while running and the data from the special `.emtrace` section, and produces the log.

## Development

If you want to tinker with the code, and need to figure out how to build, lint, and test it check
out [AGENTS.md](AGENTS.md) (also useful for some 🌈*vibe-coding*🌈). The easiest way to get a working
development environment is to use the provided [nix flake](./flake.nix).

## Comparison

There are a few other projects who do something smiliar. This table presents a comparison to those I
know, and have tried:

- [defmt](https://github.com/knurling-rs/defmt)

  - \+ more mature
  - \+ supports different kinds of encodings
  - \+ more efficient packing of format info into ELF (unfortunately I don't see how the mechanism
    they use could be workable in the C version)
  - \- always requires a custom linker script
  - \- only works in rust

- [trice](https://github.com/rokath/trice)

  - \+ more mature
  - \+ supports different kinds of encodings
  - \- preprocessing of source files is a required step
  - \- only works in C/C++

## TODO

- [x] add basic documentation
- [x] add license
- [ ] make rust's init function + magic_bytes variable into a macro to support different section
  names
- [ ] in the rust version, clarify / rework the role of the `total_size` given to the Sink's
  `begin`-function
- [x] complete adding support for tracing nested types (lists, tuples, etc.) on post-processing side
- [x] switch C tests from using `assert` to something else and combine all tests into a single
  executable
- [x] add usage examples to documentation
- [x] add download / install instructions to documentation
- [x] add basic tests
- [ ] add more detailed documentation
- [ ] split the above todo item into separate items for separate features
- [x] push to github
- [ ] add some basic benchmarks
- [ ] add CI pipeline (github actions?)
- [ ] add dedicated C++ implementation
- [x] add CMake profile-switching to justfile
- [x] add versioning information
- [x] publish rust crate to crates.io
- [x] add nix build recipe for parser
- [x] add nix build recipe for c version
- [ ] add nix build recipe for rust version
- [ ] publish to nixpkgs
- [x] make CMake version installable
- [ ] add conan recipe, and publish to conan center
- [ ] add compatibility with other binary formats (mach-o and pe)
- [ ] add COBS encoding support
- [ ] add interning support
- [ ] add sinks for common embedded transports (ARM SWO, RTT, etc.)
- [ ] add compound EMTRACE_F macros to C version where both object with known size, and
  variable-size ones can be logged in a single call
