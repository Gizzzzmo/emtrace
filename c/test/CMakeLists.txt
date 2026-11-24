add_executable(
    c_tests
    src/test_integers.c
    src/test_doubles.c
    src/test_strings.c
    src/test_mixed.c
    src/test_cobs.c
    src/test_collisions.c
    src/test_all.c
    src/include/test_framework.h
    src/include/test_suites.h
    src/include/test_utils.h
)
target_link_libraries(c_tests PRIVATE emtrace::emtrace)
target_include_directories(c_tests PRIVATE src/include)

add_test(NAME c_tests COMMAND c_tests)

if(EMTRACE_ENABLE_CXX)
    add_executable(
        cxx_tests
        src/cpp/test_main.cpp
        src/cpp/test_trace_raw.cpp
        src/cpp/test_trace_containers.cpp
        src/cpp/test_cobs.cpp
        src/cpp/test_encoding.cpp
        src/cpp/test_options_errors.cpp
        src/cpp/include/cxx_test.hpp
        src/cpp/include/test_bytes.hpp
        src/cpp/include/test_sinks.hpp
    )
    target_link_libraries(cxx_tests PRIVATE emtrace::emtrace)
    target_include_directories(cxx_tests PRIVATE src/cpp/include)

    add_test(NAME cxx_tests COMMAND cxx_tests)
endif()
