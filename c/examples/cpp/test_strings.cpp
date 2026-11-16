#include "test_utils.h"
#include <emtrace/emtrace.hpp>
#include <string>
#include <string_view>

EXPECT_OUTPUT(
    "Hello, World!\n"
    "Empty string: \n"
    "Unicode: 😀✅©\n"
    "Long string: first second third\n"
    "std::string: owned\n"
    "std::string_view: viewed\n"
    "empty std::string: \n"
);

MTRACE_MAGIC(CPP_TEST_SINK);

auto main() -> int {
    emt::init();

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

    // Test the C++ string types
    MTRACELN("std::string: {}", std::string{"owned"});
    MTRACELN("std::string_view: {}", std::string_view{"viewed"});
    MTRACELN("empty std::string: {}", std::string{});

    return 0;
}
