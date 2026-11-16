#include "test_utils.h"
#include <emtrace/emtrace.hpp>
#include <iostream>
#include <vector>

// Instead of a sink, MTRACE_MAGIC can also be given a factory: a function without parameters that
// returns the sink (or a reference to it). It is evaluated for every record. This makes it
// possible to use sinks that cannot be constructed at compile time, e.g. because they need to be
// created lazily. The lambda is converted to a function pointer, so it can't capture anything.

EXPECT_OUTPUT(
    "factory 1\n"
    "factory 2\n"
);

namespace {
auto capture() -> std::vector<unsigned char>& {
    static std::vector<unsigned char> instance;
    return instance;
}
} // namespace

MTRACE_MAGIC(capture);

auto main() -> int {
    emt::init();

    MTRACELN("factory {}", 1);
    MTRACELN("factory {}", 2);

    std::cout.write(
        reinterpret_cast<const char*>(capture().data()),
        static_cast<std::streamsize>(capture().size())
    );

    return 0;
}
