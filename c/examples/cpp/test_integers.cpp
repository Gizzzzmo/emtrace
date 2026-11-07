#include "test_utils.h"
#include <cstddef>
#include <cstdint>
#include <emtrace/emtrace.h>
#include <emtrace/emtrace.hpp>

EXPECT_OUTPUT(
    "Signed integers: -128 -32768 -2147483648 -9223372036854775808\n"
    "Unsigned integers: 255 65535 4294967295 18446744073709551615\n"
    "Size integers: 42 -42\n"
);

#ifdef TEST_COBS
EMTRACE_MAGIC_COBS(3);
#else
EMTRACE_MAGIC(3);
#endif

using std::int16_t;
using std::int32_t;
using std::int64_t;
using std::int8_t;

using std::size_t;

auto main() -> int {
    emtrace_init();

    // Test various signed integer types
    int8_t i8 = INT8_MIN;    // -128
    int16_t i16 = INT16_MIN; // -32768
    int32_t i32 = INT32_MIN; // -2147483648
    int64_t i64 = INT64_MIN; // -9223372036854775808

    MTRACELN("Signed integers: {:d} {} {} {}", i8, i16, i32, i64);

    // Test various unsigned integer types (uint8_t displays as char)
    uint8_t u8 = UINT8_MAX;    // 255
    uint16_t u16 = UINT16_MAX; // 65535
    uint32_t u32 = UINT32_MAX; // 4294967295
    uint64_t u64 = UINT64_MAX; // 18446744073709551615

    MTRACELN("Unsigned integers: {:d} {} {} {}", u8, u16, u32, u64);

    // Test size types
    size_t sz = 42;
#if !defined(__unix__) && !defined(__unix) && !(defined(__APPLE__) || defined(__MACH__))
    typedef int ssize_t;
#endif
    ssize_t ssz = -42;

    MTRACELN("Size integers: {} {}", sz, ssz);

    return 0;
}
