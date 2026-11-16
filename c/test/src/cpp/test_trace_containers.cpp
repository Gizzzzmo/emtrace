// Tests the bytes written for containers, and for ranges: lists of dynamic size are preceded by
// their length, lists of fixed size are not. Elements that are of variable size themselves
// (strings, and other dynamic lists) are framed individually.
#include "cxx_test.hpp"
#include "test_bytes.hpp"

#include <array>
#include <cstddef>
#include <deque>
#include <emtrace/emtrace.hpp>
#include <forward_list>
#include <list>
#include <ranges>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using cxxtest::Bytes;
using cxxtest::bytes_of;
using cxxtest::bytes_of_all;
using cxxtest::concat;
using cxxtest::payload;
using cxxtest::text;
using cxxtest::varint;

namespace {

auto repeat_x(int count) -> std::string {
    std::string result;
    result.assign(static_cast<std::size_t>(count), 'x');
    return result;
}

} // namespace

CXX_TEST(empty_vector_is_only_the_length) {
    Bytes capture;
    std::vector<int> empty;
    MTRACE("{}", empty, emt::opt_sink(capture));

    CXX_CHECK_EQ(payload(capture), varint(0));
}

CXX_TEST(vector_of_integers) {
    Bytes capture;
    const std::vector<int> values{1, 3, 4, 6};
    MTRACE("{}", values, emt::opt_sink(capture));

    CXX_CHECK_EQ(payload(capture), concat(varint(4), bytes_of_all(values)));
}

CXX_TEST(lists_with_dynamic_size_look_the_same) {
    const std::vector<int> vector{1, 2, 3};
    const std::list<int> list{1, 2, 3};
    const std::deque<int> deque{1, 2, 3};
    const std::set<int> set{1, 2, 3};
    int array[3] = {1, 2, 3};
    const std::span<int> span{array};
    const std::span<int, 3> static_extent_span{array};
    const auto view = std::views::iota(1, 4);
    const auto transformed =
        std::views::iota(0, 3) | std::views::transform([](int i) -> int { return i + 1; });

    Bytes expected;
    MTRACE("{}", vector, emt::opt_sink(expected));
    CXX_REQUIRE_EQ(payload(expected), concat(varint(3), bytes_of_all(vector)));

    const auto same_as_vector = [&](auto&& range) -> void {
        Bytes capture;
        MTRACE("{}", range, emt::opt_sink(capture));
        CXX_CHECK_EQ(payload(capture), payload(expected));
    };

    same_as_vector(list);
    same_as_vector(deque);
    same_as_vector(set);
    same_as_vector(span);
    same_as_vector(static_extent_span);
    same_as_vector(view);
    same_as_vector(transformed);
}

CXX_TEST(lists_with_static_size_have_no_length) {
    const std::array<int, 3> array{1, 2, 3};
    int c_array[3] = {1, 2, 3};

    Bytes from_array;
    Bytes from_c_array;
    MTRACE("{}", array, emt::opt_sink(from_array));
    MTRACE("{}", c_array, emt::opt_sink(from_c_array));

    CXX_CHECK_EQ(payload(from_array), bytes_of_all(array));
    CXX_CHECK_EQ(payload(from_c_array), bytes_of_all(array));
}

CXX_TEST(length_uses_multiple_bytes_when_needed) {
    for (const std::size_t size :
         {std::size_t{127}, std::size_t{128}, std::size_t{300}, std::size_t{20000}}) {
        Bytes capture;
        const std::vector<unsigned char> values(size, 7);
        MTRACE("{}", values, emt::opt_sink(capture));

        CXX_CHECK_EQ(payload(capture), concat(varint(size), Bytes(size, 7)));
    }
}

CXX_TEST(vector_of_strings) {
    Bytes capture;
    const std::vector<std::string> strings{"A", "vector", "of", "strings"};
    MTRACE("{}", strings, emt::opt_sink(capture));

    CXX_CHECK_EQ(
        payload(capture), concat(varint(4), text("A"), text("vector"), text("of"), text("strings"))
    );
}

