#include "test_framework.h"
#include "test_suites.h"
#include "test_utils.h"
#include <emtrace/emtrace.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define MAX_ENCODED 2048

/// Encodes the `n_chunks` chunks as a single COBS frame. Returns the size of the frame, or 0 if it
/// did not fit into `out`.
static size_t cobs_encode_chunks(
    const uint8_t* const* chunks,
    const size_t* sizes,
    size_t n_chunks,
    uint8_t* out,
    size_t out_capacity
) {
    test_buffer_t buffer = {.data = out, .capacity = out_capacity, .size = 0};
    emt_cobs_state_t state;
    emt_cobs_init(&state);
    for (size_t i = 0; i < n_chunks; i++) {
        emt_cobs_encode(chunks[i], sizes[i], &state, to_buffer, &buffer);
    }
    emt_cobs_finalize(&state, to_buffer, &buffer);
    return buffer.size;
}

static size_t cobs_encode(const uint8_t* data, size_t size, uint8_t* out, size_t out_capacity) {
    return cobs_encode_chunks(&data, &size, 1, out, out_capacity);
}

/// Decodes a single frame including its terminator. Returns false if it is malformed.
static bool cobs_decode(
    const uint8_t* encoded, size_t encoded_size, uint8_t* out, size_t out_capacity, size_t* out_size
) {
    if (encoded_size == 0 || encoded[encoded_size - 1] != 0) {
        return false;
    }
    size_t size = encoded_size - 1;
    size_t i = 0;
    size_t n = 0;
    while (i < size) {
        size_t code = encoded[i++];
        if (code == 0 || i + code - 1 > size) {
            return false;
        }
        for (size_t j = 1; j < code; j++) {
            if (encoded[i] == 0 || n == out_capacity) {
                return false;
            }
            out[n++] = encoded[i++];
        }
        if (i < size && code != 0xFF) {
            if (n == out_capacity) {
                return false;
            }
            out[n++] = 0;
        }
    }
    *out_size = n;
    return true;
}

static bool roundtrips(const uint8_t* data, size_t size) {
    uint8_t encoded[MAX_ENCODED];
    uint8_t decoded[MAX_ENCODED];
    size_t decoded_size = 0;

    size_t encoded_size = cobs_encode(data, size, encoded, sizeof(encoded));
    if (encoded_size == 0) {
        return false;
    }
    // The only zero is the terminator
    for (size_t i = 0; i + 1 < encoded_size; i++) {
        if (encoded[i] == 0) {
            return false;
        }
    }
    if (!cobs_decode(encoded, encoded_size, decoded, sizeof(decoded), &decoded_size)) {
        return false;
    }
    if (decoded_size != size) {
        return false;
    }
    if (size == 0) {
        return true;
    }
    return memcmp(decoded, data, size) == 0;
}

/// Repeatable pseudo random bytes, where about every `zero_period`-th one is zero
static void pseudo_random_bytes(uint8_t* out, size_t size, unsigned zero_period) {
    unsigned state = 12345;
    for (size_t i = 0; i < size; i++) {
        state = (state * 1103515245U) + 12345U;
        unsigned random = state >> 16;
        out[i] =
            (zero_period != 0 && random % zero_period == 0) ? 0 : (uint8_t) (1 + (random % 255));
    }
}

static bool test_cobs_short(test_context_t* ctx) {
    const uint8_t data[] = {1, 2, 3, 0, 4, 5, 0, 0, 6};
    const uint8_t expected[] = {4, 1, 2, 3, 3, 4, 5, 1, 2, 6, 0};
    uint8_t encoded[MAX_ENCODED];

    size_t size = cobs_encode(data, sizeof(data), encoded, sizeof(encoded));
    TEST_ASSERT_EQ(ctx, size, sizeof(expected), "encoded size should match");
    TEST_ASSERT(ctx, memcmp(encoded, expected, size) == 0, "encoded bytes should match");
    return true;
}

static bool test_cobs_empty(test_context_t* ctx) {
    const uint8_t expected[] = {1, 0};
    uint8_t encoded[MAX_ENCODED];

    size_t size = cobs_encode(NULL, 0, encoded, sizeof(encoded));
    TEST_ASSERT_EQ(ctx, size, sizeof(expected), "empty data is a single empty block");
    TEST_ASSERT(ctx, memcmp(encoded, expected, size) == 0, "encoded bytes should match");
    return true;
}

static bool test_cobs_only_zeros(test_context_t* ctx) {
    const uint8_t data[4] = {0, 0, 0, 0};
    const uint8_t expected[] = {1, 1, 1, 1, 1, 0};
    uint8_t encoded[MAX_ENCODED];

    size_t size = cobs_encode(data, sizeof(data), encoded, sizeof(encoded));
    TEST_ASSERT_EQ(ctx, size, sizeof(expected), "encoded size should match");
    TEST_ASSERT(ctx, memcmp(encoded, expected, size) == 0, "encoded bytes should match");
    return true;
}

