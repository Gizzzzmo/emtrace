//! End-to-end test for `magic!`'s `.section` option (C parity with overriding
//! `EMT_DEFAULT_SEC_ATTR`): the MGIC record and every record's `TRCE` static land in
//! the custom `.emtsec` section instead of `.emtrace`, and the parser finds them via
//! `emtrace --section-name .emtsec` (see tests/test_emtrace.py::test_custom_section).
//!
//! Every `traceln!` passes the matching `.section` - the parser resolves record
//! pointers as offsets within one section, so a record static in a different section
//! than the MGIC record would break decoding.
use emtrace::{expect, init, traceln};

fn main() {
    expect!(
        "Custom section works: 7\n\
         Custom section works: done\n"
    );
    emtrace::magic!(.section = ".emtsec");

    init(&mut std::io::stdout().lock()).unwrap();

    traceln!("Custom section works: {}", i32: 7, .section = ".emtsec");
    traceln!("Custom section works: done", .section = ".emtsec");
}
