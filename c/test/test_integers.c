#include "test_utils.h"
#include <assert.h>
#include <emtrace/emtrace.h>
#include <stdint.h>
#include <string.h>

int main(void) {
    uint8_t raw_buffer[128];
    test_buffer_t buffer = {.data = raw_buffer, .capacity = sizeof(raw_buffer), .size = 0};

    EMT_TEST_TRACE_F(buffer, EMT_PY_FORMAT, "{}", int, 42);

    // The buffer should contain the pointer to the format info and the integer
    assert(buffer.size == sizeof(void*) + sizeof(int));

    // Check the integer value
    int value;
    memcpy(&value, buffer.data + sizeof(void*), sizeof(int));
    assert(value == 42);

    return 0;
}
