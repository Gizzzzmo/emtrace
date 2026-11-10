#include "test_utils.h"
#include <emtrace/emtrace.hpp>

EXPECT_OUTPUT(
    "Hello from the basic test!\n"
    "An integer: 42\n"
    "A string: a string\n"
);

MTRACE_MAGIC(CPP_TEST_SINK);

auto main() -> int {
    emt::init();

    MTRACELN("Hello from the basic test!");
    int x = 42;
    MTRACELN("An integer: {}", x);
    const char* s = "a string";
    MTRACELN("A string: {}", s);
    return 0;
}
