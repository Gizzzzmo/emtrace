#include "test_framework.h"
#include "test_suites.h"
#include "test_utils.h"

#include <emtrace/emtrace.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/// Test that creating macros / other kinds of objects with identifiers VAL, ARR, SLC, etc.
/// don't prevent TRACE macros from working 

#define NAIVE_TRACE_F(buffer, formatter, fmt, ...)                                                 \
    EMT_TRACE_F(                                                                                   \
        const, formatter, to_buffer, serialize_pointer, DUMMY, DUMMY, (&(buffer)), "", fmt,        \
        __VA_ARGS__                                                                                \
    )

static bool test_c_identifier_collisions(test_context_t* ctx) {
    uint8_t raw_buffer[256];
    test_buffer_t buffer = {.data = raw_buffer, .capacity = sizeof(raw_buffer), .size = 0};

    // NOLINTBEGIN(readability-identifier-naming)
    enum { STS = 2 };
    int VAL = 42;
    int ARR[3] = {10, 20, 30};
    const char* STR = "hi";
    size_t SLC = 2;
    // NOLINTEND(readability-identifier-naming)

    NAIVE_TRACE_F(
        buffer, EMT_PY_FORMAT, "{} {} {} {} {}", VAL(int, VAL), ARR(int, ARR), STR(STR),
        STS(int, ARR, STS), SLC(int, ARR, SLC)
    );

    // Expected: ptr + int + int[3] + "hi\0" + int[2] + varint(2) + int[2]
    size_t expected_size = sizeof(emt_ptr_t) + sizeof(int) + sizeof(ARR) + 3 + (2 * sizeof(int)) +
                           1 + (2 * sizeof(int));
    TEST_ASSERT_EQ(ctx, buffer.size, expected_size, "buffer size should match all 5 arguments");

    size_t offset = sizeof(emt_ptr_t);
    int out_val = 0;
    memcpy(&out_val, buffer.data + offset, sizeof(int));
    TEST_ASSERT_EQ(ctx, out_val, 42, "VAL argument should serialize 42");
    offset += sizeof(int);

    int out_arr[3] = {0};
    memcpy(out_arr, buffer.data + offset, sizeof(out_arr));
    TEST_ASSERT_EQ(ctx, out_arr[0], 10, "ARR[0] should be 10");
    TEST_ASSERT_EQ(ctx, out_arr[2], 30, "ARR[2] should be 30");
    offset += sizeof(out_arr);

    TEST_ASSERT(ctx, strcmp((const char*) (buffer.data + offset), "hi") == 0, "STR should be 'hi'");
    offset += 3;

    int out_sts[2] = {0};
    memcpy(out_sts, buffer.data + offset, sizeof(out_sts));
    TEST_ASSERT_EQ(ctx, out_sts[0], 10, "STS[0] should be 10");
    TEST_ASSERT_EQ(ctx, out_sts[1], 20, "STS[1] should be 20");
    offset += sizeof(out_sts);

    TEST_ASSERT_EQ(ctx, buffer.data[offset], 2, "SLC varint length should be 2");
    offset += 1;

    int out_slc[2] = {0};
    memcpy(out_slc, buffer.data + offset, sizeof(out_slc));
    TEST_ASSERT_EQ(ctx, out_slc[0], 10, "SLC[0] should be 10");
    TEST_ASSERT_EQ(ctx, out_slc[1], 20, "SLC[1] should be 20");

    // Also verify 0-arg EMT_TRACE / EMT_TEST_TRACE
    buffer.size = 0;
    EMT_TEST_TRACE(buffer, EMT_NO_FORMAT, "plain");
    TEST_ASSERT_EQ(ctx, buffer.size, sizeof(emt_ptr_t), "0-arg EMT_TRACE should emit only pointer");

    return true;
}

#define VAL 42
#define ARR 100
#define STS (1 + 1)
#define SLC 1, 2, 3
#define STR "macro_str"

