#ifndef TEST_SUITES_H
#define TEST_SUITES_H

#include "test_framework.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

test_fn_t* emt_get_integer_tests(size_t* count);
test_fn_t* emt_get_double_tests(size_t* count);
test_fn_t* emt_get_string_tests(size_t* count);
test_fn_t* emt_get_mixed_tests(size_t* count);

#ifdef __cplusplus
}
#endif

#endif // TEST_SUITES_H
