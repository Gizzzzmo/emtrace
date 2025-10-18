#include "emtrace/test_framework.h"
#include "emtrace/test_suites.h"
#include "emtrace/test_utils.h"
#include <emtrace/emtrace.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static bool test_mixed_trace(test_context_t* ctx) {
    uint8_t raw_buffer[128];
    test_buffer_t buffer = {.data = raw_buffer, .capacity = sizeof(raw_buffer), .size = 0};

    EMT_TEST_TRACE_F(buffer, EMT_PY_FORMAT, "{} {} {}", int, 42, char, 'a', long, 123456L);

    size_t expected_size = sizeof(emt_ptr_t) + sizeof(int) + sizeof(char) + sizeof(long);
    TEST_ASSERT_EQ(ctx, buffer.size, expected_size, "buffer size should match expected size");

    int int_val;
    char char_val;
    long long_val;

    size_t offset = sizeof(emt_ptr_t);
    memcpy(&int_val, buffer.data + offset, sizeof(int));
    TEST_ASSERT_EQ(ctx, int_val, 42, "traced int value should be 42");
    offset += sizeof(int);

    memcpy(&char_val, buffer.data + offset, sizeof(char));
    TEST_ASSERT_EQ(ctx, char_val, 'a', "traced char value should be 'a'");
    offset += sizeof(char);

    memcpy(&long_val, buffer.data + offset, sizeof(long));
    TEST_ASSERT_EQ(ctx, long_val, 123456L, "traced long value should be 123456");

    return true;
}

test_fn_t* emt_get_mixed_tests(size_t* count) {
    static test_fn_t tests[] = {test_mixed_trace};
    *count = sizeof(tests) / sizeof(tests[0]);
    return tests;
}