CXX_TEST(different_kinds_of_strings_in_containers_look_the_same) {
    const std::vector<std::string> owned{"a", "bc"};
    const std::vector<std::string_view> views{"a", "bc"};
    const std::list<std::string> list{"a", "bc"};
    const std::vector<const char*> c_strings{"a", "bc"};

    Bytes expected;
    MTRACE("{}", owned, emt::opt_sink(expected));
    CXX_REQUIRE_EQ(payload(expected), concat(varint(2), text("a"), text("bc")));

    Bytes from_views;
    Bytes from_list;
    Bytes from_c_strings;
    MTRACE("{}", views, emt::opt_sink(from_views));
    MTRACE("{}", list, emt::opt_sink(from_list));
    MTRACE("{}", c_strings, emt::opt_sink(from_c_strings));

    CXX_CHECK_EQ(payload(from_views), payload(expected));
    CXX_CHECK_EQ(payload(from_list), payload(expected));
    CXX_CHECK_EQ(payload(from_c_strings), payload(expected));
}

CXX_TEST(empty_strings_in_a_container_are_still_terminated) {
    Bytes capture;
    const std::vector<std::string> strings{"", "", "x"};
    MTRACE("{}", strings, emt::opt_sink(capture));

    CXX_CHECK_EQ(payload(capture), concat(varint(3), text(""), text(""), text("x")));
}

CXX_TEST(static_array_of_strings_has_no_length) {
    Bytes capture;
    const std::array<std::string, 2> strings{"A", "B"};
    MTRACE("{}", strings, emt::opt_sink(capture));

    CXX_CHECK_EQ(payload(capture), concat(text("A"), text("B")));
}

CXX_TEST(nested_vectors) {
    Bytes capture;
    const std::vector<std::vector<int>> owning{{1, 2, 3, 4}, {10}};
    MTRACE("{}", owning, emt::opt_sink(capture));

    CXX_CHECK_EQ(
        payload(capture),
        concat(varint(2), varint(4), bytes_of_all(owning[0]), varint(1), bytes_of_all(owning[1]))
    );
}

CXX_TEST(vector_with_empty_vectors) {
    Bytes capture;
    const std::vector<std::vector<int>> nested{{}, {}, {5}};
    MTRACE("{}", nested, emt::opt_sink(capture));

    CXX_CHECK_EQ(payload(capture), concat(varint(3), varint(0), varint(0), varint(1), bytes_of(5)));
}

CXX_TEST(different_nestings_of_the_same_values_look_the_same) {
    const std::vector<std::vector<int>> owning{{1, 2, 3, 4}, {10}};
    const std::vector<std::span<const int>> spans{owning[0], owning[1]};
    const std::list<std::list<int>> lists{{1, 2, 3, 4}, {10}};

    Bytes expected;
    Bytes from_spans;
    Bytes from_lists;
    MTRACE("{}", owning, emt::opt_sink(expected));
    MTRACE("{}", spans, emt::opt_sink(from_spans));
    MTRACE("{}", lists, emt::opt_sink(from_lists));

    CXX_CHECK_EQ(payload(from_spans), payload(expected));
    CXX_CHECK_EQ(payload(from_lists), payload(expected));
}

CXX_TEST(vector_of_vectors_of_strings) {
    Bytes capture;
    const std::vector<std::vector<std::string>> nested{{"a", "bc"}, {}, {"d"}};
    MTRACE("{}", nested, emt::opt_sink(capture));

    CXX_CHECK_EQ(
        payload(capture),
        concat(varint(3), varint(2), text("a"), text("bc"), varint(0), varint(1), text("d"))
    );
}

