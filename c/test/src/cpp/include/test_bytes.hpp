#ifndef TEST_BYTES_HPP
#define TEST_BYTES_HPP

// Helpers for spelling out the bytes that are expected in a trace stream.

#include "cxx_test.hpp"

#include <cstddef>
#include <cstring>
#include <emtrace/emtrace.hpp>
#include <ranges>
#include <string_view>

namespace cxxtest {

/// Width of the pointer at the start of every record when using the default pointer serialization.
inline constexpr std::size_t ptr_width = emt::default_ptr_width;

/// The object representation of a value
template <typename T>
auto bytes_of(const T& value) -> Bytes {
    Bytes bytes(sizeof(T));
    std::memcpy(bytes.data(), &value, sizeof(T));
    return bytes;
}

/// The object representations of all elements of a range
template <std::ranges::range R>
auto bytes_of_all(const R& values) -> Bytes {
    Bytes bytes;
    for (const auto& value : values) {
        const Bytes element = bytes_of(value);
        bytes.insert(bytes.end(), element.begin(), element.end());
    }
    return bytes;
}

/// A string as it appears in the stream: its characters followed by a null byte
inline auto text(std::string_view str) -> Bytes {
    Bytes bytes(str.begin(), str.end());
    bytes.push_back(0);
    return bytes;
}

/// The element count of a list as it appears in the stream
inline auto varint(std::size_t value) -> Bytes {
    Bytes bytes;
    while (value >= 0x80) {
        bytes.push_back(static_cast<unsigned char>((value & 0x7F) | 0x80));
        value >>= 7;
    }
    bytes.push_back(static_cast<unsigned char>(value));
    return bytes;
}

inline void append(Bytes& /*bytes*/) {}

template <typename... Rest>
void append(Bytes& bytes, const Bytes& first, const Rest&... rest) {
    bytes.insert(bytes.end(), first.begin(), first.end());
    append(bytes, rest...);
}

template <typename... Parts>
auto concat(const Parts&... parts) -> Bytes {
    Bytes bytes;
    append(bytes, parts...);
    return bytes;
}

/// Everything that was written for a record except for its pointer
inline auto payload(const Bytes& record) -> Bytes {
    if (record.size() < ptr_width) {
        return {};
    }
    return {record.begin() + static_cast<std::ptrdiff_t>(ptr_width), record.end()};
}

/// The pointer at the start of a record
inline auto pointer_of(const Bytes& record) -> Bytes {
    return {
        record.begin(),
        record.begin() + static_cast<std::ptrdiff_t>(std::min(ptr_width, record.size()))
    };
}

} // namespace cxxtest

#endif // TEST_BYTES_HPP
