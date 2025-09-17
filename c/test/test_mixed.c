#include "test_utils.h"
#include <assert.h>
#include <emtrace/emtrace.h>
#include <stdint.h>
#include <string.h>

int main(void) {
    uint8_t raw_buffer[128];
    test_buffer_t buffer = {.data = raw_buffer, .capacity = sizeof(raw_buffer), .size = 0};

    EMT_TEST_TRACE_F(buffer, EMT_PY_FORMAT, "{} {} {}", int, 42, char, 'a', long, 123456L);

    size_t expected_size = sizeof(void*) + sizeof(int) + sizeof(char) + sizeof(long);
    assert(buffer.size == expected_size);

    int int_val;
    char char_val;
    long long_val;

    size_t offset = sizeof(void*);
    memcpy(&int_val, buffer.data + offset, sizeof(int));
    assert(int_val == 42);
    offset += sizeof(int);

    memcpy(&char_val, buffer.data + offset, sizeof(char));
    assert(char_val == 'a');
    offset += sizeof(char);

    memcpy(&long_val, buffer.data + offset, sizeof(long));
    assert(long_val == 123456L);

    return 0;
}
