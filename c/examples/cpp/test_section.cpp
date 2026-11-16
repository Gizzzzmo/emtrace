// End-to-end test for placing the trace records in a custom section instead of `.emtrace`
// (the C++ counterpart of rust/examples/test_section.rs).
//
// The MGIC record and the record of every MTRACE call end up in `.emtsec`, because that is what
// the default section attribute is set to before including the header. The parser finds them via
// `emtrace --section-name .emtsec` (see tests/test_emtrace.py::test_custom_section_cpp), which is
// why this binary is deliberately not part of the generic list of end-to-end tests.
//
// The parser resolves record pointers as offsets within a single section, so the MGIC record and
// all trace records need to be placed in the same section.
#define EMT_DEFAULT_SEC_ATTR __attribute__((used, section(".emtsec"))) const

#include "test_utils.h"
#include <emtrace/emtrace.hpp>

EXPECT_OUTPUT(
    "Custom section works: 7\n"
    "Custom section works: done\n"
);

MTRACE_MAGIC();

auto main() -> int {
    emt::init();

    MTRACELN("Custom section works: {}", 7);
    MTRACELN("Custom section works: done");

    return 0;
}
