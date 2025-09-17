#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#ifdef __GNUC__
#define TEST_EXPECTED_SECTION __attribute__((section(".emtrace.test.expected"), used))
#else
#define TEST_EXPECTED_SECTION
#endif

#define EXPECT_OUTPUT(exp)                                                                         \
    extern char expected[];                                                                        \
    TEST_EXPECTED_SECTION char expected[] = exp

#endif // TEST_UTILS_H
