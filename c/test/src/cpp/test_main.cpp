#include "cxx_test.hpp"

#include <cstddef>
#include <exception>
#include <iostream>

auto main() -> int {
    int failed_tests = 0;

    for (const cxxtest::TestCase& test : cxxtest::registry()) {
        const int failures_before = cxxtest::failure_count();
        std::cout << "[ RUN  ] " << test.name << "\n";
        try {
            test.run();
        } catch (const cxxtest::RequirementFailed&) {
            std::cerr << "stopped " << test.name << " after a failed requirement\n";
        } catch (const std::exception& exception) {
            cxxtest::failure_count()++;
            std::cerr << "unexpected exception: " << exception.what() << "\n";
        } catch (...) {
            cxxtest::failure_count()++;
            std::cerr << "unexpected exception\n";
        }

        if (cxxtest::failure_count() == failures_before) {
            std::cout << "[  OK  ] " << test.name << "\n";
        } else {
            std::cout << "[ FAIL ] " << test.name << "\n";
            failed_tests++;
        }
    }

    std::cout << "\n"
              << cxxtest::registry().size() - static_cast<std::size_t>(failed_tests) << "/"
              << cxxtest::registry().size() << " tests passed\n";
    return failed_tests == 0 ? 0 : 1;
}
