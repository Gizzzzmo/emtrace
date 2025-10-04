# Emtrace - Rust

Emtrace is a deferred logging / tracing library that works in multiple languages. This crate
contains the rust version. The full project with additional documentation can be found
[here](https://github.com/Gizzzzmo/emtrace).

## Basic Usage

Use the `trace` and `traceln` macros to output (by default via stdout) a byte stream.

```rust
use emtrace::{init, trace, traceln};

fn main() {
    init(&mut std::io::stdout().lock());
    traceln!("Hello World!");

    let a = 1;
    let b = 2;
    trace!("{} + {} = {}", i32: a, i32: b, i32: a + b);
}
```

The byte stream, combined with the information in a special data section that is created in the
compiled binary, can then be post-processed with a
[python script](https://github.com/Gizzzzmo/emtrace/blob/main/parser/emtrace.py) (install it from
pypi with `pip install emtrace`) to obtain the intended log. For more details take a look at the
[full project](https://github.com/Gizzzzmo/emtrace).

## Traceable Types

The types of the format arguments to the `trace` macro have to implement the `Trace` trait. All
primitive integers, floating point types, and strings are supported out of the box:

```rust
use emtrace::traceln;

traceln!("{} {} {} {}", u8: 0, i16: 9, f32: 3.14, str: "a string");
```

Vectors, arrays, and slices of anything that implements `Trace` also work:

```rust
use emtrace::traceln;

let numbers = vec![1, 2, 3, 4, 5];
let strings = vec!["Hello", "there!"];

traceln!("{} {}", [i32]: numbers, [&str]: strings);
```

## Features

The crate features allow you to set:

- Which unsigned type is used to encode the pointer that is part of every emitted trace. The smaller
  you make it the less data overhead each trace emits. However it also limits how many distinct
  traces you can have in your program (since each has to be uniquely identified by a pointer).

  - `ptr_u128`
  - `ptr_u64`
  - `ptr_u32`
  - `ptr_u16`
  - `ptr_u8`

- Which unsigned type is used to encode the size of objects - both those stored in the `.emtrace`
  section, and those that are sent out as part of a trace that contains variable size data.

  - `size_u128`
  - `size_u64`
  - `size_u32`
  - `size_u16`
  - `size_u8`

- to what power of two the different pieces of format info in the `.emtrace`-section are aligned.
  The pointers to the format info that are emitted with a trace are also divided by this power of
  two.

  - `alignment_power_0 `
  - `alignment_power_1 `
  - `alignment_power_2 `
  - `alignment_power_3 `
  - `alignment_power_4 `
  - `alignment_power_5 `
  - `alignment_power_6 `
  - `alignment_power_7 `
  - `alignment_power_8 `
  - `alignment_power_9 `
  - `alignment_power_10 `

Since cargo only allows boolean feature flags there is one for every type, and one for every
alignment up to two to the ten. When the ptr or size feature is active for two different sizes, then
the larger one wins. When the alignment power feature is active for two different alignments then
the smaller alignment wins.
