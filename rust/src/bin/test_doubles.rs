use emtrace::{C_STYLE_FORMAT, Out, expect, magic_address_bytes, traceln};
use std::io::{Write, stdout};

fn main() {
    expect!(
        b"Doubles: 3.14159 2.71828 nan inf -inf\n\
          3.141590 2.718280 nan inf -inf\n\
          Extreme values: 1.7976931348623157e+308 2.2250738585072014e-308\n"
    );

    stdout().lock().write_all(&magic_address_bytes()).unwrap();

    // Test basic double values (use exact values to avoid clippy warnings)
    traceln!("Doubles: {} {} {} {} {}",
        f64: std::f64::consts::PI,
        f64: std::f64::consts::E,
        f64: f64::NAN,
        f64: f64::INFINITY,
        f64: f64::NEG_INFINITY
    );

    // Test C-style formatting
    traceln!("%f %f %f %f %f",
        f64: std::f64::consts::PI,
        f64: std::f64::consts::E,
        f64: f64::NAN,
        f64: f64::INFINITY,
        f64: f64::NEG_INFINITY,
        .formatter=C_STYLE_FORMAT
    );

    // Test extreme values
    traceln!("Extreme values: {} {}",
        f64: f64::MAX,
        f64: f64::MIN_POSITIVE
    );
}
