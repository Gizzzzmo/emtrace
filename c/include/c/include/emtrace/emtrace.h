// DO NOT MODIFY DIRECTLY! Header is auto-generated from c/header-generator/emtrace_template.h using
// this python script: c/header-generator/build_macro.py Script arguments were: --max-args=32
#ifndef EMTRACE_EMTRACE_H
#define EMTRACE_EMTRACE_H
// NOLINTBEGIN(modernize-avoid-c-arrays)

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

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

#ifdef EMT_PTR_T
typedef EMT_PTR_T emt_ptr_t;
#else
typedef uint32_t emt_ptr_t;
#endif

#ifdef EMT_SIZE_T
typedef EMT_SIZE_T emt_size_t;
#else
typedef uint32_t emt_size_t;
#endif

#ifndef EMT_ALIGNMENT_POWER
#define EMT_ALIGNMENT_POWER 0
#endif

// from C23 and C++11 onwards we can use enum class with fixed underlying types instead of macros
#if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L) ||                                  \
    (defined(__cplusplus) && __cplusplus >= 201103L)
enum : emt_size_t {
    // In the format info signals whether a piece of dynamically sized data is null-terminated or
    // length-prefixed.
    EMT_NULL_TERMINATED = ///< associated bytes are variable in length and null-terminated
    (((emt_size_t) 1) << (8 * sizeof(emt_size_t) - 1)),
    EMT_LENGTH_PREFIXED = ///< associates bytes are variable in length prefixed by how many there
                          ///< will be
    (((emt_size_t) 1) << (8 * sizeof(emt_size_t) - 2)),

    // In the format info signals what formatter to use.
    EMT_PY_FORMAT = 0, ///< Use python's str.format function for formatting.
    EMT_NO_FORMAT = ///< Do not use any formatter; print the string as-is. All additional arguments
                    ///< are discarded.
    1,
    EMT_C_STYLE_FORMAT = 2, ///< Use python's C-style formatter

    EMT_ALIGNMENT = 1 << (EMT_ALIGNMENT_POWER),

    // In the magic header, signals what encoding is used (if any).
    // This is just a signal for the parser to know how to decode the bytestream.
    // You need to make sure that your output function actually encodes the data accordingly.
    EMT_ENCODING_NONE = 0,
    EMT_ENCODING_COBS = 1,
    EMT_ENCODING_COBS_PASSTHROUGH = 2,
};
#else

/// associated bytes are variable in length and null-terminated
#define EMT_NULL_TERMINATED (((emt_size_t) 1) << (8 * sizeof(emt_size_t) - 1))
/// associates bytes are variable in length prefixed by how many there will be
#define EMT_LENGTH_PREFIXED (((emt_size_t) 1) << (8 * sizeof(emt_size_t) - 2))

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

#define EMT_ALIGNMENT (1 << (EMT_ALIGNMENT_POWER))
#endif

typedef struct {
    uint8_t main[36]; ///< first 32 bytes are emtrace's magic constant.
                      ///< next byte contains the offset from start of member main to start of
                      ///< member info. final four bytes contain sizeof(emt_size_t), sizeof(void*),
                      ///< and the power of two to which all format info is aligned respectively
    emt_size_t info[4];
    // emt_size_t byteorder_id;
    // emt_size_t null_terminated;
    // emt_size_t length_prefixed;
    // emt_size_t no_format;
    // emt_size_t encoding;
} emt_magic_t;

static inline void emt_out_file(const void* data, emt_size_t size, FILE* file) {
    fwrite(data, 1, size, file);
}

static inline void emt_out_file_wrapper(const void* data, emt_size_t size, void* extra_arg) {
    FILE* file = (FILE*) extra_arg;
    fwrite(data, 1, size, file);
}

typedef struct {
    uint8_t buffer[254];
    uint8_t pos;
} emt_cobs_state_t;

static inline void emt_cobs_init(emt_cobs_state_t* cobs_state) { cobs_state->pos = 0; }

static inline void emt_cobs_encode(
    const void* data,
    size_t size,
    emt_cobs_state_t* cobs_state,
    void (*out_fn)(const void*, emt_size_t, void*),
    void* extra_arg
) {
    for (size_t i = 0; i < size; i++) {
        uint8_t byte = ((const uint8_t*) data)[i];
        if (byte == 0) {
            uint8_t ptr = cobs_state->pos + 1;
            out_fn(&ptr, 1, extra_arg);
            out_fn(cobs_state->buffer, cobs_state->pos, extra_arg);
            cobs_state->pos = 0;
            continue;
        }
        if (cobs_state->pos == 254) {
            uint8_t ptr = 0xff;
            out_fn(&ptr, 1, extra_arg);
            out_fn(cobs_state->buffer, 254, extra_arg);
            cobs_state->pos = 0;
        }
        cobs_state->buffer[cobs_state->pos] = byte;
        cobs_state->pos++;
    }
}

static inline void emt_cobs_finalize(
    emt_cobs_state_t* cobs_state, void (*out_fn)(const void*, emt_size_t, void*), void* extra_arg
) {
    if (cobs_state->pos > 0) {
        uint8_t ptr = cobs_state->pos + 1;
        out_fn(&ptr, 1, extra_arg);
        out_fn(cobs_state->buffer, cobs_state->pos, extra_arg);
    } else {
        uint8_t ptr = 1;
        out_fn(&ptr, 1, extra_arg);
    }
    uint8_t zero = 0;
    out_fn(&zero, 1, extra_arg);
}

#ifdef __cplusplus
}
#endif

#define EMT_NTH_ARG(                                                                               \
    a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, x, y, z, aa, bb, cc, dd, ee, \
    ff, gg, hh, ii, jj, kk, ll, mm, nn, oo, pp, qq, rr, ss, tt, uu, vv, ww, xx, yy, zz, aaa, bbb,  \
    ccc, ddd, eee, fff, ggg, hhh, iii, jjj, kkk, lll, mmm, nnn, ooo, ppp, qqq, rrr, sss, ttt, uuu, \
    vvv, www, xxx, yyy, zzz, aaaa, bbbb, cccc, dddd, eeee, ffff, gggg, hhhh, iiii, jjjj, kkkk,     \
    llll, mmmm, nnnn, oooo, pppp, qqqq, rrrr, ssss, tttt, uuuu, vvvv, wwww, xxxx, yyyy, zzzz,      \
    aaaaa, bbbbb, ccccc, ddddd, eeeee, fffff, ggggg, hhhhh, iiiii, jjjjj, kkkkk, lllll, mmmmm,     \
    nnnnn, ooooo, ppppp, qqqqq, rrrrr, sssss, ttttt, uuuuu, vvvvv, wwwww, xxxxx, yyyyy, zzzzz,     \
    aaaaaa, bbbbbb, cccccc, dddddd, eeeeee, ffffff, gggggg, hhhhhh, iiiiii, jjjjjj, kkkkkk,        \
    llllll, mmmmmm, nnnnnn, oooooo, pppppp, qqqqqq, rrrrrr, ssssss, tttttt, uuuuuu, vvvvvv,        \
    wwwwww, xxxxxx, yyyyyy, zzzzzz, aaaaaaa, bbbbbbb, ccccccc, ddddddd, eeeeeee, fffffff, ggggggg, \
    hhhhhhh, iiiiiii, jjjjjjj, kkkkkkk, lllllll, mmmmmmm, nnnnnnn, ooooooo, ppppppp, qqqqqqq,      \
    rrrrrrr, sssssss, ttttttt, uuuuuuu, vvvvvvv, wwwwwww, xxxxxxx, yyyyyyy, zzzzzzz, aaaaaaaa,     \
    bbbbbbbb, cccccccc, dddddddd, eeeeeeee, ffffffff, gggggggg, hhhhhhhh, iiiiiiii, jjjjjjjj,      \
    kkkkkkkk, llllllll, mmmmmmmm, nnnnnnnn, oooooooo, pppppppp, qqqqqqqq, ...                      \
)                                                                                                  \
    qqqqqqqq

#define EMT_NUM_ARGS_REST(...)                                                                     \
    EMT_NTH_ARG(                                                                                   \
        __VA_ARGS__, 196, 195, 194, 193, 192, 191, 190, 189, 188, 187, 186, 185, 184, 183, 182,    \
        181, 180, 179, 178, 177, 176, 175, 174, 173, 172, 171, 170, 169, 168, 167, 166, 165, 164,  \
        163, 162, 161, 160, 159, 158, 157, 156, 155, 154, 153, 152, 151, 150, 149, 148, 147, 146,  \
        145, 144, 143, 142, 141, 140, 139, 138, 137, 136, 135, 134, 133, 132, 131, 130, 129, 128,  \
        127, 126, 125, 124, 123, 122, 121, 120, 119, 118, 117, 116, 115, 114, 113, 112, 111, 110,  \
        109, 108, 107, 106, 105, 104, 103, 102, 101, 100, 99, 98, 97, 96, 95, 94, 93, 92, 91, 90,  \
        89, 88, 87, 86, 85, 84, 83, 82, 81, 80, 79, 78, 77, 76, 75, 74, 73, 72, 71, 70, 69, 68,    \
        67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46,    \
        45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24,    \
        23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 0    \
    )

#define EMT_FIRST_ARG(a, ...) a
#define EMT_REST_ARGS(a, ...) __VA_ARGS__

#define EMT_VAL(type, x) type, x, sizeof(type), EMT_TAG_VAL
#define EMT_ARR(type, x) type, x, sizeof(x) / sizeof(type), EMT_TAG_ARR
#define EMT_STS(type, x, len) type, x, len, EMT_TAG_STS
#define EMT_SLC(type, x, len) type, x, len, EMT_TAG_SLC
#define EMT_STR(x) const char*, x, strlen(x) + 1, EMT_TAG_STR

#ifndef EMT_NO_SIMPLE_MACROS
// NOLINTBEGIN(readability-identifier-naming)
#define VAL EMT_VAL
#define ARR EMT_ARR
#define STS EMT_STS
#define SLC EMT_SLC
#define STR EMT_STR
// NOLINTEND(readability-identifier-naming)
#endif