static bool test_object_macro_collisions(test_context_t* ctx) {
    uint8_t raw_buffer[256];
    test_buffer_t buffer = {.data = raw_buffer, .capacity = sizeof(raw_buffer), .size = 0};

    int data[3] = {7, 8, 9};

    // Direct EMT_TRACE_F call with all 5 tags colliding with object-like macros
    EMT_TRACE_F(
        const, EMT_PY_FORMAT, to_buffer, serialize_pointer, DUMMY, DUMMY, (&buffer), "",
        "{} {} {} {} {}", VAL(int, VAL + ARR), STR(STR), STS(int, data, STS), SLC(int, data, 2),
        ARR(int, data)
    );

    size_t str_len = strlen("macro_str") + 1;
    size_t expected_size = sizeof(emt_ptr_t) + sizeof(int) + str_len + (2 * sizeof(int)) + 1 +
                           (2 * sizeof(int)) + sizeof(data);
    TEST_ASSERT_EQ(ctx, buffer.size, expected_size, "buffer size should match expected payload");

    size_t offset = sizeof(emt_ptr_t);
    int out_val = 0;
    memcpy(&out_val, buffer.data + offset, sizeof(int));
    TEST_ASSERT_EQ(ctx, out_val, 142, "inner VAL + ARR should expand to 142");
    offset += sizeof(int);

    TEST_ASSERT(
        ctx, strcmp((const char*) (buffer.data + offset), "macro_str") == 0,
        "inner STR should expand to 'macro_str'"
    );
    offset += str_len;

    int out_sts[2] = {0};
    memcpy(out_sts, buffer.data + offset, sizeof(out_sts));
    TEST_ASSERT_EQ(ctx, out_sts[0], 7, "STS[0] should be 7");
    TEST_ASSERT_EQ(ctx, out_sts[1], 8, "STS[1] should be 8");
    offset += sizeof(out_sts);

    TEST_ASSERT_EQ(ctx, buffer.data[offset], 2, "SLC length prefix should be 2");
    offset += 1 + (2 * sizeof(int));

    int out_arr[3] = {0};
    memcpy(out_arr, buffer.data + offset, sizeof(out_arr));
    TEST_ASSERT_EQ(ctx, out_arr[2], 9, "ARR[2] should be 9");

    // Also verify forwarding wrapper EMT_TEST_TRACE_F with the same colliding macros
    buffer.size = 0;
    EMT_TEST_TRACE_F(buffer, EMT_PY_FORMAT, "{} {}", VAL(int, VAL), STR(STR));
    TEST_ASSERT_EQ(
        ctx, buffer.size, sizeof(emt_ptr_t) + sizeof(int) + str_len,
        "EMT_TRACE_F_FORWARD wrapper should also resist colliding macros"
    );

    return true;
}

#undef VAL
#undef ARR
#undef STS
#undef SLC
#undef STR

#define VAL(a, b) ((a) + (b))
#define ARR(x) ((x) * 2)
#define STS() 2
#define SLC(...) (1 + 1)
#define STR(a, b, c) "bad"

