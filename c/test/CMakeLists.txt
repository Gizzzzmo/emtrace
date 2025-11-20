add_executable(
    c_tests
    src/test_integers.c
    src/test_doubles.c
    src/test_strings.c
    src/test_mixed.c
    src/test_all.c
    src/include/test_framework.h
    src/include/test_suites.h
    src/include/test_utils.h
)
target_link_libraries(c_tests PRIVATE emtrace::emtrace)
target_include_directories(c_tests PRIVATE src/include)

add_test(NAME c_tests COMMAND c_tests)
