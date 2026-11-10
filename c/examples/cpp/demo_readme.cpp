#include <emtrace/emtrace.hpp>

MTRACE_MAGIC();

auto main() -> int {
    emt::init();
    MTRACELN("Hello World!");

    int a = 1;
    int b = 2;
    MTRACELN("{} + {} = {}", a, b, a + b);
}
