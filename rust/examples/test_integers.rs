use emtrace::{Out, expect, init, traceln};

fn main() {
    expect!(
        "Signed integers: -128 -32768 -2147483648 -9223372036854775808\n\
          Unsigned integers: 255 65535 4294967295 18446744073709551615\n\
          Size integers: 42 -42\n"
    );

    init(&mut std::io::stdout().lock());

    // Test various signed integer types
    traceln!("Signed integers: {:d} {} {} {}",
        i8: -128i8,
        i16: -32768i16,
        i32: -2147483648i32,
        i64: -9223372036854775808i64
    );

    // Test various unsigned integer types (u8 gets displayed as char)
    traceln!("Unsigned integers: {:d} {} {} {}",
        u8: 255u8,
        u16: 65535u16,
        u32: 4294967295u32,
        u64: 18446744073709551615u64
    );

    // Test size types
    traceln!("Size integers: {} {}",
        usize: 42usize,
        isize: -42isize
    );
}
