use emtrace::{emtrace_init, trace, traceln, C_STYLE_FORMAT};

fn main() {
    emtrace_init!().unwrap();

    trace!("{}", i32: 0);
    trace!("Yeeha {} {}\n", i32: 9, i32: 10);
    traceln!("Yeeha {}", i32: 9);
    traceln!("C style formatting: %i", i32: 9, .formatter=C_STYLE_FORMAT);
    traceln!("No formatting: {} %i")
}
