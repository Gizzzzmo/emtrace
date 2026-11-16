// Demonstrates COBS encoding of the trace stream.
//
// With emt::Cobs as the global sink every record is framed as a COBS packet, which means that it
// contains no zero bytes, and is terminated by a single zero byte. This makes it possible to
// find the beginning of the next record in a stream after data got lost, e.g. on a serial line.
// The sink that is wrapped (here the default, which writes to stdout) does the actual output.
//
// The MGIC record advertises the encoding, so the parser decodes it automatically:
// `emtrace run ./demo_cobs_cpp`.
#include <emtrace/emtrace.hpp>

MTRACE_MAGIC(emt::Cobs{});

auto main() -> int {
    emt::init();

    MTRACELN("Hello from COBS C++!");

    int a = 2;
    int b = 3;
    MTRACELN("{} + {} = {}", a, b, a + b);

    return 0;
}
