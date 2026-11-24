#include "emtrace/emtrace.h"

void foo(void);

void bar(void);
void bar(void) {
    int arr[] = {1, 2, 3};
    EMTRACE_F(
        "", SLC(int, arr, 3), SLC(int, arr, 3), SLC(int, arr, 3), SLC(int, arr, 3),
        SLC(int, arr, 3), SLC(int, arr, 3), SLC(int, arr, 3), SLC(int, arr, 3), SLC(int, arr, 3),
        SLC(int, arr, 3), SLC(int, arr, 3), SLC(int, arr, 3), SLC(int, arr, 3), SLC(int, arr, 3),
        SLC(int, arr, 3), SLC(int, arr, 3), SLC(int, arr, 3), SLC(int, arr, 3), SLC(int, arr, 3),
        SLC(int, arr, 3), SLC(int, arr, 3), SLC(int, arr, 3), SLC(int, arr, 3), SLC(int, arr, 3),
        SLC(int, arr, 3), SLC(int, arr, 3), SLC(int, arr, 3), SLC(int, arr, 3), SLC(int, arr, 3),
        SLC(int, arr, 3), SLC(int, arr, 3), SLC(int, arr, 3)
    );
    EMTRACE_F(
        "", VAL(int, 1), VAL(int, 2), VAL(int, 3), VAL(int, 4), VAL(int, 5), VAL(int, 6),
        VAL(int, 7), VAL(int, 8), VAL(int, 9), VAL(int, 10), VAL(int, 11), VAL(int, 12),
        VAL(int, 13), VAL(int, 14), VAL(int, 15), VAL(int, 16), VAL(int, 17), VAL(int, 18),
        VAL(int, 19), VAL(int, 20), VAL(int, 21), VAL(int, 22), VAL(int, 23), VAL(int, 24),
        VAL(int, 25), VAL(int, 26), VAL(int, 27), VAL(int, 28), VAL(int, 29), VAL(int, 30),
        VAL(int, 31), VAL(int, 32)
    );
}
