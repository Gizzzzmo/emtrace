#include "emtrace/emtrace.h"
#include <stdint.h>

static void bar(void) { EMTRACE("YAY\n"); }

int main(void) {
    EMTRACE_INIT();
    int8_t x = 1;
    int8_t y = 2;
    bar();
    // TRACE("Hello, World!");
    for (uint8_t k = 0; k < 1; k++) {
        EMTRACE_F("Hello, World! {:d} {{}}", int8_t, y);
        EMTRACE("  test {} {}\n");
        EMTRACE_F("Hello, World! 0x{0:x} {2:d} {1:d}", uint8_t, k, int8_t, 'a', void*, &x);
        for (uint8_t i = 5; i < 10; i++) {
            EMTRACE_F("Hello, World! {:d}", int8_t, y);
            EMTRACE("  test\n");
            EMTRACELN_F("Hello, World! 0x{0:x} {2:d} {1:d}", uint8_t, i, int8_t, 'a', void*, &x);
            EMTRACELN_F("{:-^20d}", uint8_t, i);
            for (int j = i; j > 3; j--) {
                EMTRACELN_F("|{:^18d}|", int, j);
            }
            EMTRACE(
                "--------------------\n"
                "|                  |\n"
                "--------------------\n"
            );
        }

        bar();

        EMTRACE_S("Hello World!\n");
    }
}
