#include <emtrace/emtrace.hpp>
#include <iostream>
#include <vector>

using S = emt::ChangePtr<3, 9, emt::Cobs<std::vector<unsigned char>>>;
MTRACE_MAGIC(S{});

auto main() -> int {
    emt::init();

    std::cout << emt::GlobalSink<S>::inner_sink.inner.inner.size() << std::endl;
    MTRACE("blub {}", 10, emt::opt_align<512>);
    std::cout << emt::GlobalSink<S>::inner_sink.inner.inner.size() << std::endl;
    MTRACE("blub", emt::opt_align<512>, emt::opt_sink(std::cout));
    std::cout << emt::GlobalSink<S>::inner_sink.inner.inner.size() << std::endl;

    return 0;
}