#define EMT_TRACE_F(fmt_info_attributes, formatter, out_fn, lock, unlock, extra_arg, postfix, ...) \
    do {                                                                                           \
                                                                                                   \
        struct emt_info_unlikely_to_shadow_t {                                                     \
            emt_size_t layout                                                                      \
                [5 + EMT_F_LAYOUT_SIZE(                                                            \
                         EMT_NUM_ARGS_REST(__VA_ARGS__), EMT_REST_ARGS(__VA_ARGS__, 0)             \
                     )];                                                                           \
            char fmt[sizeof(EMT_FIRST_ARG(__VA_ARGS__, 0) postfix)];                               \
            EMT_F_INFO_MEMBER(EMT_NUM_ARGS_REST(__VA_ARGS__), EMT_REST_ARGS(__VA_ARGS__, 0))       \
            char file[sizeof(__FILE__)];                                                           \
        };                                                                                         \
        fmt_info_attributes struct emt_info_unlikely_to_shadow_t emt_info_unlikely_to_shadow = {   \
            {EMT_NUM_ARGS_REST(__VA_ARGS__) / 4,                                                   \
             offsetof(struct emt_info_unlikely_to_shadow_t, fmt),                                  \
             EMT_F_LAYOUT(EMT_NUM_ARGS_REST(__VA_ARGS__), EMT_REST_ARGS(__VA_ARGS__, 0))           \
                 formatter,                                                                        \
             offsetof(struct emt_info_unlikely_to_shadow_t, file), __LINE__},                      \
            EMT_FIRST_ARG(__VA_ARGS__, 0) postfix,                                                 \
            EMT_F_INFO(EMT_NUM_ARGS_REST(__VA_ARGS__), EMT_REST_ARGS(__VA_ARGS__, 0)) __FILE__,    \
        };                                                                                         \
        emt_ptr_t emt_info_ptr_unlikely_to_shadow =                                                \
            (emt_ptr_t) ((uintptr_t) &emt_info_unlikely_to_shadow >> EMT_ALIGNMENT_POWER);         \
        lock(                                                                                      \
            (const void*) &emt_info_unlikely_to_shadow,                                            \
            EMT_F_TOTAL_SIZE(EMT_NUM_ARGS_REST(__VA_ARGS__), EMT_REST_ARGS(__VA_ARGS__, 0)),       \
            extra_arg                                                                              \
        );                                                                                         \
                                                                                                   \
        out_fn(&emt_info_ptr_unlikely_to_shadow, sizeof(emt_ptr_t), extra_arg);                    \
        EMT_F(out_fn, extra_arg, EMT_NUM_ARGS_REST(__VA_ARGS__), EMT_REST_ARGS(__VA_ARGS__, 0))    \
        unlock(                                                                                    \
            (const void*) &emt_info_unlikely_to_shadow,                                            \
            EMT_F_TOTAL_SIZE(EMT_NUM_ARGS_REST(__VA_ARGS__), EMT_REST_ARGS(__VA_ARGS__, 0)),       \
            extra_arg                                                                              \
        );                                                                                         \
    } while (0)

#define EMT_INIT(attrs, out, encoding, lock, unlock, extra_arg)                                    \
    do {                                                                                           \
        attrs emt_magic_t magic = {                                                                \
            {                                                                                      \
                0xd1,                                                                              \
                0x97,                                                                              \
                0xf5,                                                                              \
                0x22,                                                                              \
                0xd9,                                                                              \
                0x26,                                                                              \
                0x9f,                                                                              \
                0xd1,                                                                              \
                0xad,                                                                              \
                0x70,                                                                              \
                0x33,                                                                              \
                0x92,                                                                              \
                0xf6,                                                                              \
                0x59,                                                                              \
                0xdf,                                                                              \
                0xd0,                                                                              \
                0xfb,                                                                              \
                0xec,                                                                              \
                0xbd,                                                                              \
                0x60,                                                                              \
                0x97,                                                                              \
                0x13,                                                                              \
                0x25,                                                                              \
                0xe8,                                                                              \
                0x92,                                                                              \
                0x01,                                                                              \
                0xb2,                                                                              \
                0x5a,                                                                              \
                0x38,                                                                              \
                0x5d,                                                                              \
                0x9e,                                                                              \
                0xc7,                                                                              \
                offsetof(emt_magic_t, info) - offsetof(emt_magic_t, main),                         \
                sizeof(emt_size_t),                                                                \
                sizeof(emt_ptr_t),                                                                 \
                EMT_ALIGNMENT_POWER,                                                               \
            },                                                                                     \
            {                                                                                      \
                (emt_size_t) 0x0706050403020100,                                                   \
                EMT_NULL_TERMINATED,                                                               \
                EMT_LENGTH_PREFIXED,                                                               \
                encoding,                                                                          \
            }                                                                                      \
        };                                                                                         \
        lock((const void*) &magic, sizeof(magic), extra_arg);                                      \
        emt_ptr_t magic_ptr = (emt_ptr_t) ((uintptr_t) &magic >> EMT_ALIGNMENT_POWER);             \
        out((const void*) &magic_ptr, sizeof(magic_ptr), extra_arg);                               \
        unlock((const void*) &magic, sizeof(magic), extra_arg);                                    \
    } while (0)

#if defined(__GNUC__) || defined(__clang__)
#define EMT_DEFAULT_SEC_ATTR                                                                       \
    __attribute__((used, aligned(EMT_ALIGNMENT), section(".emtrace"))) static const
#elif defined(_MSC_VER)
#define EMT_DEFAULT_SEC_ATTR                                                                       \
    __declspec(align(EMT_ALIGNMENT)) __declspec(allocate(".emtrace")) static const
#endif

// for thread safety we want to lock stdout while writing a trace to it so that data from multiple
// traces cannot interleave
#if defined(unix) || defined(__unix) || defined(__unix__) ||                                       \
    (defined(__APPLE__) && defined(__MACH__))

#define EMT_FLOCK_FILE(x, y, file) flockfile(file)
#define EMT_FUNLOCK_FILE(x, y, file) funlockfile(file)

#elif defined(_WIN32)

#include <windows.h>
#define EMT_FLOCK_FILE(x, y, file) LockFileEx(file, 0, 0, 0xFFFFFFFF, 0xFFFFFFFF, NULL)
#define EMT_FUNLOCK_FILE(x, y, file) UnlockFileEx(file, 0, 0xFFFFFFFF, 0xFFFFFFFF, NULL)

#endif

#if defined(EMT_FLOCK_FILE) && defined(EMT_FUNLOCK_FILE)

#define EMT_LOCK_FILE_COBS(x, y, file)                                                             \
    emt_cobs_state_t state;                                                                        \
    emt_cobs_init(&state);                                                                         \
    EMT_FLOCK_FILE(x, y, file)

#define EMT_LOCK_FILE_COBS_PASSTHROUGH(x, y, file)                                                 \
    fwrite("", 1, 1, file);                                                                        \
    EMT_LOCK_FILE_COBS(x, y, file);

#define EMT_UNLOCK_FILE_COBS(x, y, file)                                                           \
    emt_cobs_finalize(&state, emt_out_file_wrapper, file);                                         \
    EMT_FUNLOCK_FILE(x, y, file)

#endif // EMT_FLOCK_FILE && EMT_FUNLOCK_FILE

#define EMT_OUT_FILE_COBS(ptr, size, file)                                                         \
    emt_cobs_encode((ptr), (size), &state, emt_out_file_wrapper, file)

#if defined(EMT_DEFAULT_SEC_ATTR) && defined(EMT_FLOCK_FILE) && defined(EMT_FUNLOCK_FILE)

#ifdef EMT_DEFAULT_ENCODE_COBS
#define EMT_LOCK EMT_LOCK_FILE_COBS
#define EMT_UNLOCK EMT_UNLOCK_FILE_COBS
#define EMT_OUT_FN EMT_OUT_FILE_COBS
#define EMT_ENCODING EMT_ENCODING_COBS
#else
#define EMT_LOCK EMT_FLOCK_FILE
#define EMT_UNLOCK EMT_FUNLOCK_FILE
#define EMT_OUT_FN emt_out_file
#define EMT_ENCODING EMT_ENCODING_NONE
#endif // EMT_DEFAULT_ENCODE_COBS

#define EMTRACE_F(...)                                                                             \
    EMT_TRACE_F(                                                                                   \
        EMT_DEFAULT_SEC_ATTR, EMT_PY_FORMAT, EMT_OUT_FN, EMT_LOCK, EMT_UNLOCK, stdout, "",         \
        __VA_ARGS__                                                                                \
    )
#define EMTRACE(str)                                                                               \
    EMT_TRACE_F(                                                                                   \
        EMT_DEFAULT_SEC_ATTR, EMT_NO_FORMAT, EMT_OUT_FN, EMT_LOCK, EMT_UNLOCK, stdout, "", str     \
    )

#define EMTRACELN_F(...)                                                                           \
    EMT_TRACE_F(                                                                                   \
        EMT_DEFAULT_SEC_ATTR, EMT_PY_FORMAT, EMT_OUT_FN, EMT_LOCK, EMT_UNLOCK, stdout, "\n",       \
        __VA_ARGS__                                                                                \
    )
#define EMTRACELN(str)                                                                             \
    EMT_TRACE_F(                                                                                   \
        EMT_DEFAULT_SEC_ATTR, EMT_NO_FORMAT, EMT_OUT_FN, EMT_LOCK, EMT_UNLOCK, stdout, "",         \
        str "\n"                                                                                   \
    )
#define EMTRACE_INIT()                                                                             \
    EMT_INIT(EMT_DEFAULT_SEC_ATTR, EMT_OUT_FN, EMT_ENCODING, EMT_LOCK, EMT_UNLOCK, stdout)

#endif // EMT_DEFAULT_SEC_ATTR && EMT_FLOCK_FILE && EMT_FUNLOCK_FILE
#define EMT_F_LAYOUT_SIZE_DISPATCH_EMT_TAG_VAL() 3

#define EMT_F_LAYOUT_SIZE_DISPATCH_EMT_TAG_STR() 3

#define EMT_F_LAYOUT_SIZE_DISPATCH_EMT_TAG_ARR() 7

#define EMT_F_LAYOUT_SIZE_DISPATCH_EMT_TAG_SLC() 7

#define EMT_F_LAYOUT_SIZE_DISPATCH_EMT_TAG_STS() 7

#define EMT_F_LAYOUT_SIZE_DISPATCH_HELPER(_tag) EMT_F_LAYOUT_SIZE_DISPATCH_##_tag()

#define EMT_F_LAYOUT_SIZE_DISPATCH(_tag) EMT_F_LAYOUT_SIZE_DISPATCH_HELPER(_tag)

#define EMT_F_LAYOUT_SIZE_0(_dummy) 0

#define EMT_F_LAYOUT_SIZE_4(type, x, len, tag, _dummy) EMT_F_LAYOUT_SIZE_DISPATCH(tag)

#define EMT_F_LAYOUT_SIZE_8(_0, _1, _2, _3, type, x, len, tag, _dummy)                             \
    EMT_F_LAYOUT_SIZE_4(_0, _1, _2, _3, 0) + EMT_F_LAYOUT_SIZE_DISPATCH(tag)

