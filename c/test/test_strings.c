#include "emtrace/emtrace.h"
#include "test_utils.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>

int main(void) {
    uint8_t raw_buffer[128];
    test_buffer_t buffer = {.data = raw_buffer, .capacity = sizeof(raw_buffer), .size = 0};

    const char* test_string = "Hello, C!";
    EMT_TEST_TRACE_S(buffer, "", test_string);

    // The buffer should contain the pointer to the format info and the string
    assert(buffer.size == sizeof(emt_ptr_t) + strlen(test_string) + 1);

    // Check the string value
    assert(strcmp((const char*) (buffer.data + sizeof(emt_ptr_t)), test_string) == 0);

    return 0;
}
