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

#ifdef TEST_COBS
#define CPP_TEST_SINK                                                                              \
    emt::Cobs {}
#define CPP_TEST_SINK_ALIGNED(ptr_width, alignment_power)                                          \
    emt::ChangePtr<ptr_width, alignment_power, emt::Cobs<>> {}
#else
#define CPP_TEST_SINK
#define CPP_TEST_SINK_ALIGNED(ptr_width, alignment_power)                                          \
    emt::ChangePtr<ptr_width, alignment_power> {}
#endif

#endif // TEST_UTILS_H
