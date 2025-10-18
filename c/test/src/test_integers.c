#include "emtrace/test_framework.h"
#include "emtrace/test_suites.h"
#include "emtrace/test_utils.h"
#include <emtrace/emtrace.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static bool test_integer_trace(test_context_t* ctx) {
    uint8_t raw_buffer[128];
    test_buffer_t buffer = {.data = raw_buffer, .capacity = sizeof(raw_buffer), .size = 0};

    EMT_TEST_TRACE_F(buffer, EMT_PY_FORMAT, "{}", int, 42);

    TEST_ASSERT_EQ(
        ctx, buffer.size, sizeof(emt_ptr_t) + sizeof(int),
        "buffer size should match pointer + integer size"
    );

    int value;
    memcpy(&value, buffer.data + sizeof(emt_ptr_t), sizeof(int));
    TEST_ASSERT_EQ(ctx, value, 42, "traced integer value should be 42");

    return true;
}

test_fn_t* emt_get_integer_tests(size_t* count) {
    static test_fn_t tests[] = {test_integer_trace};
    *count = sizeof(tests) / sizeof(tests[0]);
    return tests;
}
