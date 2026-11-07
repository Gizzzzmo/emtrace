// DO NOT MODIFY DIRECTLY! Header is auto-generated from c/header-generator/cxx/emtrace_header.hpp
// and c/header-generator/c/emtrace_common.h using this python script:
// c/header-generator/build_macro.py Script arguments were: ['--cpp']
#ifndef EMTRACE_EMTRACE_HPP
#define EMTRACE_EMTRACE_HPP
#ifdef EMTRACE_COMMON
#if (defined(EMTRACE_VERSION_MAJOR) && EMTRACE_VERSION_MAJOR != 0) ||                              \
    (defined(EMTRACE_VERSION_MINOR) && EMTRACE_VERSION_MINOR != 3) ||                              \
    (defined(EMTRACE_VERSION_PATCH) && EMTRACE_VERSION_PATCH != 0)
#error                                                                                             \
    "version mismatch between this header and another previously included one (probably C++ variant emtrace.hpp)"
#endif
#else // EMTRACE_COMMON
#define EMTRACE_COMMON
#define EMTRACE_VERSION_MAJOR 0
#define EMTRACE_VERSION_MINOR 3
#define EMTRACE_VERSION_PATCH 0

// NOLINTBEGIN(modernize-*,bugprone-casting-through-void)

#ifndef EMT_NO_LIBC
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define EMT_OFFSETOF offsetof
#define EMT_STRLEN strlen
typedef size_t emt_size_t;
typedef uintptr_t emt_ptr_t;
#else // not EMT_NO_LIBC

#ifndef EMT_SIZE_T
typedef unsigned long emt_size_t;
#else
typedef EMT_SIZE_T emt_size_t;
#endif // EMT_SIZE_T

#ifndef EMT_PTR_T
typedef emt_size_t emt_ptr_t;
#else
typedef EMT_PTR_T emt_ptr_t;
#endif

#ifndef EMT_STRLEN
static inline emt_size_t emt_strlen(const char* str) {
    emt_size_t len = 0;
    while (*str != '\0') {
        str++;
        len++;
    }
    return len;
}
#define EMT_STRLEN emt_strlen
#endif // EMT_STRLEN

#ifndef EMT_OFFSETOF
#error                                                                                             \
    "With EMT_NO_LIBC you have to supply your own definition of EMT_OFFSETOF before including emtrace.h"
#endif // EMT_OFFSETOF
#endif // EMT_NO_LIBC

// to switch stdout to binary mode in `emtrace_init()`
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif // _WIN32

#ifdef __cplusplus
extern "C" {
#endif

#if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L) ||                                  \
    (defined(__cplusplus) && __cplusplus >= 201103L)
#define EMT_STATIC_ASSERT(cond, msg) static_assert(cond, msg)
#define EMT_STATIC_ASSERT_INNER(cond, msg) static_assert(cond, msg)
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define EMT_STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)
#define EMT_STATIC_ASSERT_INNER(cond, msg) _Static_assert(cond, msg)
#else
#define EMT_LINE() __LINE__
#define EMT_GLUE(a, b) EMT_GLUE_HELPER(a, b)
#define EMT_GLUE_HELPER(a, b) a##b
#define EMT_STATIC_ASSERT(cond, msg) enum { EMT_GLUE(assert_line_, __LINE__) = 1 / (!!(cond)) }
#define EMT_STATIC_ASSERT_INNER(cond, msg)                                                         \
    do {                                                                                           \
        enum { assert = 1 / (!!(cond)) };                                                          \
    } while (0)
#endif

/// Version of the MGIC record type's layout.
#define EMT_MAGIC_VERSION_MAJOR 1
#define EMT_MAGIC_VERSION_MINOR 0
/// Version of the TRCE record type's layout.
#define EMT_TRACE_VERSION_MAJOR 2
#define EMT_TRACE_VERSION_MINOR 0

/// Version of the format of the data output at runtime when emitting a trace
#define EMT_RUNTIME_DATA_TRACE_VERSION_MAJOR 2
#define EMT_RUNTIME_DATA_TRACE_VERSION_MINOR 0

/// Endianness probe value stored in every record (truncated to sizeof(emt_size_t)).
#define EMT_ENDIANNESS_PROBE ((emt_size_t) 0x0706050403020100)

/// MGIC record: 'EMT' prefix (3) + metadata bytes (7) + 'MGIC' type (4).
/// Followed by record_size (emt_size_t), endianness_probe (emt_size_t), meta[35].
///
/// Byte layout of framing[14]:
///   [0..2]  = 'E', 'M', 'T'
///   [3]     = offset from record start to type string      = 10
///   [4]     = offset from record start to record_size      = EMT_OFFSETOF(emt_magic_t,
///   record_size) [5]     = offset from record start to payload          =
///   EMT_OFFSETOF(emt_magic_t, meta) [6]     = offset from record start to endianness probe =
///   EMT_OFFSETOF(emt_magic_t,
///                                                             endianness_probe)
///   [7]     = sizeof(emt_size_t)
///   [8]     = EMT_MAGIC_VERSION_MAJOR
///   [9]     = EMT_MAGIC_VERSION_MINOR
///   [10..13] = 'M', 'G', 'I', 'C'
///
/// Byte layout of meta[35]:
///   [0..31] = 32-byte magic constant
///   [32]    = sizeof(emt_ptr_t)
///   [33]    = EMT_ALIGNMENT_POWER
///   [34]    = encoding id
typedef struct {
    unsigned char framing[14];   ///< 'E','M','T' + 7 metadata bytes + 'M','G','I','C'
    emt_size_t record_size;      ///< total size of this record in bytes
    emt_size_t endianness_probe; ///< endianness probe value (see TRACE_FORMAT.md)
    unsigned char meta[35];      ///< magic(32) + sizeof_emt_ptr_t(1) + alignment_power(1) +
                                 ///<   encoding_id(1)
} emt_magic_t;

void emt_default_begin(const void* info, emt_size_t total_size, void* extra_arg);
void emt_default_out(const void* data, emt_size_t size, void* extra_arg);
void emt_default_serialize_pointer(
    const void* ptr,
    void (*out)(const void* data, emt_size_t size, void* extra_arg),
    void* extra_arg
);
void emt_default_finish(const void* info, emt_size_t total_size, void* extra_arg);
extern const emt_magic_t g_emt_default_magic;

