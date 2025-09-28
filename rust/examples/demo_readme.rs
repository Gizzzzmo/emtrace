use emtrace::{init, traceln};

fn main() {
    init(&mut std::io::stdout().lock()).unwrap();
    traceln!("Hello World!");

    let a = 1;
    let b = 2;
    traceln!("{} + {} = {}", i32: a, i32: b, i32: a + b);
}
