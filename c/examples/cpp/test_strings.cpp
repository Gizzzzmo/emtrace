#include "test_utils.h"
#include <emtrace/emtrace.h>
#include <emtrace/emtrace.hpp>

EXPECT_OUTPUT(
    "Hello, World!\n"
    "Empty string: \n"
    "Unicode: 😀✅©\n"
    "Long string: first second third\n"
);

#ifdef TEST_COBS
EMTRACE_MAGIC_COBS(3);
#else
EMTRACE_MAGIC(3);
#endif

auto main() -> int {
    emtrace_init();

    // Test basic string
    MTRACELN("Hello, World!");

    // Test empty string
    MTRACE("Empty string: ");
    MTRACELN("");

    // Test unicode strings
    const char* emoji_smile = "😀";
    const char* emoji_check = "✅";
    const char* copyright = "©";

    MTRACE("Unicode: {}{}{}\n", emoji_smile, emoji_check, copyright);

    // Test multiple strings
    const char* long_string = "first second third";

    MTRACE("Long string: ");
    MTRACE("{}\n", long_string);

    return 0;
}
