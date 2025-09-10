#include "emtrace/emtrace.h"
#include <stdint.h>

void bar(void) { EMTRACE_F("YAY\n"); }

static const char __attribute__((section(".emtrace"))) emtraceMagicConstant[32] = {
    0xd1, 0x97, 0xf5, 0x22, 0xd9, 0x26, 0x9f, 0xd1, 0xad, 0x70, 0x33, 0x92, 0xf6, 0x59, 0xdf, 0xd0,
    0xfb, 0xec, 0xbd, 0x60, 0x97, 0x13, 0x25, 0xe8, 0x92, 0x01, 0xb2, 0x5a, 0x38, 0x5d, 0x9e, 0xc7
};

int main(void) {
    const void* magic_address = emtraceMagicConstant;
    emtrace_out_file(&magic_address, sizeof(magic_address), stdout);
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