#ifndef EMT_DEFAULT_SEC_ATTR
#define EMT_DEFAULT_ALIGNMENT_POWER 8
#define EMT_DEFAULT_ALIGNMENT (1 << (EMT_DEFAULT_ALIGNMENT_POWER))
#if defined(__GNUC__) || defined(__clang__)
#define EMT_DEFAULT_SEC_ATTR                                                                       \
    __attribute__((used, aligned(EMT_DEFAULT_ALIGNMENT), section(".emtrace"))) const
#elif defined(_MSC_VER)
#define EMT_DEFAULT_SEC_ATTR                                                                       \
    __declspec(align(EMT_DEMT_DEFAULT_ALIGNMENT)) __declspec(allocate(".emtrace")) const
#endif
#endif // EMT_DEFAULT_SEC_ATTR

// from C23 and C++11 onwards we can use enum class with fixed underlying types instead of macros
#if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L) ||                                  \
    (defined(__cplusplus) && __cplusplus >= 201103L)
enum : emt_size_t {
    // In the TRCE record format info, per-argument flag indicating whether dynamically sized data
    // is static (fixed size), null-terminated, or length-prefixed.
    EMT_FLAG_STATIC = 0,          ///< associated bytes have a fixed size
    EMT_FLAG_NULL_TERMINATED = 1, ///< associated bytes are variable in length and null-terminated
    EMT_FLAG_LENGTH_PREFIXED = 2, ///< associated bytes are variable in length, prefixed by a
                                  ///< emt_size_t element count in the output stream

    // In the TRCE record format info, signals what formatter to use.
    EMT_PY_FORMAT = 0,      ///< Use python's str.format function for formatting.
    EMT_NO_FORMAT = 1,      ///< Do not use any formatter; print the string as-is. All additional
                            ///< arguments are discarded.
    EMT_C_STYLE_FORMAT = 2, ///< Use python's C-style formatter

    // In the MGIC record, signals what encoding is used (if any).
    // This is just a signal for the parser to know how to decode the bytestream.
    // You need to make sure that your output function actually encodes the data accordingly.
    EMT_ENCODING_NONE = 0,
    EMT_ENCODING_COBS = 1,
    EMT_ENCODING_COBS_PASSTHROUGH = 2,
};
#else

/// In TRCE record format info: associated bytes have a fixed size.
#define EMT_FLAG_STATIC ((emt_size_t) 0)
/// In TRCE record format info: associated bytes are variable in length and null-terminated.
#define EMT_FLAG_NULL_TERMINATED ((emt_size_t) 1)
/// In TRCE record format info: associated bytes are variable in length, prefixed by a emt_size_t
/// element count in the output stream.
#define EMT_FLAG_LENGTH_PREFIXED ((emt_size_t) 2)

/// Use python's str.format function for formatting.
#define EMT_PY_FORMAT ((emt_size_t) 0)
/// Do not use any formatter; print the string as-is. All additional arguments are discarded.
#define EMT_NO_FORMAT ((emt_size_t) 1)
/// Use python's C-style formatter
#define EMT_C_STYLE_FORMAT ((emt_size_t) 2)

/// Send raw bytestream without any formatting.
#define EMT_ENCODING_NONE ((emt_size_t) 0)
/// Use COBS encoding for the bytestream.
#define EMT_ENCODING_COBS ((emt_size_t) 1)
#define EMT_ENCODING_COBS_PASSTHROUGH ((emt_size_t) 2)
#endif

#ifndef EMT_NO_LIBC

#if (_POSIX_C_SOURCE >= 1 || _XOPEN_SOURCE || _POSIX_SOURCE || _BSD_SOURCE || _SVID_SOURCE)
#define EMT_FWRITE fwrite_unlocked
#else
#define EMT_FWRITE fwrite
#endif

static inline void emt_out_file(const void* data, emt_size_t size, FILE* file) {
    EMT_FWRITE(data, 1, size, file);
}

static inline void emt_out_file_wrapper(const void* data, emt_size_t size, void* extra_arg) {
    FILE* file = (FILE*) extra_arg;
    EMT_FWRITE(data, 1, size, file);
}
#endif // not EMT_NO_LIBC

typedef struct {
    unsigned char buffer[254];
    unsigned char pos;
} emt_cobs_state_t;

/// Initialize / reset `cobs_state`. Call before serializing a new COBS frame.
static inline void emt_cobs_init(emt_cobs_state_t* cobs_state) { cobs_state->pos = 0; }

#ifdef __cplusplus
}
#endif
// NOLINTEND(modernize-*,bugprone-casting-through-void)

#endif // EMTRACE_COMMON

#include <array>
#include <concepts>
#include <cstddef>
#include <cstring>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace emt {

template <typename... T>
struct Ts {};

template <emt_size_t N>
struct StringLiteral {
    constexpr StringLiteral(const char (&str)[N]) {
        for (emt_size_t i = 0; i < N; i++) {
            value[i] = str[i];
        }
    }

    char value[N];
};

template <emt_size_t N>
StringLiteral(const char (&str)[N]) -> StringLiteral<N>;

template <typename T, StringLiteral l = "">
struct Child {
    using Type = T;
    static constexpr StringLiteral lit = l;
};

template <typename T>
struct Trace {};

namespace internal {

template <typename T>
struct IsChildImpl {
    static constexpr bool value = false;
};

template <typename T, StringLiteral l>
struct IsChildImpl<Child<T, l>> {
    static constexpr bool value = true;
};

template <typename T>
concept IsChild = IsChildImpl<T>::value;

template <typename... Args>
struct ConcatImpl {
    using Type = Ts<>;
};

template <typename... Args1, typename... Args2>
struct ConcatImpl<Ts<Args1...>, Ts<Args2...>> {
    using Type = Ts<Args1..., Args2...>;
};

template <typename T, typename... Args>
struct ConcatImpl<T, Args...> {
    using Type = ConcatImpl<T, typename ConcatImpl<Args...>::Type>::Type;
};

template <typename... Args>
using Concat = ConcatImpl<Args...>::Type;

static_assert(std::same_as<Concat<Ts<>, Ts<>>, Ts<>>);
static_assert(std::same_as<Concat<Ts<int>>, Ts<int>>);
static_assert(std::same_as<Concat<Ts<int>, Ts<>>, Ts<int>>);
static_assert(std::same_as<Concat<Ts<int>, Ts<int>>, Ts<int, int>>);
static_assert(std::same_as<Concat<Ts<int>, Ts<int>, Ts<int>>, Ts<int, int, int>>);

template <typename... Ts, typename F>
constexpr void for_types(F&& f) {
    (f.template operator()<Ts>(), ...);
}

template <typename T>
struct ForTsImpl {};

template <typename... Args>
struct ForTsImpl<Ts<Args...>> {
    template <typename F>
    static constexpr void fn(F&& f) {
        for_types<Args...>(std::forward<F>(f));
    }
};

template <typename T, typename F>
constexpr void for_ts(F&& f) {
    ForTsImpl<T>::fn(std::forward<F>(f));
}

template <typename T>
struct GetTraceImpl {
    using Type = Trace<T>;
};

template <IsChild T>
struct GetTraceImpl<T> {
    using Type = Trace<typename T::Type>;
};

template <typename T>
using GetTrace = GetTraceImpl<T>::Type;
} // namespace internal

