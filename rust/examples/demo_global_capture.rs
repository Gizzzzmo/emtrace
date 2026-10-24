//! Demonstrates a program-lifetime global sink for the `magic!` runtime pipeline.
//!
//! `magic!` expands at item position, so it cannot receive a runtime-constructed
//! sink value. Instead, the long-lived sink state lives in a global, and the
//! `.sink_guard` expression *borrows* it once per trace record through a guard: the
//! guard owns the per-record lock span (constructed in `emt_default_begin`, dropped
//! in `emt_default_finish`), exactly like the default stdout pipeline's
//! `std::io::stdout().lock()`.
//!
//! Run with `cargo run --example demo_global_capture`.

use emtrace::{magic, traceln};
use std::io::{Write, stdout};
use std::sync::{Mutex, MutexGuard};

// The long-lived sink state: program-lifetime, owned by this global.
static CAPTURE: Mutex<Vec<u8>> = Mutex::new(Vec::new());

// The MGIC record plus the `emt_default_*` symbol implementations: the binary's
// host. Every trace runs through the global capture buffer, and each record locks
// it for its whole span (lock held from `emt_default_begin` to `emt_default_finish`,
// via the guard's `Drop`).
magic!(
    .sink_guard: MutexGuard<'static, Vec<u8>> = CAPTURE.lock().unwrap()
);

fn main() {
    traceln!("Hello {}!", i32: 1);
    traceln!("Welcome {} {} {}", &str: "to the global sink", i32: 2, f64: 3.5);

    stdout().lock().write_all(&CAPTURE.lock().unwrap()).unwrap();
}
