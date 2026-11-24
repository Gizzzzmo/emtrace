// Tests user-defined traces of structs: a simple one, whose memory is emitted as a whole (padding
// included, which is described by the padding of its children), and one whose fields are emitted
// one by one with emt::emit_fields (without padding).
#include "cxx_test.hpp"
#include "test_bytes.hpp"
#include "test_sinks.hpp"

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <emtrace/emtrace.hpp>
#include <optional>
#include <string>
#include <vector>

using cxxtest::Bytes;
using cxxtest::bytes_of;
using cxxtest::CapacityError;
using cxxtest::concat;
using cxxtest::payload;
using cxxtest::ptr_width;
using cxxtest::RecordingSink;
using cxxtest::text;

namespace {

struct Padded {
    std::uint8_t a; // followed by padding
    std::int32_t b;
    std::uint8_t c; // followed by padding
};

constexpr emt_size_t a_padding = offsetof(Padded, b) - sizeof(Padded::a);
constexpr emt_size_t c_padding = sizeof(Padded) - offsetof(Padded, c) - sizeof(Padded::c);

struct Fields {
    std::int32_t number;
    double real;
    std::string name;
    Padded padded;
};

} // namespace

template <>
struct emt::Trace<Padded> {
    using Children = emt::Ts<
        emt::Child<std::uint8_t, "a", a_padding>,
        emt::Child<std::int32_t, "b">,
        emt::Child<std::uint8_t, "c", c_padding>>;
    static constexpr emt::StringLiteral type_id = "struct";
    static constexpr emt::SizeType size_type = emt::StaticSize{1};
    static constexpr bool simple = true;

    template <typename S>
    static auto serialize(const Padded& value, S& sink) -> std::optional<typename S::Error> {
        return sink.out(&value, sizeof(Padded));
    }

    static auto size(const Padded& /*value*/) -> emt_size_t { return 1; }
};

template <>
struct emt::Trace<Fields> {
    using Children = emt::Ts<
        emt::Child<std::int32_t, "number">,
        emt::Child<double, "real">,
        emt::Child<std::string, "name">,
        emt::Child<Padded, "padded">>;
    static constexpr emt::StringLiteral type_id = "struct";
    static constexpr emt::SizeType size_type = emt::StaticSize{1};
    static constexpr bool simple = false;

    template <typename S>
    static auto serialize(const Fields& value, S& sink) -> std::optional<typename S::Error> {
        return emt::emit_fields(sink, value.number, value.real, value.name, value.padded);
    }

    static auto size(const Fields& /*value*/) -> emt_size_t { return 1; }
};

static_assert(emt::Traceable<Padded>);
static_assert(emt::Traceable<Fields>);
static_assert(a_padding > 0 && c_padding > 0);

// The type description of the simple struct covers its whole memory, padding included.
static_assert(emt::fixed_stream_size<Padded>() == sizeof(Padded));
static_assert(emt::fixed_stream_size<std::array<Padded, 3>>() == 3 * sizeof(Padded));
static_assert(!emt::fixed_stream_size<Fields>().has_value());

// All children of a struct (and their descendants) are part of the record, in pre-order.
static_assert(std::same_as<
              emt::internal::AllTypes<Fields>,
              emt::Ts<
                  Fields,
                  emt::Child<std::int32_t, "number">,
                  emt::Child<double, "real">,
                  emt::Child<std::string, "name">,
                  emt::Child<Padded, "padded">,
                  emt::Child<std::uint8_t, "a", a_padding>,
                  emt::Child<std::int32_t, "b">,
                  emt::Child<std::uint8_t, "c", c_padding>>>);

CXX_TEST(record_layout_contains_children_and_their_padding) {
    // [runtime major, runtime minor, #args, format, then per type: (name), type id, size, flag,
    // padding, #children, and finally formatter, file, line]
    constexpr auto layout = emt::internal::get_layout<Padded>(0, 0, 0, 0, 0);
    static_assert(layout.size() == 7 + 5 + (3 * 6));

    // the struct itself: one element of three children, no padding of its own
    CXX_CHECK_EQ(layout[5], emt_size_t{1});
    CXX_CHECK_EQ(layout[6], emt_size_t{EMT_FLAG_STATIC});
    CXX_CHECK_EQ(layout[7], emt_size_t{0});
    CXX_CHECK_EQ(layout[8], emt_size_t{3});

    // the paddings of its children
    CXX_CHECK_EQ(layout[9 + 4], a_padding);
    CXX_CHECK_EQ(layout[15 + 4], emt_size_t{0});
    CXX_CHECK_EQ(layout[21 + 4], c_padding);
}

CXX_TEST(simple_struct_is_emitted_with_its_padding) {
    Bytes capture;
    const Padded value{.a = 1, .b = -2, .c = 3};
    MTRACE("{}", value, emt::opt_sink(capture));

    CXX_CHECK_EQ(payload(capture), bytes_of(value));
}

CXX_TEST(arrays_of_simple_structs_are_emitted_as_a_whole) {
    Bytes capture;
    const std::array<Padded, 2> values{
        Padded{.a = 1, .b = 2, .c = 3}, Padded{.a = 4, .b = 5, .c = 6}
    };
    MTRACE("{}", values, emt::opt_sink(capture));

    CXX_CHECK_EQ(payload(capture), concat(bytes_of(values[0]), bytes_of(values[1])));
}

CXX_TEST(fields_are_emitted_one_by_one_without_padding) {
    Bytes capture;
    const Fields value{
        .number = 7, .real = 0.5, .name = "abc", .padded = Padded{.a = 1, .b = 2, .c = 3}
    };
    MTRACE("{}", value, emt::opt_sink(capture));

    CXX_CHECK_EQ(
        payload(capture),
        concat(bytes_of(value.number), bytes_of(value.real), text("abc"), bytes_of(value.padded))
    );
}

CXX_TEST(vectors_of_structs_emit_each_element) {
    Bytes capture;
    const std::vector<Fields> values{
        Fields{.number = 1, .real = 1.0, .name = "x", .padded = {}},
        Fields{.number = 2, .real = 2.0, .name = "", .padded = {}},
    };
    MTRACE("{}", values, emt::opt_sink(capture));

    CXX_CHECK_EQ(
        payload(capture),
        concat(
            cxxtest::varint(2), bytes_of(values[0].number), bytes_of(values[0].real), text("x"),
            bytes_of(values[0].padded), bytes_of(values[1].number), bytes_of(values[1].real),
            text(""), bytes_of(values[1].padded)
        )
    );
}

CXX_TEST(emit_fields_stops_at_the_first_error) {
    RecordingSink sink;
    // The pointer and the first field fit, the second field doesn't.
    sink.capacity = ptr_width + sizeof(std::int32_t) + 1;

    const Fields value{.number = 7, .real = 0.5, .name = "abc", .padded = {}};
    const std::optional<CapacityError> error = MTRACE("{}", value, emt::opt_sink(sink));

    CXX_CHECK(error.has_value());
    CXX_CHECK_EQ(error.value_or(CapacityError{0}).needed, sizeof(double));
    CXX_CHECK_EQ(sink.bytes.size(), ptr_width + sizeof(std::int32_t));
}
