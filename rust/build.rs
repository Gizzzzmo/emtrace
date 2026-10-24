//! Supplies the link search path for the cross-language interop demos.
//!
//! `examples/interop_rust_host.rs` and `examples/interop_combined.rs` link the
//! static C library built by the CMake build (`c/examples/interop_c_guest.c`)
//! through their `#[link]` attributes; this build script only adds the matching
//! `-L native` search path, so plain `cargo build` works once the C side has
//! been configured at least once (`just conf && just cb` records the current
//! build directory in `c/build/.build_dir`). Nothing is emitted while that file
//! is missing, so every other target builds and links unaffected.

use std::env;
use std::fs;
use std::path::PathBuf;

fn main() {
    let manifest_dir = PathBuf::from(env::var("CARGO_MANIFEST_DIR").expect("cargo sets it"));
    let build_dir_file = manifest_dir.join("../c/build/.build_dir");
    println!("cargo:rerun-if-changed={}", build_dir_file.display());

    let Ok(build_dir) = fs::read_to_string(&build_dir_file) else {
        return;
    };
    let build_dir = build_dir.trim();
    if build_dir.is_empty() {
        return;
    }
    // CMake places target artifacts (archives and executables alike) in the
    // `examples/` subdirectory of the build directory.
    println!("cargo:rustc-link-search=native={build_dir}/examples");
    let lib = PathBuf::from(build_dir).join("examples/libinterop_c_guest.a");
    if lib.exists() {
        // Relink the guest-linking executables when the C library is rebuilt.
        println!("cargo:rerun-if-changed={}", lib.display());
    }

    // `examples/interop_combined.rs` also links the *Rust* guest staticlib
    // (`examples/interop_rust_guest.rs`, `crate-type = ["staticlib"]`). rustc
    // does not search the examples output directory by itself, so add it -
    // derived from OUT_DIR, which follows the active target directory and
    // profile. Cargo has no dependency edge between examples, so the justfile
    // builds the staticlib before the executable that links it.
    let out_dir = PathBuf::from(env::var("OUT_DIR").expect("cargo sets it"));
    let examples_dir = out_dir
        .ancestors()
        .nth(3)
        .expect("OUT_DIR layout: target/<profile>/build/<pkg>/out")
        .join("examples");
    println!("cargo:rustc-link-search=native={}", examples_dir.display());
    let rust_lib = examples_dir.join("libinterop_rust_guest.a");
    if rust_lib.exists() {
        // Relink the combined executable when the Rust guest library is rebuilt.
        println!("cargo:rerun-if-changed={}", rust_lib.display());
    }
}
