#include "test_utils.h"
#include <emtrace/emtrace.h>
#include <stdbool.h>
#include <stdint.h>

EXPECT_OUTPUT(
    "Edge cases:\n"
    "Zero values: 0 0.0 False\n"
    "Extremes: 255 -128 4294967295 -2147483648\n"
    "No format\n"
);
// "C format: test\n";

int main(void) {
    EMTRACE_INIT();

    EMTRACELN("Edge cases:");

    // Test zero values
    int zero_int = 0;
    float zero_float = 0.0F;
    bool zero_bool = false;

    EMTRACELN_F("Zero values: {} {} {}", int, zero_int, float, zero_float, bool, zero_bool);

    // Test extreme values (uint8_t displays as character)
    uint8_t max_u8 = UINT8_MAX;    // 255 (ÿ)
    int8_t min_i8 = INT8_MIN;      // -128
    uint32_t max_u32 = UINT32_MAX; // 4294967295
    int32_t min_i32 = INT32_MIN;   // -2147483648

    EMTRACELN_F(
        "Extremes: {:d} {:d} {} {}", uint8_t, max_u8, int8_t, min_i8, uint32_t, max_u32, int32_t,
        min_i32
    );

    // Test trace without format arguments (NO_FORMAT)
    EMTRACE("No format");
    EMTRACELN("");

    // // Test C-style format
    // const char* test_str = "test";
    // EMTRACE("C format:")
    // EMTRACELN_S("C format: %s", const char*, test_str);

    return 0;
}
