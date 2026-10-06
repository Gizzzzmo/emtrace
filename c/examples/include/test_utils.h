#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#if defined(__GNUC__) || defined(__clang__)
#define TEST_EXPECTED_SECTION __attribute__((section(".emt_exp"), used))
#else
#define TEST_EXPECTED_SECTION
#endif

#define EXPECT_OUTPUT(exp)                                                                         \
    extern char expected[];                                                                        \
    TEST_EXPECTED_SECTION char expected[] = exp

#endif // TEST_UTILS_H
