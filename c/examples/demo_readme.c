#include <emtrace/emtrace.h>

int main(void) {
    EMTRACE_INIT();
    EMTRACELN("Hello World!");

    int a = 1;
    int b = 2;
    EMTRACELN_F("{} + {} = {}", VAL(int, a), VAL(int, b), VAL(int, a + b));
}
