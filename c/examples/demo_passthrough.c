#include <emtrace/emtrace.h>
#include <stdio.h>
#include <stdlib.h>

// COBS passthrough uses a null byte not just to mark the end of a frame, but also its beginning.
// Any bytes after a frame and before the next null byte can thus be considered passed through.
// Null bytes in passed-through output need to be escaped with an additionally null-byte,
// although that shouldn't be a concern with text-based output.

EMTRACE_MAGIC_COBS_PASSTHROUGH(size_t)

static int run(void) {
    emtrace_init();

    EMTRACE_F("Hello, World from emtrace!\n");
    printf("Hello, World from printf!\n");
    // Two successive null bytes should be escaped to a single null byte by emtrace
    putchar(0);
    putchar(0);

    return 0;
}

int main(int argc, char** argv) {
    // Only when called with "run" do we actually run our code.
    if (argc > 1 && strcmp(argv[1], "run") == 0) {
        return run();
    }

    // Otherwise invoke emtrace to run ourselves - this time with run
    char command[256];
    snprintf(command, sizeof(command), "emtrace %s -- run", argv[0]);
    return system(command);
}
