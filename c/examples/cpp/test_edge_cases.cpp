#include "test_utils.h"
#include <cstdint>
#include <emtrace/emtrace.h>
#include <emtrace/emtrace.hpp>
#include <limits>

EXPECT_OUTPUT(
    "Edge cases:\n"
    "Zero values: 0 0.0 False\n"
    "Extremes: 255 -128 4294967295 -2147483648\n"
    "No format {}\n"
    "C format: test\n"
);
// "C format: test\n";

#ifdef TEST_COBS
EMTRACE_MAGIC_COBS(3);
#else
EMTRACE_MAGIC(3);
#endif

using std::int32_t;
using std::int8_t;
using std::uint32_t;
using std::uint8_t;

auto main() -> int {
    emtrace_init();

    EMTRACELN("Edge cases:");

    // Test zero values
    int zero_int = 0;
    float zero_float = 0.0F;
    bool zero_bool = false;

    MTRACELN("Zero values: {} {} {}", zero_int, zero_float, zero_bool);

    // Test extreme values (uint8_t displays as character)
    uint8_t max_u8 = std::numeric_limits<uint8_t>::max();    // 255 (ÿ)
    int8_t min_i8 = std::numeric_limits<int8_t>::min();      // -128
    uint32_t max_u32 = std::numeric_limits<uint32_t>::max(); // 4294967295
    int32_t min_i32 = std::numeric_limits<int32_t>::min();   // -2147483648

    MTRACELN("Extremes: {:d} {:d} {} {}", max_u8, min_i8, max_u32, min_i32);

    // Test trace without format arguments (NO_FORMAT)
    MTRACE("No format");
    MTRACELN(" {}");

    // Test C-style format
    const char* test_str = "test";
    MTRACELN("C format: %s", test_str, emt::opt_c_style_fmt);

    return 0;
}