static constexpr emt_size_t py_fmt = EMT_PY_FORMAT;
static constexpr emt_size_t c_style_fmt = EMT_C_STYLE_FORMAT;
static constexpr emt_size_t no_fmt = EMT_NO_FORMAT;

enum class Infallible {};

struct NullTerminated {
    unsigned char terminator_length;
};
struct LengthPrefixed {};
struct StaticSize {
    emt_size_t size;
};

using SizeType = std::variant<NullTerminated, LengthPrefixed, StaticSize>;

struct DefaultSink {
    const void* record;
    emt_size_t total_size;

    using Error = Infallible;

    [[nodiscard]]
    auto begin(const void* record, emt_size_t total_size) -> std::optional<Error> {
        this->record = record;
        this->total_size = total_size;
        emt_default_begin(record, total_size, nullptr);
        return std::nullopt;
    }

    [[nodiscard]]
    static auto out(const void* data, emt_size_t size) -> std::optional<Error> {
        emt_default_out(data, size, nullptr);
        return std::nullopt;
    }

    [[nodiscard]]
    auto finish() const -> std::optional<Error> {
        emt_default_finish(record, this->total_size, nullptr);
        return std::nullopt;
    }
};

/// Error type of a sink, accepting either a sink type or a reference to one
template <typename S>
using SinkError = typename std::remove_reference_t<S>::Error;

namespace internal {

template <bool thrw = true>
struct OptThrow {
    static constexpr bool value = thrw;
};

template <emt_size_t formatter>
struct OptFormatter {
    static constexpr emt_size_t value = formatter;
};

template <typename S>
struct OptSink {
    S sink;
};

template <typename T>
struct IsOptThrowImpl {
    static constexpr bool value = false;
};

template <typename T>
struct IsOptFormatterImpl {
    static constexpr bool value = false;
};

template <typename T>
struct IsOptSinkImpl {
    static constexpr bool value = false;
};

template <bool thrw>
struct IsOptThrowImpl<OptThrow<thrw>> {
    static constexpr bool value = true;
};

template <emt_size_t formatter>
struct IsOptFormatterImpl<OptFormatter<formatter>> {
    static constexpr bool value = true;
};

template <typename T>
struct IsOptSinkImpl<OptSink<T>> {
    static constexpr bool value = true;
};

template <typename T>
concept IsOptThrow = IsOptThrowImpl<T>::value;

template <typename T>
concept IsOptFormatter = IsOptFormatterImpl<T>::value;

template <typename T>
concept IsOptSink = IsOptSinkImpl<T>::value;

template <typename T>
concept IsOpt = IsOptThrow<T> || IsOptFormatter<T> || IsOptSink<T>;

template <typename T>
struct IsStringLitImpl {
    static constexpr bool value = false;
};

template <emt_size_t n>
struct IsStringLitImpl<StringLiteral<n>> {
    static constexpr bool value = true;
};

template <typename T>
concept IsStringLit = IsStringLitImpl<T>::value;

template <typename T>
struct IsTsOfChildrenImpl {
    static constexpr bool value = false;
};

template <typename... Args>
struct IsTsOfChildrenImpl<Ts<Args...>> {
    static constexpr bool value = (true && ... && IsChild<Args>);
};

template <typename T>
concept IsTsOfChildren = IsTsOfChildrenImpl<T>::value;

template <typename T>
consteval auto constexpr_check(T t) -> T {
    return t;
}

} // namespace internal

template <typename T>
concept Traceable =
    internal::IsTsOfChildren<typename Trace<T>::Children> && requires(T t, DefaultSink s) {
        { Trace<T>::size(t) } -> std::same_as<emt_size_t>;
        { Trace<T>::serialize(t, s) } -> std::same_as<std::optional<SinkError<DefaultSink>>>;

        { internal::constexpr_check(Trace<T>::type_id) } -> internal::IsStringLit;
        { internal::constexpr_check(Trace<T>::size_type) } -> std::same_as<SizeType>;
        { internal::constexpr_check(Trace<T>::simple) } -> std::same_as<bool>;
    };

