// Host executable of the second cross-language interop demo: this program owns
// the entry point AND the emtrace magic (its `EMTRACE_MAGIC` expansion defines
// `g_emt_default_magic` and the `emt_default_*` pipeline symbols). The Rust
// static library rust/examples/interop_rust_guest.rs is linked in as a *guest*:
// it has no `magic!()` of its own, so its `traceln!` calls resolve those symbols
// at link time against this executable's definitions - records from both
// languages flow through one sink (stdio on stdout, unbuffered via
// `emtrace_init()`).
#include <stddef.h>

#include <emtrace/emtrace.h>

#include "test_utils.h"

extern void rust_side_trace(int iteration);
extern void rust_side_goodbye(void);

EXPECT_OUTPUT(
    "Rust library: iteration 0\n"
    "C executable: iteration 0\n"
    "Rust library: iteration 1\n"
    "C executable: iteration 1\n"
    "Rust library: iteration 2\n"
    "C executable: iteration 2\n"
    "Rust library: iteration 3\n"
    "C executable: iteration 3\n"
    "Rust library: iteration 4\n"
    "C executable: iteration 4\n"
    "Rust library: goodbye\n"
    "C executable: goodbye\n"
);

EMTRACE_MAGIC(size_t);

int main(void) {
    emtrace_init();
    for (int i = 0; i < 5; i++) {
        rust_side_trace(i);
        EMTRACELN_F("C executable: iteration {:d}", VAL(int, i));
    }
    rust_side_goodbye();
    EMTRACE("C executable: goodbye\n");
    return 0;
}
