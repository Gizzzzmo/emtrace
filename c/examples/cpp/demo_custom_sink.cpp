// Demonstrates the options that can be given to a single MTRACE call.
//
// They are placed after the format arguments, in any order:
//   emt::opt_sink(sink)   send this record to the given sink instead of the global one
//   emt::opt_c_style_fmt  use printf style format specifiers (also: emt::opt_py_fmt, which is the
//                         default, and emt::opt_no_fmt, which prints the string as it is)
//   emt::opt_align<N>     align the record in the binary to N bytes (a power of two)
//
// Run it with `emtrace run ./demo_custom_sink_cpp`.
#include <emtrace/emtrace.hpp>
#include <iostream>
#include <string_view>
#include <vector>

MTRACE_MAGIC();

auto main() -> int {
    // The default sink writes to stdout
    emt::init();
    MTRACELN("This record goes to stdout.");

    // Any std::vector<unsigned char> can serve as a sink. Its content is the same stream of bytes
    // that would have gone to stdout, so we can forward it later.
    std::vector<unsigned char> buffer;
    MTRACELN("This record is collected in a buffer.", emt::opt_sink(buffer));

    // std::ostreams are sinks, too. Passing std::cout explicitly is the same as the default.
    // (String literals are character arrays; wrap them to trace them as strings.)
    const std::string_view stream_name = "std::cout";
    MTRACELN("This record is written to the stream {}.", stream_name, emt::opt_sink(std::cout));

    // printf style formatting
    const char* parity = "even";
    MTRACELN(
        "%d is %s, but %.2f is not.", 2, parity, 1.5, emt::opt_c_style_fmt, emt::opt_sink(buffer)
    );

    // The default formatter would complain about the unmatched braces, so print the string as is.
    MTRACELN("Curly braces {} are printed as they are.", emt::opt_no_fmt, emt::opt_sink(buffer));

    // The records are aligned to 256 bytes by default. This can be increased for a single call.
    MTRACELN("This record is aligned to 4096 bytes.", emt::opt_align<4096>, emt::opt_sink(buffer));

    // Send the collected records to stdout
    std::cout.write(
        reinterpret_cast<const char*>(buffer.data()), static_cast<std::streamsize>(buffer.size())
    );

    return 0;
}
