#include "emtrace/emtrace.h"
#include "test_utils.h"
#include <stdint.h>

static void bar(void) { EMTRACE("YAY\n"); }

int main(void) {
    EMTRACE_INIT();
    int8_t x = 1;
    int8_t y = 2;
    bar();
    // TRACE("Hello, World!");
    for (uint8_t k = 0; k < 1; k++) {
        EMTRACE_F("Hello, World! {:d} {{}}", VAL(int8_t, y));
        EMTRACE("  test {} {}\n");
        EMTRACE_F("Hello, World! 0x{0:x} {2:d}", VAL(uint8_t, k), VAL(int8_t, 'a'));
        for (uint8_t i = 5; i < 10; i++) {
            EMTRACE_F("Hello, World! {:d}", VAL(int8_t, y));
            EMTRACE("  test\n");
            EMTRACELN_F("Hello, World! 0x{0:x} {2:d}", VAL(uint8_t, i), VAL(int8_t, 'a'));
            EMTRACELN_F("{:-^20d}", VAL(uint8_t, i));
            for (int j = i; j > 3; j--) {
                EMTRACELN_F("|{:^18d}|", VAL(int, j));
            }
            EMTRACE(
                "--------------------\n"
                "|                  |\n"
                "--------------------\n"
            );
        }

        bar();

        EMTRACE_F("{}", STR("Hello World!\n"));
    }
}
