#include <emtrace/emtrace.h>
#include <emtrace/emtrace.hpp>

EMTRACE_MAGIC(3);

auto main() -> int {
    emtrace_init();
    MTRACELN("Hello World!");

    int a = 1;
    int b = 2;
    MTRACELN("{} + {} = {}", a, b, a + b);
}
