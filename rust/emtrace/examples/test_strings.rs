use emtrace::{expect, emtrace_init, trace, traceln};

fn main() {
    expect!(
        "Hello, World!\n\
         Empty string: \n\
         Unicode: 😀 ✅ ©\n\
         Multiple strings: first second third\n"
    );

    emtrace_init!().unwrap();

    // Test basic string
    traceln!("{}", str: "Hello, World!");

    // Test empty string
    trace!("Empty string: ");
    traceln!("{}", str: "");

    // Test unicode strings
    traceln!("Unicode: {} {} {}",
        str: "😀",
        str: "✅",
        str: "©"
    );

    // Test multiple strings
    traceln!("Multiple strings: {} {} {}",
        str: "first",
        str: "second",
        str: "third"
    );
}
