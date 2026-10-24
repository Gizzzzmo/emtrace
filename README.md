# Emtrace

Super fast (benchmarks WIP), minimal overhead, tracing / logging library for
multiple languages (currently C/C++ and Rust). All processing of logged
information (like formatting) is deferred; the application only sends out those
bytes that are relevant to what you are trying to log and that aren't known at
compile time. By default this data is written to stdout, but in general the
output channel is completely customizable.

> [!Note]
>
> gcc and clang should work out of the box with [the C implementation](#in-c).
> ELF Executables (Linux, and most embedded software) have first-class support
> from [the parser](./parser/). Windows PE executables (tested with clang+msvc)
> are also known to work. On MacOS nothing has been tested.

## Usage

You will always need the emtrace python parser to process the output your binary
produces. Either directly use
[the script from this repository](./parser/emtrace.py) or install it from pypi:

```bash
pip install emtrace
```

Now you can choose the language you want to use below (either [C/C++](#in-c) or
[Rust](#in-rust)). Once you have a compiled binary (regardless of source
language) you can run it, and pipe its output into `emtrace run`, to which you
also have to supply the path to the same binary. E.g. something like this
(assuming your executable is a.out):

```bash
emtrace run a.out
```

`emtrace` also has `check`, `export`, and `inspect` subcommands for validating
and examining the emtrace data of binaries, object files, and static archives
(see `emtrace --help`).

### In C

The library currently consists of
[a single header](./c/include/c/include/emtrace/emtrace.h). Simply download, and
include it (works out of the gate with gcc, and clang).

```c
#include <emtrace/emtrace.h>

EMTRACE_MAGIC(unsigned);

int main(void) {
    emtrace_init();
    EMTRACELN("Hello World!");

    int a = 1;
    int b = 2;
    EMTRACELN_F("{} + {} = {}", VAL(int, a), VAL(int, b), VAL(int, a + b));
}
```

### In Rust

> [!Note]
>
> If you *only* need this to work in Rust, and aren't planning on incorporating
> any other language, you should probably be using
> [demft](https://github.com/knurling-rs/defmt). It is more mature, and
> featureful. For more details see [below](#comparison).

Use `cargo add emtrace` in your project to get the latest version from
[crates.io](crates.io).

```rust
use emtrace::{init, traceln};

fn main() {
    magic!(); // magic!
    init(&mut std::io::stdout().lock()).unwrap();
    traceln!("Hello World!");

    let a = 1;
    let b = 2;
    traceln!("{} + {} = {}", i32: a, i32: b, i32: a + b);
}
```

### Postprocessing Executables

The `strip` and `nonload` commands can manipulate linked binaries (`strip` works
on ELF, PE, and Mach-O; `nonload` is ELF-only):

```bash
emtrace strip a.out     # remove the .emtrace section completely
emtrace nonload a.out   # mark it non-loadable, like an (INFO) linker script section
```

Marking the `.emtrace` section of an ELF file nonload is mainly useful for
embedded programs, where usually these sections are omitted when flashing the
binary to a microcontroller, or converting it to a hex or binary file with tools
like `objcopy`. Since the section's content is only relevant for decoding the
raw output, and has no purpose at runtime this is safe to do.

Removing it completely, however, means the output can't be directly decoded
anymore. This is why there is also an `export` command that allows you to
extract the information present in the section, and save it in json form which,
in turn, emtrace can also consume to decode the program's output:

```bash
emtrace export a.out > trace_data.json
emtrace strip a.out
./a.out | emtrace run trace_data.json -i stdin
```

`-i stdin` tells emtrace not to run the given file `trace_data.json` directly as
an executable, but instead read the raw trace output of the program from stdin,
and use the json file only for decoding purposes. If a.out hadn't been stripped
one could have just as well supplied the original file, since emtrace
auto-detects the file format:

```bash
./a.out | emtrace run a.out -i stdin
```

Beyond stdin emtrace can also read from files, and sockets:

```bash
emtrace export a.out > trace_data.json
./a.out > raw_trace
emtrace run trace_data.json -i raw_trace

# assuming tracing_server is a program listening for tcp connections,
# and sending trace data to clients when connecting
./tracing_server &
emtrace run tracing_server -i 127.0.0.1:8000
```

### As a Python library

The `emtrace` package is also usable as a pure-Python library — the CLI in
`emtrace.cli` is just a thin wrapper around it:

```python
from pathlib import Path
from emtrace.engine import emtrace

path_to_exe = Path("/path/to/a.out")
emtrace(path_to_exe)
```

## Format string syntax

By default the format string syntax uses (and also extends in some ways)
python's
[`str.format`](https://docs.python.org/3/library/stdtypes.html#str.format)
function which is similar to rust's `print` and `println` functions.

## How it works

All information needed to display the trace is written, at compile time, to a
dedicated section of the output binary (`.emtrace` section by default). Only
data that can change at runtime is actually (barely) processed then, and sent
out along with a pointer to associate it to the right piece of format
information in this dedicated section. The section can be completely removed (or
made no-load) from the final binary (e.g. with objcopy), and the program will
still run.

The [parser](./emtrace) takes the data output by the program while running and
the data from the special `.emtrace` section, and produces the log.

## Development

If you want to tinker with the code, and need to figure out how to build, lint,
and test it check out [AGENTS.md](AGENTS.md) (also useful for some
🌈*vibe-coding*🌈). The easiest way to get a working development environment is to
use the provided [nix flake](./flake.nix).

## Comparison

There are a few other projects who do something smiliar. This table presents a
comparison to those I know, and have tried:

- [defmt](https://github.com/knurling-rs/defmt)

  - \+ more mature
  - \+ more efficient packing of format info into ELF (unfortunately I don't see
    how the mechanism they use could be workable in the C version)
  - \- always requires a custom linker script
  - \- only works in rust

- [trice](https://github.com/rokath/trice)

  - \+ more mature
  - \- only works in C/C++

## TODO

- [x] add basic documentation
- [x] add license
- [x] make rust's magic_bytes variable into a macro to support different section
  names
- [x] in the rust version, clarify / rework the role of the `total_size` given
  to the Sink's `begin`-function
- [x] complete adding support for tracing nested types (lists, tuples, etc.) on
  post-processing side
- [x] switch C tests from using `assert` to something else and combine all tests
  into a single executable
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
- [ ] add testing for compatibility with other binary formats (macho and pe)
- [x] add COBS encoding support
- [ ] add interning support
- [ ] add sinks for common embedded transports (ARM SWO, RTT, etc.)
- [x] add compound EMTRACE_F macros to C version where both object with known
  size, and variable-size ones can be logged in a single call
- [ ] add rust proc macro the deduce `Trace` trait on types whose fields' types
  all implement `Trace` already
- [ ] ? maybe refactor rust version to use proc macros instead of macro_rules! ?
