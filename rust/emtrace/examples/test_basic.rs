use emtrace::{emtrace_init, expect, trace, traceln};

fn main() {
    expect!(
        "Hello from the basic test!\n\
         An integer: 42\n\
         A string: a string\n"
    );
    emtrace_init!().unwrap();

    traceln!("Hello from the basic test!");
    let x: i32 = 42;
    traceln!("An integer: {}", i32: x);
    let s: &str = "a string";
    trace!("A string: ");
    traceln!("{}", str: s);
    // traceln!("A list: {}", [i32]: vec![1, 2, 3, 4]);
}
