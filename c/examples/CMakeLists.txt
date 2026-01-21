add_executable(demo_readme demo_readme.c)
target_link_libraries(demo_readme PRIVATE emtrace::emtrace)

add_executable(demo demo.c)
target_link_libraries(demo PRIVATE emtrace::emtrace)

add_executable(demo_passthrough demo_passthrough.c)
target_link_libraries(demo_passthrough PRIVATE emtrace::emtrace)

if(UNIX)
    add_executable(demo_c_socket demo_socket.c)
    target_link_libraries(demo_c_socket PRIVATE emtrace::emtrace)
endif()

if(EMTRACE_ENABLE_CXX)
    add_executable(demo_cpp demo.cpp)
    target_link_libraries(demo_cpp PRIVATE emtrace::emtrace)

    add_executable(demo_threads demo_threads.cpp)
    target_link_libraries(demo_threads PRIVATE emtrace::emtrace)
endif()

# End-to-end tests are part of examples,
# because the ctest configuration doesn't actually test anything about them.
# They are executable that simply have embedded in them a string containing the expected result
# *after* their output has been processed by emtrace.py,
# which is checked by the top-level testrunner (../../tests/test_emtrace.py).
if(EMTRACE_ENABLE_TESTS)
    set(eetests
        test_basic
        test_integers
        test_strings
        test_doubles
        test_mixed
        test_edge_cases
        test_large_numbers
        test_arr
    )
    foreach(testname ${eetests})
        add_executable(${testname} ${testname}.c)
        target_link_libraries(${testname} PRIVATE emtrace::emtrace)
    endforeach()
    foreach(testname ${eetests})
        add_executable(${testname}_cobs ${testname}.c)
        target_link_libraries(${testname}_cobs PRIVATE emtrace::emtrace)
        target_compile_definitions(${testname}_cobs PRIVATE EMT_DEFAULT_ENCODE_COBS)
    endforeach()
endif()