#define EMT_F_LAYOUT_SIZE_12(_0, _1, _2, _3, _4, _5, _6, _7, type, x, len, tag, _dummy)            \
    EMT_F_LAYOUT_SIZE_8(_0, _1, _2, _3, _4, _5, _6, _7, 0) + EMT_F_LAYOUT_SIZE_DISPATCH(tag)

#define EMT_F_LAYOUT_SIZE_16(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, type, x, len, tag, _dummy                      \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_12(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, 0) +                      \
        EMT_F_LAYOUT_SIZE_DISPATCH(tag)

#define EMT_F_LAYOUT_SIZE_20(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, type, x, len, tag, _dummy      \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_16(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, 0) +      \
        EMT_F_LAYOUT_SIZE_DISPATCH(tag)

#define EMT_F_LAYOUT_SIZE_24(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, type, x,   \
    len, tag, _dummy                                                                               \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_20(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, 0      \
    ) + EMT_F_LAYOUT_SIZE_DISPATCH(tag)

#define EMT_F_LAYOUT_SIZE_28(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, type, x, len, tag, _dummy                                                            \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_24(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, 0                                                                           \
    ) + EMT_F_LAYOUT_SIZE_DISPATCH(tag)

#define EMT_F_LAYOUT_SIZE_32(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, type, x, len, tag, _dummy                                        \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_28(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, 0                                                       \
    ) + EMT_F_LAYOUT_SIZE_DISPATCH(tag)

#define EMT_F_LAYOUT_SIZE_36(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, type, x, len, tag, _dummy                    \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_32(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, 0                                   \
    ) + EMT_F_LAYOUT_SIZE_DISPATCH(tag)

#define EMT_F_LAYOUT_SIZE_40(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, type, x, len, tag,       \
    _dummy                                                                                         \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_36(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, 0               \
    ) + EMT_F_LAYOUT_SIZE_DISPATCH(tag)

#define EMT_F_LAYOUT_SIZE_44(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27,      \
    type, x, len, tag, _dummy                                                                      \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_40(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, 0                                                                                     \
    ) + EMT_F_LAYOUT_SIZE_DISPATCH(tag)

#define EMT_F_LAYOUT_SIZE_48(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, type, x, len, tag, _dummy                                                       \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_44(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, 0                                                                 \
    ) + EMT_F_LAYOUT_SIZE_DISPATCH(tag)

#define EMT_F_LAYOUT_SIZE_52(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, type, x, len, tag, _dummy                                   \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_48(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, 0                                             \
    ) + EMT_F_LAYOUT_SIZE_DISPATCH(tag)

#define EMT_F_LAYOUT_SIZE_56(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, type, x, len, tag, _dummy               \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_52(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, 0                         \
    ) + EMT_F_LAYOUT_SIZE_DISPATCH(tag)

#define EMT_F_LAYOUT_SIZE_60(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, type, x, len, tag,  \
    _dummy                                                                                         \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_56(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, 0     \
    ) + EMT_F_LAYOUT_SIZE_DISPATCH(tag)

#define EMT_F_LAYOUT_SIZE_64(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    type, x, len, tag, _dummy                                                                      \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_60(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, 0                                                                           \
    ) + EMT_F_LAYOUT_SIZE_DISPATCH(tag)

#define EMT_F_LAYOUT_SIZE_68(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, type, x, len, tag, _dummy                                                  \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_64(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, 0                                                       \
    ) + EMT_F_LAYOUT_SIZE_DISPATCH(tag)

#define EMT_F_LAYOUT_SIZE_72(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, type, x, len, tag, _dummy                              \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_68(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, 0                                   \
    ) + EMT_F_LAYOUT_SIZE_DISPATCH(tag)

#define EMT_F_LAYOUT_SIZE_76(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, type, x, len, tag, _dummy          \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_72(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, 0               \
    ) + EMT_F_LAYOUT_SIZE_DISPATCH(tag)

#define EMT_F_LAYOUT_SIZE_80(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, type, x, len,  \
    tag, _dummy                                                                                    \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_76(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, 0                                                                                     \
    ) + EMT_F_LAYOUT_SIZE_DISPATCH(tag)

#define EMT_F_LAYOUT_SIZE_84(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, type, x, len, tag, _dummy                                                                 \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_80(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, 0                                                                 \
    ) + EMT_F_LAYOUT_SIZE_DISPATCH(tag)

#define EMT_F_LAYOUT_SIZE_88(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, type, x, len, tag, _dummy                                             \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_84(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, 0                                             \
    ) + EMT_F_LAYOUT_SIZE_DISPATCH(tag)

#define EMT_F_LAYOUT_SIZE_92(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, type, x, len, tag, _dummy                         \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_88(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, 0                         \
    ) + EMT_F_LAYOUT_SIZE_DISPATCH(tag)

#define EMT_F_LAYOUT_SIZE_96(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, type, x, len, tag, _dummy     \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_92(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, 0     \
    ) + EMT_F_LAYOUT_SIZE_DISPATCH(tag)

#define EMT_F_LAYOUT_SIZE_100(                                                                     \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, type, x,  \
    len, tag, _dummy                                                                               \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_96(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, 0                                                                           \
    ) + EMT_F_LAYOUT_SIZE_DISPATCH(tag)

#define EMT_F_LAYOUT_SIZE_104(                                                                     \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, type, x, len, tag, _dummy                                                            \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_100(                                                                         \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, 0                                                       \
    ) + EMT_F_LAYOUT_SIZE_DISPATCH(tag)

#define EMT_F_LAYOUT_SIZE_108(                                                                     \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, type, x, len, tag, _dummy                                        \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_104(                                                                         \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, 0                                   \
    ) + EMT_F_LAYOUT_SIZE_DISPATCH(tag)

#define EMT_F_LAYOUT_SIZE_112(                                                                     \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, type, x, len, tag, _dummy                    \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_108(                                                                         \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, 0               \
    ) + EMT_F_LAYOUT_SIZE_DISPATCH(tag)

#define EMT_F_LAYOUT_SIZE_116(                                                                     \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, type, x, len, tag,       \
    _dummy                                                                                         \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_112(                                                                         \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e,  \
        _6f, 0                                                                                     \
    ) + EMT_F_LAYOUT_SIZE_DISPATCH(tag)

#define EMT_F_LAYOUT_SIZE_120(                                                                     \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73,      \
    type, x, len, tag, _dummy                                                                      \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_116(                                                                         \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e,  \
        _6f, _70, _71, _72, _73, 0                                                                 \
    ) + EMT_F_LAYOUT_SIZE_DISPATCH(tag)

#define EMT_F_LAYOUT_SIZE_124(                                                                     \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, \
    _75, _76, _77, type, x, len, tag, _dummy                                                       \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_120(                                                                         \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e,  \
        _6f, _70, _71, _72, _73, _74, _75, _76, _77, 0                                             \
    ) + EMT_F_LAYOUT_SIZE_DISPATCH(tag)

#define EMT_F_LAYOUT_SIZE_128(                                                                     \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, \
    _75, _76, _77, _78, _79, _7a, _7b, type, x, len, tag, _dummy                                   \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_124(                                                                         \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e,  \
        _6f, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _7a, _7b, 0                         \
    ) + EMT_F_LAYOUT_SIZE_DISPATCH(tag)

#define EMT_F_LAYOUT_SIZE(x, ...) EMT_F_LAYOUT_SIZE_HELPER(x, __VA_ARGS__)
#define EMT_F_LAYOUT_SIZE_HELPER(x, ...) EMT_F_LAYOUT_SIZE_##x(__VA_ARGS__)
#define EMT_F_INFO_MEMBER_DISPATCH_EMT_TAG_VAL(type, name) char name[sizeof(#type)];

#define EMT_F_INFO_MEMBER_DISPATCH_EMT_TAG_STR(type, name) char name[sizeof("string")];

#define EMT_F_INFO_MEMBER_DISPATCH_EMT_TAG_ARR(type, name)                                         \
    char name[sizeof("list")];                                                                     \
    char name##_child_name[sizeof("")];                                                            \
    char name##_child_type_id[sizeof(#type)];

#define EMT_F_INFO_MEMBER_DISPATCH_EMT_TAG_SLC(type, name)                                         \
    char name[sizeof("list")];                                                                     \
    char name##_child_name[sizeof("")];                                                            \
    char name##_child_type_id[sizeof(#type)];

#define EMT_F_INFO_MEMBER_DISPATCH_EMT_TAG_STS(type, name)                                         \
    char name[sizeof("list")];                                                                     \
    char name##_child_name[sizeof("")];                                                            \
    char name##_child_type_id[sizeof(#type)];

#define EMT_F_INFO_MEMBER_DISPATCH_HELPER(type, name, _tag)                                        \
    EMT_F_INFO_MEMBER_DISPATCH_##_tag(type, name)

#define EMT_F_INFO_MEMBER_DISPATCH(type, name, _tag)                                               \
    EMT_F_INFO_MEMBER_DISPATCH_HELPER(type, name, _tag)

#define EMT_F_INFO_MEMBER_0(_dummy)

#define EMT_F_INFO_MEMBER_4(type, x, len, tag, _dummy) EMT_F_INFO_MEMBER_DISPATCH(type, type0, tag)

#define EMT_F_INFO_MEMBER_8(_0, _1, _2, _3, type, x, len, tag, _dummy)                             \
    EMT_F_INFO_MEMBER_4(_0, _1, _2, _3, 0) EMT_F_INFO_MEMBER_DISPATCH(type, type1, tag)

#define EMT_F_INFO_MEMBER_12(_0, _1, _2, _3, _4, _5, _6, _7, type, x, len, tag, _dummy)            \
    EMT_F_INFO_MEMBER_8(_0, _1, _2, _3, _4, _5, _6, _7, 0)                                         \
    EMT_F_INFO_MEMBER_DISPATCH(type, type2, tag)

#define EMT_F_INFO_MEMBER_16(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, type, x, len, tag, _dummy                      \
)                                                                                                  \
    EMT_F_INFO_MEMBER_12(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, 0)                        \
    EMT_F_INFO_MEMBER_DISPATCH(type, type3, tag)

#define EMT_F_INFO_MEMBER_20(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, type, x, len, tag, _dummy      \
)                                                                                                  \
    EMT_F_INFO_MEMBER_16(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, 0)        \
    EMT_F_INFO_MEMBER_DISPATCH(type, type4, tag)

#define EMT_F_INFO_MEMBER_24(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, type, x,   \
    len, tag, _dummy                                                                               \
)                                                                                                  \
    EMT_F_INFO_MEMBER_20(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, 0      \
    )                                                                                              \
    EMT_F_INFO_MEMBER_DISPATCH(type, type5, tag)

#define EMT_F_INFO_MEMBER_28(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, type, x, len, tag, _dummy                                                            \
)                                                                                                  \
    EMT_F_INFO_MEMBER_24(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, 0                                                                           \
    )                                                                                              \
    EMT_F_INFO_MEMBER_DISPATCH(type, type6, tag)

