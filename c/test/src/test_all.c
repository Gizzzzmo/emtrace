#include "test_framework.h"
#include "test_suites.h"
#include <stddef.h>
#include <stdio.h>

static void output_to_stdout(const char* message) { fputs(message, stdout); }

int main(void) {
    test_context_t ctx = {.output = output_to_stdout};
    test_result_t total_result = {.total = 0, .passed = 0, .failed = 0};

    size_t count;
    test_fn_t* tests;
    test_result_t result;

    const char* test_names_int[] = {"test_integer_trace"};
    tests = emt_get_integer_tests(&count);
    result = run_tests(&ctx, tests, count, test_names_int);
    total_result.total += result.total;
    total_result.passed += result.passed;
    total_result.failed += result.failed;

    const char* test_names_double[] = {"test_double_trace"};
    tests = emt_get_double_tests(&count);
    result = run_tests(&ctx, tests, count, test_names_double);
    total_result.total += result.total;
    total_result.passed += result.passed;
    total_result.failed += result.failed;

    const char* test_names_string[] = {"test_string_trace"};
    tests = emt_get_string_tests(&count);
    result = run_tests(&ctx, tests, count, test_names_string);
    total_result.total += result.total;
    total_result.passed += result.passed;
    total_result.failed += result.failed;

    const char* test_names_mixed[] = {"test_mixed_trace"};
    tests = emt_get_mixed_tests(&count);
    result = run_tests(&ctx, tests, count, test_names_mixed);
    total_result.total += result.total;
    total_result.passed += result.passed;
    total_result.failed += result.failed;

    const char* test_names_cobs[] = {
        "test_cobs_short",
        "test_cobs_empty",
        "test_cobs_only_zeros",
        "test_cobs_one_full_block",
        "test_cobs_longer_than_a_block",
        "test_cobs_zero_after_full_block",
        "test_cobs_reuse_state",
        "test_cobs_chunking_is_irrelevant",
        "test_cobs_roundtrip_lengths",
        "test_cobs_roundtrip_around_max_block",
    };
    tests = emt_get_cobs_tests(&count);
    result = run_tests(&ctx, tests, count, test_names_cobs);
    total_result.total += result.total;
    total_result.passed += result.passed;
    total_result.failed += result.failed;

    const char* test_names_collisions[] = {
        "test_c_identifier_collisions",
        "test_object_macro_collisions",
        "test_function_macro_collisions",
        "test_max_args_with_collision",
    };
    tests = emt_get_collision_tests(&count);
    result = run_tests(&ctx, tests, count, test_names_collisions);
    total_result.total += result.total;
    total_result.passed += result.passed;
    total_result.failed += result.failed;

    printf("\n========================================\n");
    printf("Test Results: %zu/%zu passed", total_result.passed, total_result.total);
    if (total_result.failed > 0) {
        printf(", %zu FAILED\n", total_result.failed);
    } else {
        printf("\n");
    }
    printf("========================================\n");

    return total_result.failed == 0 ? 0 : 1;
}
