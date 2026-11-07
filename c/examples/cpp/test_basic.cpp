#include "test_utils.h"
#include <emtrace/emtrace.h>
#include <emtrace/emtrace.hpp>

EXPECT_OUTPUT(
    "Hello from the basic test!\n"
    "An integer: 42\n"
    "A string: a string\n"
);

#ifdef TEST_COBS
EMTRACE_MAGIC_COBS(3);
#else
EMTRACE_MAGIC(3);
#endif

auto main() -> int {
    emtrace_init();
    MTRACELN("Hello from the basic test!");
    int x = 42;
    MTRACELN("An integer: {}", x);
    const char* s = "a string";
    MTRACELN("A string: {}", s);
    return 0;
}