#define EMT_F_INFO_MEMBER_32(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, type, x, len, tag, _dummy                                        \
)                                                                                                  \
    EMT_F_INFO_MEMBER_28(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, 0                                                       \
    )                                                                                              \
    EMT_F_INFO_MEMBER_DISPATCH(type, type7, tag)

#define EMT_F_INFO_MEMBER_36(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, type, x, len, tag, _dummy                    \
)                                                                                                  \
    EMT_F_INFO_MEMBER_32(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, 0                                   \
    )                                                                                              \
    EMT_F_INFO_MEMBER_DISPATCH(type, type8, tag)

#define EMT_F_INFO_MEMBER_40(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, type, x, len, tag,       \
    _dummy                                                                                         \
)                                                                                                  \
    EMT_F_INFO_MEMBER_36(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, 0               \
    )                                                                                              \
    EMT_F_INFO_MEMBER_DISPATCH(type, type9, tag)

#define EMT_F_INFO_MEMBER_44(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27,      \
    type, x, len, tag, _dummy                                                                      \
)                                                                                                  \
    EMT_F_INFO_MEMBER_40(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, 0                                                                                     \
    )                                                                                              \
    EMT_F_INFO_MEMBER_DISPATCH(type, type10, tag)

#define EMT_F_INFO_MEMBER_48(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, type, x, len, tag, _dummy                                                       \
)                                                                                                  \
    EMT_F_INFO_MEMBER_44(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, 0                                                                 \
    )                                                                                              \
    EMT_F_INFO_MEMBER_DISPATCH(type, type11, tag)

#define EMT_F_INFO_MEMBER_52(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, type, x, len, tag, _dummy                                   \
)                                                                                                  \
    EMT_F_INFO_MEMBER_48(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, 0                                             \
    )                                                                                              \
    EMT_F_INFO_MEMBER_DISPATCH(type, type12, tag)

#define EMT_F_INFO_MEMBER_56(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, type, x, len, tag, _dummy               \
)                                                                                                  \
    EMT_F_INFO_MEMBER_52(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, 0                         \
    )                                                                                              \
    EMT_F_INFO_MEMBER_DISPATCH(type, type13, tag)

#define EMT_F_INFO_MEMBER_60(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, type, x, len, tag,  \
    _dummy                                                                                         \
)                                                                                                  \
    EMT_F_INFO_MEMBER_56(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, 0     \
    )                                                                                              \
    EMT_F_INFO_MEMBER_DISPATCH(type, type14, tag)

#define EMT_F_INFO_MEMBER_64(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    type, x, len, tag, _dummy                                                                      \
)                                                                                                  \
    EMT_F_INFO_MEMBER_60(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, 0                                                                           \
    )                                                                                              \
    EMT_F_INFO_MEMBER_DISPATCH(type, type15, tag)

#define EMT_F_INFO_MEMBER_68(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, type, x, len, tag, _dummy                                                  \
)                                                                                                  \
    EMT_F_INFO_MEMBER_64(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, 0                                                       \
    )                                                                                              \
    EMT_F_INFO_MEMBER_DISPATCH(type, type16, tag)

#define EMT_F_INFO_MEMBER_72(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, type, x, len, tag, _dummy                              \
)                                                                                                  \
    EMT_F_INFO_MEMBER_68(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, 0                                   \
    )                                                                                              \
    EMT_F_INFO_MEMBER_DISPATCH(type, type17, tag)

#define EMT_F_INFO_MEMBER_76(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, type, x, len, tag, _dummy          \
)                                                                                                  \
    EMT_F_INFO_MEMBER_72(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, 0               \
    )                                                                                              \
    EMT_F_INFO_MEMBER_DISPATCH(type, type18, tag)

#define EMT_F_INFO_MEMBER_80(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, type, x, len,  \
    tag, _dummy                                                                                    \
)                                                                                                  \
    EMT_F_INFO_MEMBER_76(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, 0                                                                                     \
    )                                                                                              \
    EMT_F_INFO_MEMBER_DISPATCH(type, type19, tag)

#define EMT_F_INFO_MEMBER_84(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, type, x, len, tag, _dummy                                                                 \
)                                                                                                  \
    EMT_F_INFO_MEMBER_80(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, 0                                                                 \
    )                                                                                              \
    EMT_F_INFO_MEMBER_DISPATCH(type, type20, tag)

#define EMT_F_INFO_MEMBER_88(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, type, x, len, tag, _dummy                                             \
)                                                                                                  \
    EMT_F_INFO_MEMBER_84(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, 0                                             \
    )                                                                                              \
    EMT_F_INFO_MEMBER_DISPATCH(type, type21, tag)

#define EMT_F_INFO_MEMBER_92(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, type, x, len, tag, _dummy                         \
)                                                                                                  \
    EMT_F_INFO_MEMBER_88(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, 0                         \
    )                                                                                              \
    EMT_F_INFO_MEMBER_DISPATCH(type, type22, tag)

#define EMT_F_INFO_MEMBER_96(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, type, x, len, tag, _dummy     \
)                                                                                                  \
    EMT_F_INFO_MEMBER_92(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, 0     \
    )                                                                                              \
    EMT_F_INFO_MEMBER_DISPATCH(type, type23, tag)

#define EMT_F_INFO_MEMBER_100(                                                                     \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, type, x,  \
    len, tag, _dummy                                                                               \
)                                                                                                  \
    EMT_F_INFO_MEMBER_96(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, 0                                                                           \
    )                                                                                              \
    EMT_F_INFO_MEMBER_DISPATCH(type, type24, tag)

#define EMT_F_INFO_MEMBER_104(                                                                     \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, type, x, len, tag, _dummy                                                            \
)                                                                                                  \
    EMT_F_INFO_MEMBER_100(                                                                         \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, 0                                                       \
    )                                                                                              \
    EMT_F_INFO_MEMBER_DISPATCH(type, type25, tag)

#define EMT_F_INFO_MEMBER_108(                                                                     \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, type, x, len, tag, _dummy                                        \
)                                                                                                  \
    EMT_F_INFO_MEMBER_104(                                                                         \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, 0                                   \
    )                                                                                              \
    EMT_F_INFO_MEMBER_DISPATCH(type, type26, tag)

#define EMT_F_INFO_MEMBER_112(                                                                     \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, type, x, len, tag, _dummy                    \
)                                                                                                  \
    EMT_F_INFO_MEMBER_108(                                                                         \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, 0               \
    )                                                                                              \
    EMT_F_INFO_MEMBER_DISPATCH(type, type27, tag)

#define EMT_F_INFO_MEMBER_116(                                                                     \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, type, x, len, tag,       \
    _dummy                                                                                         \
)                                                                                                  \
    EMT_F_INFO_MEMBER_112(                                                                         \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e,  \
        _6f, 0                                                                                     \
    )                                                                                              \
    EMT_F_INFO_MEMBER_DISPATCH(type, type28, tag)

#define EMT_F_INFO_MEMBER_120(                                                                     \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73,      \
    type, x, len, tag, _dummy                                                                      \
)                                                                                                  \
    EMT_F_INFO_MEMBER_116(                                                                         \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e,  \
        _6f, _70, _71, _72, _73, 0                                                                 \
    )                                                                                              \
    EMT_F_INFO_MEMBER_DISPATCH(type, type29, tag)

#define EMT_F_INFO_MEMBER_124(                                                                     \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, \
    _75, _76, _77, type, x, len, tag, _dummy                                                       \
)                                                                                                  \
    EMT_F_INFO_MEMBER_120(                                                                         \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e,  \
        _6f, _70, _71, _72, _73, _74, _75, _76, _77, 0                                             \
    )                                                                                              \
    EMT_F_INFO_MEMBER_DISPATCH(type, type30, tag)

#define EMT_F_INFO_MEMBER_128(                                                                     \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, \
    _75, _76, _77, _78, _79, _7a, _7b, type, x, len, tag, _dummy                                   \
)                                                                                                  \
    EMT_F_INFO_MEMBER_124(                                                                         \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e,  \
        _6f, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _7a, _7b, 0                         \
    )                                                                                              \
    EMT_F_INFO_MEMBER_DISPATCH(type, type31, tag)

#define EMT_F_INFO_MEMBER(x, ...) EMT_F_INFO_MEMBER_HELPER(x, __VA_ARGS__)
#define EMT_F_INFO_MEMBER_HELPER(x, ...) EMT_F_INFO_MEMBER_##x(__VA_ARGS__)
#define EMT_F_LAYOUT_DISPATCH_EMT_TAG_VAL(name, len, type)                                         \
    offsetof(struct emt_info_unlikely_to_shadow_t, name), sizeof(type), 0,

#define EMT_F_LAYOUT_DISPATCH_EMT_TAG_STR(name, len, type)                                         \
    offsetof(struct emt_info_unlikely_to_shadow_t, name), EMT_NULL_TERMINATED, 0,

