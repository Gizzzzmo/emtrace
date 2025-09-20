use emtrace::{Out, expect, init, traceln};

fn main() {
    expect!(
        b"Large integers:\n\
          i128: 170141183460469231731687303715884105727\n\
          u128: 340282366920938463463374607431768211455\n\
          Negative i128: -170141183460469231731687303715884105728\n"
    );

    init(&mut std::io::stdout().lock());

    traceln!("Large integers:");

    // Test 128-bit integers
    traceln!("i128: {}", i128: i128::MAX);
    traceln!("u128: {}", u128: u128::MAX);
    traceln!("Negative i128: {}", i128: i128::MIN);
}
