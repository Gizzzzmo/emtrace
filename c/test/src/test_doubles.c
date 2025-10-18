#include "emtrace/test_framework.h"
#include "emtrace/test_suites.h"
#include "emtrace/test_utils.h"
#include <emtrace/emtrace.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static bool test_double_trace(test_context_t* ctx) {
    uint8_t raw_buffer[128];
    test_buffer_t buffer = {.data = raw_buffer, .capacity = sizeof(raw_buffer), .size = 0};

    EMT_TEST_TRACE_F(buffer, EMT_PY_FORMAT, "{}", double, 3.14);

    TEST_ASSERT_EQ(
        ctx, buffer.size, sizeof(emt_ptr_t) + sizeof(double),
        "buffer size should match pointer + double size"
    );

    double value;
    memcpy(&value, buffer.data + sizeof(emt_ptr_t), sizeof(double));
    TEST_ASSERT_EQ(ctx, value, 3.14, "traced double value should be 3.14");

    return true;
}

test_fn_t* emt_get_double_tests(size_t* count) {
    static test_fn_t tests[] = {test_double_trace};
    *count = sizeof(tests) / sizeof(tests[0]);
    return tests;
}