#define EMT_F_LAYOUT_DISPATCH_EMT_TAG_ARR(name, len, type)                                         \
    offsetof(struct emt_info_unlikely_to_shadow_t, name), (emt_size_t) 1 * (len), 1,               \
        offsetof(struct emt_info_unlikely_to_shadow_t, name##_child_name), sizeof(type), 0,        \
        offsetof(struct emt_info_unlikely_to_shadow_t, name##_child_type_id),

#define EMT_F_LAYOUT_DISPATCH_EMT_TAG_STS(name, len, type)                                         \
    offsetof(struct emt_info_unlikely_to_shadow_t, name), (emt_size_t) 1 * (len), 1,               \
        offsetof(struct emt_info_unlikely_to_shadow_t, name##_child_name), sizeof(type), 0,        \
        offsetof(struct emt_info_unlikely_to_shadow_t, name##_child_type_id),

#define EMT_F_LAYOUT_DISPATCH_EMT_TAG_SLC(name, len, type)                                         \
    offsetof(struct emt_info_unlikely_to_shadow_t, name), EMT_LENGTH_PREFIXED, 1,                  \
        offsetof(struct emt_info_unlikely_to_shadow_t, name##_child_name), sizeof(type), 0,        \
        offsetof(struct emt_info_unlikely_to_shadow_t, name##_child_type_id),

#define EMT_F_LAYOUT_DISPATCH_HELPER(name, len, type, _tag)                                        \
    EMT_F_LAYOUT_DISPATCH_##_tag(name, len, type)

#define EMT_F_LAYOUT_DISPATCH(name, len, type, _tag)                                               \
    EMT_F_LAYOUT_DISPATCH_HELPER(name, len, type, _tag)

#define EMT_F_LAYOUT_0(_dummy)

#define EMT_F_LAYOUT_4(type, x, len, tag, _dummy) EMT_F_LAYOUT_DISPATCH(type0, len, type, tag)

#define EMT_F_LAYOUT_8(_0, _1, _2, _3, type, x, len, tag, _dummy)                                  \
    EMT_F_LAYOUT_4(_0, _1, _2, _3, 0) EMT_F_LAYOUT_DISPATCH(type1, len, type, tag)

#define EMT_F_LAYOUT_12(_0, _1, _2, _3, _4, _5, _6, _7, type, x, len, tag, _dummy)                 \
    EMT_F_LAYOUT_8(_0, _1, _2, _3, _4, _5, _6, _7, 0) EMT_F_LAYOUT_DISPATCH(type2, len, type, tag)

#define EMT_F_LAYOUT_16(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, type, x, len, tag, _dummy) \
    EMT_F_LAYOUT_12(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, 0)                             \
    EMT_F_LAYOUT_DISPATCH(type3, len, type, tag)

#define EMT_F_LAYOUT_20(                                                                           \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, type, x, len, tag, _dummy      \
)                                                                                                  \
    EMT_F_LAYOUT_16(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, 0)             \
    EMT_F_LAYOUT_DISPATCH(type4, len, type, tag)

#define EMT_F_LAYOUT_24(                                                                           \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, type, x,   \
    len, tag, _dummy                                                                               \
)                                                                                                  \
    EMT_F_LAYOUT_20(                                                                               \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, 0      \
    )                                                                                              \
    EMT_F_LAYOUT_DISPATCH(type5, len, type, tag)

#define EMT_F_LAYOUT_28(                                                                           \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, type, x, len, tag, _dummy                                                            \
)                                                                                                  \
    EMT_F_LAYOUT_24(                                                                               \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, 0                                                                           \
    )                                                                                              \
    EMT_F_LAYOUT_DISPATCH(type6, len, type, tag)

#define EMT_F_LAYOUT_32(                                                                           \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, type, x, len, tag, _dummy                                        \
)                                                                                                  \
    EMT_F_LAYOUT_28(                                                                               \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, 0                                                       \
    )                                                                                              \
    EMT_F_LAYOUT_DISPATCH(type7, len, type, tag)

#define EMT_F_LAYOUT_36(                                                                           \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, type, x, len, tag, _dummy                    \
)                                                                                                  \
    EMT_F_LAYOUT_32(                                                                               \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, 0                                   \
    )                                                                                              \
    EMT_F_LAYOUT_DISPATCH(type8, len, type, tag)

#define EMT_F_LAYOUT_40(                                                                           \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, type, x, len, tag,       \
    _dummy                                                                                         \
)                                                                                                  \
    EMT_F_LAYOUT_36(                                                                               \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, 0               \
    )                                                                                              \
    EMT_F_LAYOUT_DISPATCH(type9, len, type, tag)

#define EMT_F_LAYOUT_44(                                                                           \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27,      \
    type, x, len, tag, _dummy                                                                      \
)                                                                                                  \
    EMT_F_LAYOUT_40(                                                                               \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, 0                                                                                     \
    )                                                                                              \
    EMT_F_LAYOUT_DISPATCH(type10, len, type, tag)

#define EMT_F_LAYOUT_48(                                                                           \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, type, x, len, tag, _dummy                                                       \
)                                                                                                  \
    EMT_F_LAYOUT_44(                                                                               \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, 0                                                                 \
    )                                                                                              \
    EMT_F_LAYOUT_DISPATCH(type11, len, type, tag)

#define EMT_F_LAYOUT_52(                                                                           \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, type, x, len, tag, _dummy                                   \
)                                                                                                  \
    EMT_F_LAYOUT_48(                                                                               \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, 0                                             \
    )                                                                                              \
    EMT_F_LAYOUT_DISPATCH(type12, len, type, tag)

#define EMT_F_LAYOUT_56(                                                                           \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, type, x, len, tag, _dummy               \
)                                                                                                  \
    EMT_F_LAYOUT_52(                                                                               \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, 0                         \
    )                                                                                              \
    EMT_F_LAYOUT_DISPATCH(type13, len, type, tag)

#define EMT_F_LAYOUT_60(                                                                           \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, type, x, len, tag,  \
    _dummy                                                                                         \
)                                                                                                  \
    EMT_F_LAYOUT_56(                                                                               \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, 0     \
    )                                                                                              \
    EMT_F_LAYOUT_DISPATCH(type14, len, type, tag)

#define EMT_F_LAYOUT_64(                                                                           \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    type, x, len, tag, _dummy                                                                      \
)                                                                                                  \
    EMT_F_LAYOUT_60(                                                                               \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, 0                                                                           \
    )                                                                                              \
    EMT_F_LAYOUT_DISPATCH(type15, len, type, tag)

#define EMT_F_LAYOUT_68(                                                                           \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, type, x, len, tag, _dummy                                                  \
)                                                                                                  \
    EMT_F_LAYOUT_64(                                                                               \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, 0                                                       \
    )                                                                                              \
    EMT_F_LAYOUT_DISPATCH(type16, len, type, tag)

#define EMT_F_LAYOUT_72(                                                                           \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, type, x, len, tag, _dummy                              \
)                                                                                                  \
    EMT_F_LAYOUT_68(                                                                               \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, 0                                   \
    )                                                                                              \
    EMT_F_LAYOUT_DISPATCH(type17, len, type, tag)

#define EMT_F_LAYOUT_76(                                                                           \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, type, x, len, tag, _dummy          \
)                                                                                                  \
    EMT_F_LAYOUT_72(                                                                               \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, 0               \
    )                                                                                              \
    EMT_F_LAYOUT_DISPATCH(type18, len, type, tag)

#define EMT_F_LAYOUT_80(                                                                           \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, type, x, len,  \
    tag, _dummy                                                                                    \
)                                                                                                  \
    EMT_F_LAYOUT_76(                                                                               \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, 0                                                                                     \
    )                                                                                              \
    EMT_F_LAYOUT_DISPATCH(type19, len, type, tag)

#define EMT_F_LAYOUT_84(                                                                           \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, type, x, len, tag, _dummy                                                                 \
)                                                                                                  \
    EMT_F_LAYOUT_80(                                                                               \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, 0                                                                 \
    )                                                                                              \
    EMT_F_LAYOUT_DISPATCH(type20, len, type, tag)

#define EMT_F_LAYOUT_88(                                                                           \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, type, x, len, tag, _dummy                                             \
)                                                                                                  \
    EMT_F_LAYOUT_84(                                                                               \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, 0                                             \
    )                                                                                              \
    EMT_F_LAYOUT_DISPATCH(type21, len, type, tag)

#define EMT_F_LAYOUT_92(                                                                           \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, type, x, len, tag, _dummy                         \
)                                                                                                  \
    EMT_F_LAYOUT_88(                                                                               \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, 0                         \
    )                                                                                              \
    EMT_F_LAYOUT_DISPATCH(type22, len, type, tag)

#define EMT_F_LAYOUT_96(                                                                           \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, type, x, len, tag, _dummy     \
)                                                                                                  \
    EMT_F_LAYOUT_92(                                                                               \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, 0     \
    )                                                                                              \
    EMT_F_LAYOUT_DISPATCH(type23, len, type, tag)

#define EMT_F_LAYOUT_100(                                                                          \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, type, x,  \
    len, tag, _dummy                                                                               \
)                                                                                                  \
    EMT_F_LAYOUT_96(                                                                               \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, 0                                                                           \
    )                                                                                              \
    EMT_F_LAYOUT_DISPATCH(type24, len, type, tag)

#define EMT_F_LAYOUT_104(                                                                          \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, type, x, len, tag, _dummy                                                            \
)                                                                                                  \
    EMT_F_LAYOUT_100(                                                                              \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, 0                                                       \
    )                                                                                              \
    EMT_F_LAYOUT_DISPATCH(type25, len, type, tag)

#define EMT_F_LAYOUT_108(                                                                          \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, type, x, len, tag, _dummy                                        \
)                                                                                                  \
    EMT_F_LAYOUT_104(                                                                              \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, 0                                   \
    )                                                                                              \
    EMT_F_LAYOUT_DISPATCH(type26, len, type, tag)

#define EMT_F_LAYOUT_112(                                                                          \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, type, x, len, tag, _dummy                    \
)                                                                                                  \
    EMT_F_LAYOUT_108(                                                                              \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, 0               \
    )                                                                                              \
    EMT_F_LAYOUT_DISPATCH(type27, len, type, tag)

#define EMT_F_LAYOUT_116(                                                                          \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, type, x, len, tag,       \
    _dummy                                                                                         \
)                                                                                                  \
    EMT_F_LAYOUT_112(                                                                              \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e,  \
        _6f, 0                                                                                     \
    )                                                                                              \
    EMT_F_LAYOUT_DISPATCH(type28, len, type, tag)

#define EMT_F_LAYOUT_120(                                                                          \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73,      \
    type, x, len, tag, _dummy                                                                      \
)                                                                                                  \
    EMT_F_LAYOUT_116(                                                                              \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e,  \
        _6f, _70, _71, _72, _73, 0                                                                 \
    )                                                                                              \
    EMT_F_LAYOUT_DISPATCH(type29, len, type, tag)

#define EMT_F_LAYOUT_124(                                                                          \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, \
    _75, _76, _77, type, x, len, tag, _dummy                                                       \
)                                                                                                  \
    EMT_F_LAYOUT_120(                                                                              \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e,  \
        _6f, _70, _71, _72, _73, _74, _75, _76, _77, 0                                             \
    )                                                                                              \
    EMT_F_LAYOUT_DISPATCH(type30, len, type, tag)

#define EMT_F_LAYOUT_128(                                                                          \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, \
    _75, _76, _77, _78, _79, _7a, _7b, type, x, len, tag, _dummy                                   \
)                                                                                                  \
    EMT_F_LAYOUT_124(                                                                              \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e,  \
        _6f, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _7a, _7b, 0                         \
    )                                                                                              \
    EMT_F_LAYOUT_DISPATCH(type31, len, type, tag)

#define EMT_F_LAYOUT(x, ...) EMT_F_LAYOUT_HELPER(x, __VA_ARGS__)
#define EMT_F_LAYOUT_HELPER(x, ...) EMT_F_LAYOUT_##x(__VA_ARGS__)
#define EMT_F_INFO_DISPATCH_EMT_TAG_VAL(type) #type,

#define EMT_F_INFO_DISPATCH_EMT_TAG_STR(type) "string",

#define EMT_F_INFO_DISPATCH_EMT_TAG_ARR(type) "list", "", #type,

#define EMT_F_INFO_DISPATCH_EMT_TAG_SLC(type) "list", "", #type,

#define EMT_F_INFO_DISPATCH_EMT_TAG_STS(type) "list", "", #type,

