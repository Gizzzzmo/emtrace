#include "test_utils.h"
#include <emtrace/emtrace.hpp>

EXPECT_OUTPUT(
    "True False\n"
    "True False\n"
);

MTRACE_MAGIC(CPP_TEST_SINK);

auto main() -> int {
    emt::init();

    MTRACELN("{} {}", true, false);
    MTRACELN("%r %r", true, false, emt::opt_c_style_fmt);

    return 0;
}
