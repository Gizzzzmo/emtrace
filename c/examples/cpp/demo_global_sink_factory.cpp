// Demonstrates a global sink that is created lazily by a factory.
//
// Some sinks can't be created when the program starts, e.g. because they need to open a file
// first. For those, MTRACE_MAGIC takes a function without parameters that returns the sink (or a
// reference to it) instead of a sink value. It is called once for every record.
//
// This demo writes the trace stream to a file. The factory is called for the first time when
// emt::init() writes the first record, which is when the file gets opened. Decode the stream with
// `emtrace run ./demo_global_sink_factory_cpp --input demo_trace.bin`.
#include <emtrace/emtrace.hpp>
#include <fstream>
#include <iostream>
#include <ostream>

namespace {

constexpr const char* trace_file = "demo_trace.bin";

// A reference to a std::ostream is a sink out of the box.
auto trace_stream() -> std::ostream& {
    static std::ofstream file{trace_file, std::ios::binary};
    return file;
}

} // namespace

// A lambda that doesn't capture anything converts to the function pointer MTRACE_MAGIC needs.
MTRACE_MAGIC([]() -> std::ostream& { return trace_stream(); });

auto main() -> int {
    emt::init();

    MTRACELN("Hello from the factory!");

    for (int i = 0; i < 3; i++) {
        MTRACELN("record number {}", i);
    }

    std::cout << "trace written to " << trace_file << "\n";

    return 0;
}
