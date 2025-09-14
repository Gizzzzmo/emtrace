#include "test_utils.h"
#include <emtrace/emtrace.h>

const char g_emt_expected_output[] EMT_TEST_EXPECTED_SECTION = "Hello from the basic test!\n"
                                                               "An integer: 42\n"
                                                               "A string: a string\n";

int main(void) {
    EMTRACE_INIT();
    EMTRACELN("Hello from the basic test!");
    int x = 42;
    EMTRACELN_F("An integer: {}", int, x);
    const char* s = "a string";
    EMTRACE("A string: ");
    EMTRACELN_S(s);
    return 0;
}
