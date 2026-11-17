#include "test_utils.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <emtrace/emtrace.hpp>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

// Structs are traced by implementing emt::Trace for them, with the type id "struct": a struct is a
// composite with a single element (`emt::StaticSize{1}`), whose children are the struct's fields.
// The parser decodes it into a dict of the fields, which can be accessed in format strings with
// `{0.field}` or `{0[field]}`.
//
// There are two ways of emitting a struct, which both handle the padding between its fields:
//
// - `Point` is trivially copyable, so its memory is emitted as a whole, padding bytes included.
//   Its Trace is *simple*: the padding after each field is part of its type description, so that
//   the parser can skip it. Containers of simple types (like `std::array<Point, 2>` below) are
//   emitted with a single copy as well.
// - `Measurement` contains a `std::string`, so its memory can't be emitted as-is: its fields are
//   emitted one by one instead, without the padding between them (which also saves bandwidth).

MTRACE_MAGIC(CPP_TEST_SINK);

struct Point {
    bool visible; // followed by padding, since `x` is aligned to 4 bytes
    std::int32_t x;
    std::int16_t y; // followed by padding up to the end of the struct
};

// The padding after a field is the gap up to the next field, or up to the end of the struct.
constexpr emt_size_t visible_padding =
    offsetof(Point, x) - offsetof(Point, visible) - sizeof(Point::visible);
constexpr emt_size_t x_padding = offsetof(Point, y) - offsetof(Point, x) - sizeof(Point::x);
constexpr emt_size_t y_padding = sizeof(Point) - offsetof(Point, y) - sizeof(Point::y);

static_assert(std::is_trivially_copyable_v<Point> && std::is_standard_layout_v<Point>);
static_assert(visible_padding > 0 && y_padding > 0, "Point should have padding");

template <>
struct emt::Trace<Point> : emt::internal::SimpleTrace<
                               "struct",
                               Point,
                               emt::Ts<
                                   emt::Child<bool, "visible", visible_padding>,
                                   emt::Child<std::int32_t, "x", x_padding>,
                                   emt::Child<std::int16_t, "y", y_padding>>> {};

// The type description of a simple type has to cover exactly the bytes of its memory.
static_assert(emt::fixed_stream_size<Point>() == sizeof(Point));

struct Measurement {
    std::uint16_t channel; // followed by padding, but only in memory
    double value;
    std::string unit;
    Point location;
};

template <>
struct emt::Trace<Measurement> {
    using Children = emt::Ts<
        emt::Child<std::uint16_t, "channel">,
        emt::Child<double, "value">,
        emt::Child<std::string, "unit">,
        emt::Child<Point, "location">>;
    static constexpr emt::StringLiteral type_id = "struct";
    static constexpr emt::SizeType size_type = emt::StaticSize{1};
    static constexpr bool simple = false;

    template <typename S>
    static auto serialize(const Measurement& measurement, S& sink)
        -> std::optional<typename S::Error> {
        return emt::emit_fields(
            sink, measurement.channel, measurement.value, measurement.unit, measurement.location
        );
    }

    static auto size(const Measurement& /*measurement*/) -> emt_size_t { return 1; }
};

static_assert(!emt::fixed_stream_size<Measurement>().has_value());

EXPECT_OUTPUT(
    "point: {'visible': True, 'x': 1, 'y': -2}\n"
    "fields: x=1, y=-2\n"
    "separators: visible=True, x=1, y=-2\n"
    "corners: visible=False, x=0, y=0; visible=True, x=640, y=480\n"
    "path: [{'visible': True, 'x': 1, 'y': 2}, {'visible': False, 'x': -3, 'y': 4}]\n"
    "measurement: {'channel': 3, 'value': 21.5, 'unit': 'degC', 'location': {'visible': True, "
    "'x': 10, 'y': 20}}\n"
    "21.5 degC at x=10\n"
    "{\n"
    "    'channel': 3,\n"
    "    'value': 21.5,\n"
    "    'unit': 'degC',\n"
    "    'location': {\n"
    "        'visible': True,\n"
    "        'x': 10,\n"
    "        'y': 20\n"
    "    }\n"
    "}\n"
    "measurements: 1.5 V, 2.25 A\n"
);

auto main() -> int {
    emt::init();

    const Point point{
        .visible = true,
        .x = 1,
        .y = -2,
    };
    MTRACELN("point: {}", point);
    MTRACELN("fields: x={0.x}, y={0[y]}", point);
    MTRACELN("separators: {:, *=*}", point);

    const std::array<Point, 2> corners{
        Point{.visible = false, .x = 0, .y = 0},
        Point{.visible = true, .x = 640, .y = 480},
    };
    MTRACELN("corners: {:; *, *=*}", corners);

    const std::vector<Point> path{
        Point{.visible = true, .x = 1, .y = 2},
        Point{.visible = false, .x = -3, .y = 4},
    };
    MTRACELN("path: {}", path);

    const Measurement measurement{
        .channel = 3,
        .value = 21.5,
        .unit = "degC",
        .location = Point{.visible = true, .x = 10, .y = 20},
    };
    MTRACELN("measurement: {}", measurement);
    MTRACELN("{0.value} {0.unit} at x={0.location.x}", measurement);
    MTRACELN("{:#}", measurement);

    const std::vector<Measurement> measurements{
        Measurement{
            .channel = 1,
            .value = 1.5,
            .unit = "V",
            .location = {},
        },
        Measurement{
            .channel = 2,
            .value = 2.25,
            .unit = "A",
            .location = {},
        },
    };
    MTRACELN("measurements: {0[0].value} {0[0].unit}, {0[1].value} {0[1].unit}", measurements);

    return 0;
}