static bool test_function_macro_collisions(test_context_t* ctx) {
    uint8_t raw_buffer[256];
    test_buffer_t buffer = {.data = raw_buffer, .capacity = sizeof(raw_buffer), .size = 0};

    int data[3] = {11, 22, 33};

    // Function-like macros with matching, mismatched, zero, and variadic arities
    EMT_TEST_TRACE_F(
        buffer, EMT_PY_FORMAT, "{} {} {} {} {}", VAL(int, VAL(10, ARR(5))), STR("ok"),
        STS(int, data, STS()), SLC(int, data, SLC(9, 8, 7)), ARR(int, data)
    );

    size_t offset = sizeof(emt_ptr_t);
    int out_val = 0;
    memcpy(&out_val, buffer.data + offset, sizeof(int));
    TEST_ASSERT_EQ(ctx, out_val, 20, "nested VAL(10, ARR(5)) should expand to 20");
    offset += sizeof(int);

    TEST_ASSERT(ctx, strcmp((const char*) (buffer.data + offset), "ok") == 0, "STR should be 'ok'");
    offset += 3;

    int out_sts[2] = {0};
    memcpy(out_sts, buffer.data + offset, sizeof(out_sts));
    TEST_ASSERT_EQ(ctx, out_sts[0], 11, "STS[0] should be 11");
    TEST_ASSERT_EQ(ctx, out_sts[1], 22, "STS[1] should be 22");
    offset += sizeof(out_sts);

    TEST_ASSERT_EQ(ctx, buffer.data[offset], 2, "SLC varint from SLC(...) should be 2");
    offset += 1 + (2 * sizeof(int));

    int out_arr[3] = {0};
    memcpy(out_arr, buffer.data + offset, sizeof(out_arr));
    TEST_ASSERT_EQ(ctx, out_arr[2], 33, "ARR[2] should be 33");

    return true;
}

#undef VAL
#undef ARR
#undef STS
#undef SLC
#undef STR

#define VAL 999

static bool test_max_args_with_collision(test_context_t* ctx) {
    uint8_t raw_buffer[512];
    test_buffer_t buffer = {.data = raw_buffer, .capacity = sizeof(raw_buffer), .size = 0};

    EMT_TEST_TRACE_F(
        buffer, EMT_PY_FORMAT,
        "{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}"
        "{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}",
        VAL(int, 1), VAL(int, 2), VAL(int, 3), VAL(int, 4), VAL(int, 5), VAL(int, 6), VAL(int, 7),
        VAL(int, 8), VAL(int, 9), VAL(int, 10), VAL(int, 11), VAL(int, 12), VAL(int, 13),
        VAL(int, 14), VAL(int, 15), VAL(int, 16), VAL(int, 17), VAL(int, 18), VAL(int, 19),
        VAL(int, 20), VAL(int, 21), VAL(int, 22), VAL(int, 23), VAL(int, 24), VAL(int, 25),
        VAL(int, 26), VAL(int, 27), VAL(int, 28), VAL(int, 29), VAL(int, 30), VAL(int, 31),
        VAL(int, 32), VAL(int, 33), VAL(int, 34), VAL(int, 35), VAL(int, 36), VAL(int, 37),
        VAL(int, 38), VAL(int, 39), VAL(int, 40), VAL(int, 41), VAL(int, 42), VAL(int, 43),
        VAL(int, 44), VAL(int, 45), VAL(int, 46), VAL(int, 47), VAL(int, 48), VAL(int, 49),
        VAL(int, 50), VAL(int, 51), VAL(int, 52), VAL(int, 53), VAL(int, 54), VAL(int, 55),
        VAL(int, 56), VAL(int, 57), VAL(int, 58), VAL(int, 59), VAL(int, 60), VAL(int, 61),
        VAL(int, 62), VAL(int, 63), VAL(int, VAL)
    );

    TEST_ASSERT_EQ(
        ctx, buffer.size, sizeof(emt_ptr_t) + (64 * sizeof(int)),
        "buffer size should match pointer + 64 ints"
    );

    int first = 0;
    int last = 0;
    memcpy(&first, buffer.data + sizeof(emt_ptr_t), sizeof(int));
    memcpy(&last, buffer.data + sizeof(emt_ptr_t) + (63 * sizeof(int)), sizeof(int));
    TEST_ASSERT_EQ(ctx, first, 1, "first argument should be 1");
    TEST_ASSERT_EQ(ctx, last, 999, "64th argument VAL should expand to 999");

    return true;
}

#undef VAL

test_fn_t* emt_get_collision_tests(size_t* count) {
    static test_fn_t tests[] = {
        test_c_identifier_collisions,
        test_object_macro_collisions,
        test_function_macro_collisions,
        test_max_args_with_collision,
    };
    *count = sizeof(tests) / sizeof(tests[0]);
    return tests;
}
