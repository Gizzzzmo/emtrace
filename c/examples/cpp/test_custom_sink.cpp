#include "test_utils.h"
#include <emtrace/emtrace.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// Individual MTRACE calls can send their output somewhere else than the default sink with
// emt::opt_sink. Other options, like the formatter and the alignment of the record, can be given
// along with it. All of them are placed after the format arguments, and can be given in any order.
//
// The sinks used here are compatible with the default one (same pointer width, and alignment),
// so the records they receive can be decoded using the information in the MGIC record.

EXPECT_OUTPUT(
    "default sink: 1\n"
    "vector: 2\n"
    "vector with C style formatting: 3\n"
    "no formatting here: {}\n"
    "aligned vector: 4\n"
    "stringstream: 5\n"
    "stringstream: 6\n"
    "temporary sink and options in any order: 7\n"
    "back to the default sink: 8\n"
    "stdout: 9\n"
);

MTRACE_MAGIC();

auto main() -> int {
    emt::init();

    std::vector<unsigned char> vector_sink;
    std::ostringstream string_stream;
    std::ostream& stream_sink = string_stream;

    MTRACELN("default sink: {}", 1);

    MTRACELN("vector: {}", 2, emt::opt_sink(vector_sink));
    MTRACELN(
        "vector with C style formatting: %d", 3, emt::opt_c_style_fmt, emt::opt_sink(vector_sink)
    );
    MTRACELN("no formatting here: {}", emt::opt_no_fmt, emt::opt_sink(vector_sink));
    MTRACELN("aligned vector: {}", 4, emt::opt_align<4096>, emt::opt_sink(vector_sink));

    MTRACELN("stringstream: {}", 5, emt::opt_sink(stream_sink));
    MTRACELN("stringstream: {}", 6, emt::opt_sink(stream_sink));

    std::vector<unsigned char> temporary;
    MTRACELN(
        "temporary sink and options in any order: {}", emt::opt_sink(temporary),
        emt::opt_align<512>, 7
    );

    // Nothing has been sent to stdout so far apart from the very first record and the line
    // using the default sink. Now forward everything that has been captured.
    std::cout.write(
        reinterpret_cast<const char*>(vector_sink.data()),
        static_cast<std::streamsize>(vector_sink.size())
    );
    const std::string captured = string_stream.str();
    std::cout.write(captured.data(), static_cast<std::streamsize>(captured.size()));
    std::cout.write(
        reinterpret_cast<const char*>(temporary.data()),
        static_cast<std::streamsize>(temporary.size())
    );

    MTRACELN("back to the default sink: {}", 8);
    MTRACELN("stdout: {}", 9, emt::opt_sink(std::cout));

    return 0;
}
