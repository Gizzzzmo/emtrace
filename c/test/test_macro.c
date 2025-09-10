#include "emtrace.h"

void foo(void)
{
    void (*out)(const void*, size_t) = NULL;

#define TRACE(...) EMTRACE(out, __VA_ARGS__)

    EMTRACE_F("", int, 1);
    EMTRACE_F("", int, 1, int, 1);
    EMTRACE_F("", int, 1, int, 1, int, 1);
    EMTRACE_F("", int, 1, int, 1, int, 1, int, 1);
    EMTRACE_F("", int, 1, int, 1, int, 1, int, 1, int, 1);
    EMTRACE_F("", int, 1, int, 1, int, 1, int, 1, int, 1, int, 1);
    EMTRACE_F("", int, 1, int, 1, int, 1, int, 1, int, 1, int, 1, int, 1);
    EMTRACE_F("", int, 1, int, 1, int, 1, int, 1, int, 1, int, 1, int, 1, int, 1);
}