namespace internal {

template <typename... Args>
constexpr auto get_num_non_opt_args() -> emt_size_t {
    return (emt_size_t{0} + ... + (IsOpt<Args> ? 0 : 1));
}

static_assert(get_num_non_opt_args<>() == 0);
static_assert(get_num_non_opt_args<int>() == 1);
static_assert(get_num_non_opt_args<int, char, long>() == 3);
static_assert(get_num_non_opt_args<int, OptThrow<true>, OptSink<int&>>() == 1);
static_assert(get_num_non_opt_args<OptFormatter<EMT_PY_FORMAT>>() == 0);
static_assert(get_num_non_opt_args<int, OptThrow<true>, int>() == 2);

template <typename... Args>
constexpr auto get_throw() -> bool {
    bool value = false;
    for_types<Args...>([&]<typename T>() -> void {
        if constexpr (IsOptThrow<T>) {
            value = T::value;
        }
    });
    return value;
}

static_assert(!get_throw<>());
static_assert(!get_throw<int, int, int>());
static_assert(!get_throw<OptThrow<false>>());
static_assert(get_throw<OptThrow<true>>());
static_assert(get_throw<OptThrow<false>, OptThrow<true>>());
static_assert(get_throw<int, int, OptThrow<false>, int, OptThrow<true>, int>());

template <typename... Args>
constexpr auto get_formatter() -> emt_size_t {
    emt_size_t value = get_num_non_opt_args<Args...>() == 0 ? EMT_NO_FORMAT : EMT_PY_FORMAT;

    for_types<Args...>([&]<typename T>() -> void {
        if constexpr (IsOptFormatter<T>) {
            value = T::value;
        }
    });
    return value;
}

static_assert(EMT_NO_FORMAT == get_formatter<>());
static_assert(EMT_NO_FORMAT == get_formatter<OptThrow<true>>());
static_assert(EMT_NO_FORMAT == get_formatter<OptSink<int&>>());
static_assert(EMT_PY_FORMAT == get_formatter<int>());
static_assert(EMT_PY_FORMAT == get_formatter<int, int, int>());
static_assert(EMT_PY_FORMAT == get_formatter<OptFormatter<EMT_PY_FORMAT>>());
static_assert(EMT_C_STYLE_FORMAT == get_formatter<OptFormatter<EMT_C_STYLE_FORMAT>>());
static_assert(
    EMT_C_STYLE_FORMAT ==
    get_formatter<OptFormatter<EMT_PY_FORMAT>, OptFormatter<EMT_C_STYLE_FORMAT>>()
);
static_assert(
    EMT_C_STYLE_FORMAT == get_formatter<
                              int,
                              int,
                              OptFormatter<EMT_PY_FORMAT>,
                              int,
                              OptFormatter<EMT_C_STYLE_FORMAT>,
                              int>()
);

constexpr auto get_sink() -> DefaultSink { return DefaultSink{}; }

template <typename T>
constexpr auto get_sink(const T&) -> DefaultSink {
    return DefaultSink{};
}

template <typename T>
    requires(IsOptSink<T>)
constexpr auto get_sink(const T& t) -> decltype(auto) {
    if constexpr (std::is_rvalue_reference_v<decltype(t.sink)>) {
        return std::move(t.sink);
    } else {
        return t.sink;
    }
}

template <typename T, typename U, typename... Args>
    requires(IsOptSink<U>)
constexpr auto get_sink(const T&, const U& u, const Args&... args) -> decltype(auto) {
    return get_sink(u, args...);
}

template <typename T, typename U, typename... Args>
    requires(!IsOptSink<U>)
constexpr auto get_sink(const T& t, const U&, const Args&... args) -> decltype(auto) {
    return get_sink(t, args...);
}

template <typename... Args>
struct AllTypesImpl {
    using Type = Ts<>;
};

template <>
struct AllTypesImpl<Ts<>> {
    using Type = Ts<>;
};

template <typename T, typename... Args>
    requires(IsOpt<T>)
struct AllTypesImpl<T, Args...> {
    using Type = Concat<typename AllTypesImpl<Args...>::Type>;
};

template <typename T, typename... Args>
struct AllTypesImpl<T, Args...> {
    using Type = Concat<
        Ts<T>,
        typename AllTypesImpl<typename Trace<T>::Children>::Type,
        typename AllTypesImpl<Args...>::Type>;
};

template <typename T, StringLiteral l, typename... Args, StringLiteral... ls>
struct AllTypesImpl<Ts<Child<T, l>, Child<Args, ls>...>> {
    using Type = Concat<
        Ts<Child<T, l>>,
        typename AllTypesImpl<typename Trace<T>::Children>::Type,
        typename AllTypesImpl<Child<Args, ls>...>::Type>;
};

template <typename... Ts>
using AllTypes = AllTypesImpl<Ts...>::Type;

template <StringLiteral type_id_, typename T, typename Children_ = Ts<>>
    requires(IsTsOfChildren<Children_>)
struct SimpleTrace {
    using Children = Children_;
    static constexpr StringLiteral type_id = type_id_;
    static constexpr SizeType size_type = StaticSize{sizeof(T)};
    static constexpr bool simple = true;
    template <typename S>
    static auto serialize(const T& val, S&& sink) -> std::optional<SinkError<S>> {
        return std::forward<S>(sink).out(&val, sizeof(T));
    }
    static auto size(const T&) -> emt_size_t { return sizeof(T); }
};

} // namespace internal

template <typename T>
    requires(std::unsigned_integral<T>)
struct Trace<T> : internal::SimpleTrace<"unsigned", T> {};

static_assert(Traceable<unsigned int>);

template <typename T>
    requires(std::signed_integral<T>)
struct Trace<T> : internal::SimpleTrace<"signed", T> {};

static_assert(Traceable<int>);

template <>
struct Trace<bool> : internal::SimpleTrace<"bool", bool> {};

template <>
struct Trace<char> : internal::SimpleTrace<"char", char> {};

template <>
struct Trace<unsigned char> : internal::SimpleTrace<"char", unsigned char> {};

template <>
struct Trace<signed char> : internal::SimpleTrace<"signed char", signed char> {};

template <typename T>
    requires(std::floating_point<T>)
struct Trace<T> : internal::SimpleTrace<"float", T> {};

static_assert(Traceable<float>);
static_assert(Traceable<double>);

template <>
struct Trace<std::string> {
    using Children = Ts<>;
    static constexpr StringLiteral type_id = "string";
    static constexpr SizeType size_type = NullTerminated{1};
    static constexpr bool simple = false;
    template <typename S>
    static auto serialize(const std::string& str, S&& sink) -> std::optional<SinkError<S>> {
        return std::forward<S>(sink).out(str.data(), str.size());
    }
    static auto size(const std::string& str) -> emt_size_t { return str.size(); }
};

static_assert(Traceable<std::string>);

template <>
struct Trace<std::string_view> {
    using Children = Ts<>;
    static constexpr StringLiteral type_id = "string";
    static constexpr SizeType size_type = NullTerminated{1};
    static constexpr bool simple = false;
    template <typename S>
    static auto serialize(std::string_view sv, S&& sink) -> std::optional<SinkError<S>> {
        return std::forward<S>(sink).out(sv.data(), sv.size());
    }
    static auto size(std::string_view sv) -> emt_size_t { return sv.size(); }
};

template <>
struct Trace<const char*> {
    using Children = Ts<>;
    static constexpr StringLiteral type_id = "string";
    static constexpr SizeType size_type = NullTerminated{1};
    static constexpr bool simple = false;
    template <typename S>
    static auto serialize(const char* str, S&& sink) -> std::optional<SinkError<S>> {
        return std::forward<S>(sink).out(str, std::strlen(str));
    }
    static auto size(const char* str) -> emt_size_t { return std::strlen(str); }
};

static_assert(Traceable<const char*>);

template <typename T>
    requires(Traceable<T>)
struct Trace<std::vector<T>> {
    using Children = Ts<Child<T>>;
    static constexpr StringLiteral type_id = "list";
    static constexpr SizeType size_type = LengthPrefixed{};
    static constexpr bool simple = false;
    template <typename S>
    static auto serialize(const std::vector<T>& vec, S&& sink) -> std::optional<SinkError<S>> {
        return Trace<std::span<T>>::serialize(std::span<const T>{vec}, std::forward<S>(sink));
    }
    static auto size(const std::vector<T>& vec) -> emt_size_t { return vec.size(); }
};

