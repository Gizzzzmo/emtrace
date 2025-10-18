add_library(
    c_tests
    STATIC
    src/test_integers.c
    src/test_doubles.c
    src/test_strings.c
    src/test_mixed.c
)
target_include_directories(c_tests PUBLIC include)
target_link_libraries(c_tests PRIVATE emtrace::emtrace)

add_executable(c_test_all src/test_all.c)
target_link_libraries(c_test_all PRIVATE c_tests)

add_test(NAME c_all_tests COMMAND c_test_all)
