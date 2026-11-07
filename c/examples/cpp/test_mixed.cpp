#include "test_utils.h"
#include <cstdint>
#include <emtrace/emtrace.h>
#include <emtrace/emtrace.hpp>

EXPECT_OUTPUT(
    "Mixed types: 42 3.140000104904175 True\n"
    "Complex format: Value=100, Active=False, Ratio=0.5\n"
    "Numbers: \x01 \x02 \x03 \x04 \x05\n"
);

#ifdef TEST_COBS
EMTRACE_MAGIC_COBS(3);
#else
EMTRACE_MAGIC(3);
#endif

using std::uint8_t;

auto main() -> int {
    emtrace_init();

    // Test mixing different types in one trace
    int int_val = 42;
    float float_val = 3.140000104904175F; // Use exact expected value
    bool bool_val = true;

    MTRACELN("Mixed types: {} {} {}", int_val, float_val, bool_val);

    // Test more complex formatting
    int complex_int = 100;
    bool complex_bool = false;
    double complex_double = 0.5;

    MTRACELN(
        "Complex format: Value={}, Active={}, Ratio={}", complex_int, complex_bool, complex_double
    );

    // Test multiple same-type values (uint8_t shows as characters, not numbers)
    uint8_t n1 = 1;
    uint8_t n2 = 2;
    uint8_t n3 = 3;
    uint8_t n4 = 4;
    uint8_t n5 = 5;

    MTRACELN("Numbers: {} {} {} {} {}", n1, n2, n3, n4, n5);

    return 0;
}
