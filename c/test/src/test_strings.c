#include "emtrace/emtrace.h"
#include "emtrace/test_framework.h"
#include "emtrace/test_suites.h"
#include "emtrace/test_utils.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static bool test_string_trace(test_context_t* ctx) {
    uint8_t raw_buffer[128];
    test_buffer_t buffer = {.data = raw_buffer, .capacity = sizeof(raw_buffer), .size = 0};

    const char* test_string = "Hello, C!";
    EMT_TEST_TRACE_S(buffer, "", test_string);

    TEST_ASSERT_EQ(
        ctx, buffer.size, sizeof(emt_ptr_t) + strlen(test_string) + 1,
        "buffer size should match pointer + string length + null terminator"
    );

    TEST_ASSERT(
        ctx, strcmp((const char*) (buffer.data + sizeof(emt_ptr_t)), test_string) == 0,
        "traced string should match expected value"
    );

    return true;
}

test_fn_t* emt_get_string_tests(size_t* count) {
    static test_fn_t tests[] = {test_string_trace};
    *count = sizeof(tests) / sizeof(tests[0]);
    return tests;
}
