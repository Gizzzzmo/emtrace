#include "emtrace/emtrace.h"
#include <thread>

auto main() -> int {
    auto work = []() {
        for (int i = 0; i < 1000000; i++) {
            EMTRACE("Just a string\n");
            EMTRACE_F(
                "Here are a few numbers: {} {} {} {}\n", VAL(int, 1), VAL(int, 2), VAL(int, 3),
                VAL(int, 4)
            );
        }
    };
    EMTRACE_INIT();
    auto t1 = std::thread(work);
    auto t2 = std::thread(work);
    t1.join();
    t2.join();
    return 0;
}