static bool test_cobs_one_full_block(test_context_t* ctx) {
    uint8_t data[254];
    uint8_t encoded[MAX_ENCODED];
    memset(data, 1, sizeof(data));

    size_t size = cobs_encode(data, sizeof(data), encoded, sizeof(encoded));
    TEST_ASSERT_EQ(ctx, size, 256, "a full block takes a code byte and the terminator");
    TEST_ASSERT_EQ(ctx, encoded[0], 0xFF, "a full block has code 0xff");
    TEST_ASSERT(ctx, memcmp(encoded + 1, data, 254) == 0, "block content should match");
    TEST_ASSERT_EQ(ctx, encoded[255], 0, "frame should be terminated");
    return true;
}

static bool test_cobs_longer_than_a_block(test_context_t* ctx) {
    uint8_t data[256];
    uint8_t encoded[MAX_ENCODED];
    const uint8_t tail[] = {3, 1, 1, 0};
    memset(data, 1, sizeof(data));

    size_t size = cobs_encode(data, sizeof(data), encoded, sizeof(encoded));
    TEST_ASSERT_EQ(ctx, size, 259, "encoded size should match");
    TEST_ASSERT_EQ(ctx, encoded[0], 0xFF, "first block is full");
    TEST_ASSERT(ctx, memcmp(encoded + 1, data, 254) == 0, "first block content should match");
    TEST_ASSERT(ctx, memcmp(encoded + 255, tail, sizeof(tail)) == 0, "second block should match");
    return true;
}

static bool test_cobs_zero_after_full_block(test_context_t* ctx) {
    uint8_t data[255];
    memset(data, 1, 254);
    data[254] = 0;

    TEST_ASSERT(ctx, roundtrips(data, sizeof(data)), "the zero after a full block must survive");
    return true;
}

static bool test_cobs_reuse_state(test_context_t* ctx) {
    const uint8_t expected[] = {3, 'a', 'b', 0, 3, 'c', 'd', 0};
    uint8_t out[MAX_ENCODED];
    test_buffer_t buffer = {.data = out, .capacity = sizeof(out), .size = 0};
    emt_cobs_state_t state;

    // The state is reinitialized to start a new frame, so the second frame is independent of the
    // first
    emt_cobs_init(&state);
    emt_cobs_encode("ab", 2, &state, to_buffer, &buffer);
    emt_cobs_finalize(&state, to_buffer, &buffer);
    emt_cobs_init(&state);
    emt_cobs_encode("cd", 2, &state, to_buffer, &buffer);
    emt_cobs_finalize(&state, to_buffer, &buffer);

    TEST_ASSERT_EQ(ctx, buffer.size, sizeof(expected), "encoded size should match");
    TEST_ASSERT(ctx, memcmp(out, expected, buffer.size) == 0, "encoded bytes should match");
    return true;
}

static bool test_cobs_chunking_is_irrelevant(test_context_t* ctx) {
    uint8_t data[700];
    uint8_t whole[MAX_ENCODED];
    uint8_t chunked[MAX_ENCODED];
    pseudo_random_bytes(data, sizeof(data), 9);

    const uint8_t* chunks[3] = {data, data + 1, data + 301};
    const size_t sizes[3] = {1, 300, sizeof(data) - 301};

    size_t whole_size = cobs_encode(data, sizeof(data), whole, sizeof(whole));
    size_t chunked_size = cobs_encode_chunks(chunks, sizes, 3, chunked, sizeof(chunked));
    TEST_ASSERT_EQ(ctx, chunked_size, whole_size, "sizes should match");
    TEST_ASSERT(ctx, memcmp(whole, chunked, whole_size) == 0, "bytes should match");
    return true;
}

static bool test_cobs_roundtrip_lengths(test_context_t* ctx) {
    const unsigned zero_periods[] = {0, 1, 2, 7, 300};
    uint8_t data[800];

    for (size_t p = 0; p < sizeof(zero_periods) / sizeof(zero_periods[0]); p++) {
        for (size_t size = 0; size < sizeof(data); size += (size < 40 ? 1 : 37)) {
            pseudo_random_bytes(data, size, zero_periods[p]);
            TEST_ASSERT(ctx, roundtrips(data, size), "data should survive a roundtrip");
        }
    }
    return true;
}

static bool test_cobs_roundtrip_around_max_block(test_context_t* ctx) {
    uint8_t data[270];

    for (size_t run = 252; run <= 256; run++) {
        for (size_t trailing_zeros = 0; trailing_zeros <= 2; trailing_zeros++) {
            memset(data, 7, run);
            memset(data + run, 0, trailing_zeros);
            // with and without something non-zero at the end
            data[run + trailing_zeros] = 9;
            TEST_ASSERT(
                ctx, roundtrips(data, run + trailing_zeros + 1), "data should survive a roundtrip"
            );
            TEST_ASSERT(
                ctx, roundtrips(data, run + trailing_zeros), "data should survive a roundtrip"
            );
        }
    }
    return true;
}

test_fn_t* emt_get_cobs_tests(size_t* count) {
    static test_fn_t tests[] = {
        test_cobs_short,
        test_cobs_empty,
        test_cobs_only_zeros,
        test_cobs_one_full_block,
        test_cobs_longer_than_a_block,
        test_cobs_zero_after_full_block,
        test_cobs_reuse_state,
        test_cobs_chunking_is_irrelevant,
        test_cobs_roundtrip_lengths,
        test_cobs_roundtrip_around_max_block,
    };
    *count = sizeof(tests) / sizeof(tests[0]);
    return tests;
}
