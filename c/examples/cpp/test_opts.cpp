#include "emtrace/emtrace.hpp"
#include <iostream>
#include <vector>

MTRACE_MAGIC();

using emt::opt_c_style_fmt;
using emt::opt_sink;
using emt::opt_throw;

auto main() -> int {
    emt::init();

    int x = 10;
    std::vector<unsigned char> sink{};
    std::array<int, 11> arr{};
    std::vector<char> vec{1, 2, 3};
    MTRACELN("asldkj {} {} {}", arr, vec, x, opt_throw);
    MTRACELN("asldkj {} {} {}", arr, vec, x, opt_throw, opt_sink(sink));
    MTRACELN("asldkj %d", x, opt_c_style_fmt, opt_sink(sink));
    for (unsigned char c : sink) {
        std::cout << c;
    }
}
