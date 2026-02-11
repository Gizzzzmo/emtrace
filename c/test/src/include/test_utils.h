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

typedef uintptr_t emt_ptr_t;

static inline void begin(const void* info, size_t total_size, void* extra_arg) {
    emt_ptr_t ptr = (emt_ptr_t) (uintptr_t) info;
    to_buffer(&ptr, sizeof(ptr), extra_arg);
    (void) info;
    (void) total_size;
    (void) extra_arg;
}

#define EMT_TEST_FINISH_DUMMY(a, b, c)

#define EMT_TEST_TRACE_F(buffer, formatter, ...)                                                   \
    EMT_TRACE_F(                                                                                   \
        const, formatter, uint16_t, to_buffer, begin, EMT_TEST_FINISH_DUMMY, (&buffer), "",        \
        __VA_ARGS__                                                                                \
    )

#endif // EMTRACE_TEST_UTILS_H
