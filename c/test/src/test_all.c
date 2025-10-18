#include "emtrace/test_framework.h"
#include "emtrace/test_suites.h"
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
