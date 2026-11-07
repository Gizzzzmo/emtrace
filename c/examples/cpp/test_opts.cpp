#include "emtrace/emtrace.h"
#include "emtrace/emtrace.hpp"
#include <iostream>

EMTRACE_MAGIC(3);

using emt::opt_c_style_fmt;
using emt::opt_sink;
using emt::opt_throw;

auto main() -> int {
    emtrace_init();

    int x = 10;
    emt::VecSink v_sink{};
    std::array<int, 11> arr{};
    std::vector<char> vec{1, 2, 3};
    MTRACELN("asldkj {} {} {}", arr, vec, x, opt_throw);
    MTRACELN("asldkj {} {} {}", arr, vec, x, opt_throw, opt_sink(v_sink));
    MTRACELN("asldkj %d", x, opt_c_style_fmt, opt_sink(v_sink));
    for (unsigned char c : v_sink.buffer) {
        std::cout << c;
    }
}
