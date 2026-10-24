# Emtrace - Rust

Emtrace is a deferred logging / tracing library that works in multiple
languages. This crate contains the rust version. The full project with
additional documentation can be found
[here](https://github.com/Gizzzzmo/emtrace).

## Basic Usage

Use the `trace` and `traceln` macros to output (by default via stdout) a byte
stream.

```rust
use emtrace::{init, magic, trace, traceln};

magic!(); // magic!
init(&mut std::io::stdout().lock());
traceln!("Hello World!");

let a = 1;
let b = 2;
trace!("{} + {} = {}", i32: a, i32: b, i32: a + b);
```

The byte stream, combined with the information in a special data section that is
created in the compiled binary, can then be post-processed with a
[python script](https://github.com/Gizzzzmo/emtrace/blob/main/parser/emtrace.py)
(install it from pypi with `pip install emtrace`) to obtain the intended log.
For more details take a look at the
[full project](https://github.com/Gizzzzmo/emtrace).

## Traceable Types

The types of the format arguments to the `trace` macro have to implement the
`Trace` trait. All primitive integers, floating point types, and strings are
supported out of the box:

```rust
use emtrace::{magic, traceln};

magic!(); // defines the binary's magic record and runtime symbols
traceln!("{} {} {} {}", u8: 0, i16: 9, f32: 3.14, str: "a string");
```

Vectors, arrays, and slices of anything that implements `Trace` also work:

```rust
use emtrace::{magic, traceln};

magic!();
let numbers = vec![1, 2, 3, 4, 5];
let strings = vec!["Hello", "there!"];

traceln!("{} {}", [i32]: numbers, [&str]: strings);
```
