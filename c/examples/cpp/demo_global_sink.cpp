// Demonstrates a global sink that is given to MTRACE_MAGIC by value.
//
// Every MTRACE call that doesn't name a sink of its own (see demo_custom_sink.cpp) sends its
// record to the global sink. Here that is a std::vector, which collects the raw trace stream in
// memory. Instead of stdout the application decides when and where the collected data goes,
// e.g. over a network connection, or into a file.
//
// Run it with `emtrace run ./demo_global_sink_cpp`. The statistics printed to stderr are not
// part of the trace stream.
#include <emtrace/emtrace.hpp>
#include <iostream>
#include <vector>

using Buffer = std::vector<unsigned char>;

// Defines the MGIC record, which tells the parser how to read the stream, and stores the sink in
// a global variable. The sink has to be constant initializable, which is the case for an empty
// std::vector.
MTRACE_MAGIC(Buffer{});

namespace {

// The global sink can be reached via the sink's type.
auto collected() -> const Buffer& { return emt::GlobalSink<Buffer>::inner_sink; }

} // namespace

auto main() -> int {
    // Writes the pointer to the MGIC record to the stream, which the parser needs to make sense of
    // everything that follows. This is also collected by the global sink.
    emt::init();
    std::cerr << "after init: " << collected().size() << " bytes\n";

    MTRACELN("Hello from the global sink!");
    std::cerr << "after a string: " << collected().size() << " bytes\n";

    // A record consists of a pointer to the format information in the binary, followed by the
    // bytes of the arguments. There is no formatting at runtime.
    int a = 2;
    int b = 3;
    MTRACELN("{} + {} = {}", a, b, a + b);
    std::cerr << "after three integers: " << collected().size() << " bytes\n";

    // Nothing has reached stdout yet. Now send everything that was collected.
    std::cout.write(
        reinterpret_cast<const char*>(collected().data()),
        static_cast<std::streamsize>(collected().size())
    );

    return 0;
}
