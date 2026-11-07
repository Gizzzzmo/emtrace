#include "test_utils.h"
#include <cstdint>
#include <emtrace/emtrace.h>
#include <emtrace/emtrace.hpp>
#include <limits>

EXPECT_OUTPUT(
    "Large integers:\n"
    "int64_t max: 9223372036854775807\n"
    "uint64_t max: 18446744073709551615\n"
    "int64_t min: -9223372036854775808\n"
);

#ifdef TEST_COBS
EMTRACE_MAGIC_COBS(3);
#else
EMTRACE_MAGIC(3);
#endif

using std::int64_t;
using std::uint64_t;

auto main() -> int {
    emtrace_init();

    MTRACELN("Large integers:");

    // Test maximum 64-bit signed integer
    int64_t max_i64 = std::numeric_limits<int64_t>::max(); // 9223372036854775807
    MTRACELN("int64_t max: {}", max_i64);

    // Test maximum 64-bit unsigned integer
    uint64_t max_u64 = std::numeric_limits<uint64_t>::max(); // 18446744073709551615
    MTRACELN("uint64_t max: {}", max_u64);

    // Test minimum 64-bit signed integer
    int64_t min_i64 = std::numeric_limits<int64_t>::min(); // -9223372036854775808
    MTRACELN("int64_t min: {}", min_i64);

    return 0;
}
