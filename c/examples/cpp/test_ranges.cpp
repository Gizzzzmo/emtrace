#include "test_utils.h"
#include <deque>
#include <emtrace/emtrace.hpp>
#include <forward_list>
#include <iterator>
#include <list>
#include <map>
#include <ranges>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <vector>

// Any sized range whose elements are traceable can be traced, not just the std containers that
// have a dedicated Trace specialization. This covers a variety of range kinds: views, adaptors
// chains, containers with non-contiguous storage, ranges with prvalue elements, and ranges of
// ranges.

EXPECT_OUTPUT(
    "transform: [1, 4, 9, 16, 25, 36, 49, 64, 81, 100]\n"
    "transform then take: [1, 4, 9]\n"
    "floating point elements: [2.0, 1.5, 1.0, 0.5]\n"
    "iota: [0, 1, 2, 3, 4]\n"
    "empty: [] []\n"
    "reverse: [6, 5, 4, 3, 2, 1]\n"
    "take: [1, 2, 3]\n"
    "drop: [3, 4, 5, 6]\n"
    "ref view: [1, 2, 3, 4, 5, 6]\n"
    "std::list: [7, 8, 9]\n"
    "std::deque: [10, 11]\n"
    "std::set: [1, 2, 3]\n"
    "map keys: [1, 2]\n"
    "map values: [1.5, 2.5]\n"
    "take from list: [7, 8]\n"
    "sized subrange of list: [8, 9]\n"
    "span with static extent: [3, 2]\n"
    "counted: [4, 3]\n"
    "std::list of std::string: ['x', 'yy']\n"
    "string_view elements: ['odd', 'even', 'odd', 'even', 'odd', 'even']\n"
    "ranges of ranges: [[0], [0, 1], [0, 1, 2]]\n"
    "vector of lists: [[1], [2, 3], []]\n"
    "chars of a const range: [char(0x61), char(0x62)]\n"
);

MTRACE_MAGIC(CPP_TEST_SINK);

namespace views = std::views;

auto main() -> int {
    emt::init();

    std::vector<int> vec{1, 2, 3, 4, 5, 6};
    std::list<int> list{7, 8, 9};
    std::deque<int> deque{10, 11};
    std::set<int> set{3, 1, 2};
    std::map<int, double> map{{1, 1.5}, {2, 2.5}};
    int arr[4] = {4, 3, 2, 1};
    int squares[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    const std::vector<char> chars{'a', 'b'};
    std::list<std::string> strings{"x", "yy"};
    std::vector<std::list<int>> lists{{1}, {2, 3}, {}};

    auto square = [](int x) -> int { return x * x; };
    auto halve = [](int x) -> double { return x * 0.5; };
    auto parity = [](int x) -> std::string_view { return x % 2 != 0 ? "odd" : "even"; };
    auto up_to = [](int n) -> auto { return views::iota(0, n); };

    MTRACELN("transform: {}", squares | views::transform(square));
    MTRACELN("transform then take: {}", squares | views::transform(square) | views::take(3));
    MTRACELN("floating point elements: {}", arr | views::transform(halve));

    MTRACELN("iota: {}", views::iota(0, 5));
    MTRACELN("empty: {} {}", views::iota(0, 0), views::empty<int>);
    MTRACELN("reverse: {}", vec | views::reverse);
    MTRACELN("take: {}", vec | views::take(3));
    MTRACELN("drop: {}", vec | views::drop(2));
    MTRACELN("ref view: {}", views::all(vec));

    MTRACELN("std::list: {}", list);
    MTRACELN("std::deque: {}", deque);
    MTRACELN("std::set: {}", set);
    MTRACELN("map keys: {}", map | views::keys);
    MTRACELN("map values: {}", map | views::values);
    MTRACELN("take from list: {}", list | views::take(2));
    MTRACELN(
        "sized subrange of list: {}", std::ranges::subrange(std::next(list.begin()), list.end(), 2)
    );
    MTRACELN("span with static extent: {}", std::span<int, 3>{arr, 3} | views::drop(1));
    MTRACELN("counted: {}", views::counted(arr, 2));

    MTRACELN("std::list of std::string: {}", strings);
    MTRACELN("string_view elements: {}", vec | views::transform(parity));

    MTRACELN("ranges of ranges: {}", views::iota(1, 4) | views::transform(up_to));
    MTRACELN("vector of lists: {}", lists);

    MTRACELN("chars of a const range: {}", chars | views::all);

    // Ranges that don't know their size up front, or that can't be iterated through a const
    // reference can't be traced.
    static_assert(!emt::Traceable<std::forward_list<int>>);
    static_assert(!emt::Traceable<decltype(vec | views::filter(square))>);
    static_assert(!emt::Traceable<decltype(list | views::drop(1))>);

    return 0;
}
