//! Host executable of the first cross-language interop demo (C guest library).
//!
//! This program owns the entry point AND the emtrace magic: the `magic!()`
//! expansion below defines `g_emt_default_magic` and the `emt_default_*`
//! pipeline symbols, and `init` emits the single MGIC record through the
//! default sink (stdout via std).
//!
//! The static C library `c/examples/interop_c_guest.c` is linked in as a
//! *guest*: it has no `EMTRACE_MAGIC` of its own, so its `EMTRACE` calls resolve
//! those symbols at link time against this executable's definitions - records
//! from both languages flow through one sink. This is the pattern to use in
//! real projects: every static library (in either language) is a guest of the
//! executable that hosts the magic next to its entry point.
//!
//! Build order: configure and build the C side first (`just conf && just cb`);
//! `rust/build.rs` then supplies the link search path for this example. The
//! whole demo is built and run by `just interop`.

use emtrace::{expect, init, traceln};

emtrace::magic!();

#[link(name = "interop_c_guest", kind = "static")]
unsafe extern "C" {
    fn c_side_trace(iteration: i32);
    fn c_side_goodbye();
}

fn main() {
    expect!(
        "C library: iteration 0\n\
         Rust executable: iteration 0\n\
         C library: iteration 1\n\
         Rust executable: iteration 1\n\
         C library: iteration 2\n\
         Rust executable: iteration 2\n\
         C library: iteration 3\n\
         Rust executable: iteration 3\n\
         C library: iteration 4\n\
         Rust executable: iteration 4\n\
         C library: goodbye\n\
         Rust executable: goodbye\n"
    );

    init(&mut std::io::stdout().lock()).unwrap();

    const ITERATIONS: i32 = 5;

    for i in 0..ITERATIONS {
        unsafe { c_side_trace(i) };
        traceln!("Rust executable: iteration {}", i32: i);
    }
    unsafe { c_side_goodbye() };
    traceln!("Rust executable: goodbye");
}
