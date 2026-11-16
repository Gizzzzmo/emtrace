// Tests the COBS encoder, which frames every record so that it contains no zero bytes, and is
// terminated by a single zero byte.
#include "cxx_test.hpp"
#include "test_bytes.hpp"

#include <algorithm>
#include <cstddef>
#include <emtrace/emtrace.hpp>
#include <initializer_list>
#include <optional>

using cxxtest::Bytes;
using cxxtest::concat;

namespace {

/// Encodes the given chunks as a single packet
auto cobs_encode(std::initializer_list<Bytes> chunks) -> Bytes {
    Bytes encoded;
    emt::Sink<Bytes&> sink{encoded, nullptr, 0};
    emt::CobsEncoder encoder{};
    for (const Bytes& chunk : chunks) {
        (void) encoder.encode(sink, chunk.data(), chunk.size());
    }
    (void) encoder.finalize(sink);
    return encoded;
}

auto cobs_encode(const Bytes& data) -> Bytes { return cobs_encode({data}); }

/// Decodes a single packet, including its terminator.
auto cobs_decode(const Bytes& encoded) -> std::optional<Bytes> {
    if (encoded.empty() || encoded.back() != 0) {
        return std::nullopt;
    }

    const std::size_t size = encoded.size() - 1;
    Bytes decoded;
    std::size_t i = 0;
    while (i < size) {
        const std::size_t code = encoded[i++];
        if (code == 0 || i + code - 1 > size) {
            return std::nullopt;
        }
        for (std::size_t j = 1; j < code; j++) {
            if (encoded[i] == 0) {
                return std::nullopt;
            }
            decoded.push_back(encoded[i++]);
        }
        if (i < size && code != 0xFF) {
            decoded.push_back(0);
        }
    }
    return decoded;
}

/// A repeatable sequence of bytes, where `zero_period` is the typical distance between zeros.
auto pseudo_random_bytes(std::size_t size, unsigned zero_period) -> Bytes {
    Bytes data;
    unsigned state = 12345;
    for (std::size_t i = 0; i < size; i++) {
        state = (state * 1103515245U) + 12345U;
        const unsigned random = state >> 16;
        if (zero_period != 0 && random % zero_period == 0) {
            data.push_back(0);
        } else {
            data.push_back(static_cast<unsigned char>(1 + (random % 255)));
        }
    }
    return data;
}

} // namespace

CXX_TEST(cobs_encodes_short_data) {
    CXX_CHECK_EQ(
        cobs_encode(Bytes{1, 2, 3, 0, 4, 5, 0, 0, 6}), (Bytes{4, 1, 2, 3, 3, 4, 5, 1, 2, 6, 0})
    );
}

CXX_TEST(cobs_encodes_nothing_as_an_empty_block) {
    CXX_CHECK_EQ(cobs_encode(Bytes{}), (Bytes{1, 0}));
}

CXX_TEST(cobs_encodes_only_zeros) {
    CXX_CHECK_EQ(cobs_encode(Bytes(4, 0)), (Bytes{1, 1, 1, 1, 1, 0}));
}

CXX_TEST(cobs_encodes_exactly_one_full_block) {
    const Bytes encoded = cobs_encode(Bytes(254, 1));

    CXX_REQUIRE_EQ(encoded.size(), 256U);
    CXX_CHECK_EQ(encoded.front(), 0xFF);
    CXX_CHECK_EQ(Bytes(encoded.begin() + 1, encoded.begin() + 255), Bytes(254, 1));
    CXX_CHECK_EQ(encoded.back(), 0);
}

CXX_TEST(cobs_splits_data_longer_than_a_block) {
    const Bytes encoded = cobs_encode(Bytes(256, 1));

    CXX_REQUIRE_EQ(encoded.size(), 259U);
    CXX_CHECK_EQ(encoded[0], 0xFF);
    CXX_CHECK_EQ(Bytes(encoded.begin() + 1, encoded.begin() + 255), Bytes(254, 1));
    CXX_CHECK_EQ(Bytes(encoded.begin() + 255, encoded.end()), (Bytes{3, 1, 1, 0}));
}

CXX_TEST(cobs_encodes_zero_after_a_full_block) {
    Bytes data(254, 1);
    data.push_back(0);

    const Bytes encoded = cobs_encode(data);
    CXX_CHECK_EQ(cobs_decode(encoded), std::optional<Bytes>{data});
}

CXX_TEST(cobs_encoder_can_be_used_for_multiple_packets) {
    // finalize resets the encoder, so the second packet doesn't carry the state of the first
    Bytes encoded;
    emt::Sink<Bytes&> sink{encoded, nullptr, 0};
    emt::CobsEncoder encoder{};
    (void) encoder.encode(sink, "ab", 2);
    (void) encoder.finalize(sink);
    (void) encoder.encode(sink, "cd", 2);
    (void) encoder.finalize(sink);

    CXX_CHECK_EQ(encoded, (Bytes{3, 'a', 'b', 0, 3, 'c', 'd', 0}));
}

CXX_TEST(cobs_encoding_does_not_depend_on_how_data_is_chunked) {
    const Bytes data = pseudo_random_bytes(700, 9);
    const Bytes whole = cobs_encode(data);

    Bytes first_half(data.begin(), data.begin() + 301);
    Bytes second_half(data.begin() + 301, data.end());
    CXX_CHECK_EQ(cobs_encode({first_half, second_half}), whole);
}

CXX_TEST(cobs_roundtrips_data_of_many_lengths_and_zero_densities) {
    for (const unsigned zero_period : {0U, 1U, 2U, 7U, 300U}) {
        for (std::size_t size = 0; size < 800; size += (size < 40 ? 1 : 37)) {
            const Bytes data = pseudo_random_bytes(size, zero_period);
            const Bytes encoded = cobs_encode(data);

            CXX_CHECK_EQ(std::count(encoded.begin(), encoded.end(), 0), 1);
            CXX_CHECK_EQ(cobs_decode(encoded), std::optional<Bytes>{data});
        }
    }
}

CXX_TEST(cobs_roundtrips_blocks_with_lengths_around_the_maximum) {
    for (std::size_t run = 252; run <= 256; run++) {
        for (const std::size_t trailing : {std::size_t{0}, std::size_t{1}, std::size_t{2}}) {
            Bytes data(run, 7);
            data.insert(data.end(), trailing, 0);
            data.push_back(9);

            CXX_CHECK_EQ(cobs_decode(cobs_encode(data)), std::optional<Bytes>{data});
            data.pop_back();
            CXX_CHECK_EQ(cobs_decode(cobs_encode(data)), std::optional<Bytes>{data});
        }
    }
}
