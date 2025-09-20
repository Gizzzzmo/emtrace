# Emtrace

Super fast (benchmarks WIP), minimal overhead, tracing / logging library for multiple languages
(currently C/C++ and Rust). All processing of logged information (like formatting) is deferred; the
application only sends out those bytes that are relevant to what you are trying to log and that
aren't known at compile time. By default this data is written to stdout, but in general the output
channel is completely customizable.

> [!Note]
>
> Does not currently work with binaries that run on Windows, or Mac OS. More generally it only
> supports the ELF binary format.

In C:

```c
#include <emtrace/emtrace.h>

int main() {
    EMTRACE_INIT();
    EMTRACELN("Hello World!");

    int a = 1;
    int b = 2;
    EMTRACELN("{} + {} = {}", int, a, int, b, int, a + b);
}
```

In Rust:

```rust
use emtrace::{Out, init, traceln};

fn main() {
    init(&mut std::io::stdout().lock());
    traceln!("Hello World!");

    let a = 1;
    let b = 2;
    traceln!("{} + {} = {}", i32: a, i32: b, i32: a + b);
}
```

Designed for low-latency, deeply-embedded, as well as bandwidth-constrained applications.

## How it works

All information needed to display the trace is written, at compile time, to a dedicated section of
the output binary (`.emtrace` section by default, currently only supports ELF executables). Only
data that can change at runtime is actually (barely) processed then, and sent out along with a
pointer to associate it to the right piece of format information in this dedicated section. The
section can be completely removed (or made no-load) from the final binary (e.g. with objcopy), and
the program will still run.

The [post-processing script](./emtrace.py) takes the data output by the program while running and
the data from the special `.emtrace` section, and produces the log.

## TODO

- [x] add basic documentation
- [x] add license
- [ ] make rust's init function + magic_bytes variable into a macro to support different section
  names
- [ ] complete adding support for tracing nested types (lists, tuples, etc.) on post-processing side
- [ ] switch C tests from using `assert` to something else and combine all tests into a single
  executable
- [x] add usage examples to documentation
- [ ] add download / install instructions to documentation
- [x] add basic tests
- [ ] add more detailed documentation
- [ ] split the above todo item into separate items for separate features
- [x] push to github
- [ ] add some basic benchmarks
- [ ] add CI pipeline (github actions?)
- [ ] add dedicated C++ implementation
- [ ] add CMake profile-switching to justfile
- [ ] add versioning information
- [ ] publish rust crate to crates.io
- [ ] add nix build recipe and publish to nixpkgs
- [ ] make CMake version installable, add conan recipe, and publish to conan center
- [ ] add compatibility with other binary formats (mach-o and pe)
