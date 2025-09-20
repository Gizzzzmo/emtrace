use emtrace::{C_STYLE_FORMAT, Out, expect, init, trace, traceln};

fn main() {
    expect!(
        b"Edge cases:\n\
          Zero values: 0 0.0 False \n\
          Extremes: 255 -128 4294967295 -2147483648\n\
          No format\n\
          C format: test\n"
    );

    init(&mut std::io::stdout().lock());

    traceln!("Edge cases:");

    // Test zero values
    traceln!("Zero values: {} {} {} {}",
        i32: 0,
        f32: 0.0,
        bool: false,
        str: ""
    );

    // Test extreme values (u8 shows as character)
    traceln!("Extremes: {:d} {:d} {} {}",
        u8: u8::MAX,
        i8: i8::MIN,
        u32: u32::MAX,
        i32: i32::MIN
    );

    // Test trace without format arguments (NO_FORMAT)
    trace!("No format");
    traceln!("");

    // Test C-style format
    traceln!("C format: %s", str: "test", .formatter=C_STYLE_FORMAT);
}
