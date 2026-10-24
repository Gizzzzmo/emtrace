use emtrace::{CobsConsumer, expect, init, magic, traceln};

fn main() {
    expect!("Hello from COBS Rust!\n2 + 3 = 5\n");
    // COBS-encoded pipeline: `magic!` wraps a `CobsConsumer` around stdout, and the
    // magic record advertises `EMT_ENCODING_COBS`. The initialization frame must be
    // COBS-encoded as well, so hand `init` the same kind of sink.
    magic!(.cobs);

    let mut sink = CobsConsumer::new(std::io::stdout().lock());
    init(&mut sink).unwrap();
    drop(sink);

    traceln!("Hello from COBS Rust!");
    traceln!("{} + {} = {}", i32: 2, i32: 3, i32: 5);
}
