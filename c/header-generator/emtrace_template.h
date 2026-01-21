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
            unsigned char magic[6];                                                                \
            emt_size_t layout                                                                      \
                [5 + EMT_F_LAYOUT_SIZE(                                                            \
                         EMT_NUM_ARGS_REST(__VA_ARGS__), EMT_REST_ARGS(__VA_ARGS__, 0)             \
                     )];                                                                           \
            char fmt[sizeof(EMT_FIRST_ARG(__VA_ARGS__, 0) postfix)];                               \
            EMT_F_INFO_MEMBER(EMT_NUM_ARGS_REST(__VA_ARGS__), EMT_REST_ARGS(__VA_ARGS__, 0))       \
            char file[sizeof(__FILE__)];                                                           \
        };                                                                                         \
        fmt_info_attributes struct emt_info_unlikely_to_shadow_t emt_info_unlikely_to_shadow = {   \
            {'T', 'R', 'A', 'C', 'E', offsetof(struct emt_info_unlikely_to_shadow_t, layout)},     \
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
#define EMT_FLOCK_FILE(x, y, file) LockFile(file, 0, 0, 0xFFFFFFFF, 0xFFFFFFFF)
#define EMT_FUNLOCK_FILE(x, y, file) UnlockFile(file, 0, 0, 0xFFFFFFFF, 0xFFFFFFFF)

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

#ifdef _MSC_VER
#define EMT_MACRO_ARGS_CAP 127
#endif
