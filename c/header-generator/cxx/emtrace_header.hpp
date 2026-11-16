#include "../c/emtrace_common.h"

#include <array>
#include <concepts>
#include <cstddef>
#include <cstring>
#include <exception>
#include <ios>
#include <iostream>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <syncstream>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

// NOLINTBEGIN(readability-named-parameter)
namespace emt {

template <typename Error>
struct TraceException : std::exception {
    Error e;
    template <typename... Args>
    explicit TraceException(Args&&... args) : e{std::forward<Args>(args)...} {}

    [[nodiscard]] auto what() const noexcept -> const char* override {
        if constexpr (requires(Error e) {
                          { e.what() } -> std::same_as<const char*>;
                      }) {
            return e.what();
        } else {
            return "Error while emitting Trace";
        }
    }
};

template <typename... T>
struct Ts {};

template <emt_size_t N>
    requires(N >= 1)
struct StringLiteral {
    constexpr StringLiteral(const char (&str)[N]) {
        for (emt_size_t i = 0; i < N; i++) {
            value[i] = str[i];
        }
    }

    constexpr StringLiteral() : value{} {}

    template <emt_size_t M>
    constexpr auto operator+(const StringLiteral<M>& other) -> StringLiteral<N + M - 1> {
        StringLiteral<N + M - 1> concat{};
        for (emt_size_t i = 0; i < N - 1; i++) {
            concat.value[i] = value[i];
        }

        for (emt_size_t i = 0; i < M; i++) {
            concat.value[i + N - 1] = other.value[i];
        }

        return concat;
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
struct Sink {};

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

/// Error type of a sink, accepting either a sink type or a reference to one
template <typename S>
using SinkError = Sink<S>::Error;

template <typename T>
concept IsSink = requires(T t, Sink<T> sink_impl, const void* ptr, emt_size_t size) {
    typename Sink<T>::Error;
    { Sink<T>{std::forward<T>(t), ptr, size} } -> std::same_as<Sink<T>>;
    { sink_impl.begin() } -> std::same_as<std::optional<SinkError<T>>>;
    { Sink<T>::out_ptr(sink_impl, ptr) } -> std::same_as<std::optional<SinkError<T>>>;
    { sink_impl.out(ptr, size) } -> std::same_as<std::optional<SinkError<T>>>;
    { std::move(sink_impl).finish() } -> std::same_as<std::optional<SinkError<T>>>;
};

inline constexpr emt_size_t default_ptr_width = 3;

struct StdoutSink {};

template <>
struct Sink<StdoutSink&> {
    using Error = Infallible;
    static constexpr unsigned char encoding_id = EMT_ENCODING_NONE;
    static constexpr unsigned char ptr_width = default_ptr_width;
    static constexpr unsigned char alignment_power = EMT_DEFAULT_ALIGNMENT_POWER;

    Sink(StdoutSink, const void*, emt_size_t) : synced_stdout(std::cout) {}

    static auto begin() -> std::optional<Error> { return std::nullopt; }

    template <typename S>
    static auto out_ptr(S&& sink_impl, const void* record)
        -> std::optional<typename std::remove_reference_t<S>::Error> {
        emt_ptr_t shifted = reinterpret_cast<emt_ptr_t>(record) >> alignment_power;

        std::array<unsigned char, ptr_width> bytes{};
        for (emt_size_t i = 0; i < ptr_width; i++) {
            bytes[i] = static_cast<unsigned char>(shifted >> (i * 8));
        }

        return std::forward<S>(sink_impl).out(bytes.data(), bytes.size());
    }

    auto out(const void* ptr, emt_size_t size) -> std::optional<Error> {
        synced_stdout.write(reinterpret_cast<const char*>(ptr), static_cast<std::streamsize>(size));
        return std::nullopt;
    }

    static auto finish() -> std::optional<Error> { return std::nullopt; }

private:
    std::osyncstream synced_stdout;
};

static_assert(IsSink<StdoutSink&>);

template <>
struct Sink<std::ostream&> {
    using Error = Infallible;
    static constexpr unsigned char encoding_id = EMT_ENCODING_NONE;
    static constexpr unsigned char ptr_width = default_ptr_width;
    static constexpr unsigned char alignment_power = EMT_DEFAULT_ALIGNMENT_POWER;

    Sink(std::ostream& stream, const void*, emt_size_t) : synced_stdout(stream) {}

    static auto begin() -> std::optional<Error> { return std::nullopt; }

    template <typename SI>
    [[nodiscard]]
    static auto out_ptr(SI&& sink_impl, const void* record)
        -> std::optional<typename std::remove_reference_t<SI>::Error> {
        emt_ptr_t shifted = reinterpret_cast<emt_ptr_t>(record) >> alignment_power;

        std::array<unsigned char, ptr_width> bytes{};
        for (emt_size_t i = 0; i < ptr_width; i++) {
            bytes[i] = static_cast<unsigned char>(shifted >> (i * 8));
        }

        return std::forward<SI>(sink_impl).out(bytes.data(), bytes.size());
    }

    auto out(const void* ptr, emt_size_t size) -> std::optional<Error> {
        synced_stdout.write(reinterpret_cast<const char*>(ptr), static_cast<std::streamsize>(size));
        return std::nullopt;
    }

    static auto finish() -> std::optional<Error> { return std::nullopt; }

private:
    std::osyncstream synced_stdout;
};

struct DefaultSink {};

template <>
struct Sink<DefaultSink> {
    const void* record;
    emt_size_t total_size;

    using Error = Infallible;

    Sink(const DefaultSink, const void* record, emt_size_t total_size)
        : record(record), total_size(total_size) {}

    [[nodiscard]] auto begin() const -> std::optional<Error> {
        emt_default_begin(record, total_size, nullptr);
        return std::nullopt;
    }

    template <typename S>
    [[nodiscard]] static auto out_ptr(S&& sink_impl, const void* record)
        -> std::optional<typename std::remove_reference_t<S>::Error> {
        emt_default_serialize_pointer(
            record,
            [](const void* data, emt_size_t size, void* extra_arg) -> void {
                (void) std::forward<S>(*reinterpret_cast<std::remove_reference_t<S>*>(extra_arg))
                    .out(data, size);
            },
            &sink_impl
        );
        return std::nullopt;
    }

    [[nodiscard]]
    static auto out(const void* data, emt_size_t size) -> std::optional<Error> {
        emt_default_out(data, size, nullptr);
        return std::nullopt;
    }

    [[nodiscard]] auto finish() const -> std::optional<Error> {
        emt_default_finish(record, total_size, nullptr);
        return std::nullopt;
    }
};

/// Shifts the record address right by `SHIFT` bits, then emits the `N_BYTES`
/// least-significant bytes (little-endian) through the sink.
template <emt_size_t N_BYTES, unsigned char SHIFT, typename S = StdoutSink>
    requires(
        (IsSink<S> || IsSink<S&>) && N_BYTES > 0 && N_BYTES <= sizeof(emt_ptr_t) &&
        SHIFT < sizeof(emt_ptr_t) * 8
    )
struct ChangePtr {
    S inner;
};

template <emt_size_t N_BYTES, unsigned char SHIFT, typename S>
    requires(
        IsSink<S&> && N_BYTES > 0 && N_BYTES <= sizeof(emt_ptr_t) && SHIFT < sizeof(emt_ptr_t) * 8
    )
struct Sink<ChangePtr<N_BYTES, SHIFT, S>&> : Sink<S&> {
    static constexpr emt_size_t ptr_width = N_BYTES;
    static constexpr unsigned char alignment_power = SHIFT;
    using Error = Sink<S&>::Error;

    Sink(ChangePtr<ptr_width, alignment_power, S>& s, const void* record, emt_size_t total_size)
        : Sink<S&>{s.inner, record, total_size} {}

    template <typename SI>
    static auto out_ptr(SI&& sink_impl, const void* record)
        -> std::optional<typename std::remove_reference_t<SI>::Error> {
        emt_ptr_t shifted = reinterpret_cast<emt_ptr_t>(record) >> alignment_power;

        std::array<unsigned char, ptr_width> bytes{};
        for (emt_size_t i = 0; i < ptr_width; i++) {
            bytes[i] = static_cast<unsigned char>(shifted >> (i * 8));
        }

        return std::forward<SI>(sink_impl).out(bytes.data(), bytes.size());
    }
};

template <typename S, emt_size_t N_BYTES, unsigned char SHIFT>
    requires(
        IsSink<S> && N_BYTES > 0 && N_BYTES <= sizeof(emt_ptr_t) && SHIFT < sizeof(emt_ptr_t) * 8
    )
struct Sink<ChangePtr<N_BYTES, SHIFT, S>> : Sink<S> {
    static constexpr emt_size_t ptr_width = N_BYTES;
    static constexpr unsigned char alignment_power = SHIFT;
    using Error = Sink<S>::Error;

    Sink(ChangePtr<ptr_width, alignment_power, S>&& s, const void* record, emt_size_t total_size)
        : Sink<S>{std::move(s.inner), record, total_size} {}

    template <typename SI>
    static auto out_ptr(SI&& sink_impl, const void* record)
        -> std::optional<typename std::remove_reference_t<SI>::Error> {
        emt_ptr_t shifted = reinterpret_cast<emt_ptr_t>(record) >> alignment_power;

        std::array<unsigned char, ptr_width> bytes{};
        for (emt_size_t i = 0; i < ptr_width; i++) {
            bytes[i] = static_cast<unsigned char>(shifted >> (i * 8));
        }

        return std::forward<SI>(sink_impl).out(bytes.data(), bytes.size());
    }
};

static_assert(IsSink<ChangePtr<1, 1, DefaultSink>>);

struct CobsEncoder {
    template <typename S>
    auto encode(S& sink_impl, const void* data, emt_size_t size)
        -> std::optional<typename S::Error> {
        const auto* bytes = reinterpret_cast<const unsigned char*>(data);

        for (emt_size_t i = 0; i < size; i++) {
            unsigned char byte = bytes[i];
            // A full block is written with the code 0xff, which means that it is not followed
            // by an (implicit) zero. So it has to be written before looking at the next byte, even
            // if that is a zero.
            if (pos == 254) {
                unsigned char ptr = 0xff;
                if (auto err = sink_impl.out(&ptr, 1)) {
                    return err;
                }
                if (auto err = sink_impl.out(buffer.data(), 254)) {
                    return err;
                }
                pos = 0;
            }
            if (byte == 0) {
                unsigned char ptr = pos + 1;
                if (auto err = sink_impl.out(&ptr, 1)) {
                    return err;
                }

                if (auto err = sink_impl.out(buffer.data(), pos)) {
                    return err;
                }
                pos = 0;
                continue;
            }
            buffer[pos] = byte;
            pos++;
        }

        return std::nullopt;
    }

    template <typename S>
    auto finalize(S& sink_impl) -> std::optional<typename S::Error> {
        if (pos > 0) {
            unsigned char ptr = pos + 1;
            if (auto err = sink_impl.out(&ptr, 1)) {
                return err;
            }
            if (auto err = sink_impl.out(buffer.data(), pos)) {
                return err;
            }
        } else {
            unsigned char ptr = 1;
            if (auto err = sink_impl.out(&ptr, 1)) {
                return err;
            }
        }
        unsigned char zero = 0;
        if (auto err = sink_impl.out(&zero, 1)) {
            return err;
        }
        pos = 0;
        return std::nullopt;
    }

private:
    std::array<unsigned char, 254> buffer;
    unsigned char pos{0};
};

template <typename S = StdoutSink>
    requires(
        (IsSink<S> && Sink<S>::encoding_id == EMT_ENCODING_NONE) ||
        (IsSink<S&> && Sink<S&>::encoding_id == EMT_ENCODING_NONE)
    )
struct Cobs {
    S inner;
};

Cobs() -> Cobs<StdoutSink>;

template <typename S>
    requires(IsSink<S> && Sink<S>::encoding_id == EMT_ENCODING_NONE)
struct Sink<Cobs<S>> {
    using Error = Sink<S>::Error;
    static constexpr unsigned char encoding_id = EMT_ENCODING_COBS;
    static constexpr emt_size_t ptr_width = Sink<S>::ptr_width;
    static constexpr unsigned char alignment_power = Sink<S>::alignment_power;

    Sink(Cobs<S>&& sink, const void* record, emt_size_t total_size)
        : inner{std::move(sink.inner), record, total_size} {}

    [[nodiscard]] auto begin() const -> std::optional<Error> { return inner.begin(); }

    template <typename SI>
    [[nodiscard]] static auto out_ptr(SI&& sink_impl, const void* record)
        -> std::optional<typename std::remove_reference_t<SI>::Error> {
        return Sink<S>::out_ptr(std::forward<SI>(sink_impl), record);
    }

    [[nodiscard]]
    auto out(const void* data, emt_size_t size) -> std::optional<Error> {
        return encoder.encode(inner, data, size);
    }

    [[nodiscard]] auto finish() const -> std::optional<Error> {
        if (auto err = encoder.finalize(inner)) {
            return err;
        }

        return std::move(inner).finish();
    }

private:
    Sink<S> inner;
    CobsEncoder encoder{};
};

template <typename S>
    requires(IsSink<S&> && Sink<S&>::encoding_id == EMT_ENCODING_NONE)
struct Sink<Cobs<S>&> {
    using Error = Sink<S&>::Error;
    static constexpr unsigned char encoding_id = EMT_ENCODING_COBS;
    static constexpr emt_size_t ptr_width = Sink<S&>::ptr_width;
    static constexpr unsigned char alignment_power = Sink<S&>::alignment_power;

    Sink(Cobs<S>& sink, const void* record, emt_size_t total_size)
        : inner{sink.inner, record, total_size} {}

    [[nodiscard]] auto begin() -> std::optional<Error> { return inner.begin(); }

    template <typename SI>
    [[nodiscard]] static auto out_ptr(SI&& sink_impl, const void* record)
        -> std::optional<typename std::remove_reference_t<SI>::Error> {
        return Sink<S&>::out_ptr(std::forward<SI>(sink_impl), record);
    }

    [[nodiscard]]
    auto out(const void* data, emt_size_t size) -> std::optional<Error> {
        return encoder.encode(inner, data, size);
    }

    [[nodiscard]] auto finish() && -> std::optional<Error> {
        if (auto err = encoder.finalize(inner)) {
            return err;
        }

        return std::move(inner).finish();
    }

private:
    Sink<S&> inner;
    CobsEncoder encoder{};
};

namespace internal {

template <bool thrw = true>
struct OptThrow {
    static constexpr bool value = thrw;
};

template <emt_size_t formatter>
struct OptFormatter {
    static constexpr emt_size_t value = formatter;
};

template <emt_size_t alignment>
    requires((alignment & (alignment - 1)) == 0)
struct OptAlignment {
    static constexpr emt_size_t value = alignment;
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
struct IsOptAlignmentImpl {
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

template <emt_size_t alignment>
struct IsOptAlignmentImpl<OptAlignment<alignment>> {
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
concept IsOptAlignment = IsOptAlignmentImpl<T>::value;

template <typename T>
concept IsOptSink = IsOptSinkImpl<T>::value;

template <typename T>
concept IsOpt = IsOptThrow<T> || IsOptFormatter<T> || IsOptAlignment<T> || IsOptSink<T>;

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
    internal::IsTsOfChildren<typename Trace<T>::Children> && requires(T t, Sink<DefaultSink> s) {
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

template <typename... Args>
constexpr auto get_alignment() -> emt_size_t {
    emt_size_t value = EMT_DEFAULT_ALIGNMENT;

    for_types<Args...>([&]<typename T>() -> void {
        if constexpr (IsOptAlignment<T>) {
            value = T::value;
        }
    });
    return value;
}

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
    static auto serialize(const T& val, S& sink) -> std::optional<typename S::Error> {
        return sink.out(&val, sizeof(T));
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
    static auto serialize(const std::string& str, S& sink_impl)
        -> std::optional<typename S::Error> {
        return sink_impl.out(str.data(), str.size());
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
    static auto serialize(std::string_view sv, S& sink_impl) -> std::optional<typename S::Error> {
        return sink_impl.out(sv.data(), sv.size());
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
    static auto serialize(const char* str, S& sink) -> std::optional<typename S::Error> {
        return sink.out(str, std::strlen(str));
    }
    static auto size(const char* str) -> emt_size_t { return std::strlen(str); }
};

static_assert(Traceable<const char*>);

namespace internal {

template <typename S>
auto serialize_varint(S& sink_impl, emt_size_t len) -> std::optional<typename S::Error> {
    unsigned char bytes[((sizeof(emt_size_t) * 8) + 6) / 7];

    emt_size_t remaining = len;
    for (size_t i = 0; i < sizeof(bytes); i++) {
        bytes[i] = static_cast<unsigned char>(remaining) & 0x7F;

        remaining >>= 7;
        if (remaining == 0) {
            return sink_impl.out(bytes, i + 1);
        }
        bytes[i] |= 0x80;
    }
    assert(false);
}

/// Emits `val` the way a value of type `T` appears in the runtime stream: length-prefixed types
/// are preceded by their element count, null-terminated types are followed by their terminator.
/// Used for top-level arguments as well as for the elements of containers.
template <typename T, typename S>
auto emit_value(S& sink_impl, const T& val) -> std::optional<typename S::Error> {
    if constexpr (std::holds_alternative<LengthPrefixed>(Trace<T>::size_type)) {
        auto error = serialize_varint(sink_impl, Trace<T>::size(val));
        if (error.has_value()) {
            return error;
        }
    }
    auto error = Trace<T>::serialize(val, sink_impl);
    if (error.has_value()) {
        return error;
    }

    if constexpr (std::holds_alternative<NullTerminated>(Trace<T>::size_type)) {
        std::array<unsigned char, std::get<NullTerminated>(Trace<T>::size_type).terminator_length>
            null_bytes{};
        return sink_impl.out(null_bytes.data(), sizeof(null_bytes));
    }
    return std::nullopt;
}

} // namespace internal

template <typename T>
    requires(Traceable<T>)
struct Trace<std::vector<T>> {
    using Children = Ts<Child<T>>;
    static constexpr StringLiteral type_id = "list";
    static constexpr SizeType size_type = LengthPrefixed{};
    static constexpr bool simple = false;
    template <typename S>
    static auto serialize(const std::vector<T>& vec, S& sink_impl)
        -> std::optional<typename S::Error> {
        return Trace<std::span<T>>::serialize(std::span<const T>{vec}, sink_impl);
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
    static auto serialize(const T* data, S& sink_impl) -> std::optional<typename S::Error> {
        if constexpr (Trace<T>::simple) {
            return sink_impl.out(data, sizeof(T) * N);
        }

        for (size_t i = 0; i < N; i++) {
            auto error = internal::emit_value<T>(sink_impl, data[i]);
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
    static auto serialize(const std::array<T, N>& arr, S& sink_impl)
        -> std::optional<typename S::Error> {
        if constexpr (Trace<T>::simple) {
            return sink_impl.out(arr.data(), sizeof(T) * N);
        }

        for (const T& el : arr) {
            auto error = internal::emit_value<T>(sink_impl, el);
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
    static constexpr bool simple = false;
    template <typename S>
    static auto serialize(const std::span<const T>& slice, S& sink_impl)
        -> std::optional<typename S::Error> {
        if constexpr (Trace<T>::simple) {
            return sink_impl.out(slice.data(), sizeof(T) * slice.size());
        }

        for (const T& el : slice) {
            auto error = internal::emit_value<T>(sink_impl, el);
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
        std::ranges::sized_range<const R> &&
        Traceable<std::remove_cvref_t<std::ranges::range_reference_t<const R>>> &&
        std::ranges::contiguous_range<const R> &&
        Trace<std::remove_cvref_t<std::ranges::range_reference_t<const R>>>::simple
    )
struct Trace<R> {
    using T = std::remove_cvref_t<std::ranges::range_reference_t<const R>>;

    using Children = Ts<Child<T>>;
    static constexpr StringLiteral type_id = "list";
    static constexpr SizeType size_type = LengthPrefixed{};
    static constexpr bool simple = false;
    template <typename S>
    static auto serialize(const R& range, S& sink_impl) -> std::optional<typename S::Error> {
        return sink_impl.out(std::ranges::data(range), sizeof(T) * std::ranges::size(range));
    }
    static auto size(const R& range) -> emt_size_t { return std::ranges::size(range); }
};

template <typename R>
    requires(
        std::ranges::sized_range<const R> &&
        Traceable<std::remove_cvref_t<std::ranges::range_reference_t<const R>>>
    )
struct Trace<R> {
    using T = std::remove_cvref_t<std::ranges::range_reference_t<const R>>;

    using Children = Ts<Child<T>>;
    static constexpr StringLiteral type_id = "list";
    static constexpr SizeType size_type = LengthPrefixed{};
    static constexpr bool simple = false;
    template <typename S>
    static auto serialize(const R& range, S& sink_impl) -> std::optional<typename S::Error> {
        for (const T& el : range) {
            auto error = internal::emit_value<T>(sink_impl, el);
            if (error.has_value()) {
                return error;
            }
        }
        return std::nullopt;
    }
    static auto size(const R& range) -> emt_size_t { return std::ranges::size(range); }
};

namespace internal {

template <typename S, typename T>
    requires(IsOpt<T>)
auto emit_arg(S&, const T&) -> std::optional<typename S::Error> {
    return std::nullopt;
}

template <typename S, typename T>
    requires(Traceable<T>)
auto emit_arg(S& sink_impl, const T& arg) -> std::optional<typename S::Error> {
    return emit_value<T>(sink_impl, arg);
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
    return 7 + (get_num_non_opt_args<Args...>() * 5) + (get_num_descendants<Args...>() * 6);
}

static_assert(get_type_layout_len<int>() == 12);
static_assert(get_type_layout_len<int, int>() == 17);
static_assert(get_type_layout_len<std::vector<int>>() == 18);
static_assert(get_type_layout_len<int, OptThrow<true>>() == 12);
static_assert(get_type_layout_len<int, OptThrow<true>, std::vector<int>>() == 23);

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
        arr[i + 3] = 0;
        if constexpr (IsChild<T>) {
            arr[i + 4] = get_num_children<typename T::Type>();
        } else {
            arr[i + 4] = get_num_children<T>();
        }

        index += sizeof(Trce::type_id.value);
        i += 5;
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
consteval auto make_format_info(emt_size_t formatter_id, emt_size_t source_line)
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
            number += static_cast<emt_size_t>(fmt.value[i] - '0');
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
    requires(IsSink<S> && ((IsOpt<Args> || Traceable<Args>) && ...))
[[nodiscard]] auto emit_trace(const void* record, S&& sink, const Args&... args)
    -> std::conditional_t<
        throw_on_error || std::same_as<SinkError<S>, Infallible>,
        void,
        std::optional<SinkError<S>>> {
    Sink<S> sink_impl{std::forward<S>(sink), record, 0}; // todo fix total_size
    using Error = SinkError<S>;

    // Each step is only performed if all the previous ones succeeded.
    std::optional<Error> error = sink_impl.begin();

    if (!error.has_value()) {
        error = Sink<S>::out_ptr(sink_impl, record);
    }

    if (!error.has_value()) {
        (void) ((error = emit_arg(sink_impl, args), error.has_value()) || ...);
    }

    if (!error.has_value()) {
        error = std::move(sink_impl).finish();
    }

    if constexpr (!std::same_as<Error, Infallible>) {
        if constexpr (throw_on_error) {
            if (error.has_value()) {
                throw TraceException<Error>{std::move(error.value())};
            }
        } else {
            return error;
        }
    }
}

} // namespace internal

template <>
struct Sink<std::vector<unsigned char>&> {
    std::vector<unsigned char>& buffer;
    using Error = Infallible;
    static constexpr unsigned char encoding_id = EMT_ENCODING_NONE;
    static constexpr emt_size_t ptr_width = default_ptr_width;
    static constexpr unsigned char alignment_power = EMT_DEFAULT_ALIGNMENT_POWER;

    Sink(std::vector<unsigned char>& vec, const void*, emt_size_t) : buffer(vec) {}

    [[nodiscard]]
    static auto begin() -> std::optional<Error> {
        return std::nullopt;
    }

    template <typename SI>
    [[nodiscard]] static auto out_ptr(SI&& sink_impl, const void* record)
        -> std::optional<typename std::remove_reference_t<SI>::Error> {
        emt_ptr_t shifted = reinterpret_cast<emt_ptr_t>(record) >> alignment_power;

        std::array<unsigned char, ptr_width> bytes{};
        for (emt_size_t i = 0; i < ptr_width; i++) {
            bytes[i] = static_cast<unsigned char>(shifted >> (i * 8));
        }

        return std::forward<SI>(sink_impl).out(bytes.data(), bytes.size());
    }

    [[nodiscard]]
    auto out(const void* data, emt_size_t size) -> std::optional<Error> {
        if (size == 0) {
            // data may be null (e.g. for an empty vector), which memcpy doesn't accept
            return std::nullopt;
        }
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

static_assert(IsSink<std::vector<unsigned char>&>);
static_assert(IsSink<ChangePtr<1, 1, std::vector<unsigned char>>&>);
static_assert(IsSink<Cobs<std::vector<unsigned char>>&>);

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

template <emt_size_t alignment>
    requires((alignment & (alignment - 1)) == 0)
static constexpr internal::OptAlignment<alignment> opt_align = internal::OptAlignment<alignment>{};

template <typename S>
constexpr auto opt_sink(S&& t) -> auto {
    using SinkRef = std::conditional_t<std::is_reference_v<S>, S, S&&>;
    return internal::OptSink<SinkRef>{std::forward<S>(t)};
}
static_assert(1 == get_sink(internal::OptSink{1}));
static_assert(2 == get_sink(internal::OptSink{1}, internal::OptSink{2}));
static_assert(std::same_as<decltype(internal::get_sink()), DefaultSink>);
static_assert(std::same_as<decltype(internal::get_sink(opt_sink(1))), int&&>);
static_assert(std::same_as<decltype(internal::get_sink(1, 2, opt_sink(1), 2)), int&&>);
static_assert(
    std::same_as<decltype(internal::get_sink(opt_sink(1), opt_sink(1U))), unsigned int&&>
);

template <typename S, S (*get_sink)(void) = nullptr>
    requires(IsSink<S> || IsSink<S&>)
struct GlobalSink {};

template <typename S, S (*get_sink)(void)>
    requires(get_sink != nullptr && IsSink<S>)
struct GlobalSink<S, get_sink> {
    using SinkImpl = Sink<S&>;

    // Mirrors the value specialization's sink storage; holds the factory here.
    constinit static S (*inner_sink)();

    static auto begin(const void* record, emt_size_t total_size, void*) -> void {
        SinkImpl& sink_impl = *new (sink_impl_slot.data()) SinkImpl{get_sink(), record, total_size};

        auto error = sink_impl.begin();
        if constexpr (!std::same_as<SinkError<S&>, Infallible>) {
            if (error.has_value()) {
                sink_impl.~SinkImpl();
                std::abort();
            }
        }
    }

    static auto out_ptr(const void* record, void (*)(const void*, emt_size_t, void*), void*)
        -> void {
        SinkImpl& sink_impl = *reinterpret_cast<SinkImpl*>(sink_impl_slot.data());

        auto error = SinkImpl::out_ptr(sink_impl, record);
        if constexpr (!std::same_as<SinkError<S&>, Infallible>) {
            if (error.has_value()) {
                sink_impl.~SinkImpl();
                std::abort();
            }
        }
    }

    static auto out(const void* data, emt_size_t size, void*) -> void {
        SinkImpl& sink_impl = *reinterpret_cast<SinkImpl*>(sink_impl_slot.data());

        auto error = sink_impl.out(data, size);
        if constexpr (!std::same_as<SinkError<S&>, Infallible>) {
            if (error.has_value()) {
                sink_impl.~SinkImpl();
                std::abort();
            }
        }
    }

    static auto finish(const void*, emt_size_t, void*) -> void {
        SinkImpl& sink_impl = *reinterpret_cast<SinkImpl*>(sink_impl_slot.data());

        auto error = std::move(sink_impl).finish();
        sink_impl.~SinkImpl(); // NOLINT(bugprone-use-after-move)
        if constexpr (!std::same_as<SinkError<S&>, Infallible>) {
            if (error.has_value()) {
                std::abort();
            }
        }
    }

private:
    alignas(
        SinkImpl
    ) static thread_local constinit std::array<unsigned char, sizeof(SinkImpl)> sink_impl_slot;
};

template <typename S, S (*get_sink)(void)>
    requires(IsSink<S&> && get_sink == nullptr)
struct GlobalSink<S, get_sink> {
    constinit static S inner_sink;
    using SinkImpl = Sink<S&>;

    static auto begin(const void* record, emt_size_t total_size, void*) -> void {
        SinkImpl& sink_impl = *new (sink_impl_slot.data()) SinkImpl{inner_sink, record, total_size};

        auto error = sink_impl.begin();
        if constexpr (!std::same_as<SinkError<S&>, Infallible>) {
            if (error.has_value()) {
                sink_impl.~SinkImpl();
                std::abort();
            }
        }
    }

    static auto out_ptr(const void* record, void (*)(const void*, emt_size_t, void*), void*)
        -> void {
        SinkImpl& sink_impl = *reinterpret_cast<SinkImpl*>(sink_impl_slot.data());

        auto error = SinkImpl::out_ptr(sink_impl, record);
        if constexpr (!std::same_as<SinkError<S&>, Infallible>) {
            if (error.has_value()) {
                sink_impl.~SinkImpl();
                std::abort();
            }
        }
    }

    static auto out(const void* data, emt_size_t size, void*) -> void {
        SinkImpl& sink_impl = *reinterpret_cast<SinkImpl*>(sink_impl_slot.data());

        auto error = sink_impl.out(data, size);
        if constexpr (!std::same_as<SinkError<S&>, Infallible>) {
            if (error.has_value()) {
                sink_impl.~SinkImpl();
                std::abort();
            }
        }
    }

    static auto finish(const void*, emt_size_t, void*) -> void {
        SinkImpl& sink_impl = *reinterpret_cast<SinkImpl*>(sink_impl_slot.data());

        auto error = std::move(sink_impl).finish();
        sink_impl.~SinkImpl(); // NOLINT(bugprone-use-after-move)
        if constexpr (!std::same_as<SinkError<S&>, Infallible>) {
            if (error.has_value()) {
                std::abort();
            }
        }
    }

private:
    alignas(
        SinkImpl
    ) static thread_local constinit std::array<unsigned char, sizeof(SinkImpl)> sink_impl_slot;
};

struct MagicRecordConfig {
    unsigned char encoding = EMT_ENCODING_NONE;
    unsigned char ptr_width = static_cast<unsigned char>(Sink<StdoutSink&>::ptr_width);
    unsigned char alignment_power = Sink<StdoutSink&>::alignment_power;
};

template <typename S = StdoutSink>
    requires(IsSink<S&>)
struct MagicConfig {
    using SinkType = S;

    S sink;

    [[nodiscard]]
    static constexpr auto get_factory() -> S (*)() {
        return nullptr;
    }
};

template <typename S>
    requires(IsSink<S>)
struct MagicConfigFactory {
    using SinkType = S;

    S (*sink)();

    [[nodiscard]]
    constexpr auto get_factory() const -> S (*)() {
        return sink;
    }
};

template <typename S>
    requires(IsSink<std::remove_cvref_t<S>&>)
constexpr auto make_magic_config(S&& sink) -> MagicConfig<std::remove_cvref_t<S>> {
    return {std::forward<S>(sink)};
}

template <typename S>
    requires(IsSink<S>)
constexpr auto make_magic_config(S (*sink)()) -> MagicConfigFactory<S> {
    return {sink};
}

template <typename F>
    requires(
        std::is_class_v<std::remove_cvref_t<F>> && !IsSink<std::remove_cvref_t<F>&> &&
        requires(F& fn) {
            { +fn } -> std::convertible_to<decltype(fn()) (*)()>;
        }
    )
constexpr auto make_magic_config(F&& fn) -> MagicConfigFactory<decltype(std::declval<F&>()())> {
    return {+std::forward<F>(fn)};
}

constexpr auto make_magic_config() -> MagicConfig<> { return {}; }

/// Builds the MGIC record. The config values are promises about how the trace
/// stream is encoded and how record pointers are serialized; creating the record
/// does not enforce them.
[[nodiscard]] constexpr auto make_magic_record(MagicRecordConfig config = {}) noexcept
    -> emt_magic_t {
    static_assert(sizeof(emt_magic_t) <= 0xFF);
    static_assert(offsetof(emt_magic_t, record_size) <= 0xFF);
    static_assert(offsetof(emt_magic_t, meta) <= 0xFF);
    static_assert(offsetof(emt_magic_t, endianness_probe) <= 0xFF);

    emt_magic_t record{};

    record.framing[0] = 'E';
    record.framing[1] = 'M';
    record.framing[2] = 'T';
    record.framing[3] = 10;
    record.framing[4] = static_cast<unsigned char>(offsetof(emt_magic_t, record_size));
    record.framing[5] = static_cast<unsigned char>(offsetof(emt_magic_t, meta));
    record.framing[6] = static_cast<unsigned char>(offsetof(emt_magic_t, endianness_probe));
    record.framing[7] = static_cast<unsigned char>(sizeof(emt_size_t));
    record.framing[8] = EMT_MAGIC_VERSION_MAJOR;
    record.framing[9] = EMT_MAGIC_VERSION_MINOR;
    record.framing[10] = 'M';
    record.framing[11] = 'G';
    record.framing[12] = 'I';
    record.framing[13] = 'C';

    record.record_size = sizeof(emt_magic_t);
    record.endianness_probe = EMT_ENDIANNESS_PROBE;

    constexpr std::array<unsigned char, 32> magic = {
        0xd1, 0x97, 0xf5, 0x22, 0xd9, 0x26, 0x9f, 0xd1, 0xad, 0x70, 0x33,
        0x92, 0xf6, 0x59, 0xdf, 0xd0, 0xfb, 0xec, 0xbd, 0x60, 0x97, 0x13,
        0x25, 0xe8, 0x92, 0x01, 0xb2, 0x5a, 0x38, 0x5d, 0x9e, 0xc7,
    };

    for (std::size_t i = 0; i < magic.size(); i++) {
        record.meta[i] = magic[i];
    }
    record.meta[32] = config.ptr_width;
    record.meta[33] = config.alignment_power;
    record.meta[34] = config.encoding;

    return record;
}

template <typename S>
    requires(IsSink<S&>) && requires {
        std::integral_constant<emt_size_t, Sink<S&>::ptr_width>{};
        std::integral_constant<unsigned char, Sink<S&>::alignment_power>{};
        std::integral_constant<unsigned char, Sink<S&>::encoding_id>{};
    }
[[nodiscard]] consteval auto make_magic_record() noexcept -> emt_magic_t {
    return make_magic_record(
        MagicRecordConfig{
            .encoding = Sink<S&>::encoding_id,
            .ptr_width = Sink<S&>::ptr_width,
            .alignment_power = Sink<S&>::alignment_power,
        }
    );
}

static_assert(make_magic_record().record_size == sizeof(emt_magic_t));
static_assert(make_magic_record().meta[32] == default_ptr_width);
static_assert(make_magic_record().meta[33] == EMT_DEFAULT_ALIGNMENT_POWER);
static_assert(make_magic_record().meta[34] == EMT_ENCODING_NONE);

/// Emits the serialized pointer to the MGIC record, which anchors the runtime
/// stream to the section. Requires the default pipeline symbols supplied by
/// `EMT_CPP_GLOBAL_SINK` (or a compatible
/// host).
inline void init() {
#ifdef _WIN32
    // Prevent byte `0xa` from being replaced with the sequence `0xd` `0xa`.
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    Sink<DefaultSink> sink{DefaultSink{}, &::g_emt_default_magic, sizeof(emt_magic_t)};
    (void) sink.begin();
    (void) emt::Sink<emt::DefaultSink>::out_ptr(sink, &::g_emt_default_magic);
    (void) sink.finish();
}

#define EMT_CPP_TRACE_IMPL(fmt_info_attributes, postfix, fmt, ...)                                 \
    []<typename... Args>(const Args&... args) -> auto {                                            \
        using ::emt::StringLiteral;                                                                \
        constexpr ::emt_size_t formatter_id = ::emt::internal::get_formatter<Args...>();           \
        constexpr auto full_fmt_string = StringLiteral{fmt} + StringLiteral{postfix};              \
        static_assert(((::emt::internal::IsOpt<Args> || ::emt::Traceable<Args>) && ...));          \
        static_assert(::emt::internal::format_string_ok<full_fmt_string, Args...>(formatter_id));  \
        alignas(::emt::internal::get_alignment<Args...>())                                         \
            fmt_info_attributes static constexpr auto fmt_info =                                   \
                ::emt::internal::make_format_info<full_fmt_string, __FILE__, Args...>(             \
                    formatter_id, __LINE__                                                         \
                );                                                                                 \
        return ::emt::internal::emit_trace<::emt::internal::get_throw<Args...>()>(                 \
            &fmt_info, ::emt::internal::get_sink(args...), args...                                 \
        );                                                                                         \
    }(__VA_ARGS__)

#define EMT_CPP_TRACE(fmt_info_attributes, ...)                                                    \
    EMT_CPP_TRACE_IMPL(fmt_info_attributes, "", __VA_ARGS__)
#define EMT_CPP_TRACELN(fmt_info_attributes, ...)                                                  \
    EMT_CPP_TRACE_IMPL(fmt_info_attributes, "\n", __VA_ARGS__)

#ifdef EMT_DEFAULT_ATTR
#define MTRACE(...) EMT_CPP_TRACE(EMT_DEFAULT_SEC_ATTR, __VA_ARGS__)
#define MTRACELN(...) EMT_CPP_TRACELN(EMT_DEFAULT_SEC_ATTR, __VA_ARGS__)
#endif

#define EMT_CPP_GLOBAL_SINK(...)                                                                                           \
    namespace emt::impl {                                                                                                  \
    constexpr auto cfg = ::emt::make_magic_config(__VA_ARGS__);                                                            \
    using CfgType = decltype(cfg);                                                                                         \
    using SinkType = CfgType::SinkType;                                                                                    \
    using GlobalSinkType = ::emt::GlobalSink<SinkType, cfg.get_factory()>;                                                 \
    using SinkImplType = GlobalSinkType::SinkImpl;                                                                         \
    };                                                                                                                     \
    template <> /*NOLINTNEXTLINE(clang-diagnostic-unneeded-internal-declaration,bugprone-throwing-static-initialization)*/ \
    decltype(::emt::impl::GlobalSinkType::inner_sink)(::emt::impl::GlobalSinkType::inner_sink){                            \
        ::emt::impl::cfg.sink                                                                                              \
    };                                                                                                                     \
    template <>                                                                                                            \
    [[maybe_unused]] std::array<unsigned char, sizeof(::emt::impl::SinkImplType)> thread_local ::                          \
        emt::impl::GlobalSinkType::sink_impl_slot{};                                                                       \
                                                                                                                           \
    auto emt_default_begin(const void* info, emt_size_t total_size, void* extra_arg) -> void {                             \
        ::emt::impl::GlobalSinkType::begin(info, total_size, extra_arg);                                                   \
    }                                                                                                                      \
                                                                                                                           \
    auto emt_default_serialize_pointer(                                                                                    \
        const void* ptr, void (*out)(const void*, emt_size_t, void*), void* extra_arg                                      \
    ) -> void {                                                                                                            \
        ::emt::impl::GlobalSinkType::out_ptr(ptr, out, extra_arg);                                                         \
    }                                                                                                                      \
                                                                                                                           \
    auto emt_default_out(const void* data, emt_size_t size, void* extra_arg) -> void {                                     \
        ::emt::impl::GlobalSinkType::out(data, size, extra_arg);                                                           \
    }                                                                                                                      \
                                                                                                                           \
    auto emt_default_finish(const void* info, emt_size_t total_size, void* extra_arg) -> void {                            \
        ::emt::impl::GlobalSinkType::finish(info, total_size, extra_arg);                                                  \
    }

// NOLINTBEGIN(clang-diagnostic-missing-designated-field-initializers)
#define EMT_CPP_MAGIC(attrs, ...)                                                                  \
    EMT_CPP_GLOBAL_SINK(__VA_ARGS__)                                                               \
    alignas(1 << ::emt::impl::SinkImplType::alignment_power)                                       \
        attrs emt_magic_t g_emt_default_magic = ::emt::make_magic_record<::emt::impl::SinkType>()
// NOLINTEND(clang-diagnostic-missing-designated-field-initializers)

#define MTRACE_MAGIC(...) EMT_CPP_MAGIC(EMT_DEFAULT_SEC_ATTR, __VA_ARGS__)

} // namespace emt
// NOLINTEND(readability-named-parameter)
