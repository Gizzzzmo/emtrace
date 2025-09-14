cmake_minimum_required(VERSION 3.10)

set(EMTRACE_ENABLE_DEMOS ON CACHE BOOL "Build demos")
set(EMTRACE_ENABLE_TESTS ON CACHE BOOL "Build tests")
set(EMTRACE_ENABLE_CXX ON CACHE BOOL "Enable dedicated C++ integration")

set(LANGUAGES C)
if(EMTRACE_ENABLE_CXX)
    list(APPEND LANGUAGES CXX)
endif()
if(NOT EMTRACE_ENABLE_DEMOS AND NOT EMTRACE_ENABLE_TESTS)
    set(LANGUAGES "")
endif()

project(emtrace LANGUAGES ${LANGUAGES})

if(EMTRACE_ENABLE_CXX)
    message(WARNING "Configured with EMTRACE_ENABLE_CXX=ON, but dedicated C++ integration doesn't exist yet.")
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

add_library(emtrace INTERFACE ./include/c/include/emtrace/emtrace.h)
target_include_directories(emtrace INTERFACE ./include/c/include)
add_library(emtrace::emtrace ALIAS emtrace)

if(EMTRACE_ENABLE_DEMOS)
    add_subdirectory(demos)
endif()

if(EMTRACE_ENABLE_TESTS)
    add_subdirectory(test)
endif()

if(
    PROJECT_IS_TOP_LEVEL
    AND CMAKE_EXPORT_COMPILE_COMMANDS
    AND NOT ${CMAKE_BINARY_DIR}/compile_commands.json
        STREQUAL
        ${CMAKE_SOURCE_DIR}/build/compile_commands.json
)
    execute_process(
        COMMAND
            ${CMAKE_COMMAND} -E create_symlink
            ${CMAKE_BINARY_DIR}/compile_commands.json
            ${CMAKE_SOURCE_DIR}/build/compile_commands.json
    )
endif()