#define EMT_F_INFO_DISPATCH_HELPER(type, _tag) EMT_F_INFO_DISPATCH_##_tag(type)

#define EMT_F_INFO_DISPATCH(type, _tag) EMT_F_INFO_DISPATCH_HELPER(type, _tag)

#define EMT_F_INFO_0(_dummy)

#define EMT_F_INFO_4(type, x, len, tag, _dummy) EMT_F_INFO_DISPATCH(type, tag)

#define EMT_F_INFO_8(_0, _1, _2, _3, type, x, len, tag, _dummy)                                    \
    EMT_F_INFO_4(_0, _1, _2, _3, 0) EMT_F_INFO_DISPATCH(type, tag)

#define EMT_F_INFO_12(_0, _1, _2, _3, _4, _5, _6, _7, type, x, len, tag, _dummy)                   \
    EMT_F_INFO_8(_0, _1, _2, _3, _4, _5, _6, _7, 0) EMT_F_INFO_DISPATCH(type, tag)

#define EMT_F_INFO_16(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, type, x, len, tag, _dummy)   \
    EMT_F_INFO_12(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, 0) EMT_F_INFO_DISPATCH(type, tag)

#define EMT_F_INFO_20(                                                                             \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, type, x, len, tag, _dummy      \
)                                                                                                  \
    EMT_F_INFO_16(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, 0)               \
    EMT_F_INFO_DISPATCH(type, tag)

#define EMT_F_INFO_24(                                                                             \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, type, x,   \
    len, tag, _dummy                                                                               \
)                                                                                                  \
    EMT_F_INFO_20(                                                                                 \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, 0      \
    )                                                                                              \
    EMT_F_INFO_DISPATCH(type, tag)

#define EMT_F_INFO_28(                                                                             \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, type, x, len, tag, _dummy                                                            \
)                                                                                                  \
    EMT_F_INFO_24(                                                                                 \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, 0                                                                           \
    )                                                                                              \
    EMT_F_INFO_DISPATCH(type, tag)

#define EMT_F_INFO_32(                                                                             \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, type, x, len, tag, _dummy                                        \
)                                                                                                  \
    EMT_F_INFO_28(                                                                                 \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, 0                                                       \
    )                                                                                              \
    EMT_F_INFO_DISPATCH(type, tag)

#define EMT_F_INFO_36(                                                                             \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, type, x, len, tag, _dummy                    \
)                                                                                                  \
    EMT_F_INFO_32(                                                                                 \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, 0                                   \
    )                                                                                              \
    EMT_F_INFO_DISPATCH(type, tag)

#define EMT_F_INFO_40(                                                                             \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, type, x, len, tag,       \
    _dummy                                                                                         \
)                                                                                                  \
    EMT_F_INFO_36(                                                                                 \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, 0               \
    )                                                                                              \
    EMT_F_INFO_DISPATCH(type, tag)

#define EMT_F_INFO_44(                                                                             \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27,      \
    type, x, len, tag, _dummy                                                                      \
)                                                                                                  \
    EMT_F_INFO_40(                                                                                 \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, 0                                                                                     \
    )                                                                                              \
    EMT_F_INFO_DISPATCH(type, tag)

#define EMT_F_INFO_48(                                                                             \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, type, x, len, tag, _dummy                                                       \
)                                                                                                  \
    EMT_F_INFO_44(                                                                                 \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, 0                                                                 \
    )                                                                                              \
    EMT_F_INFO_DISPATCH(type, tag)

#define EMT_F_INFO_52(                                                                             \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, type, x, len, tag, _dummy                                   \
)                                                                                                  \
    EMT_F_INFO_48(                                                                                 \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, 0                                             \
    )                                                                                              \
    EMT_F_INFO_DISPATCH(type, tag)

#define EMT_F_INFO_56(                                                                             \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, type, x, len, tag, _dummy               \
)                                                                                                  \
    EMT_F_INFO_52(                                                                                 \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, 0                         \
    )                                                                                              \
    EMT_F_INFO_DISPATCH(type, tag)

#define EMT_F_INFO_60(                                                                             \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, type, x, len, tag,  \
    _dummy                                                                                         \
)                                                                                                  \
    EMT_F_INFO_56(                                                                                 \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, 0     \
    )                                                                                              \
    EMT_F_INFO_DISPATCH(type, tag)

#define EMT_F_INFO_64(                                                                             \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    type, x, len, tag, _dummy                                                                      \
)                                                                                                  \
    EMT_F_INFO_60(                                                                                 \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, 0                                                                           \
    )                                                                                              \
    EMT_F_INFO_DISPATCH(type, tag)

#define EMT_F_INFO_68(                                                                             \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, type, x, len, tag, _dummy                                                  \
)                                                                                                  \
    EMT_F_INFO_64(                                                                                 \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, 0                                                       \
    )                                                                                              \
    EMT_F_INFO_DISPATCH(type, tag)

#define EMT_F_INFO_72(                                                                             \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, type, x, len, tag, _dummy                              \
)                                                                                                  \
    EMT_F_INFO_68(                                                                                 \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, 0                                   \
    )                                                                                              \
    EMT_F_INFO_DISPATCH(type, tag)

#define EMT_F_INFO_76(                                                                             \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, type, x, len, tag, _dummy          \
)                                                                                                  \
    EMT_F_INFO_72(                                                                                 \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, 0               \
    )                                                                                              \
    EMT_F_INFO_DISPATCH(type, tag)

#define EMT_F_INFO_80(                                                                             \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, type, x, len,  \
    tag, _dummy                                                                                    \
)                                                                                                  \
    EMT_F_INFO_76(                                                                                 \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, 0                                                                                     \
    )                                                                                              \
    EMT_F_INFO_DISPATCH(type, tag)

#define EMT_F_INFO_84(                                                                             \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, type, x, len, tag, _dummy                                                                 \
)                                                                                                  \
    EMT_F_INFO_80(                                                                                 \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, 0                                                                 \
    )                                                                                              \
    EMT_F_INFO_DISPATCH(type, tag)

#define EMT_F_INFO_88(                                                                             \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, type, x, len, tag, _dummy                                             \
)                                                                                                  \
    EMT_F_INFO_84(                                                                                 \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, 0                                             \
    )                                                                                              \
    EMT_F_INFO_DISPATCH(type, tag)

#define EMT_F_INFO_92(                                                                             \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, type, x, len, tag, _dummy                         \
)                                                                                                  \
    EMT_F_INFO_88(                                                                                 \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, 0                         \
    )                                                                                              \
    EMT_F_INFO_DISPATCH(type, tag)

#define EMT_F_INFO_96(                                                                             \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, type, x, len, tag, _dummy     \
)                                                                                                  \
    EMT_F_INFO_92(                                                                                 \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, 0     \
    )                                                                                              \
    EMT_F_INFO_DISPATCH(type, tag)

#define EMT_F_INFO_100(                                                                            \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, type, x,  \
    len, tag, _dummy                                                                               \
)                                                                                                  \
    EMT_F_INFO_96(                                                                                 \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, 0                                                                           \
    )                                                                                              \
    EMT_F_INFO_DISPATCH(type, tag)

#define EMT_F_INFO_104(                                                                            \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, type, x, len, tag, _dummy                                                            \
)                                                                                                  \
    EMT_F_INFO_100(                                                                                \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, 0                                                       \
    )                                                                                              \
    EMT_F_INFO_DISPATCH(type, tag)

#define EMT_F_INFO_108(                                                                            \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, type, x, len, tag, _dummy                                        \
)                                                                                                  \
    EMT_F_INFO_104(                                                                                \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, 0                                   \
    )                                                                                              \
    EMT_F_INFO_DISPATCH(type, tag)

#define EMT_F_INFO_112(                                                                            \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, type, x, len, tag, _dummy                    \
)                                                                                                  \
    EMT_F_INFO_108(                                                                                \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, 0               \
    )                                                                                              \
    EMT_F_INFO_DISPATCH(type, tag)

#define EMT_F_INFO_116(                                                                            \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, type, x, len, tag,       \
    _dummy                                                                                         \
)                                                                                                  \
    EMT_F_INFO_112(                                                                                \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e,  \
        _6f, 0                                                                                     \
    )                                                                                              \
    EMT_F_INFO_DISPATCH(type, tag)

#define EMT_F_INFO_120(                                                                            \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73,      \
    type, x, len, tag, _dummy                                                                      \
)                                                                                                  \
    EMT_F_INFO_116(                                                                                \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e,  \
        _6f, _70, _71, _72, _73, 0                                                                 \
    )                                                                                              \
    EMT_F_INFO_DISPATCH(type, tag)

#define EMT_F_INFO_124(                                                                            \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, \
    _75, _76, _77, type, x, len, tag, _dummy                                                       \
)                                                                                                  \
    EMT_F_INFO_120(                                                                                \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e,  \
        _6f, _70, _71, _72, _73, _74, _75, _76, _77, 0                                             \
    )                                                                                              \
    EMT_F_INFO_DISPATCH(type, tag)

#define EMT_F_INFO_128(                                                                            \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, \
    _75, _76, _77, _78, _79, _7a, _7b, type, x, len, tag, _dummy                                   \
)                                                                                                  \
    EMT_F_INFO_124(                                                                                \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e,  \
        _6f, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _7a, _7b, 0                         \
    )                                                                                              \
    EMT_F_INFO_DISPATCH(type, tag)

#define EMT_F_INFO(x, ...) EMT_F_INFO_HELPER(x, __VA_ARGS__)
#define EMT_F_INFO_HELPER(x, ...) EMT_F_INFO_##x(__VA_ARGS__)
#define EMT_F_TOTAL_SIZE_0(_dummy) sizeof(emt_ptr_t)

#define EMT_F_TOTAL_SIZE_4(type, x, len, tag, _dummy) (sizeof(emt_ptr_t) + (len))

#define EMT_F_TOTAL_SIZE_8(_0, _1, _2, _3, type, x, len, tag, _dummy)                              \
    EMT_F_TOTAL_SIZE_4(_0, _1, _2, _3, 0) + (len)

#define EMT_F_TOTAL_SIZE_12(_0, _1, _2, _3, _4, _5, _6, _7, type, x, len, tag, _dummy)             \
    EMT_F_TOTAL_SIZE_8(_0, _1, _2, _3, _4, _5, _6, _7, 0) + (len)

#define EMT_F_TOTAL_SIZE_16(                                                                       \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, type, x, len, tag, _dummy                      \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_12(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, 0) + (len)

#define EMT_F_TOTAL_SIZE_20(                                                                       \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, type, x, len, tag, _dummy      \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_16(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, 0) + (len)

