#include "emtrace/emtrace.h"
#include <cstddef>

EMTRACE_MAGIC(size_t);

auto main() -> int {
    emtrace_init();

    EMTRACELN("kjalsdjla");
    EMTRACELN_F("Here we go again {}", VAL(int, 8));
}
