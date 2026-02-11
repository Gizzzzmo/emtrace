#include "test_utils.h"
#include <emtrace/emtrace.h>

EXPECT_OUTPUT(
    "Hello from the basic test!\n"
    "An integer: 42\n"
    "A string: a string\n"
);

#ifdef TEST_COBS
EMTRACE_MAGIC_COBS(size_t)
#else
EMTRACE_MAGIC(size_t)
#endif

int main(void) {
    emtrace_init();
    EMTRACELN("Hello from the basic test!");
    int x = 42;
    EMTRACELN_F("An integer: {}", VAL(int, x));
    const char* s = "a string";
    EMTRACELN_F("A string: {}", STR(s));
    return 0;
}
