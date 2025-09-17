#include "test_utils.h"
#include <emtrace/emtrace.h>
#include <stdint.h>
#include <stdlib.h>

EXPECT_OUTPUT(
    "Signed integers: -128 -32768 -2147483648 -9223372036854775808\n"
    "Unsigned integers: 255 65535 4294967295 18446744073709551615\n"
    "Size integers: 42 -42\n"
);

int main(void) {
    EMTRACE_INIT();

    // Test various signed integer types
    int8_t i8 = INT8_MIN;    // -128
    int16_t i16 = INT16_MIN; // -32768
    int32_t i32 = INT32_MIN; // -2147483648
    int64_t i64 = INT64_MIN; // -9223372036854775808

    EMTRACELN_F(
        "Signed integers: {:d} {} {} {}", int8_t, i8, int16_t, i16, int32_t, i32, int64_t, i64
    );

    // Test various unsigned integer types (uint8_t displays as char)
    uint8_t u8 = UINT8_MAX;    // 255
    uint16_t u16 = UINT16_MAX; // 65535
    uint32_t u32 = UINT32_MAX; // 4294967295
    uint64_t u64 = UINT64_MAX; // 18446744073709551615

    EMTRACELN_F(
        "Unsigned integers: {:d} {} {} {}", uint8_t, u8, uint16_t, u16, uint32_t, u32, uint64_t, u64
    );

    // Test size types
    size_t sz = 42;
    ssize_t ssz = -42;

    EMTRACELN_F("Size integers: {} {}", size_t, sz, ssize_t, ssz);

    return 0;
}
