#include "test_utils.h"
#include <emtrace/emtrace.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

EXPECT_OUTPUT(
    "Signed integers: -128 -32768 -2147483648 -9223372036854775808\n"
    "Unsigned integers: 255 65535 4294967295 18446744073709551615\n"
    "Size integers: 42 -42\n"
);

#ifdef TEST_COBS
EMTRACE_MAGIC_COBS(size_t)
#else
EMTRACE_MAGIC(size_t)
#endif

int main(void) {
    emtrace_init();

    // Test various signed integer types
    int8_t i8 = INT8_MIN;    // -128
    int16_t i16 = INT16_MIN; // -32768
    int32_t i32 = INT32_MIN; // -2147483648
    int64_t i64 = INT64_MIN; // -9223372036854775808

    EMTRACELN_F(
        "Signed integers: {:d} {} {} {}", VAL(int8_t, i8), VAL(int16_t, i16), VAL(int32_t, i32),
        VAL(int64_t, i64)
    );

    // Test various unsigned integer types (uint8_t displays as char)
    uint8_t u8 = UINT8_MAX;    // 255
    uint16_t u16 = UINT16_MAX; // 65535
    uint32_t u32 = UINT32_MAX; // 4294967295
    uint64_t u64 = UINT64_MAX; // 18446744073709551615

    EMTRACELN_F(
        "Unsigned integers: {:d} {} {} {}", VAL(uint8_t, u8), VAL(uint16_t, u16),
        VAL(uint32_t, u32), VAL(uint64_t, u64)
    );

    // Test size types
    size_t sz = 42;
#if !defined(__unix__) && !defined(__unix) && !(defined(__APPLE__) || defined(__MACH__))
    typedef int ssize_t;
#endif
    ssize_t ssz = -42;

    EMTRACELN_F("Size integers: {} {}", VAL(size_t, sz), VAL(ssize_t, ssz));

    return 0;
}