CXX_TEST(vector_of_static_arrays_is_one_block) {
    Bytes capture;
    const std::vector<std::array<int, 2>> pairs{{1, 2}, {3, 4}, {5, 6}};
    MTRACE("{}", pairs, emt::opt_sink(capture));

    CXX_CHECK_EQ(
        payload(capture),
        concat(
            varint(3), bytes_of(1), bytes_of(2), bytes_of(3), bytes_of(4), bytes_of(5), bytes_of(6)
        )
    );
}

CXX_TEST(static_array_of_dynamic_lists) {
    Bytes capture;
    const std::array<std::vector<int>, 2> lists{std::vector<int>{1}, std::vector<int>{2, 3}};
    MTRACE("{}", lists, emt::opt_sink(capture));

    CXX_CHECK_EQ(
        payload(capture), concat(varint(1), bytes_of(1), varint(2), bytes_of(2), bytes_of(3))
    );
}

CXX_TEST(static_array_of_static_arrays_of_strings) {
    Bytes capture;
    const std::array<std::array<std::string, 2>, 2> strings{{{"x", "y"}, {"z", "w"}}};
    MTRACE("{}", strings, emt::opt_sink(capture));

    CXX_CHECK_EQ(payload(capture), concat(text("x"), text("y"), text("z"), text("w")));
}

CXX_TEST(vector_of_spans) {
    Bytes capture;
    std::vector<int> values{1, 2, 3};
    const std::vector<std::span<int>> spans{
        std::span<int>{values}.first(2), std::span<int>{values}.last(1)
    };
    MTRACE("{}", spans, emt::opt_sink(capture));

    CXX_CHECK_EQ(
        payload(capture),
        concat(varint(2), varint(2), bytes_of(1), bytes_of(2), varint(1), bytes_of(3))
    );
}

CXX_TEST(ranges_of_ranges) {
    Bytes capture;
    const auto up_to = [](int n) -> auto { return std::views::iota(0, n); };
    const auto triangle = std::views::iota(1, 4) | std::views::transform(up_to);
    MTRACE("{}", triangle, emt::opt_sink(capture));

    CXX_CHECK_EQ(
        payload(capture), concat(
                              varint(3), varint(1), bytes_of(0), varint(2), bytes_of(0),
                              bytes_of(1), varint(3), bytes_of(0), bytes_of(1), bytes_of(2)
                          )
    );
}

CXX_TEST(ranges_with_prvalue_elements) {
    Bytes capture;
    const std::vector<int> values{1, 2, 3};
    const auto strings = values | std::views::transform(repeat_x);
    MTRACE("{}", strings, emt::opt_sink(capture));

    CXX_CHECK_EQ(payload(capture), concat(varint(3), text("x"), text("xx"), text("xxx")));
}

CXX_TEST(ranges_over_const_elements) {
    // elements are const references, but serialize like elements of the plain type
    Bytes plain;
    Bytes constant;
    std::vector<char> chars{'a', 'b'};
    const std::vector<char>& const_chars = chars;
    MTRACE("{}", chars | std::views::all, emt::opt_sink(plain));
    MTRACE("{}", const_chars | std::views::all, emt::opt_sink(constant));

    CXX_CHECK_EQ(payload(plain), concat(varint(2), bytes_of('a'), bytes_of('b')));
    CXX_CHECK_EQ(payload(constant), payload(plain));
}

CXX_TEST(ranges_that_cannot_be_traced) {
    constexpr auto always = [](int) -> bool { return true; };
    using Filtered = decltype(std::declval<std::vector<int>&>() | std::views::filter(always));
    using Dropped = decltype(std::declval<std::list<int>&>() | std::views::drop(1));

    // not sized
    static_assert(!emt::Traceable<std::forward_list<int>>);
    static_assert(!emt::Traceable<Filtered>);
    // cannot be iterated through a const reference
    static_assert(!emt::Traceable<Dropped>);
    // elements that cannot be traced
    static_assert(!emt::Traceable<std::vector<void*>>);
    static_assert(emt::Traceable<std::vector<std::vector<std::string>>>);
}
