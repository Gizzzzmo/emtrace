add_executable(test_basic test_basic.c)
target_link_libraries(test_basic PRIVATE emtrace::emtrace)
target_include_directories(test_basic PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(test_integers test_integers.c)
target_link_libraries(test_integers PRIVATE emtrace::emtrace)
target_include_directories(test_integers PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(test_strings test_strings.c)
target_link_libraries(test_strings PRIVATE emtrace::emtrace)
target_include_directories(test_strings PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(test_doubles test_doubles.c)
target_link_libraries(test_doubles PRIVATE emtrace::emtrace)
target_include_directories(test_doubles PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(test_mixed test_mixed.c)
target_link_libraries(test_mixed PRIVATE emtrace::emtrace)
target_include_directories(test_mixed PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(test_edge_cases test_edge_cases.c)
target_link_libraries(test_edge_cases PRIVATE emtrace::emtrace)
target_include_directories(test_edge_cases PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(test_large_numbers test_large_numbers.c)
target_link_libraries(test_large_numbers PRIVATE emtrace::emtrace)
target_include_directories(test_large_numbers PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
