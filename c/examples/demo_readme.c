#include <emtrace/emtrace.h>

EMTRACE_MAGIC(uintptr_t)

int main(void) {
    emtrace_init();
    EMTRACELN("Hello World!");

    int a = 1;
    int b = 2;
    EMTRACELN_F("{} + {} = {}", VAL(int, a), VAL(int, b), VAL(int, a + b));
}
