use emtrace::{Out, expect, init, traceln};

fn main() {
    expect!(
        "Mixed types: 42 3.140000104904175 True hello\n\
          Complex format: Value=100, Name=test, Active=False, Ratio=0.5\n\
          Numbers: \x01 \x02 \x03 \x04 \x05\n"
    );

    init(&mut std::io::stdout().lock());

    // Test mixing different types in one trace
    traceln!("Mixed types: {} {} {} {}",
        i32: 42,
        f32: 3.140000104904175f32,
        bool: true,
        str: "hello"
    );

    // Test more complex formatting
    traceln!("Complex format: Value={}, Name={}, Active={}, Ratio={}",
        i32: 100,
        str: "test",
        bool: false,
        f64: 0.5
    );

    // Test multiple same-type values (u8 shows as characters, not numbers)
    traceln!("Numbers: {} {} {} {} {}",
        u8: 1u8,
        u8: 2u8,
        u8: 3u8,
        u8: 4u8,
        u8: 5u8
    );
}
