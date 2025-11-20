#ifndef EMTRACE_TEST_FRAMEWORK_H
#define EMTRACE_TEST_FRAMEWORK_H

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    void (*output)(const char* message);
} test_context_t;

typedef struct {
    size_t total;
    size_t passed;
    size_t failed;
} test_result_t;

typedef bool (*test_fn_t)(test_context_t* ctx);

#define TEST_ASSERT(ctx, condition, message)                                                       \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            if ((ctx)->output) {                                                                   \
                (ctx)->output("FAIL: " message " at " __FILE__ ":");                               \
                (ctx)->output(__func__);                                                           \
                (ctx)->output("\n");                                                               \
            }                                                                                      \
            return false;                                                                          \
        }                                                                                          \
    } while (0)

#define TEST_ASSERT_EQ(ctx, actual, expected, message)                                             \
    TEST_ASSERT((ctx), (actual) == (expected), message)

static inline test_result_t run_tests(
    test_context_t* ctx, test_fn_t* tests, size_t num_tests, const char** test_names
) {
    test_result_t result = {.total = num_tests, .passed = 0, .failed = 0};

    for (size_t i = 0; i < num_tests; i++) {
        if (ctx->output && test_names) {
            ctx->output("Running: ");
            ctx->output(test_names[i]);
            ctx->output("\n");
        }

        if (tests[i](ctx)) {
            result.passed++;
            if (ctx->output) {
                ctx->output("  PASS\n");
            }
        } else {
            result.failed++;
        }
    }

    return result;
}

#endif // EMTRACE_TEST_FRAMEWORK_H