static_assert(Traceable<std::vector<int>>);

template <typename T, size_t N>
    requires(Traceable<T>)
struct Trace<T[N]> {
    using Children = Ts<Child<T>>;
    static constexpr StringLiteral type_id = "list";
    static constexpr SizeType size_type = StaticSize{N};
    static constexpr bool simple = Trace<T>::simple;
    template <typename S>
    static auto serialize(const T* data, S&& sink) -> std::optional<SinkError<S>> {
        if constexpr (Trace<T>::simple) {
            return std::forward<S>(sink).out(data, sizeof(T) * N);
        }

        for (size_t i = 0; i < N; i++) {
            auto error = Trace<T>::serialize(data[i], std::forward<S>(sink));
            if (error.has_value()) {
                return error;
            }
        }
        return std::nullopt;
    }
    static auto size(const T*) -> emt_size_t { return N; }
};

static_assert(Traceable<int[10]>);

template <typename T, size_t N>
    requires(Traceable<T>)
struct Trace<std::array<T, N>> {
    using Children = Ts<Child<T>>;
    static constexpr StringLiteral type_id = "list";
    static constexpr SizeType size_type = StaticSize{N};
    static constexpr bool simple = Trace<T>::simple;
    template <typename S>
    static auto serialize(const std::array<T, N>& arr, S&& sink) -> std::optional<SinkError<S>> {
        if constexpr (Trace<T>::simple) {
            return std::forward<S>(sink).out(arr.data(), sizeof(T) * N);
        }

        for (const T& el : arr) {
            auto error = Trace<T>::serialize(el, std::forward<S>(sink));
            if (error.has_value()) {
                return error;
            }
        }
        return std::nullopt;
    }
    static auto size(const std::array<T, N>&) -> emt_size_t { return N; }
};

static_assert(Traceable<std::array<int, 10>>);

template <typename T>
    requires(Traceable<T>)
struct Trace<std::span<T>> {
    using Children = Ts<Child<T>>;
    static constexpr StringLiteral type_id = "list";
    static constexpr SizeType size_type = LengthPrefixed{};
    static constexpr bool simple = true;
    template <typename S>
    static auto serialize(const std::span<const T>& slice, S&& sink)
        -> std::optional<SinkError<S>> {
        if constexpr (Trace<T>::simple) {
            return std::forward<S>(sink).out(slice.data(), sizeof(T) * slice.size());
        }

        for (const T& el : slice) {
            auto error = Trace<T>::serialize(el, std::forward<S>(sink));
            if (error.has_value()) {
                return error;
            }
        }
        return std::nullopt;
    }
    static auto size(const std::span<T>& slice) -> emt_size_t { return slice.size(); }
};

static_assert(Traceable<std::span<int>>);

template <typename R>
    requires(
        std::ranges::sized_range<R> &&
        Traceable<std::remove_reference_t<std::ranges::range_reference_t<R>>>
    )
struct Trace<R> {
    using T = std::remove_reference_t<std::ranges::range_reference_t<R>>;

    using Children = Ts<Child<T>>;
    static constexpr StringLiteral type_id = "list";
    static constexpr SizeType size_type = LengthPrefixed{};
    static constexpr bool simple = false;
    template <typename S>
    static auto serialize(const R& range, S&& sink) -> std::optional<SinkError<S>> {
        for (const T& el : range) {
            auto error = Trace<T>::serialize(el, std::forward<S>(sink));
            if (error.has_value()) {
                return error;
            }
        }
        return std::nullopt;
    }
    static auto size(const R& range) -> emt_size_t { return std::ranges::size(range); }
};

