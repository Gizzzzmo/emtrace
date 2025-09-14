use emtrace::{Out, expect, magic_address_bytes, trace, traceln};
use std::io::{Write, stdout};

fn main() {
    expect!(
        b"Hello, World!\n\
          Empty string: \n\
          Unicode: \xF0\x9F\x98\x80 \xE2\x9C\x85 \xC2\xA9\n\
          Multiple strings: first second third\n"
    );

    stdout().lock().write_all(&magic_address_bytes()).unwrap();

    // Test basic string
    traceln!("{}", &str: "Hello, World!");

    // Test empty string
    trace!("Empty string: ");
    traceln!("{}", &str: "");

    // Test unicode strings
    traceln!("Unicode: {} {} {}",
        &str: "😀",
        &str: "✅",
        &str: "©"
    );

    // Test multiple strings
    traceln!("Multiple strings: {} {} {}",
        &str: "first",
        &str: "second",
        &str: "third"
    );
}
