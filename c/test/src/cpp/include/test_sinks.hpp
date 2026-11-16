#ifndef TEST_SINKS_HPP
#define TEST_SINKS_HPP

#include "cxx_test.hpp"

#include <array>
#include <cstddef>
#include <emtrace/emtrace.hpp>
#include <limits>
#include <optional>
#include <type_traits>
#include <utility>

namespace cxxtest {

struct CapacityError {
    std::size_t needed;
};

/// A sink that remembers what was sent to it, and fails once its capacity is exhausted.
struct RecordingSink {
    Bytes bytes;
    std::size_t capacity = std::numeric_limits<std::size_t>::max();
    const void* last_record = nullptr;
    int begins = 0;
    int finishes = 0;
};

} // namespace cxxtest

template <>
struct emt::Sink<cxxtest::RecordingSink&> {
    using Error = cxxtest::CapacityError;
    static constexpr unsigned char encoding_id = EMT_ENCODING_NONE;
    static constexpr emt_size_t ptr_width = emt::default_ptr_width;
    static constexpr unsigned char alignment_power = EMT_DEFAULT_ALIGNMENT_POWER;

    Sink(cxxtest::RecordingSink& recording, const void* /*record*/, emt_size_t /*total_size*/)
        : recording(recording) {}

    auto begin() -> std::optional<Error> {
        recording.begins++;
        return std::nullopt;
    }

    template <typename SI>
    static auto out_ptr(SI&& sink_impl, const void* record)
        -> std::optional<typename std::remove_reference_t<SI>::Error> {
        sink_impl.recording.last_record = record;
        const emt_ptr_t shifted = reinterpret_cast<emt_ptr_t>(record) >> alignment_power;

        std::array<unsigned char, ptr_width> bytes{};
        for (emt_size_t i = 0; i < ptr_width; i++) {
            bytes[i] = static_cast<unsigned char>(shifted >> (i * 8));
        }

        return std::forward<SI>(sink_impl).out(bytes.data(), bytes.size());
    }

    auto out(const void* data, emt_size_t size) -> std::optional<Error> {
        if (size > recording.capacity - recording.bytes.size()) {
            return Error{size};
        }

        const auto* begin = static_cast<const unsigned char*>(data);
        recording.bytes.insert(recording.bytes.end(), begin, begin + size);
        return std::nullopt;
    }

    auto finish() -> std::optional<Error> {
        recording.finishes++;
        return std::nullopt;
    }

    cxxtest::RecordingSink& recording;
};

static_assert(emt::IsSink<cxxtest::RecordingSink&>);

#endif // TEST_SINKS_HPP
