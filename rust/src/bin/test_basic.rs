use emtrace::{Out, magic_address_bytes, trace, traceln};
use std::io::{Write, stdout};

#[unsafe(link_section = ".emtrace.test.expected")]
static EXPECTED_OUTPUT: [u8; 61] =
    *b"Hello from the basic test!\nAn integer: 42\nA string: a string\n";

fn main() {
    // Prevent the linker from optimizing away the EXPECTED_OUTPUT section.
    let _ = unsafe { std::ptr::read_volatile(&EXPECTED_OUTPUT) };

    stdout().lock().write_all(&magic_address_bytes()).unwrap();
    traceln!("Hello from the basic test!");
    let x: i32 = 42;
    traceln!("An integer: {}", i32: x);
    let s: &str = "a string";
    trace!("A string: ");
    traceln!("{}", &str: s);
}
