#include "test_utils.h"
#include <emtrace/emtrace.h>
#include <stdbool.h>
#include <stdint.h>

EXPECT_OUTPUT(
    "Mixed types: 42 3.140000104904175 True\n"
    "Complex format: Value=100, Active=False, Ratio=0.5\n"
    "Numbers: \x01 \x02 \x03 \x04 \x05\n"
);

int main(void) {
    EMTRACE_INIT();

    // Test mixing different types in one trace
    int int_val = 42;
    float float_val = 3.140000104904175F; // Use exact expected value
    bool bool_val = true;

    EMTRACELN_F("Mixed types: {} {} {}", int, int_val, float, float_val, bool, bool_val);

    // Test more complex formatting
    int complex_int = 100;
    bool complex_bool = false;
    double complex_double = 0.5;

    EMTRACELN_F(
        "Complex format: Value={}, Active={}, Ratio={}", int, complex_int, bool, complex_bool,
        double, complex_double
    );

    // Test multiple same-type values (uint8_t shows as characters, not numbers)
    uint8_t n1 = 1;
    uint8_t n2 = 2;
    uint8_t n3 = 3;
    uint8_t n4 = 4;
    uint8_t n5 = 5;

    EMTRACELN_F(
        "Numbers: {} {} {} {} {}", uint8_t, n1, uint8_t, n2, uint8_t, n3, uint8_t, n4, uint8_t, n5
    );

    return 0;
}
