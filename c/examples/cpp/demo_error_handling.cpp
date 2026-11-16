// Demonstrates how to implement your own sink, and how it reports errors.
//
// A sink is a specialization of emt::Sink for a reference to the type that holds the data. For
// every record an object of this specialization is created, and the library calls
//   begin()                  once, before anything else,
//   out_ptr(sink, record)    to write the pointer to the format information of the record,
//   out(data, size)          for every piece of the arguments, and finally
//   finish()                 once, after everything else.
// Any of them can fail by returning an error instead of std::nullopt. By default, the error is
// returned from the MTRACE call (emt::opt_no_throw). With emt::opt_throw an exception is thrown.
//
// The sink here can only hold 32 bytes. A record that doesn't fit leaves a partial record behind,
// which is dropped again. The records that did fit are forwarded to stdout in the end, so that
// they can be decoded with `emtrace run ./demo_error_handling_cpp`. The messages about the errors
// are printed to stderr.
#include <array>
#include <cstddef>
#include <emtrace/emtrace.hpp>
#include <iostream>
#include <optional>
#include <string>
#include <type_traits>

struct StaticBuffer {
    static constexpr std::size_t capacity = 32;

    std::array<unsigned char, capacity> data{};
    std::size_t size = 0;
    // size after the last record that was written completely
    std::size_t committed = 0;
};

struct OverflowError {};

template <>
struct emt::Sink<StaticBuffer&> {
    using Error = OverflowError;

    // What the parser needs to know about the stream, if this was used as a global sink
    static constexpr unsigned char encoding_id = EMT_ENCODING_NONE;
    static constexpr emt_size_t ptr_width = emt::default_ptr_width;
    static constexpr unsigned char alignment_power = EMT_DEFAULT_ALIGNMENT_POWER;

    Sink(StaticBuffer& buffer, const void* /*record*/, emt_size_t /*total_size*/)
        : buffer(buffer) {}

    [[nodiscard]] static auto begin() -> std::optional<Error> { return std::nullopt; }

    // Writes the address of the record, shifted right by alignment_power, as ptr_width bytes
    template <typename SI>
    [[nodiscard]] static auto out_ptr(SI&& sink_impl, const void* record)
        -> std::optional<typename std::remove_reference_t<SI>::Error> {
        const emt_ptr_t shifted = reinterpret_cast<emt_ptr_t>(record) >> alignment_power;

        std::array<unsigned char, ptr_width> bytes{};
        for (emt_size_t i = 0; i < ptr_width; i++) {
            bytes[i] = static_cast<unsigned char>(shifted >> (i * 8));
        }

        return std::forward<SI>(sink_impl).out(bytes.data(), bytes.size());
    }

    [[nodiscard]] auto out(const void* data, emt_size_t size) -> std::optional<Error> {
        if (size > StaticBuffer::capacity - buffer.size) {
            return OverflowError{};
        }
        const auto* bytes = static_cast<const unsigned char*>(data);
        for (emt_size_t i = 0; i < size; i++) {
            buffer.data[buffer.size + i] = bytes[i];
        }
        buffer.size += size;
        return std::nullopt;
    }

    [[nodiscard]] static auto finish() -> std::optional<Error> { return std::nullopt; }

private:
    StaticBuffer& buffer;
};

static_assert(emt::IsSink<StaticBuffer&>);

MTRACE_MAGIC();

namespace {

// Keeps the record if it was written successfully, and drops what is left of it otherwise.
void commit_or_roll_back(const std::optional<OverflowError>& error, StaticBuffer& buffer) {
    if (error.has_value()) {
        buffer.size = buffer.committed;
        std::cerr << "overflow while tracing, dropped the partial record";
    } else {
        buffer.committed = buffer.size;
        std::cerr << "success";
    }
    std::cerr << " (buffer holds " << buffer.size << " of " << StaticBuffer::capacity
              << " bytes)\n";
}

} // namespace

auto main() -> int {
    emt::init();

    StaticBuffer buffer;

    // A pointer, and a string with its terminator: 3 + 6 bytes
    commit_or_roll_back(MTRACELN("Hello {}", std::string{"world"}, emt::opt_sink(buffer)), buffer);

    // A pointer and 4 integers: 3 + 16 bytes
    commit_or_roll_back(MTRACELN("{} {} {} {}", 1, 2, 3, 4, emt::opt_sink(buffer)), buffer);

    // Only 4 bytes are left, which is enough for the pointer, but not for the integer after it
    commit_or_roll_back(MTRACELN("{} {}", 5, 6, emt::opt_sink(buffer)), buffer);

    // With emt::opt_throw the error ends up in an exception, instead of the return value
    try {
        MTRACELN("Hello {}", std::string{"world"}, emt::opt_throw, emt::opt_sink(buffer));
    } catch (...) {
        std::cerr << "caught an exception\n";
        buffer.size = buffer.committed;
    }

    // Hand over the collected records
    std::cout.write(
        reinterpret_cast<const char*>(buffer.data.data()), static_cast<std::streamsize>(buffer.size)
    );

    return 0;
}
