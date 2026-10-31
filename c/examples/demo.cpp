#include "emtrace/emtrace.h"

EMTRACE_MAGIC(3);

auto main() -> int {
    emtrace_init();

    EMTRACELN("kjalsdjla");
    EMTRACELN_F("Here we go again {}", VAL(int, 8));
}
