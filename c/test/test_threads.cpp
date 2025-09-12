#include "emtrace/emtrace.h"
#include <thread>

int main() {
    auto work = []() {
        for (int i = 0; i < 1000000; i++) {
            EMTRACE("Just a string\n");
            EMTRACE_F("Here are a few numbers: {} {} {} {}\n", int, 1, int, 2, int, 3, int, 4);
        }
    };
    EMTRACE_INIT();
    auto t1 = std::thread(work);
    auto t2 = std::thread(work);
    t1.join();
    t2.join();
    return 0;
}
