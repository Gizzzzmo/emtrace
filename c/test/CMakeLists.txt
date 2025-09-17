add_executable(c_test_integers test_integers.c)
target_include_directories(c_test_integers PRIVATE include)
target_link_libraries(c_test_integers PRIVATE emtrace::emtrace)

add_test(NAME c_integer_test COMMAND c_test_integers)

add_executable(c_test_doubles test_doubles.c)
target_include_directories(c_test_doubles PRIVATE include)
target_link_libraries(c_test_doubles PRIVATE emtrace::emtrace)

add_test(NAME c_double_test COMMAND c_test_doubles)

add_executable(c_test_strings test_strings.c)
target_include_directories(c_test_strings PRIVATE include)
target_link_libraries(c_test_strings PRIVATE emtrace::emtrace)

add_test(NAME c_string_test COMMAND c_test_strings)

add_executable(c_test_mixed test_mixed.c)
target_include_directories(c_test_mixed PRIVATE include)
target_link_libraries(c_test_mixed PRIVATE emtrace::emtrace)

add_test(NAME c_mixed_test COMMAND c_test_mixed)