namespace internal {

template <typename S>
auto serialize_varint(S&& sink, emt_size_t len) -> std::optional<SinkError<S>> {
    unsigned char bytes[((sizeof(emt_size_t) * 8) + 6) / 7];

    emt_size_t remaining = len;
    for (size_t i = 0; i < sizeof(bytes); i++) {
        bytes[i] = (unsigned char) remaining & 0x7F;

        remaining >>= 7;
        if (remaining == 0) {
            return std::forward<S>(sink).out(bytes, i + 1);
        }
        bytes[i] |= 0x80;
    }
    assert(false);
}

template <typename S, typename T>
    requires(IsOpt<T>)
auto emit_arg(S&&, const T&) -> std::optional<SinkError<S>> {
    return std::nullopt;
}

template <typename S, typename T>
    requires(Traceable<T>)
auto emit_arg(S&& sink, const T& arg) -> std::optional<SinkError<S>> {
    if constexpr (std::holds_alternative<LengthPrefixed>(Trace<T>::size_type)) {
        auto error = serialize_varint(std::forward<S>(sink), Trace<T>::size(arg));
        if (error.has_value()) {
            return error;
        }
    }
    auto error = Trace<T>::serialize(arg, std::forward<S>(sink));
    if (error.has_value()) {
        return error;
    }

    if constexpr (std::holds_alternative<NullTerminated>(Trace<T>::size_type)) {
        std::array<unsigned char, std::get<NullTerminated>(Trace<T>::size_type).terminator_length>
            null_bytes{};
        return sink.out(null_bytes.data(), sizeof(null_bytes));
    }
    return std::nullopt;
}

static_assert(std::same_as<AllTypes<int>, Ts<int>>);
static_assert(std::same_as<AllTypes<int, int>, Ts<int, int>>);
static_assert(std::same_as<AllTypes<std::vector<int>>, Ts<std::vector<int>, Child<int>>>);
static_assert(std::same_as<AllTypes<int, std::vector<int>>, Ts<int, std::vector<int>, Child<int>>>);
static_assert(std::same_as<AllTypes<std::vector<int>, int>, Ts<std::vector<int>, Child<int>, int>>);
static_assert(std::same_as<
              AllTypes<std::vector<int>, std::vector<int>>,
              Ts<std::vector<int>, Child<int>, std::vector<int>, Child<int>>>);
static_assert(std::same_as<
              AllTypes<std::vector<std::vector<int>>>,
              Ts<std::vector<std::vector<int>>, Child<std::vector<int>>, Child<int>>>);

template <typename... Args>
    requires((IsOpt<Args> || Traceable<Args>) && ...)
constexpr auto get_num_descendants() -> emt_size_t {
    emt_size_t n_descendants = 0;

    for_ts<AllTypes<Args...>>([&]<typename T>() -> void {
        if constexpr (IsChild<T>) {
            n_descendants += 1;
        }
    });

    return n_descendants;
}

static_assert(get_num_descendants<int>() == 0);
static_assert(get_num_descendants<int, char, long>() == 0);
static_assert(get_num_descendants<std::vector<int>>() == 1);
static_assert(get_num_descendants<int, std::vector<int>>() == 1);
static_assert(get_num_descendants<std::vector<int>, std::vector<int>>() == 2);
static_assert(get_num_descendants<std::vector<std::vector<int>>>() == 2);
static_assert(get_num_descendants<int, OptFormatter<EMT_PY_FORMAT>>() == 0);

template <typename... Args>
    requires((IsOpt<Args> || Traceable<Args>) && ...)
constexpr auto get_num_children() -> emt_size_t {
    emt_size_t n_children = 0;
    for_types<Args...>([&]<typename T>() -> void {
        if constexpr (!IsOpt<T>) {
            for_ts<typename Trace<T>::Children>([&]<typename C>() -> void { n_children++; });
        }
    });
    return n_children;
}

static_assert(get_num_children<int>() == 0);
static_assert(get_num_children<int, char, long>() == 0);
static_assert(get_num_children<std::vector<int>>() == 1);
static_assert(get_num_children<int, std::vector<int>>() == 1);
static_assert(get_num_children<std::vector<int>, std::vector<int>>() == 2);
static_assert(get_num_children<std::vector<std::vector<int>>>() == 1);
static_assert(get_num_children<int, OptFormatter<EMT_PY_FORMAT>>() == 0);
static_assert(get_num_children<int, OptThrow<true>>() == 0);

template <typename... Args>
    requires((IsOpt<Args> || Traceable<Args>) && ...)
constexpr auto get_num_type_strings() -> emt_size_t {
    return get_num_non_opt_args<Args...>() + (get_num_descendants<Args...>() * 2);
}

static_assert(get_num_type_strings<int>() == 1);
static_assert(get_num_type_strings<int, int>() == 2);
static_assert(get_num_type_strings<std::vector<int>>() == 3);
static_assert(get_num_type_strings<int, std::vector<int>>() == 4);
static_assert(get_num_type_strings<std::vector<std::vector<int>>>() == 5);
static_assert(get_num_type_strings<int, OptThrow<true>>() == 1);
static_assert(get_num_type_strings<int, int, OptSink<int&>>() == 2);

template <typename... Args>
    requires((IsOpt<Args> || Traceable<Args>) && ...)
constexpr auto get_type_string_length() -> emt_size_t {
    emt_size_t len = 0;

    for_ts<AllTypes<Args...>>([&]<typename T>() constexpr -> void {
        if constexpr (IsChild<T>) {
            len += sizeof(T::lit.value);
        }
        using Trce = GetTrace<T>;
        len += sizeof(Trce::type_id.value);
    });

    return len;
}

template <typename... Args>
    requires((IsOpt<Args> || Traceable<Args>) && ...)
constexpr auto get_type_layout_len() -> emt_size_t {
    return 7 + (get_num_non_opt_args<Args...>() * 4) + (get_num_descendants<Args...>() * 5);
}

static_assert(get_type_layout_len<int>() == 11);
static_assert(get_type_layout_len<int, int>() == 15);
static_assert(get_type_layout_len<std::vector<int>>() == 16);
static_assert(get_type_layout_len<int, OptThrow<true>>() == 11);
static_assert(get_type_layout_len<int, OptThrow<true>, std::vector<int>>() == 20);

template <typename... Args>
    requires((IsOpt<Args> || Traceable<Args>) && ...)
constexpr auto get_layout(
    emt_size_t formatter_id,
    emt_size_t source_line,
    emt_size_t fmt_offset,
    emt_size_t type_strings_offset,
    emt_size_t file_offset
) -> std::array<emt_size_t, get_type_layout_len<Args...>()> {
    std::array<emt_size_t, get_type_layout_len<Args...>()> arr{
        EMT_RUNTIME_DATA_TRACE_VERSION_MAJOR, EMT_RUNTIME_DATA_TRACE_VERSION_MINOR,
        get_num_non_opt_args<Args...>(), fmt_offset
    };

    emt_size_t index = type_strings_offset;
    emt_size_t i = 4;

    for_ts<AllTypes<Args...>>([&]<typename T>() constexpr -> void {
        arr[i] = index;
        if constexpr (IsChild<T>) {
            index += sizeof(T::lit.value);
            i++;
        }

        using Trce = GetTrace<T>;
        arr[i] = index;
        if (std::holds_alternative<StaticSize>(Trce::size_type)) {
            arr[i + 1] = std::get<StaticSize>(Trce::size_type).size;
            arr[i + 2] = EMT_FLAG_STATIC;
        } else if (std::holds_alternative<LengthPrefixed>(Trce::size_type)) {
            arr[i + 1] = 0;
            arr[i + 2] = EMT_FLAG_LENGTH_PREFIXED;
        } else if (std::holds_alternative<NullTerminated>(Trce::size_type)) {
            arr[i + 1] = std::get<NullTerminated>(Trce::size_type).terminator_length;
            arr[i + 2] = EMT_FLAG_NULL_TERMINATED;
        } else {
            arr[i + 1] = 0;
            arr[i + 2] = 0;
        }
        if constexpr (IsChild<T>) {
            arr[i + 3] = get_num_children<typename T::Type>();
        } else {
            arr[i + 3] = get_num_children<T>();
        }

        index += sizeof(Trce::type_id.value);
        i += 4;
    });

    arr[i] = formatter_id;
    arr[i + 1] = file_offset;
    arr[i + 2] = source_line;

    return arr;
}

template <StringLiteral fmt, StringLiteral file_, typename... Args>
    requires((IsOpt<Args> || Traceable<Args>) && ...)
struct FormatInfo {
    unsigned char frameing[14];
    emt_size_t record_size;
    emt_size_t endianness_probe;
    emt_size_t layout[get_type_layout_len<Args...>()];
    char format[sizeof(fmt)];
    char type_strings[get_type_string_length<Args...>() + 1];
    char file[sizeof(file_)];
};

template <StringLiteral fmt, StringLiteral file_, typename... Args>
    requires((IsOpt<Args> || Traceable<Args>) && ...)
constexpr auto make_format_info(emt_size_t formatter_id, emt_size_t source_line)
    -> FormatInfo<fmt, file_, Args...> {
    using FmtInfo = FormatInfo<fmt, file_, Args...>;

    FmtInfo fmt_info{
        .frameing =
            {'E', 'M', 'T', 10, offsetof(FmtInfo, record_size), offsetof(FmtInfo, layout),
             offsetof(FmtInfo, endianness_probe), sizeof(emt_size_t), EMT_TRACE_VERSION_MAJOR,
             EMT_TRACE_VERSION_MINOR, 'T', 'R', 'C', 'E'},
        .record_size = sizeof(FmtInfo),
        .endianness_probe = EMT_ENDIANNESS_PROBE,
        .layout = {0},
        .format = {0},
        .type_strings = {0},
        .file = {0},
    };

    for (emt_size_t i = 0; i < sizeof(fmt.value); i++) {
        fmt_info.format[i] = fmt.value[i];
    }

    auto layout = get_layout<Args...>(
        formatter_id, source_line, offsetof(FmtInfo, format), offsetof(FmtInfo, type_strings),
        offsetof(FmtInfo, file)
    );

    for (emt_size_t i = 0; i < layout.size(); i++) {
        fmt_info.layout[i] = layout[i];
    }

    emt_size_t index = 0;
    for_ts<AllTypes<Args...>>([&]<typename T>() constexpr -> void {
        if constexpr (IsChild<T>) {
            for (emt_size_t i = 0; i < sizeof(T::lit.value); i++) {
                fmt_info.type_strings[index] = T::lit.value[i];
                index++;
            }
        }

        using Trce = GetTrace<T>;
        for (emt_size_t i = 0; i < sizeof(Trce::type_id); i++) {
            fmt_info.type_strings[index] = Trce::type_id.value[i];
            index++;
        }
    });

    for (emt_size_t i = 0; i < sizeof(file_.value); i++) {
        fmt_info.file[i] = file_.value[i];
    }

    return fmt_info;
};

template <StringLiteral fmt>
class PyFormatValidator {
    static constexpr auto n = sizeof(fmt.value);

