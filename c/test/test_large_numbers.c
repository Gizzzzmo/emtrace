#include "test_utils.h"
#include <emtrace/emtrace.h>
#include <limits.h>
#include <stdint.h>

// Note: C doesn't have native 128-bit integers like Rust,
// so we'll use 64-bit integers as the largest available type
const char g_emt_expected_output[] EMT_TEST_EXPECTED_SECTION =
    "Large integers:\n"
    "int64_t max: 9223372036854775807\n"
    "uint64_t max: 18446744073709551615\n"
    "int64_t min: -9223372036854775808\n";

int main(void) {
    EMTRACE_INIT();

    EMTRACELN("Large integers:");

    // Test maximum 64-bit signed integer
    int64_t max_i64 = INT64_MAX; // 9223372036854775807
    EMTRACELN_F("int64_t max: {}", int64_t, max_i64);

    // Test maximum 64-bit unsigned integer
    uint64_t max_u64 = UINT64_MAX; // 18446744073709551615
    EMTRACELN_F("uint64_t max: {}", uint64_t, max_u64);

    // Test minimum 64-bit signed integer
    int64_t min_i64 = INT64_MIN; // -9223372036854775808
    EMTRACELN_F("int64_t min: {}", int64_t, min_i64);

    return 0;
}
