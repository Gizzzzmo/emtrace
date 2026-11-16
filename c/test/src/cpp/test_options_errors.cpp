// Tests the options that can be passed to MTRACE, and how errors of a sink are reported.
#include "cxx_test.hpp"
#include "test_bytes.hpp"
#include "test_sinks.hpp"

#include <cstddef>
#include <cstdint>
#include <emtrace/emtrace.hpp>
#include <optional>
#include <string>

using cxxtest::Bytes;
using cxxtest::bytes_of;
using cxxtest::CapacityError;
using cxxtest::ptr_width;
using cxxtest::RecordingSink;

namespace {

auto is_aligned(const void* address, std::size_t alignment) -> bool {
    return reinterpret_cast<std::uintptr_t>(address) % alignment == 0;
}

} // namespace

CXX_TEST(custom_sink_receives_the_whole_record) {
    RecordingSink sink;
    MTRACE("{} {}", 1, 2, emt::opt_sink(sink));

    CXX_CHECK_EQ(sink.begins, 1);
    CXX_CHECK_EQ(sink.finishes, 1);
    CXX_CHECK_EQ(sink.bytes.size(), ptr_width + (2 * sizeof(int)));
    CXX_CHECK(sink.last_record != nullptr);
}

CXX_TEST(custom_sink_is_reused_across_records) {
    RecordingSink sink;
    MTRACE("{}", 1, emt::opt_sink(sink));
    MTRACE("{}", 2, emt::opt_sink(sink));
    MTRACE("{}", 3, emt::opt_sink(sink));

    CXX_CHECK_EQ(sink.begins, 3);
    CXX_CHECK_EQ(sink.finishes, 3);
    CXX_CHECK_EQ(sink.bytes.size(), 3 * (ptr_width + sizeof(int)));
}

CXX_TEST(records_are_aligned_to_256_bytes_by_default) {
    for (int i = 0; i < 4; i++) {
        RecordingSink sink;
        MTRACE("{}", i, emt::opt_sink(sink));
        CXX_CHECK(is_aligned(sink.last_record, 256));
    }
}

CXX_TEST(opt_align_increases_the_alignment) {
    RecordingSink sink;

    MTRACE("{}", 1, emt::opt_align<512>, emt::opt_sink(sink));
    CXX_CHECK(is_aligned(sink.last_record, 512));

    MTRACE("{}", 1, emt::opt_sink(sink), emt::opt_align<4096>);
    CXX_CHECK(is_aligned(sink.last_record, 4096));

    MTRACE("no args", emt::opt_align<8192>, emt::opt_sink(sink));
    CXX_CHECK(is_aligned(sink.last_record, 8192));
}

CXX_TEST(alignment_does_not_change_the_data_written) {
    RecordingSink default_aligned;
    RecordingSink aligned;
    MTRACE("{} {}", 1, std::string{"a"}, emt::opt_sink(default_aligned));
    MTRACE("{} {}", 1, std::string{"a"}, emt::opt_align<2048>, emt::opt_sink(aligned));

    CXX_CHECK_EQ(cxxtest::payload(aligned.bytes), cxxtest::payload(default_aligned.bytes));
}

CXX_TEST(errors_are_returned_by_default) {
    RecordingSink sink;
    sink.capacity = ptr_width + sizeof(int);

    const std::optional<CapacityError> fits = MTRACE("{}", 1, emt::opt_sink(sink));
    CXX_CHECK(!fits.has_value());

    const std::optional<CapacityError> overflow = MTRACE("{}", 1, emt::opt_sink(sink));
    CXX_CHECK(overflow.has_value());
    CXX_CHECK_EQ(overflow.value_or(CapacityError{0}).needed, ptr_width);
}

CXX_TEST(errors_in_the_arguments_are_returned) {
    RecordingSink sink;
    sink.capacity = ptr_width + 2;

    // The pointer fits, the integer doesn't
    const auto error = MTRACE("{}", 1, emt::opt_sink(sink));
    CXX_CHECK(error.has_value());
    CXX_CHECK_EQ(error.value_or(CapacityError{0}).needed, sizeof(int));
}

CXX_TEST(explicitly_not_throwing_returns_the_error) {
    RecordingSink sink;
    sink.capacity = 0;

    const auto error = MTRACE("{}", 1, emt::opt_no_throw, emt::opt_sink(sink));
    CXX_CHECK(error.has_value());
}

CXX_TEST(opt_throw_throws_instead_of_returning) {
    RecordingSink sink;
    sink.capacity = 0;

    bool thrown = false;
    try {
        MTRACE("{}", 1, emt::opt_throw, emt::opt_sink(sink));
    } catch (...) { thrown = true; }
    CXX_CHECK(thrown);
}

CXX_TEST(opt_throw_does_not_throw_without_an_error) {
    RecordingSink sink;

    bool thrown = false;
    try {
        MTRACE("{}", 1, emt::opt_throw, emt::opt_sink(sink));
    } catch (...) { thrown = true; }
    CXX_CHECK(!thrown);
    CXX_CHECK_EQ(sink.bytes.size(), ptr_width + sizeof(int));
}

CXX_TEST(opt_throw_fails_at_every_stage) {
    // capacity for the pointer, but not the argument, and for nothing
    for (const std::size_t capacity : {std::size_t{0}, ptr_width, ptr_width + 3}) {
        RecordingSink sink;
        sink.capacity = capacity;

        bool thrown = false;
        try {
            MTRACE("{}", 1, emt::opt_throw, emt::opt_sink(sink));
        } catch (...) { thrown = true; }
        CXX_CHECK(thrown);
    }
}

CXX_TEST(formatter_options_do_not_change_the_data) {
    RecordingSink py;
    RecordingSink c_style;
    MTRACE("{}", 5, emt::opt_py_fmt, emt::opt_sink(py));
    MTRACE("%d", 5, emt::opt_c_style_fmt, emt::opt_sink(c_style));

    CXX_CHECK_EQ(cxxtest::payload(py.bytes), bytes_of(5));
    CXX_CHECK_EQ(cxxtest::payload(c_style.bytes), bytes_of(5));
}

CXX_TEST(no_format_option_allows_braces_in_the_string) {
    RecordingSink sink;
    MTRACE("{} {{ }}", emt::opt_no_fmt, emt::opt_sink(sink));

    CXX_CHECK_EQ(sink.bytes.size(), ptr_width);
}

CXX_TEST(options_are_recognized_in_any_position) {
    static_assert(
        emt::internal::get_formatter<int, emt::internal::OptFormatter<EMT_C_STYLE_FORMAT>, int>() ==
        EMT_C_STYLE_FORMAT
    );
    static_assert(emt::internal::get_alignment<emt::internal::OptAlignment<1024>, int>() == 1024);
    static_assert(emt::internal::get_alignment<int>() == EMT_DEFAULT_ALIGNMENT);
    static_assert(emt::internal::get_throw<int, emt::internal::OptThrow<true>>());
    static_assert(!emt::internal::get_throw<int>());
}
