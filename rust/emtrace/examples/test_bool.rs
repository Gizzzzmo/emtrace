use emtrace::{C_STYLE_FORMAT, emtrace_init, expect, trace};

fn main() {
    expect!("True FalseTrue False");

    emtrace_init!().unwrap();
    trace!("{} {}", bool: true, bool: false);
    trace!("%r %r", bool: true, bool: false, .formatter=C_STYLE_FORMAT);
}
