use emtrace::{expect, init, trace, traceln};

fn main() {
    expect!(
        "Hello from the basic test!\n\
         An integer: 42\n\
         A string: a string\n"
    );
    init(&mut std::io::stdout().lock()).unwrap();

    traceln!("Hello from the basic test!");
    let x: i32 = 42;
    traceln!("An integer: {}", i32: x);
    let s: &str = "a string";
    trace!("A string: ");
    traceln!("{}", str: s);
    // traceln!("A list: {}", [i32]: vec![1, 2, 3, 4]);
}
