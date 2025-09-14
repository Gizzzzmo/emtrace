use emtrace::{C_STYLE_FORMAT, Out, magic_address_bytes, trace, traceln};
use std::io::Write;

fn main() {
    std::io::stdout()
        .lock()
        .write_all(&magic_address_bytes())
        .unwrap();
    trace!("{}", i32: 0);
    trace!("Yeeha {} {}\n", i32: 9, i32: 10);
    traceln!("Yeeha {}", i32: 9);
    traceln!("C style formatting: %i", i32: 9, .formatter=C_STYLE_FORMAT);
    traceln!("No formatting: {} %i")
}
