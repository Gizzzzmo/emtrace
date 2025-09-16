use emtrace::{Out, expect, magic_address_bytes, trace, traceln};
use std::io::{Write, stdout};

fn main() {
    expect!(
        b"Hello from the basic test!\n\
          An integer: 42\n\
          A string: a string\n"
    );

    stdout().lock().write_all(&magic_address_bytes()).unwrap();
    traceln!("Hello from the basic test!");
    let x: i32 = 42;
    traceln!("An integer: {}", i32: x);
    let s: &str = "a string";
    trace!("A string: ");
    traceln!("{}", str: *s);
}
