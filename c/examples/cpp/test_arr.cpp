#include "test_utils.h"
#include <array>
#include <cstdint>
#include <emtrace/emtrace.hpp>
#include <span>

EXPECT_OUTPUT(
    "[1, 2, 3, 4, 5, 6, 7, 8, 9, 10]\n"
    "[1, 2, 3, 4, 5, 6, 7, 8, 9, 10]\n"
    "1f, 1f, 1f, 1f\n"
    "11\n"
    "11\n"
    "11\n"
    "11\n"
    "11\n"
    "[[0, 0], [0, 0]]\n"
);

MTRACE_MAGIC(CPP_TEST_SINK);

using std::int16_t;
using std::int8_t;

auto main() -> int {
    emt::init();

    int arr1[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    std::span<int> ptr{arr1};
    MTRACELN("{}\n{}", arr1, ptr);

    int16_t arr2[8] = {31, 31, 31, 31, 31, 31, 31, 31};
    std::span<int16_t> slice{arr2, 4};
    MTRACELN("{:, *x}", slice);

    int8_t arr3[5] = {3, 3, 3, 3, 3};
    MTRACELN("{:\n*b}", arr3);

    std::array<int[2], 2> nested_arr{};
    MTRACELN("{}", nested_arr);
    return 0;
}
