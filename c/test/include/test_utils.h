#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include "emtrace/emtrace.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// A simple buffer to capture trace output
typedef struct {
    unsigned char* data;
    size_t capacity;
    size_t size;
} test_buffer_t;

// The custom output function that writes to our buffer
static inline void to_buffer(const void* data, size_t size, void* extra_arg) {
    test_buffer_t* buffer = (test_buffer_t*) extra_arg;
    if (buffer->size + size > buffer->capacity) {
        // For simplicity, we'll just fail if the buffer is too small.
        // A real test framework might reallocate.
        return;
    }
    memcpy(buffer->data + buffer->size, data, size);
    buffer->size += size;
}

// Dummy lock/unlock functions, as we are single-threaded in tests
static inline void emt_test_lock(const void* a, size_t b, void* c) {
    (void) a;
    (void) b;
    (void) c;
}
static inline void emt_test_unlock(const void* a, size_t b, void* c) {
    (void) a;
    (void) b;
    (void) c;
}

#define EMT_TEST_TRACE_F(buffer, formatter, ...)                                                   \
    EMT_TRACE_F(/* attributes */ static const, formatter, to_buffer, emt_test_lock,                \
                emt_test_unlock, (&buffer), "", __VA_ARGS__)

#define EMT_TEST_TRACE_S(buffer, postfix, str)                                                     \
    EMT_TRACE_S(/* attributes */ static const, to_buffer, emt_test_lock, emt_test_unlock,          \
                &(buffer), postfix, str)

#endif // TEST_UTILS_H
