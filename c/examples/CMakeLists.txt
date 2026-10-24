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

# Cross-language interop demos. In all of them the executable that owns the
# entry point contains the emtrace magic (a single `magic!()`/`EMTRACE_MAGIC` per
# binary), and every statically linked library - in either language - has its
# trace calls resolve the default symbols against the extern C `emtest_default_`
# definitions.
#
# Demo 1: this static library is the C guest; the Rust executables
# ../rust/examples/interop_rust_host.rs and ../rust/examples/interop_combined.rs
# host the magic and link it, with the link search path supplied by rust/build.rs.
add_library(interop_c_guest STATIC interop_c_guest.c)
target_link_libraries(interop_c_guest PRIVATE emtrace::emtrace)
# The Rust hosts link this archive with rust-lld and -nodefaultlibs, which
# provide no sanitizer runtime: keep the demo library sanitizer-free regardless
# of the configured profile (the profiles add -fsanitize=undefined to the Debug
# and RelWithDebInfo flags).
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(interop_c_guest PRIVATE -fno-sanitize=address,undefined)
endif()

# Demo 2: this executable supplies the magic and links with a Rust static library:
# ../rust/examples/interop_rust_guest.rs is linked in as the guest. the target is
# skipped until the archive exists (cd rust && cargo build --example interop_rust_guest).
set(EMTRACE_RUST_EXAMPLES_DIR
    "${CMAKE_SOURCE_DIR}/../rust/target/debug/examples"
    CACHE PATH
    "Directory containing cargo-built example artifacts"
)
set(INTEROP_RUST_LIB "${EMTRACE_RUST_EXAMPLES_DIR}/libinterop_rust_guest.a")
if(EMTRACE_ENABLE_TESTS AND EXISTS "${INTEROP_RUST_LIB}")
    add_executable(interop_c_host interop_c_host.c)
    target_link_libraries(
        interop_c_host
        PRIVATE
            emtrace::emtrace
            "${INTEROP_RUST_LIB}"
            # Companion libraries expected behind a Rust staticlib on GNU targets.
            pthread
            dl
            m
    )
    target_include_directories(interop_c_host PRIVATE include)
elseif(EMTRACE_ENABLE_TESTS)
    message(
        STATUS
        "Skipping interop_c_guest: ${INTEROP_RUST_LIB} not found (build: cargo build --example interop_rust_host)"
    )
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
        test_too_many_records
    )
    foreach(testname ${eetests})
        add_executable(${testname} ${testname}.c)
        target_link_libraries(${testname} PRIVATE emtrace::emtrace)
        target_include_directories(${testname} PRIVATE include)
    endforeach()
    foreach(testname ${eetests})
        add_executable(${testname}_cobs ${testname}.c)
        target_link_libraries(${testname}_cobs PRIVATE emtrace::emtrace)
        target_compile_definitions(${testname}_cobs PRIVATE TEST_COBS)
        target_include_directories(${testname}_cobs PRIVATE include)
    endforeach()
endif()
