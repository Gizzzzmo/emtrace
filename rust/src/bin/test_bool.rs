use emtrace::{C_STYLE_FORMAT, Out, expect, magic_address_bytes, trace};
use std::io::{Write, stdout};

fn main() {
    expect!(b"True FalseTrue False");

    stdout().lock().write_all(&magic_address_bytes()).unwrap();
    trace!("{} {}", bool: true, bool: false);
    trace!("%r %r", bool: true, bool: false, .formatter=C_STYLE_FORMAT);
}
