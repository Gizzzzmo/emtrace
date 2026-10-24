// Guest static library of the first cross-language interop demo: it is linked
// into the Rust executables rust/examples/interop_rust_host.rs and
// rust/examples/interop_combined.rs, which host the emtrace magic. This library
// deliberately has no `EMTRACE_MAGIC` of its own - its `EMTRACE` calls resolve
// the `emt_default_*` symbols at link time against the *executable's*
// definitions, so its records flow through the host's single sink alongside the
// host's own records.
#include "emtrace/emtrace.h"

// Exported ABI, linked from the Rust host executables named in the comment above.
void c_side_trace(int iteration);
void c_side_goodbye(void);

void c_side_trace(int iteration) { EMTRACELN_F("C library: iteration {:d}", VAL(int, iteration)); }

void c_side_goodbye(void) { EMTRACE("C library: goodbye\n"); }
