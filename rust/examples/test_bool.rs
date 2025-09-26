use emtrace::{C_STYLE_FORMAT, Out, expect, init, trace};

fn main() {
    expect!("True FalseTrue False");

    init(&mut std::io::stdout().lock());
    trace!("{} {}", bool: true, bool: false);
    trace!("%r %r", bool: true, bool: false, .formatter=C_STYLE_FORMAT);
}
