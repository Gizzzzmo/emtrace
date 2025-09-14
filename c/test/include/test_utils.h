#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#ifdef __GNUC__
#define EMT_TEST_EXPECTED_SECTION __attribute__((section(".emtrace.test.expected"), used))
#else
#define EMT_TEST_EXPECTED_SECTION
#endif

#endif // TEST_UTILS_H