#define EMT_F_TOTAL_SIZE_24(                                                                       \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, type, x,   \
    len, tag, _dummy                                                                               \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_20(                                                                           \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, 0      \
    ) + (len)

#define EMT_F_TOTAL_SIZE_28(                                                                       \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, type, x, len, tag, _dummy                                                            \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_24(                                                                           \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, 0                                                                           \
    ) + (len)

#define EMT_F_TOTAL_SIZE_32(                                                                       \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, type, x, len, tag, _dummy                                        \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_28(                                                                           \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, 0                                                       \
    ) + (len)

#define EMT_F_TOTAL_SIZE_36(                                                                       \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, type, x, len, tag, _dummy                    \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_32(                                                                           \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, 0                                   \
    ) + (len)

#define EMT_F_TOTAL_SIZE_40(                                                                       \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, type, x, len, tag,       \
    _dummy                                                                                         \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_36(                                                                           \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, 0               \
    ) + (len)

#define EMT_F_TOTAL_SIZE_44(                                                                       \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27,      \
    type, x, len, tag, _dummy                                                                      \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_40(                                                                           \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, 0                                                                                     \
    ) + (len)

#define EMT_F_TOTAL_SIZE_48(                                                                       \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, type, x, len, tag, _dummy                                                       \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_44(                                                                           \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, 0                                                                 \
    ) + (len)

#define EMT_F_TOTAL_SIZE_52(                                                                       \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, type, x, len, tag, _dummy                                   \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_48(                                                                           \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, 0                                             \
    ) + (len)

#define EMT_F_TOTAL_SIZE_56(                                                                       \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, type, x, len, tag, _dummy               \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_52(                                                                           \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, 0                         \
    ) + (len)

#define EMT_F_TOTAL_SIZE_60(                                                                       \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, type, x, len, tag,  \
    _dummy                                                                                         \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_56(                                                                           \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, 0     \
    ) + (len)

#define EMT_F_TOTAL_SIZE_64(                                                                       \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    type, x, len, tag, _dummy                                                                      \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_60(                                                                           \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, 0                                                                           \
    ) + (len)

#define EMT_F_TOTAL_SIZE_68(                                                                       \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, type, x, len, tag, _dummy                                                  \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_64(                                                                           \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, 0                                                       \
    ) + (len)

#define EMT_F_TOTAL_SIZE_72(                                                                       \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, type, x, len, tag, _dummy                              \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_68(                                                                           \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, 0                                   \
    ) + (len)

#define EMT_F_TOTAL_SIZE_76(                                                                       \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, type, x, len, tag, _dummy          \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_72(                                                                           \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, 0               \
    ) + (len)

#define EMT_F_TOTAL_SIZE_80(                                                                       \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, type, x, len,  \
    tag, _dummy                                                                                    \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_76(                                                                           \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, 0                                                                                     \
    ) + (len)

#define EMT_F_TOTAL_SIZE_84(                                                                       \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, type, x, len, tag, _dummy                                                                 \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_80(                                                                           \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, 0                                                                 \
    ) + (len)

#define EMT_F_TOTAL_SIZE_88(                                                                       \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, type, x, len, tag, _dummy                                             \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_84(                                                                           \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, 0                                             \
    ) + (len)

#define EMT_F_TOTAL_SIZE_92(                                                                       \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, type, x, len, tag, _dummy                         \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_88(                                                                           \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, 0                         \
    ) + (len)

#define EMT_F_TOTAL_SIZE_96(                                                                       \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, type, x, len, tag, _dummy     \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_92(                                                                           \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, 0     \
    ) + (len)

#define EMT_F_TOTAL_SIZE_100(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, type, x,  \
    len, tag, _dummy                                                                               \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_96(                                                                           \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, 0                                                                           \
    ) + (len)

#define EMT_F_TOTAL_SIZE_104(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, type, x, len, tag, _dummy                                                            \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_100(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, 0                                                       \
    ) + (len)

#define EMT_F_TOTAL_SIZE_108(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, type, x, len, tag, _dummy                                        \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_104(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, 0                                   \
    ) + (len)

#define EMT_F_TOTAL_SIZE_112(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, type, x, len, tag, _dummy                    \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_108(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, 0               \
    ) + (len)

#define EMT_F_TOTAL_SIZE_116(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, type, x, len, tag,       \
    _dummy                                                                                         \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_112(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e,  \
        _6f, 0                                                                                     \
    ) + (len)

#define EMT_F_TOTAL_SIZE_120(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73,      \
    type, x, len, tag, _dummy                                                                      \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_116(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e,  \
        _6f, _70, _71, _72, _73, 0                                                                 \
    ) + (len)

#define EMT_F_TOTAL_SIZE_124(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, \
    _75, _76, _77, type, x, len, tag, _dummy                                                       \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_120(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e,  \
        _6f, _70, _71, _72, _73, _74, _75, _76, _77, 0                                             \
    ) + (len)

#define EMT_F_TOTAL_SIZE_128(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, \
    _75, _76, _77, _78, _79, _7a, _7b, type, x, len, tag, _dummy                                   \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_124(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e,  \
        _6f, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _7a, _7b, 0                         \
    ) + (len)

#define EMT_F_TOTAL_SIZE(x, ...) EMT_F_TOTAL_SIZE_HELPER(x, __VA_ARGS__)
#define EMT_F_TOTAL_SIZE_HELPER(x, ...) EMT_F_TOTAL_SIZE_##x(__VA_ARGS__)
#define EMT_F_DISPATCH_EMT_TAG_VAL(type, x, len, out_fn, extra_arg)                                \
    {                                                                                              \
        type temp = x;                                                                             \
        out_fn((const void*) &temp, sizeof(type), extra_arg);                                      \
    }

#define EMT_F_DISPATCH_EMT_TAG_STR(type, x, len, out_fn, extra_arg)                                \
    {                                                                                              \
        const char* temp = x;                                                                      \
        out_fn(temp, (emt_size_t) (strlen(temp) + 1), extra_arg);                                  \
    }

#define EMT_F_DISPATCH_EMT_TAG_ARR(type, x, len, out_fn, extra_arg)                                \
    {                                                                                              \
        const type(*temp)[] = &(x);                                                                \
        out_fn(temp, sizeof(x), extra_arg);                                                        \
    }

#define EMT_F_DISPATCH_EMT_TAG_SLC(type, x, len, out_fn, extra_arg)                                \
    {                                                                                              \
        const type* emt_temp_unlikely_to_shadow = x;                                               \
        emt_size_t size = len;                                                                     \
        out_fn(&size, sizeof(size), extra_arg);                                                    \
        out_fn(emt_temp_unlikely_to_shadow, size * sizeof(type), extra_arg);                       \
    }

#define EMT_F_DISPATCH_EMT_TAG_STS(type, x, len, out_fn, extra_arg)                                \
    {                                                                                              \
        const type* temp = x;                                                                      \
        out_fn(temp, (emt_size_t) sizeof(type) * (len), extra_arg);                                \
    }

#define EMT_F_DISPATCH_HELPER(type, x, len, out_fn, extra_arg, _tag)                               \
    EMT_F_DISPATCH_##_tag(type, x, len, out_fn, extra_arg)

#define EMT_F_DISPATCH(type, x, len, out_fn, extra_arg, _tag)                                      \
    EMT_F_DISPATCH_HELPER(type, x, len, out_fn, extra_arg, _tag)

#define EMT_F_0(out_fn, extra_arg, _dummy)

#define EMT_F_4(out_fn, extra_arg, type, x, len, tag, _dummy)                                      \
    EMT_F_DISPATCH(type, x, len, out_fn, extra_arg, tag)

#define EMT_F_8(out_fn, extra_arg, _0, _1, _2, _3, type, x, len, tag, _dummy)                      \
    EMT_F_4(out_fn, extra_arg, _0, _1, _2, _3, 0)                                                  \
    EMT_F_DISPATCH(type, x, len, out_fn, extra_arg, tag)

#define EMT_F_12(out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, type, x, len, tag, _dummy)     \
    EMT_F_8(out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, 0)                                  \
    EMT_F_DISPATCH(type, x, len, out_fn, extra_arg, tag)

#define EMT_F_16(                                                                                  \
    out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, type, x, len, tag, _dummy   \
)                                                                                                  \
    EMT_F_12(out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, 0)                 \
    EMT_F_DISPATCH(type, x, len, out_fn, extra_arg, tag)

#define EMT_F_20(                                                                                  \
    out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, type, x,    \
    len, tag, _dummy                                                                               \
)                                                                                                  \
    EMT_F_16(out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, 0) \
    EMT_F_DISPATCH(type, x, len, out_fn, extra_arg, tag)

#define EMT_F_24(                                                                                  \
    out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11,   \
    _12, _13, type, x, len, tag, _dummy                                                            \
)                                                                                                  \
    EMT_F_20(                                                                                      \
        out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10,    \
        _11, _12, _13, 0                                                                           \
    )                                                                                              \
    EMT_F_DISPATCH(type, x, len, out_fn, extra_arg, tag)

#define EMT_F_28(                                                                                  \
    out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11,   \
    _12, _13, _14, _15, _16, _17, type, x, len, tag, _dummy                                        \
)                                                                                                  \
    EMT_F_24(                                                                                      \
        out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10,    \
        _11, _12, _13, _14, _15, _16, _17, 0                                                       \
    )                                                                                              \
    EMT_F_DISPATCH(type, x, len, out_fn, extra_arg, tag)

#define EMT_F_32(                                                                                  \
    out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11,   \
    _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, type, x, len, tag, _dummy                    \
)                                                                                                  \
    EMT_F_28(                                                                                      \
        out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10,    \
        _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, 0                                   \
    )                                                                                              \
    EMT_F_DISPATCH(type, x, len, out_fn, extra_arg, tag)

#define EMT_F_36(                                                                                  \
    out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11,   \
    _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, type, x, len, tag,       \
    _dummy                                                                                         \
)                                                                                                  \
    EMT_F_32(                                                                                      \
        out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10,    \
        _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, 0               \
    )                                                                                              \
    EMT_F_DISPATCH(type, x, len, out_fn, extra_arg, tag)

#define EMT_F_40(                                                                                  \
    out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11,   \
    _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23,      \
    type, x, len, tag, _dummy                                                                      \
)                                                                                                  \
    EMT_F_36(                                                                                      \
        out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10,    \
        _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22,  \
        _23, 0                                                                                     \
    )                                                                                              \
    EMT_F_DISPATCH(type, x, len, out_fn, extra_arg, tag)

#define EMT_F_44(                                                                                  \
    out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11,   \
    _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, \
    _25, _26, _27, type, x, len, tag, _dummy                                                       \
)                                                                                                  \
    EMT_F_40(                                                                                      \
        out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10,    \
        _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22,  \
        _23, _24, _25, _26, _27, 0                                                                 \
    )                                                                                              \
    EMT_F_DISPATCH(type, x, len, out_fn, extra_arg, tag)

