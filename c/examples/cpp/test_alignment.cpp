#include "test_utils.h"
#include <cstddef>
#include <emtrace/emtrace.hpp>
#include <string>
#include <vector>

// By default records are aligned to 256 bytes, which allows pointers to them to be shifted by 8
// bits before they are written to the stream. Using a bigger shift saves more space in the output
// per record, but also requires a bigger alignment of the records.
//
// The shift is part of the sink given to MTRACE_MAGIC (emt::ChangePtr<ptr_width, shift, inner>),
// the alignment of each record is set with emt::opt_align. Here the pointers are written with a
// shift of 10 bits, so every record needs to be aligned to at least 1024 bytes. Bigger alignments
// are fine, too.

EXPECT_OUTPUT(
    "no arguments\n"
    "aligned to 1024: 1\n"
    "aligned to 4096: 2 3.5 text\n"
    "a list: [1, 2, 3]\n"
    "a list of strings: ['a', 'bc']\n"
);

constexpr unsigned char test_alignment_power = 10;
constexpr unsigned char test_ptr_width = 3;

MTRACE_MAGIC(CPP_TEST_SINK_ALIGNED(test_ptr_width, test_alignment_power));

static_assert(emt::impl::SinkImplType::alignment_power == test_alignment_power);
static_assert(emt::impl::SinkImplType::ptr_width == test_ptr_width);

auto main() -> int {
    emt::init();

    constexpr std::size_t alignment = std::size_t{1} << test_alignment_power;

    MTRACELN("no arguments", emt::opt_align<alignment>);
    MTRACELN("aligned to 1024: {}", 1, emt::opt_align<1024>);
    MTRACELN("aligned to 4096: {} {} {}", 2, 3.5, std::string{"text"}, emt::opt_align<4096>);

    std::vector<int> ints{1, 2, 3};
    MTRACELN("a list: {}", ints, emt::opt_align<alignment>);

    std::vector<std::string> strings{"a", "bc"};
    MTRACELN("a list of strings: {}", strings, emt::opt_align<2 * alignment>);

    return 0;
}
