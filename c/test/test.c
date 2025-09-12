#include "emtrace/emtrace.h"
#include <stdint.h>

void bar(void) { EMTRACE_F("YAY\n"); }

int main(void) {
    EMTRACE_INIT();
    int x = 1;
    int y = 2;
    void* ptr = &x;
    bar();
    // TRACE("Hello, World!");
    for (int i = 0; i < 1; i++) {
        EMTRACE_F("Hello, World! {:d} {{}}", int8_t, y);
        EMTRACE("  test {} {}\n");
        EMTRACE_F("Hello, World! 0x{0:x} {2:d} {1:d}", int8_t, i, int8_t, 'a', void*, &x);
        for (int i = 5; i < 10; i++) {
            EMTRACE_F("Hello, World! {:d}", int8_t, y);
            EMTRACE("  test\n");
            EMTRACELN_F("Hello, World! 0x{0:x} {2:d} {1:d}", int8_t, i, int8_t, 'a', void*, &x);
            EMTRACELN_F("{:-^20d}", uint8_t, i);
            for (int j = i; j > 3; j--) {
                EMTRACELN_F("|{:^18d}|", uint8_t, j);
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
