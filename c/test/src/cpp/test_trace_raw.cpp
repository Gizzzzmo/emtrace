// Tests the raw bytes that tracing values of the basic types writes to a sink. The format
// information lives in the binary, so a record consists of just a pointer to it followed by the
// bytes of the arguments.
#include "cxx_test.hpp"
#include "test_bytes.hpp"

#include <cstdint>
#include <emtrace/emtrace.hpp>
#include <string>
#include <string_view>
#include <vector>

using cxxtest::Bytes;
using cxxtest::bytes_of;
using cxxtest::concat;
using cxxtest::payload;
using cxxtest::pointer_of;
using cxxtest::ptr_width;
using cxxtest::text;

CXX_TEST(integer_argument) {
    Bytes capture;
    MTRACE("{}", 32, emt::opt_sink(capture));

    CXX_REQUIRE_EQ(capture.size(), ptr_width + sizeof(int));
    CXX_CHECK_EQ(payload(capture), bytes_of(32));
}

CXX_TEST(no_arguments_writes_only_the_pointer) {
    Bytes capture;
    MTRACE("nothing but a string", emt::opt_sink(capture));

    CXX_CHECK_EQ(capture.size(), ptr_width);
}

CXX_TEST(arguments_are_written_in_order_without_padding) {
    Bytes capture;
    MTRACE(
        "{} {} {} {} {} {} {}", true, 'c', std::int8_t{-3}, std::uint16_t{513}, std::int64_t{-5},
        1.5F, 2.25, emt::opt_sink(capture)
    );

    CXX_CHECK_EQ(
        payload(capture),
        concat(
            bytes_of(true), bytes_of('c'), bytes_of(std::int8_t{-3}), bytes_of(std::uint16_t{513}),
            bytes_of(std::int64_t{-5}), bytes_of(1.5F), bytes_of(2.25)
        )
    );
}

CXX_TEST(strings_are_null_terminated) {
    Bytes capture;
    const std::string owned = "owned";
    const std::string_view view = "view";
    const char* c_string = "c string";

    MTRACE("{} {} {}", owned, view, c_string, emt::opt_sink(capture));

    CXX_CHECK_EQ(payload(capture), concat(text("owned"), text("view"), text("c string")));
}

CXX_TEST(empty_string_is_only_the_terminator) {
    Bytes capture;
    MTRACE("{}", std::string{}, emt::opt_sink(capture));

    CXX_CHECK_EQ(payload(capture), Bytes{0});
}

CXX_TEST(strings_and_numbers_can_be_mixed) {
    Bytes capture;
    MTRACE("{} {} {}", 1, std::string{"two"}, 3.0, emt::opt_sink(capture));

    CXX_CHECK_EQ(payload(capture), concat(bytes_of(1), text("two"), bytes_of(3.0)));
}

CXX_TEST(options_can_come_before_the_arguments) {
    Bytes before;
    Bytes after;
    MTRACE("{}", emt::opt_sink(before), 7);
    MTRACE("{}", 7, emt::opt_sink(after));

    CXX_CHECK_EQ(payload(before), payload(after));
    CXX_CHECK_EQ(payload(before), bytes_of(7));
}

CXX_TEST(a_call_site_always_writes_the_same_pointer) {
    std::vector<Bytes> pointers;
    for (int i = 0; i < 3; i++) {
        Bytes capture;
        MTRACE("{}", i, emt::opt_sink(capture));
        pointers.push_back(pointer_of(capture));
    }

    CXX_CHECK_EQ(pointers[0], pointers[1]);
    CXX_CHECK_EQ(pointers[1], pointers[2]);
}

CXX_TEST(different_call_sites_write_different_pointers) {
    Bytes first;
    Bytes second;
    MTRACE("{}", 1, emt::opt_sink(first));
    MTRACE("{}", 1, emt::opt_sink(second));

    CXX_CHECK(pointer_of(first) != pointer_of(second));
}

CXX_TEST(traceln_and_trace_share_the_same_data) {
    Bytes plain;
    Bytes with_newline;
    MTRACE("{}", 5, emt::opt_sink(plain));
    MTRACELN("{}", 5, emt::opt_sink(with_newline));

    CXX_CHECK_EQ(payload(plain), payload(with_newline));
}