    const emt_size_t n_args;

    bool manual_field_numbering = false;
    bool auto_field_numbering = false;
    emt_size_t n_fields = 0;
    emt_size_t i = 0;

    [[nodiscard]] constexpr auto is_numeric() const -> bool {
        return fmt.value[i] >= '0' && fmt.value[i] <= '9';
    }

    constexpr auto parse_number() -> emt_size_t {
        emt_size_t number = 0;
        for (emt_size_t j = 0; i < n; i++, j++) {
            if (!is_numeric()) {
                return number;
            }
            number *= 10;
            number += (emt_size_t) (fmt.value[i] - '0');
        }
        return number;
    }

    constexpr auto validate_replacement() -> void {
        if (is_numeric()) {
            if (auto_field_numbering) {
                i = n + 1;
                return;
            }
            if (parse_number() >= n_args) {
                i = n + 1;
                return;
            }
            manual_field_numbering = true;
        } else {
            if (manual_field_numbering) {
                i = n + 1;
                return;
            }
            auto_field_numbering = true;
        }
    }

    constexpr auto parse_inner_replacement() -> void {
        validate_replacement();

        for (; i < n; i++) {
            if (fmt.value[i] == '}') {
                n_fields++;
                return;
            }
        }
    }

    constexpr auto parse_spec() -> void {
        for (; i < n; i++) {
            if (fmt.value[i] == '}') {
                n_fields++;
                i++;
                return;
            }
            if (fmt.value[i] == '{') {
                i++;
                parse_inner_replacement();
            }
        }
    }

    constexpr auto parse_replacement() -> void {
        validate_replacement();

        for (; i < n; i++) {
            if (fmt.value[i] == '}') {
                n_fields += 1;
                return;
            }
            if (fmt.value[i] == ':') {
                i++;
                parse_spec();
                return;
            }
        }
    }

    constexpr PyFormatValidator(emt_size_t n_args) : n_args(n_args) {}

    constexpr auto do_validate() -> bool {
        for (; i < n; i++) {
            if (fmt.value[i] == '{') {
                if (i + 1 == n) {
                    return false;
                }
                if (fmt.value[i + 1] == '{') {
                    i++;
                    continue;
                }
                i++;
                parse_replacement();
            } else if (fmt.value[i] == '}') {
                if (i + 1 == n) {
                    return false;
                }
                if (fmt.value[i + 1] != '}') {
                    return false;
                }
                i++;
            }
        }

        if (auto_field_numbering && n_fields > n_args) {
            return false;
        }

        return (i <= n);
    }

public:
    [[nodiscard]] static constexpr auto validate(emt_size_t n_args) -> bool {
        return PyFormatValidator<fmt>{n_args}.do_validate();
    }
};

static_assert(PyFormatValidator<"">::validate(0));
static_assert(PyFormatValidator<"">::validate(1));
static_assert(PyFormatValidator<"">::validate(2));
static_assert(PyFormatValidator<"">::validate(1 << 15));

static_assert(!PyFormatValidator<"{">::validate(0));
static_assert(!PyFormatValidator<"}">::validate(0));
static_assert(!PyFormatValidator<"{}">::validate(0));
static_assert(!PyFormatValidator<"{{}">::validate(0));

static_assert(PyFormatValidator<"{{">::validate(0));
static_assert(PyFormatValidator<"}}">::validate(0));

static_assert(PyFormatValidator<"{}">::validate(1));
static_assert(PyFormatValidator<"{:}">::validate(1));
static_assert(PyFormatValidator<"{}{}">::validate(2));
static_assert(PyFormatValidator<"{}{}{}{}">::validate(4));
static_assert(PyFormatValidator<"{} {} {} {} {}">::validate(5));

static_assert(PyFormatValidator<"{0} {0} {0} {0} {0}">::validate(1));
static_assert(!PyFormatValidator<"{0} {0} {0} {0} {0}">::validate(0));

static_assert(!PyFormatValidator<"{7}">::validate(7));
static_assert(PyFormatValidator<"{7}">::validate(8));

static_assert(PyFormatValidator<"{12345}">::validate(20000));

static_assert(!PyFormatValidator<"{:{}}">::validate(1));
static_assert(PyFormatValidator<"{:{}}">::validate(2));
static_assert(!PyFormatValidator<"{:{0}}">::validate(2));
static_assert(!PyFormatValidator<"{0:{}}">::validate(2));
static_assert(PyFormatValidator<"{0:{0}}">::validate(1));

template <StringLiteral l>
constexpr auto validate_c_style_format(emt_size_t n_args) -> bool {
    constexpr auto n = sizeof(l.value);
    emt_size_t n_format_specs = 0;

    for (emt_size_t i = 0; i < n; i++) {
        if (l.value[i] != '%') {
            continue;
        }
        if (i + 1 == n) {
            return false;
        }
        i++;

        if (l.value[i] != '%') {
            n_format_specs++;
        }
    }

    return n_format_specs == n_args;
}

static_assert(!validate_c_style_format<"%">(0));
static_assert(validate_c_style_format<"%%">(0));

static_assert(!validate_c_style_format<"%d">(0));
static_assert(!validate_c_style_format<"%d">(2));
static_assert(validate_c_style_format<"%d">(1));

template <StringLiteral fmt, typename... Args>
constexpr auto format_string_ok(emt_size_t formatter_id) -> bool {
    emt_size_t n_non_opt_args = get_num_non_opt_args<Args...>();

    switch (formatter_id) {
    case EMT_NO_FORMAT:
        return n_non_opt_args == 0;
    case EMT_PY_FORMAT:
        return PyFormatValidator<fmt>::validate(n_non_opt_args);
    case EMT_C_STYLE_FORMAT:
        return validate_c_style_format<fmt>(n_non_opt_args);
    default:
        return true;
    }
}

template <bool throw_on_error, typename S, typename... Args>
    requires((IsOpt<Args> || Traceable<Args>) && ...)
[[nodiscard]] auto emit_trace(const void* record, S&& sink, const Args&... args)
    -> std::conditional_t<
        throw_on_error || std::same_as<SinkError<S>, Infallible>,
        void,
        std::optional<SinkError<S>>> {
    std::optional<SinkError<S>> error = sink.begin(record, 0);

    emt_default_serialize_pointer( // todo: make ptr serializer modular
        record,
        [](const void* data, emt_size_t size, void* sink) -> void {
            (void) (*reinterpret_cast<std::remove_reference_t<S>*>(sink)).out(data, size);
        },
        &sink
    );

    if constexpr (!std::same_as<SinkError<S>, Infallible>) {
        if (error.has_value()) {
            if constexpr (throw_on_error) {
                throw 1; // todo: create exception type
            }
            return error;
        }
    }

    (void) ((error = emit_arg(std::forward<S>(sink), args), error.has_value()) || ...);

    if constexpr (!std::same_as<SinkError<S>, Infallible>) {
        if (error.has_value()) {
            if constexpr (throw_on_error) {
                throw 1;
            }
            return error;
        }
    }

    error = sink.finish();
    if constexpr (!std::same_as<SinkError<S>, Infallible>) {
        if (error.has_value()) {
            if constexpr (throw_on_error) {
                throw 1;
            }
            return error;
        }
    }
}

} // namespace internal

