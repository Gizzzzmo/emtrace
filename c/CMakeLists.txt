cmake_minimum_required(VERSION 3.10)

project(emtrace)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(emtrace INTERFACE)
target_include_directories(emtrace INTERFACE include)
add_library(emtrace::emtrace ALIAS emtrace)

# add_executable(emtrace ./emtrace.cpp)
# target_link_libraries(blub fmt)
add_executable(test_c test/test.c)
target_link_libraries(test_c PRIVATE emtrace::emtrace)
add_executable(test_c_socket test/test_socket.c)
target_link_libraries(test_c_socket PRIVATE emtrace::emtrace)
