add_executable(demo demo.cpp)
target_link_libraries(demo PRIVATE emtrace::emtrace)

add_executable(demo_cpp demo.c)
target_link_libraries(demo_cpp PRIVATE emtrace::emtrace)

add_executable(demo_c_socket demo_socket.c)
target_link_libraries(demo_c_socket PRIVATE emtrace::emtrace)

add_executable(demo_threads demo_threads.cpp)
target_link_libraries(demo_threads PRIVATE emtrace::emtrace)
target_compile_options(demo_threads PRIVATE -fsanitize=thread)
target_link_options(demo_threads PRIVATE -fsanitize=thread)
