#include "test_utils.h"
#include <emtrace/emtrace.hpp>

#ifndef __SIZEOF_INT128__
#error "this test needs a compiler that provides 128 bit integers"
#endif

EXPECT_OUTPUT(
    "i128 max: 170141183460469231731687303715884105727\n"
    "u128 max: 340282366920938463463374607431768211455\n"
    "i128 min: -170141183460469231731687303715884105728\n"
    "mixed: 1 -1 1\n"
);

MTRACE_MAGIC(CPP_TEST_SINK);

auto main() -> int {
    emt::init();

    constexpr unsigned __int128 u128_max = ~static_cast<unsigned __int128>(0);
    constexpr auto i128_max = static_cast<__int128>(u128_max >> 1);
    constexpr __int128 i128_min = -i128_max - 1;

    MTRACELN("i128 max: {}", i128_max);
    MTRACELN("u128 max: {}", u128_max);
    MTRACELN("i128 min: {}", i128_min);

    // wide and narrow integers can be mixed freely
    constexpr unsigned __int128 one = 1;
    constexpr __int128 minus_one = -1;
    MTRACELN("mixed: {} {} {}", one, minus_one, 1);

    return 0;
}
