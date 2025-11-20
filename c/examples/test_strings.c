#include "test_utils.h"
#include <emtrace/emtrace.h>

EXPECT_OUTPUT(
    "Hello, World!\n"
    "Empty string: \n"
    "Unicode: 😀✅©\n"
    "Long string: first second third\n"
);

int main(void) {
    EMTRACE_INIT();

    // Test basic string
    EMTRACELN("Hello, World!");

    // Test empty string
    EMTRACE("Empty string: ");
    EMTRACELN("");

    // Test unicode strings
    const char* emoji_smile = "😀";
    const char* emoji_check = "✅";
    const char* copyright = "©";

    EMTRACE("Unicode: ");
    EMTRACE_S(emoji_smile);
    EMTRACE_S(emoji_check);
    EMTRACE_S(copyright);
    EMTRACELN("");

    // Test multiple strings
    const char* long_string = "first second third";

    EMTRACE("Long string: ");
    EMTRACELN_S(long_string);

    return 0;
}
