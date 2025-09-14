use emtrace::{C_STYLE_FORMAT, Out, expect, magic_address_bytes, traceln};
use std::io::{Write, stdout};

fn main() {
    expect!(
        b"1.0 0.5 nan inf -inf\n\
          1.000000 0.500000 nan inf -inf\n"
    );

    stdout().lock().write_all(&magic_address_bytes()).unwrap();
    traceln!("{} {} {} {} {}", f32: 1.0, f32: 0.5, f32: f32::NAN, f32: f32::INFINITY, f32: f32::NEG_INFINITY);
    traceln!("%f %f %f %f %f", f32: 1.0, f32: 0.5, f32: f32::NAN, f32: f32::INFINITY, f32: f32::NEG_INFINITY, .formatter=C_STYLE_FORMAT);
}
