#include "test_utils.h"
#include <assert.h>
#include <emtrace/emtrace.h>
#include <stdint.h>
#include <string.h>

int main(void) {
    uint8_t raw_buffer[128];
    test_buffer_t buffer = {.data = raw_buffer, .capacity = sizeof(raw_buffer), .size = 0};

    EMT_TEST_TRACE_F(buffer, EMT_PY_FORMAT, "{}", double, 3.14);

    // The buffer should contain the pointer to the format info and the double
    assert(buffer.size == sizeof(emt_ptr_t) + sizeof(double));

    // Check the double value
    double value;
    memcpy(&value, buffer.data + sizeof(emt_ptr_t), sizeof(double));
    assert(value == 3.14);

    return 0;
}
