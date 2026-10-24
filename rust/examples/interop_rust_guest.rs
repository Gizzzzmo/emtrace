//! Guest static library of the second cross-language interop demo (C host exe).
//!
//! Built as a `staticlib` (see the `[[example]]` entry in `Cargo.toml`), it is
//! linked into the C executable `c/examples/interop_c_host.c`, which hosts the
//! emtrace magic. This library deliberately has no `magic!()` of its own - its
//! `traceln!` calls expand to *references* to `g_emt_default_magic` and the
//! `emt_default_*` pipeline symbols, which stay undefined in the archive and are
//! resolved at the final C link against the executable's definitions. Records
//! from both languages flow through the C host's single sink (stdio on stdout).
//!
//! Note the flip side for real projects: if a Rust library *hosts* the magic
//! instead (as in `interop_rust_host.rs`) while a C executable owns `main`,
//! Rust's `lang_start` never runs - so the stdout buffer behind the default sink
//! gets no automatic flush at process exit and records would be lost unless the
//! host exposes an explicit flush hook.

use emtrace::traceln;

#[unsafe(no_mangle)]
pub extern "C" fn rust_side_trace(iteration: i32) {
    traceln!("Rust library: iteration {}", i32: iteration);
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_side_goodbye() {
    traceln!("Rust library: goodbye");
}
