#include "test_utils.h"
#include <emtrace/emtrace.h>
#include <stdint.h>

EXPECT_OUTPUT(
    "[1, 2, 3, 4, 5, 6, 7, 8, 9, 10]\n"
    "[1, 2, 3, 4, 5, 6, 7, 8, 9, 10]\n"
    "1f, 1f, 1f, 1f, 1f, 1f, 1f, 1f\n"
    "11\n"
    "11\n"
    "11\n"
    "11\n"
    "11"
);

int main(void) {
    EMTRACE_INIT();
    int arr1[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    const int* ptr = arr1;
    EMTRACELN_F("{}\n{}", ARR(int, arr1), STS(int, ptr, 10));

    int16_t arr2[8] = {31, 31, 31, 31, 31, 31, 31, 31};
    EMTRACELN_F("{:, *x}", SLC(int16_t, arr2, 8));

    int8_t arr3[5] = {3, 3, 3, 3, 3};
    EMTRACE_F("{:\n*b}", ARR(int8_t, arr3));
    return 0;
}
