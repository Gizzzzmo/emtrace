#include "test_utils.h"
#include <emtrace/emtrace.hpp>
#include <limits>

EXPECT_OUTPUT(
    "1.0 0.5 nan inf -inf\n"
    "1.000000 0.500000 nan inf -inf\n"
);

MTRACE_MAGIC(CPP_TEST_SINK);

auto main() -> int {
    emt::init();

    constexpr float nan = std::numeric_limits<float>::quiet_NaN();
    constexpr float inf = std::numeric_limits<float>::infinity();

    MTRACELN("{} {} {} {} {}", 1.0F, 0.5F, nan, inf, -inf);
    MTRACELN("%f %f %f %f %f", 1.0F, 0.5F, nan, inf, -inf, emt::opt_c_style_fmt);

    return 0;
}
