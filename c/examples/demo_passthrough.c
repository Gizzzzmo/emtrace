#include <emtrace/emtrace.h>
#include <stdio.h>
#include <stdlib.h>

// COBS passthrough uses a null byte not just to mark the end of a frame, but also its beginning.
// Any bytes after a frame and before the next null byte can thus be considered passed through.
// Null bytes in passed-through output need to be escaped with an additionally null-byte,
// although that shouldn't be a concern with text-based output.

#undef EMT_ENCODING
#undef EMT_LOCK
#undef EMT_UNLOCK
#undef EMT_OUT_FN

#define EMT_ENCODING EMT_ENCODING_COBS_PASSTHROUGH
#define EMT_LOCK EMT_LOCK_FILE_COBS_PASSTHROUGH
#define EMT_UNLOCK EMT_UNLOCK_FILE_COBS
#define EMT_OUT_FN EMT_OUT_FILE_COBS

static int run(void) {
    EMTRACE_INIT();

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
