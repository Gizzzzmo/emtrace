#include "emtrace/emtrace.h"

void foo(void);

#define V EMT_VAL
#define S EMT_STR
#define A EMT_SLC

void bar(void);
void bar(void) {
    int arr[] = {1, 2, 3};
    EMTRACE_F(
        "", A(int, arr, 3), A(int, arr, 3), A(int, arr, 3), A(int, arr, 3), A(int, arr, 3),
        A(int, arr, 3), A(int, arr, 3), A(int, arr, 3), A(int, arr, 3), A(int, arr, 3),
        A(int, arr, 3), A(int, arr, 3), A(int, arr, 3), A(int, arr, 3), A(int, arr, 3),
        A(int, arr, 3), A(int, arr, 3), A(int, arr, 3), A(int, arr, 3), A(int, arr, 3),
        A(int, arr, 3), A(int, arr, 3), A(int, arr, 3), A(int, arr, 3), A(int, arr, 3),
        A(int, arr, 3), A(int, arr, 3), A(int, arr, 3), A(int, arr, 3), A(int, arr, 3),
        A(int, arr, 3), A(int, arr, 3)
    );
    EMTRACE_F(
        "", V(int, 1), V(int, 2), V(int, 3), V(int, 4), V(int, 5), V(int, 6), V(int, 7), V(int, 8),
        V(int, 9), V(int, 10), V(int, 11), V(int, 12), V(int, 13), V(int, 14), V(int, 15),
        V(int, 16), V(int, 17), V(int, 18), V(int, 19), V(int, 20), V(int, 21), V(int, 22),
        V(int, 23), V(int, 24), V(int, 25), V(int, 26), V(int, 27), V(int, 28), V(int, 29),
        V(int, 30), V(int, 31), V(int, 32)
    );
}
