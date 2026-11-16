#ifndef CXX_TEST_HPP
#define CXX_TEST_HPP

// A minimal test framework for the C++ implementation. Tests register themselves at startup, and
// are run in sequence by `main`. A failing check is reported, but doesn't stop the test, while a
// failing requirement does.

#include <iostream>
#include <ranges>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace cxxtest {

using Bytes = std::vector<unsigned char>;

struct TestCase {
    const char* name;
    void (*run)();
};

inline auto registry() -> std::vector<TestCase>& {
    static std::vector<TestCase> tests;
    return tests;
}

inline auto failure_count() -> int& {
    static int count = 0;
    return count;
}

struct Registrar {
    Registrar(const char* name, void (*run)()) {
        registry().push_back({
            .name = name,
            .run = run,
        });
    }
};

/// Thrown by failed requirements to stop the current test.
struct RequirementFailed {};

template <typename T>
auto describe(const T& value) -> std::string {
    std::ostringstream out;
    if constexpr (std::is_integral_v<T>) {
        out << +value;
    } else if constexpr (requires {
                             value.has_value();
                             *value;
                         }) {
        out << (value.has_value() ? "some(" + describe(*value) + ")" : "none");
    } else if constexpr (std::ranges::range<T> && !std::is_convertible_v<T, std::string_view>) {
        out << "[";
        bool first = true;
        for (const auto& element : value) {
            out << (first ? "" : ", ") << describe(element);
            first = false;
        }
        out << "]";
    } else if constexpr (requires { out << value; }) {
        out << value;
    } else {
        out << "<value>";
    }
    return out.str();
}

inline void report_failure(
    const char* expression, const char* file, int line, const std::string& details
) {
    failure_count()++;
    std::cerr << file << ":" << line << ": check failed: " << expression;
    if (!details.empty()) {
        std::cerr << "\n    " << details;
    }
    std::cerr << "\n";
}

/// Integers that std::cmp_equal accepts: all but bool and the character types
template <typename T>
inline constexpr bool is_comparable_integer =
    std::is_integral_v<T> && !std::is_same_v<std::remove_cv_t<T>, bool> &&
    !std::is_same_v<std::remove_cv_t<T>, char> && !std::is_same_v<std::remove_cv_t<T>, wchar_t> &&
    !std::is_same_v<std::remove_cv_t<T>, char8_t> &&
    !std::is_same_v<std::remove_cv_t<T>, char16_t> &&
    !std::is_same_v<std::remove_cv_t<T>, char32_t>;

/// The value of an integer that std::cmp_equal doesn't accept, as an unsigned number
template <typename T>
constexpr auto widen(T value) -> unsigned long long {
    if constexpr (std::is_same_v<std::remove_cv_t<T>, bool>) {
        return value ? 1U : 0U;
    } else {
        return static_cast<unsigned long long>(static_cast<std::make_unsigned_t<T>>(value));
    }
}

template <typename A, typename B>
auto equal(const A& actual, const B& expected) -> bool {
    if constexpr (is_comparable_integer<A> && is_comparable_integer<B>) {
        return std::cmp_equal(actual, expected);
    } else if constexpr (std::is_integral_v<A> && std::is_integral_v<B>) {
        return widen(actual) == widen(expected);
    } else {
        return actual == expected;
    }
}

template <typename A, typename B>
auto check_equal(
    const A& actual, const B& expected, const char* expression, const char* file, int line
) -> bool {
    if (equal(actual, expected)) {
        return true;
    }
    report_failure(
        expression, file, line,
        "actual:   " + describe(actual) + "\n    expected: " + describe(expected)
    );
    return false;
}

inline auto check(bool condition, const char* expression, const char* file, int line) -> bool {
    if (!condition) {
        report_failure(expression, file, line, "");
    }
    return condition;
}

} // namespace cxxtest

// NOLINTBEGIN(cppcoreguidelines-macro-usage)
#define CXX_TEST(name)                                                                             \
    static void name();                                                                            \
    static const ::cxxtest::Registrar name##_registrar{#name, name}; /* NOLINT */                  \
    static void name()

#define CXX_CHECK(condition) (void) ::cxxtest::check((condition), #condition, __FILE__, __LINE__)

#define CXX_CHECK_EQ(actual, expected)                                                             \
    (void) ::cxxtest::check_equal(                                                                 \
        (actual), (expected), #actual " == " #expected, __FILE__, __LINE__                         \
    )

#define CXX_REQUIRE_EQ(actual, expected)                                                           \
    do {                                                                                           \
        if (!::cxxtest::check_equal(                                                               \
                (actual), (expected), #actual " == " #expected, __FILE__, __LINE__                 \
            )) {                                                                                   \
            throw ::cxxtest::RequirementFailed{};                                                  \
        }                                                                                          \
    } while (false)
// NOLINTEND(cppcoreguidelines-macro-usage)

#endif // CXX_TEST_HPP
