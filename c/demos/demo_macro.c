#include "emtrace/emtrace.h"

static void foo(void) {
    EMTRACE_F("", int, 1);
    EMTRACE_F("", int, 1, int, 1);
    EMTRACE_F("", int, 1, int, 1, int, 1);
    EMTRACE_F("", int, 1, int, 1, int, 1, int, 1);
    EMTRACE_F("", int, 1, int, 1, int, 1, int, 1, int, 1);
    EMTRACE_F("", int, 1, int, 1, int, 1, int, 1, int, 1, int, 1);
    EMTRACE_F("", int, 1, int, 1, int, 1, int, 1, int, 1, int, 1, int, 1);
    EMTRACE_F("", int, 1, int, 1, int, 1, int, 1, int, 1, int, 1, int, 1, int, 1);
}
