#ifndef EMTRACE_TEST_UTILS_H
#define EMTRACE_TEST_UTILS_H

#include "emtrace/emtrace.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// A simple buffer to capture trace output
typedef struct {
    uint8_t* data;
    size_t capacity;
    size_t size;
} test_buffer_t;

// The custom output function that writes to our buffer
static inline void to_buffer(const void* data, size_t size, void* extra_arg) {
    test_buffer_t* buffer = (test_buffer_t*) extra_arg;
    if (buffer->size + size > buffer->capacity) {
        // For simplicity, we'll just fail if the buffer is too small.
        return;
    }
    memcpy(buffer->data + buffer->size, data, size);
    buffer->size += size;
}

#define EMT_TEST_LOCK_DUMMY(a, b, c)

#define EMT_TEST_TRACE_F(buffer, formatter, ...)                                                   \
    EMT_TRACE_F(                                                                                   \
        static const, formatter, to_buffer, EMT_TEST_LOCK_DUMMY, EMT_TEST_LOCK_DUMMY, (&buffer),   \
        "", __VA_ARGS__                                                                            \
    )

#endif // EMTRACE_TEST_UTILS_H
