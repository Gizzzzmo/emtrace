add_executable(test_basic test_basic.c)
target_link_libraries(test_basic PRIVATE emtrace::emtrace)
target_include_directories(test_basic PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