#define EMT_CPP_TRACE_IMPL(fmt_info_attributes, postfix, fmt, ...)                                 \
    []<typename... Args>(const Args&... args) -> auto {                                            \
        constexpr emt_size_t formatter_id = emt::internal::get_formatter<Args...>();               \
        static_assert(((emt::internal::IsOpt<Args> || emt::Traceable<Args>) && ...));              \
        static_assert(emt::internal::format_string_ok<fmt postfix, Args...>(formatter_id));        \
        static fmt_info_attributes constexpr auto fmt_info =                                       \
            emt::internal::make_format_info<fmt postfix, __FILE__, Args...>(                       \
                formatter_id, __LINE__                                                             \
            );                                                                                     \
        return emt::internal::emit_trace<emt::internal::get_throw<Args...>()>(                     \
            &fmt_info, emt::internal::get_sink(args...), args...                                   \
        );                                                                                         \
    }(__VA_ARGS__)

#define EMT_CPP_TRACE(fmt_info_attributes, ...)                                                    \
    EMT_CPP_TRACE_IMPL(fmt_info_attributes, "", __VA_ARGS__)
#define EMT_CPP_TRACELN(fmt_info_attributes, ...)                                                  \
    EMT_CPP_TRACE_IMPL(fmt_info_attributes, "\n", __VA_ARGS__)

#ifdef EMT_DEFAULT_SEC_ATTR
#define MTRACE(...) EMT_CPP_TRACE(EMT_DEFAULT_SEC_ATTR, __VA_ARGS__)
#define MTRACELN(...) EMT_CPP_TRACELN(EMT_DEFAULT_SEC_ATTR, __VA_ARGS__)
#endif

struct VecSink {
    std::vector<unsigned char> buffer;
    using Error = Infallible;

    [[nodiscard]]
    static auto begin(const void*, emt_size_t) -> std::optional<Error> {
        return std::nullopt;
    }

    [[nodiscard]]
    auto out(const void* data, emt_size_t size) -> std::optional<Error> {
        buffer.reserve(std::max(buffer.size() + size, buffer.size() * 2));
        buffer.resize(buffer.size() + size);
        std::memcpy(buffer.data() + buffer.size() - size, data, size);

        return std::nullopt;
    }

    [[nodiscard]]
    static auto finish() -> std::optional<Error> {
        return std::nullopt;
    }
};

static constexpr internal::OptThrow<true> opt_throw = internal::OptThrow{};
static constexpr internal::OptThrow<false> opt_no_throw = internal::OptThrow<false>{};

template <emt_size_t formatter>
static constexpr internal::OptFormatter<formatter> opt_fmt = internal::OptFormatter<formatter>{};

static constexpr internal::OptFormatter<emt::no_fmt> opt_no_fmt =
    internal::OptFormatter<emt::no_fmt>{};

static constexpr internal::OptFormatter<emt::py_fmt> opt_py_fmt =
    internal::OptFormatter<emt::py_fmt>{};

static constexpr internal::OptFormatter<emt::c_style_fmt> opt_c_style_fmt =
    internal::OptFormatter<emt::c_style_fmt>{};

template <typename T>
constexpr auto opt_sink(T&& t) -> auto {
    using SinkRef = std::conditional_t<std::is_reference_v<T>, T, T&&>;
    return internal::OptSink<SinkRef>{std::forward<T>(t)};
}
static_assert(1 == get_sink(internal::OptSink{1}));
static_assert(2 == get_sink(internal::OptSink{1}, internal::OptSink{2}));
static_assert(std::same_as<decltype(internal::get_sink()), DefaultSink>);
static_assert(std::same_as<decltype(internal::get_sink(opt_sink(1))), int&&>);
static_assert(std::same_as<decltype(internal::get_sink(1, 2, opt_sink(1), 2)), int&&>);
static_assert(
    std::same_as<decltype(internal::get_sink(opt_sink(1), opt_sink(1U))), unsigned int&&>
);

} // namespace emt

#endif // EMTRACE_EMTRACE_HPP
