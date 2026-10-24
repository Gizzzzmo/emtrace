//! Combined cross-language interop demo: ONE executable hosts the magic and
//! links BOTH guest libraries - the C static library
//! `c/examples/interop_c_guest.c` and the Rust staticlib
//! `rust/examples/interop_rust_guest.rs`. Neither library has a magic of its
//! own; all three artifacts' trace calls resolve the `emt_default_*` symbols
//! against this executable's `magic!()` definitions, so records from three
//! artifacts in two languages interleave through a single sink. This is the
//! pattern real projects use: any number of static libraries, one magic, hosted
//! next to the entry point.
//!
//! Build order as in `interop_rust_host.rs`; run everything with `just interop`.

use emtrace::{expect, init, traceln};

emtrace::magic!();

#[link(name = "interop_c_guest", kind = "static")]
unsafe extern "C" {
    fn c_side_trace(iteration: i32);
    fn c_side_goodbye();
}

#[link(name = "interop_rust_guest", kind = "static")]
unsafe extern "C" {
    fn rust_side_trace(iteration: i32);
    fn rust_side_goodbye();
}

fn main() {
    expect!(
        "C library: iteration 0\n\
         Rust library: iteration 0\n\
         Rust executable: iteration 0\n\
         C library: iteration 1\n\
         Rust library: iteration 1\n\
         Rust executable: iteration 1\n\
         C library: iteration 2\n\
         Rust library: iteration 2\n\
         Rust executable: iteration 2\n\
         C library: iteration 3\n\
         Rust library: iteration 3\n\
         Rust executable: iteration 3\n\
         C library: iteration 4\n\
         Rust library: iteration 4\n\
         Rust executable: iteration 4\n\
         C library: goodbye\n\
         Rust library: goodbye\n\
         Rust executable: goodbye\n"
    );

    init(&mut std::io::stdout().lock()).unwrap();

    const ITERATIONS: i32 = 5;

    for i in 0..ITERATIONS {
        unsafe { c_side_trace(i) };
        unsafe { rust_side_trace(i) };
        traceln!("Rust executable: iteration {}", i32: i);
    }
    unsafe { c_side_goodbye() };
    unsafe { rust_side_goodbye() };
    traceln!("Rust executable: goodbye");
}
