#include "test_utils.h"
#include <array>
#include <cstdint>
#include <emtrace/emtrace.hpp>
#include <span>
#include <string>
#include <string_view>
#include <vector>

// Containers whose elements are themselves variable in size (strings, other lists) need every
// element to be framed individually: strings are null-terminated and dynamic lists carry their
// length. Containers of fixed size elements are written as a single block of bytes.

EXPECT_OUTPUT(
    "[1, 2, 3, 4, 5, 6, 7, 8, 9, 10]\n"
    "[1, 2, 3, 4, 5, 6, 7, 8, 9, 10]\n"
    "[]\n"
    "1f, 1f, 1f, 1f, 1f, 1f, 1f, 1f\n"
    "['A', 'vector', 'of', 'string_views']\n"
    "A vector of strings\n"
    "['An', 'array', 'of', 'C', 'strings']\n"
    "['An', 'array', 'of', 'strings']\n"
    "11 11 11 11\n"
    "11 11 11 11\n"
    "11 11 11 11\n"
    "11 11 11 11\n"
    "[[1, 2, 3, 4, 5, 6], [3, 3, 3, 3]]\n"
    "[1, 2, 3, 4, 5, 6][3, 3, 3, 3]\n"
    "[['a', 'bc'], [], ['d']]\n"
    "[[1, 2], [3]]\n"
    "[[1, 2], [3, 4], [5, 6]]\n"
    "[[1], [2, 3]]\n"
    "[['x', 'y'], ['z', 'w']]\n"
);

MTRACE_MAGIC(CPP_TEST_SINK);

using std::uint16_t;

auto main() -> int {
    emt::init();

    std::vector<int> ints{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    MTRACELN("{}\n{}", ints, std::span<int>{ints});

    std::vector<int> no_ints;
    MTRACELN("{}", no_ints);

    std::array<uint16_t, 8> shorts{31, 31, 31, 31, 31, 31, 31, 31};
    MTRACELN("{:, *x}", shorts);

    std::vector<std::string_view> views{"A", "vector", "of", "string_views"};
    std::vector<std::string> strings{"A", "vector", "of", "strings"};
    MTRACELN("{}\n{: *}", views, strings);

    std::array<const char*, 5> c_strings{"An", "array", "of", "C", "strings"};
    MTRACELN("{}", c_strings);

    std::array<std::string, 4> string_array{"An", "array", "of", "strings"};
    MTRACELN("{}", string_array);

    std::array<std::array<uint16_t, 4>, 4> nested_static{};
    for (auto& row : nested_static) {
        row.fill(3);
    }
    MTRACELN("{:\n* *b}", nested_static);

    std::vector<std::vector<int>> nested_vectors{{1, 2, 3, 4, 5, 6}, {3, 3, 3, 3}};
    MTRACELN("{}", nested_vectors);
    MTRACELN("{:*}", nested_vectors);

    std::vector<std::vector<std::string>> nested_strings{{"a", "bc"}, {}, {"d"}};
    MTRACELN("{}", nested_strings);

    std::vector<std::span<int>> spans{
        std::span<int>{ints}.first(2),
        std::span<int>{ints}.subspan(2, 1),
    };
    // spans of {1, 2} and {3}
    MTRACELN("{}", spans);

    std::vector<std::array<int, 2>> pairs{{1, 2}, {3, 4}, {5, 6}};
    MTRACELN("{}", pairs);

    std::array<std::vector<int>, 2> static_of_dynamic{std::vector<int>{1}, std::vector<int>{2, 3}};
    MTRACELN("{}", static_of_dynamic);

    std::array<std::array<std::string, 2>, 2> static_of_static{{{"x", "y"}, {"z", "w"}}};
    MTRACELN("{}", static_of_static);

    return 0;
}
