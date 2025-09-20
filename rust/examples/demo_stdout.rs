use emtrace::{C_STYLE_FORMAT, Out, init, trace, traceln};

fn main() {
    init(&mut std::io::stdout().lock());

    trace!("{}", i32: 0);
    trace!("Yeeha {} {}\n", i32: 9, i32: 10);
    traceln!("Yeeha {}", i32: 9);
    traceln!("C style formatting: %i", i32: 9, .formatter=C_STYLE_FORMAT);
    traceln!("No formatting: {} %i")
}
