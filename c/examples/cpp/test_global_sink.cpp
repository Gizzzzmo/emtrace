#include "test_utils.h"
#include <emtrace/emtrace.hpp>
#include <iostream>
#include <vector>

// MTRACE_MAGIC takes the sink that is used by every MTRACE call without emt::opt_sink. A sink
// passed by value, like the vector here, is stored in a global variable, which is accessible via
// emt::GlobalSink<SinkType>::inner_sink.
//
// Because the first record emt::init() writes also goes to the global sink, nothing reaches stdout
// until we write out what has been collected.

EXPECT_OUTPUT(
    "collected 1\n"
    "collected 2\n"
    "sent directly to stdout 3\n"
);

using Capture = std::vector<unsigned char>;
MTRACE_MAGIC(Capture{});

auto main() -> int {
    emt::init();

    MTRACELN("collected {}", 1);
    MTRACELN("collected {}", 2);

    const Capture& capture = emt::GlobalSink<Capture>::inner_sink;
    std::cout.write(
        reinterpret_cast<const char*>(capture.data()), static_cast<std::streamsize>(capture.size())
    );

    // emt::opt_sink still overrides the global sink for a single call
    MTRACELN("sent directly to stdout {}", 3, emt::opt_sink(std::cout));

    return 0;
}
