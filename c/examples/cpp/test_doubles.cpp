#include "test_utils.h"
#include <emtrace/emtrace.h>
#include <emtrace/emtrace.hpp>
#include <limits>
#include <numbers>

EXPECT_OUTPUT(
    "Doubles: 3.14159 2.71828 nan inf -inf\n"
    "3.141593 2.718282 nan inf -inf\n"
    "Extreme values: 1.7976931348623157e+308 2.2250738585072014e-308\n"
);

#ifdef TEST_COBS
EMTRACE_MAGIC_COBS(3);
#else
EMTRACE_MAGIC(3);
#endif

auto main() -> int {
    emtrace_init();

    // Test basic double values
    double nan_val = std::numeric_limits<double>::quiet_NaN();
    double inf_val = std::numeric_limits<double>::infinity();
    double neg_inf = -inf_val;

    MTRACELN(
        "Doubles: {:.5f} {:.5f} {} {} {}", std::numbers::pi, std::numbers::e, nan_val, inf_val,
        neg_inf
    );

    MTRACELN(
        "%f %f %f %f %f", std::numbers::pi, std::numbers::e, nan_val, inf_val, neg_inf,
        emt::opt_c_style_fmt
    );

    // Test extreme values
    double max_val = std::numeric_limits<double>::max(); // 1.7976931348623157e+308
    double min_val = std::numeric_limits<double>::min(); // 2.2250738585072014e-308

    MTRACELN("Extreme values: {} {}", max_val, min_val);

    return 0;
}
