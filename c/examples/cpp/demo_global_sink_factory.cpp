#include <emtrace/emtrace.hpp>
#include <iostream>

MTRACE_MAGIC([]() -> std::ostream& { return std::cout; });

auto main() -> int {
    emt::init();

    MTRACE("factory {}", 10);

    return 0;
}