#define EMT_F_48(                                                                                  \
    out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11,   \
    _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, \
    _25, _26, _27, _28, _29, _2a, _2b, type, x, len, tag, _dummy                                   \
)                                                                                                  \
    EMT_F_44(                                                                                      \
        out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10,    \
        _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22,  \
        _23, _24, _25, _26, _27, _28, _29, _2a, _2b, 0                                             \
    )                                                                                              \
    EMT_F_DISPATCH(type, x, len, out_fn, extra_arg, tag)

#define EMT_F_52(                                                                                  \
    out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11,   \
    _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, \
    _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, type, x, len, tag, _dummy               \
)                                                                                                  \
    EMT_F_48(                                                                                      \
        out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10,    \
        _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22,  \
        _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, 0                         \
    )                                                                                              \
    EMT_F_DISPATCH(type, x, len, out_fn, extra_arg, tag)

#define EMT_F_56(                                                                                  \
    out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11,   \
    _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, \
    _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, type, x, len, tag,  \
    _dummy                                                                                         \
)                                                                                                  \
    EMT_F_52(                                                                                      \
        out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10,    \
        _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22,  \
        _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, 0     \
    )                                                                                              \
    EMT_F_DISPATCH(type, x, len, out_fn, extra_arg, tag)

#define EMT_F_60(                                                                                  \
    out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11,   \
    _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, \
    _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, \
    type, x, len, tag, _dummy                                                                      \
)                                                                                                  \
    EMT_F_56(                                                                                      \
        out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10,    \
        _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22,  \
        _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34,  \
        _35, _36, _37, 0                                                                           \
    )                                                                                              \
    EMT_F_DISPATCH(type, x, len, out_fn, extra_arg, tag)

#define EMT_F_64(                                                                                  \
    out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11,   \
    _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, \
    _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, \
    _38, _39, _3a, _3b, type, x, len, tag, _dummy                                                  \
)                                                                                                  \
    EMT_F_60(                                                                                      \
        out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10,    \
        _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22,  \
        _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34,  \
        _35, _36, _37, _38, _39, _3a, _3b, 0                                                       \
    )                                                                                              \
    EMT_F_DISPATCH(type, x, len, out_fn, extra_arg, tag)

#define EMT_F_68(                                                                                  \
    out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11,   \
    _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, \
    _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, \
    _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, type, x, len, tag, _dummy                              \
)                                                                                                  \
    EMT_F_64(                                                                                      \
        out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10,    \
        _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22,  \
        _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34,  \
        _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, 0                                   \
    )                                                                                              \
    EMT_F_DISPATCH(type, x, len, out_fn, extra_arg, tag)

#define EMT_F_72(                                                                                  \
    out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11,   \
    _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, \
    _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, \
    _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, type, x, len, tag, _dummy          \
)                                                                                                  \
    EMT_F_68(                                                                                      \
        out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10,    \
        _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22,  \
        _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34,  \
        _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, 0               \
    )                                                                                              \
    EMT_F_DISPATCH(type, x, len, out_fn, extra_arg, tag)

#define EMT_F_76(                                                                                  \
    out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11,   \
    _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, \
    _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, \
    _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, type, x, len,  \
    tag, _dummy                                                                                    \
)                                                                                                  \
    EMT_F_72(                                                                                      \
        out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10,    \
        _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22,  \
        _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34,  \
        _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46,  \
        _47, 0                                                                                     \
    )                                                                                              \
    EMT_F_DISPATCH(type, x, len, out_fn, extra_arg, tag)

#define EMT_F_80(                                                                                  \
    out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11,   \
    _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, \
    _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, \
    _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, \
    _4b, type, x, len, tag, _dummy                                                                 \
)                                                                                                  \
    EMT_F_76(                                                                                      \
        out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10,    \
        _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22,  \
        _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34,  \
        _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46,  \
        _47, _48, _49, _4a, _4b, 0                                                                 \
    )                                                                                              \
    EMT_F_DISPATCH(type, x, len, out_fn, extra_arg, tag)

#define EMT_F_84(                                                                                  \
    out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11,   \
    _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, \
    _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, \
    _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, \
    _4b, _4c, _4d, _4e, _4f, type, x, len, tag, _dummy                                             \
)                                                                                                  \
    EMT_F_80(                                                                                      \
        out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10,    \
        _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22,  \
        _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34,  \
        _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46,  \
        _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, 0                                             \
    )                                                                                              \
    EMT_F_DISPATCH(type, x, len, out_fn, extra_arg, tag)

#define EMT_F_88(                                                                                  \
    out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11,   \
    _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, \
    _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, \
    _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, \
    _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, type, x, len, tag, _dummy                         \
)                                                                                                  \
    EMT_F_84(                                                                                      \
        out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10,    \
        _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22,  \
        _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34,  \
        _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46,  \
        _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, 0                         \
    )                                                                                              \
    EMT_F_DISPATCH(type, x, len, out_fn, extra_arg, tag)

#define EMT_F_92(                                                                                  \
    out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11,   \
    _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, \
    _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, \
    _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, \
    _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, type, x, len, tag, _dummy     \
)                                                                                                  \
    EMT_F_88(                                                                                      \
        out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10,    \
        _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22,  \
        _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34,  \
        _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46,  \
        _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, 0     \
    )                                                                                              \
    EMT_F_DISPATCH(type, x, len, out_fn, extra_arg, tag)

#define EMT_F_96(                                                                                  \
    out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11,   \
    _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, \
    _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, \
    _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, \
    _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, type, x,  \
    len, tag, _dummy                                                                               \
)                                                                                                  \
    EMT_F_92(                                                                                      \
        out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10,    \
        _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22,  \
        _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34,  \
        _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46,  \
        _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58,  \
        _59, _5a, _5b, 0                                                                           \
    )                                                                                              \
    EMT_F_DISPATCH(type, x, len, out_fn, extra_arg, tag)

#define EMT_F_100(                                                                                 \
    out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11,   \
    _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, \
    _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, \
    _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, \
    _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, \
    _5e, _5f, type, x, len, tag, _dummy                                                            \
)                                                                                                  \
    EMT_F_96(                                                                                      \
        out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10,    \
        _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22,  \
        _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34,  \
        _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46,  \
        _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58,  \
        _59, _5a, _5b, _5c, _5d, _5e, _5f, 0                                                       \
    )                                                                                              \
    EMT_F_DISPATCH(type, x, len, out_fn, extra_arg, tag)

#define EMT_F_104(                                                                                 \
    out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11,   \
    _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, \
    _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, \
    _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, \
    _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, \
    _5e, _5f, _60, _61, _62, _63, type, x, len, tag, _dummy                                        \
)                                                                                                  \
    EMT_F_100(                                                                                     \
        out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10,    \
        _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22,  \
        _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34,  \
        _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46,  \
        _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58,  \
        _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, _62, _63, 0                                   \
    )                                                                                              \
    EMT_F_DISPATCH(type, x, len, out_fn, extra_arg, tag)

#define EMT_F_108(                                                                                 \
    out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11,   \
    _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, \
    _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, \
    _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, \
    _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, \
    _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, type, x, len, tag, _dummy                    \
)                                                                                                  \
    EMT_F_104(                                                                                     \
        out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10,    \
        _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22,  \
        _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34,  \
        _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46,  \
        _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58,  \
        _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, 0               \
    )                                                                                              \
    EMT_F_DISPATCH(type, x, len, out_fn, extra_arg, tag)

#define EMT_F_112(                                                                                 \
    out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11,   \
    _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, \
    _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, \
    _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, \
    _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, \
    _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, type, x, len, tag,       \
    _dummy                                                                                         \
)                                                                                                  \
    EMT_F_108(                                                                                     \
        out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10,    \
        _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22,  \
        _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34,  \
        _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46,  \
        _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58,  \
        _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a,  \
        _6b, 0                                                                                     \
    )                                                                                              \
    EMT_F_DISPATCH(type, x, len, out_fn, extra_arg, tag)

#define EMT_F_116(                                                                                 \
    out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11,   \
    _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, \
    _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, \
    _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, \
    _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, \
    _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f,      \
    type, x, len, tag, _dummy                                                                      \
)                                                                                                  \
    EMT_F_112(                                                                                     \
        out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10,    \
        _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22,  \
        _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34,  \
        _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46,  \
        _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58,  \
        _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a,  \
        _6b, _6c, _6d, _6e, _6f, 0                                                                 \
    )                                                                                              \
    EMT_F_DISPATCH(type, x, len, out_fn, extra_arg, tag)

#define EMT_F_120(                                                                                 \
    out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11,   \
    _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, \
    _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, \
    _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, \
    _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, \
    _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, \
    _71, _72, _73, type, x, len, tag, _dummy                                                       \
)                                                                                                  \
    EMT_F_116(                                                                                     \
        out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10,    \
        _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22,  \
        _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34,  \
        _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46,  \
        _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58,  \
        _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a,  \
        _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, 0                                             \
    )                                                                                              \
    EMT_F_DISPATCH(type, x, len, out_fn, extra_arg, tag)

#define EMT_F_124(                                                                                 \
    out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11,   \
    _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, \
    _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, \
    _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, \
    _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, \
    _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, \
    _71, _72, _73, _74, _75, _76, _77, type, x, len, tag, _dummy                                   \
)                                                                                                  \
    EMT_F_120(                                                                                     \
        out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10,    \
        _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22,  \
        _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34,  \
        _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46,  \
        _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58,  \
        _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a,  \
        _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, _75, _76, _77, 0                         \
    )                                                                                              \
    EMT_F_DISPATCH(type, x, len, out_fn, extra_arg, tag)

#define EMT_F_128(                                                                                 \
    out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11,   \
    _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, \
    _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, \
    _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, \
    _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, \
    _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, \
    _71, _72, _73, _74, _75, _76, _77, _78, _79, _7a, _7b, type, x, len, tag, _dummy               \
)                                                                                                  \
    EMT_F_124(                                                                                     \
        out_fn, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10,    \
        _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22,  \
        _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34,  \
        _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46,  \
        _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58,  \
        _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a,  \
        _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _7a, _7b, 0     \
    )                                                                                              \
    EMT_F_DISPATCH(type, x, len, out_fn, extra_arg, tag)

#define EMT_F(out_fn, extra_arg, x, ...) EMT_F_HELPER(out_fn, extra_arg, x, __VA_ARGS__)
#define EMT_F_HELPER(out_fn, extra_arg, x, ...) EMT_F_##x(out_fn, extra_arg, __VA_ARGS__)
// NOLINTEND(modernize-avoid-c-arrays)

#endif // EMTRACE_EMTRACE_H
