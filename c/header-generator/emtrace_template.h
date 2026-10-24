#ifndef EMT_NO_LIBC
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define EMT_OFFSETOF offsetof
#define EMT_STRLEN strlen
typedef size_t emt_size_t;
#else

#ifndef EMT_SIZE_T
typedef unsigned long emt_size_t;
#else
typedef EMT_SIZE_T emt_size_t;
#endif // EMT_SIZE_T

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
#define EMT_TRACE_VERSION_MAJOR 1
#define EMT_TRACE_VERSION_MINOR 0

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

#ifndef EMT_DEFAULT_LENGTH_TYPE
#define EMT_DEFAULT_LENGTH_TYPE unsigned int
#endif

#ifndef EMT_DEFAULT_ENCODING
#define EMT_DEFAULT_ENCODING EMT_ENCODING_NONE
#endif

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
#endif // EMT_NO_LIBC

typedef struct {
    unsigned char buffer[254];
    unsigned char pos;
} emt_cobs_state_t;

static inline void emt_cobs_init(emt_cobs_state_t* cobs_state) { cobs_state->pos = 0; }

static inline void emt_cobs_encode(
    const void* data,
    emt_size_t size,
    emt_cobs_state_t* cobs_state,
    void (*out_fn)(const void*, emt_size_t, void*),
    void* extra_arg
) {
    for (emt_size_t i = 0; i < size; i++) {
        unsigned char byte = ((const unsigned char*) data)[i];
        if (byte == 0) {
            unsigned char ptr = cobs_state->pos + 1;
            out_fn(&ptr, 1, extra_arg);
            out_fn(cobs_state->buffer, cobs_state->pos, extra_arg);
            cobs_state->pos = 0;
            continue;
        }
        if (cobs_state->pos == 254) {
            unsigned char ptr = 0xff;
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
        unsigned char ptr = cobs_state->pos + 1;
        out_fn(&ptr, 1, extra_arg);
        out_fn(cobs_state->buffer, cobs_state->pos, extra_arg);
    } else {
        unsigned char ptr = 1;
        out_fn(&ptr, 1, extra_arg);
    }
    unsigned char zero = 0;
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

// clang-format off
#define EMT_VAL(type, x)                    type,        x, sizeof(type),                        , EMT_TAG_VAL
#define EMT_ARR(type, x)                    type,        x, sizeof(x) / sizeof(type),            , EMT_TAG_ARR
#define EMT_STS(type, x, len)               type,        x, len,                                 , EMT_TAG_STS
#define EMT_SLC(type, x, len)               type,        x, len,                                 , EMT_TAG_SLC
#define EMT_ESLC(type, x, length_type, len) type,        x, len,                      length_type, EMT_TAG_ESLC
#define EMT_STR(x)                          const char*, x, EMT_STRLEN(x) + 1,                   , EMT_TAG_STR
// clang-format on

#ifndef EMT_NO_SIMPLE_MACROS
// NOLINTBEGIN(readability-identifier-naming)
#define VAL EMT_VAL
#define ARR EMT_ARR
#define STS EMT_STS
#define SLC EMT_SLC
#define ESLC EMT_ESLC
#define STR EMT_STR
// NOLINTEND(readability-identifier-naming)
#endif

#define EMT_TRACE_F(                                                                               \
    fmt_info_attributes, formatter, length_type, out_fn, ptr_serializer, begin, finish, extra_arg, \
    postfix, ...                                                                                   \
)                                                                                                  \
    do {                                                                                           \
                                                                                                   \
        struct emt_info_unlikely_to_shadow_t {                                                     \
            unsigned char framing[14];                                                             \
            emt_size_t record_size;                                                                \
            emt_size_t endianness_probe;                                                           \
            emt_size_t layout                                                                      \
                [5 + EMT_F_LAYOUT_SIZE(                                                            \
                         EMT_NUM_ARGS_REST(__VA_ARGS__), EMT_REST_ARGS(__VA_ARGS__, 0)             \
                     )];                                                                           \
            char fmt[sizeof(EMT_FIRST_ARG(__VA_ARGS__, 0) postfix)];                               \
            EMT_F_INFO_MEMBER(EMT_NUM_ARGS_REST(__VA_ARGS__), EMT_REST_ARGS(__VA_ARGS__, 0))       \
            char file[sizeof(__FILE__)];                                                           \
        };                                                                                         \
        static fmt_info_attributes struct emt_info_unlikely_to_shadow_t                            \
            emt_info_unlikely_to_shadow = {                                                        \
                {'E', 'M', 'T', 10,                                                                \
                 (unsigned char) EMT_OFFSETOF(struct emt_info_unlikely_to_shadow_t, record_size),  \
                 (unsigned char) EMT_OFFSETOF(struct emt_info_unlikely_to_shadow_t, layout),       \
                 (unsigned char)                                                                   \
                     EMT_OFFSETOF(struct emt_info_unlikely_to_shadow_t, endianness_probe),         \
                 (unsigned char) sizeof(emt_size_t), EMT_TRACE_VERSION_MAJOR,                      \
                 EMT_TRACE_VERSION_MINOR, 'T', 'R', 'C', 'E'},                                     \
                sizeof(struct emt_info_unlikely_to_shadow_t),                                      \
                EMT_ENDIANNESS_PROBE,                                                              \
                {EMT_NUM_ARGS_REST(__VA_ARGS__) / 5,                                               \
                 EMT_OFFSETOF(struct emt_info_unlikely_to_shadow_t, fmt),                          \
                 EMT_F_LAYOUT(                                                                     \
                     length_type, EMT_NUM_ARGS_REST(__VA_ARGS__), EMT_REST_ARGS(__VA_ARGS__, 0)    \
                 ) formatter,                                                                      \
                 EMT_OFFSETOF(struct emt_info_unlikely_to_shadow_t, file), __LINE__},              \
                EMT_FIRST_ARG(__VA_ARGS__, 0) postfix,                                             \
                EMT_F_INFO(EMT_NUM_ARGS_REST(__VA_ARGS__), EMT_REST_ARGS(__VA_ARGS__, 0))          \
                    __FILE__,                                                                      \
        };                                                                                         \
        begin(                                                                                     \
            (const void*) &emt_info_unlikely_to_shadow,                                            \
            EMT_F_TOTAL_SIZE(EMT_NUM_ARGS_REST(__VA_ARGS__), EMT_REST_ARGS(__VA_ARGS__, 0)),       \
            extra_arg                                                                              \
        );                                                                                         \
        ptr_serializer(&emt_info_unlikely_to_shadow, out_fn, extra_arg);                           \
                                                                                                   \
        EMT_F(                                                                                     \
            out_fn, length_type, extra_arg, EMT_NUM_ARGS_REST(__VA_ARGS__),                        \
            EMT_REST_ARGS(__VA_ARGS__, 0)                                                          \
        )                                                                                          \
        finish(                                                                                    \
            (const void*) &emt_info_unlikely_to_shadow,                                            \
            EMT_F_TOTAL_SIZE(EMT_NUM_ARGS_REST(__VA_ARGS__), EMT_REST_ARGS(__VA_ARGS__, 0)),       \
            extra_arg                                                                              \
        );                                                                                         \
    } while (0)

#define EMT_MAGIC(encoding, emt_ptr_t, alignment_power)                                            \
    {                                                                                              \
        {                                                                                          \
            'E',                                                                                   \
            'M',                                                                                   \
            'T',                                                                                   \
            10,                                                                                    \
            (unsigned char) EMT_OFFSETOF(emt_magic_t, record_size),                                \
            (unsigned char) EMT_OFFSETOF(emt_magic_t, meta),                                       \
            (unsigned char) EMT_OFFSETOF(emt_magic_t, endianness_probe),                           \
            (unsigned char) sizeof(emt_size_t),                                                    \
            EMT_MAGIC_VERSION_MAJOR,                                                               \
            EMT_MAGIC_VERSION_MINOR,                                                               \
            'M',                                                                                   \
            'G',                                                                                   \
            'I',                                                                                   \
            'C',                                                                                   \
        },                                                                                         \
        sizeof(emt_magic_t),                                                                       \
        EMT_ENDIANNESS_PROBE,                                                                      \
        {                                                                                          \
            0xd1,                                                                                  \
            0x97,                                                                                  \
            0xf5,                                                                                  \
            0x22,                                                                                  \
            0xd9,                                                                                  \
            0x26,                                                                                  \
            0x9f,                                                                                  \
            0xd1,                                                                                  \
            0xad,                                                                                  \
            0x70,                                                                                  \
            0x33,                                                                                  \
            0x92,                                                                                  \
            0xf6,                                                                                  \
            0x59,                                                                                  \
            0xdf,                                                                                  \
            0xd0,                                                                                  \
            0xfb,                                                                                  \
            0xec,                                                                                  \
            0xbd,                                                                                  \
            0x60,                                                                                  \
            0x97,                                                                                  \
            0x13,                                                                                  \
            0x25,                                                                                  \
            0xe8,                                                                                  \
            0x92,                                                                                  \
            0x01,                                                                                  \
            0xb2,                                                                                  \
            0x5a,                                                                                  \
            0x38,                                                                                  \
            0x5d,                                                                                  \
            0x9e,                                                                                  \
            0xc7,                                                                                  \
            (unsigned char) sizeof(emt_ptr_t),                                                     \
            alignment_power,                                                                       \
            (unsigned char) (encoding),                                                            \
        },                                                                                         \
    }

#ifndef EMT_NO_LIBC

// for thread safety we want to lock stdout while writing a trace to it so that data from multiple
// traces cannot interleave
#if _POSIX_C_SOURCE >= 199309L || _POSIX_C_SOURCE || _BSD_SOURCE || _SVID_SOURCE

#define EMT_FLOCK_FILE(x, y, file)                                                                 \
    do {                                                                                           \
        flockfile(file);                                                                           \
        (void) (x);                                                                                \
        (void) (y);                                                                                \
    } while (0)
#define EMT_FUNLOCK_FILE(x, y, file)                                                               \
    do {                                                                                           \
        funlockfile(file);                                                                         \
        (void) (x);                                                                                \
        (void) (y);                                                                                \
    } while (0)

#elif defined(_WIN32)

#include <windows.h>
#define EMT_FLOCK_FILE(x, y, file)                                                                 \
    do {                                                                                           \
        LockFile(file, 0, 0, 0xFFFFFFFF, 0xFFFFFFFF);                                              \
        (void) (x);                                                                                \
        (void) (y);                                                                                \
    } while (0)
#define EMT_FUNLOCK_FILE(x, y, file)                                                               \
    do {                                                                                           \
        UnlockFile(file, 0, 0, 0xFFFFFFFF, 0xFFFFFFFF);                                            \
        (void) (x);                                                                                \
        (void) (y);                                                                                \
    } while (0)

#endif

#define EMT_OUT_FILE_COBS(ptr, size, file)                                                         \
    emt_cobs_encode((ptr), (size), &state, emt_out_file_wrapper, file)

#if defined(EMT_FLOCK_FILE) && defined(EMT_FUNLOCK_FILE)

#define EMT_LOCK_FILE_COBS(x, y, file)                                                             \
    emt_cobs_state_t state;                                                                        \
    EMT_FLOCK_FILE(x, y, file)                                                                     \
    emt_cobs_init(&state);

#define EMT_LOCK_FILE_COBS_PASSTHROUGH(x, y, file)                                                 \
    EMT_LOCK_FILE_COBS(x, y, file);                                                                \
    EMT_FWRITE("", 1, 1, file);

#define EMT_UNLOCK_FILE_COBS(x, y, file)                                                           \
    emt_cobs_finalize(&state, emt_out_file_wrapper, file);                                         \
    EMT_FUNLOCK_FILE(x, y, file)

#define EMT_FILE_COBS_PASSTHROUGH(attrs, emt_ptr_t, alignment_power, fp)                           \
    static emt_cobs_state_t emt_cobs_state;                                                        \
    static int emt_record_in_flight;                                                               \
    void emt_default_begin(const void* info, emt_size_t total_size, void* extra_arg) {             \
        (void) extra_arg; /* unused */                                                             \
        assert(!emt_record_in_flight && "emtrace: nested or concurrent trace record in flight");   \
        emt_record_in_flight = 1;                                                                  \
        EMT_FLOCK_FILE(info, total_size, (fp));                                                    \
        emt_cobs_init(&emt_cobs_state);                                                            \
        emt_out_file("", 1, (fp));                                                                 \
    }                                                                                              \
    void emt_default_serialize_pointer(                                                            \
        const void* ptr, void (*out)(const void* data, emt_size_t size, void* extra_arg),          \
        void* extra_arg                                                                            \
    ) {                                                                                            \
        (void) extra_arg;                                                                          \
        emt_ptr_t serialized_ptr = (emt_ptr_t) ((uintptr_t) ptr >> (alignment_power));             \
        out((const void*) &serialized_ptr, sizeof(emt_ptr_t), extra_arg);                          \
    }                                                                                              \
    void emt_default_out(const void* data, emt_size_t total_size, void* extra_arg) {               \
        (void) extra_arg; /* unused */                                                             \
        emt_cobs_encode(data, total_size, &emt_cobs_state, emt_out_file_wrapper, (fp));            \
    }                                                                                              \
    void emt_default_finish(const void* info, emt_size_t total_size, void* extra_arg) {            \
        (void) extra_arg; /* unused */                                                             \
        emt_cobs_finalize(&emt_cobs_state, emt_out_file_wrapper, (fp));                            \
        EMT_FUNLOCK_FILE(info, total_size, (fp));                                                  \
        emt_record_in_flight = 0;                                                                  \
    }                                                                                              \
    attrs emt_magic_t g_emt_default_magic =                                                        \
        EMT_MAGIC(EMT_ENCODING_COBS_PASSTHROUGH, emt_ptr_t, alignment_power)

#define EMT_FILE_COBS(attrs, emt_ptr_t, alignment_power, fp)                                       \
    static emt_cobs_state_t emt_cobs_state;                                                        \
    static int emt_record_in_flight;                                                               \
    void emt_default_begin(const void* info, emt_size_t total_size, void* extra_arg) {             \
        (void) extra_arg; /* unused */                                                             \
        assert(!emt_record_in_flight && "emtrace: nested or concurrent trace record in flight");   \
        emt_record_in_flight = 1;                                                                  \
        EMT_FLOCK_FILE(info, total_size, (fp));                                                    \
        emt_cobs_init(&emt_cobs_state);                                                            \
    }                                                                                              \
    void emt_default_serialize_pointer(                                                            \
        const void* ptr, void (*out)(const void* data, emt_size_t size, void* extra_arg),          \
        void* extra_arg                                                                            \
    ) {                                                                                            \
        (void) extra_arg;                                                                          \
        emt_ptr_t serialized_ptr = (emt_ptr_t) ((uintptr_t) ptr >> (alignment_power));             \
        out((const void*) &serialized_ptr, sizeof(emt_ptr_t), extra_arg);                          \
    }                                                                                              \
    void emt_default_out(const void* data, emt_size_t total_size, void* extra_arg) {               \
        (void) extra_arg; /* unused */                                                             \
        emt_cobs_encode(data, total_size, &emt_cobs_state, emt_out_file_wrapper, (fp));            \
    }                                                                                              \
    void emt_default_finish(const void* info, emt_size_t total_size, void* extra_arg) {            \
        (void) extra_arg; /* unused */                                                             \
        emt_cobs_finalize(&emt_cobs_state, emt_out_file_wrapper, (fp));                            \
        EMT_FUNLOCK_FILE(info, total_size, (fp));                                                  \
        emt_record_in_flight = 0;                                                                  \
    }                                                                                              \
    attrs emt_magic_t g_emt_default_magic = EMT_MAGIC(EMT_ENCODING_COBS, emt_ptr_t, alignment_power)

#define EMT_FILE(attrs, emt_ptr_t, alignment_power, fp)                                            \
    static int emt_record_in_flight;                                                               \
    void emt_default_begin(const void* info, emt_size_t total_size, void* extra_arg) {             \
        (void) info;                                                                               \
        (void) total_size;                                                                         \
        (void) extra_arg; /* unused */                                                             \
        assert(!emt_record_in_flight && "emtrace: nested or concurrent trace record in flight");   \
        emt_record_in_flight = 1;                                                                  \
        EMT_FLOCK_FILE(info, total_size, (fp));                                                    \
    }                                                                                              \
    void emt_default_serialize_pointer(                                                            \
        const void* ptr, void (*out)(const void* data, emt_size_t size, void* extra_arg),          \
        void* extra_arg                                                                            \
    ) {                                                                                            \
        (void) extra_arg;                                                                          \
        emt_ptr_t serialized_ptr = (emt_ptr_t) ((uintptr_t) ptr >> (alignment_power));             \
        out((const void*) &serialized_ptr, sizeof(emt_ptr_t), extra_arg);                          \
    }                                                                                              \
    void emt_default_out(const void* data, emt_size_t total_size, void* extra_arg) {               \
        (void) extra_arg; /* unused */                                                             \
        emt_out_file(data, total_size, (fp));                                                      \
    }                                                                                              \
    void emt_default_finish(const void* info, emt_size_t total_size, void* extra_arg) {            \
        (void) info;                                                                               \
        (void) total_size;                                                                         \
        (void) extra_arg; /* unused */                                                             \
        EMT_FUNLOCK_FILE(info, total_size, (fp));                                                  \
        emt_record_in_flight = 0;                                                                  \
    }                                                                                              \
    attrs emt_magic_t g_emt_default_magic = EMT_MAGIC(EMT_ENCODING_NONE, emt_ptr_t, alignment_power)

#endif // EMT_FLOCK_FILE && EMT_FUNLOCK_FILE

#endif // EMT_NO_LIBC

#define EMT_INIT(magic_ptr, serialize_ptr, out_fn, begin, finish, extra_arg)                       \
    do {                                                                                           \
        begin((const void*) (magic_ptr), sizeof(emt_magic_t), extra_arg);                          \
        serialize_ptr((const void*) (magic_ptr), out_fn, extra_arg);                               \
        finish((const void*) (magic_ptr), sizeof(emt_magic_t), extra_arg);                         \
    } while (0)

#if defined(EMT_DEFAULT_SEC_ATTR) && defined(EMT_DEFAULT_ALIGNMENT_POWER)

#define EMTRACE_F(...)                                                                             \
    EMT_TRACE_F(                                                                                   \
        EMT_DEFAULT_SEC_ATTR, EMT_PY_FORMAT, EMT_DEFAULT_LENGTH_TYPE, emt_default_out,             \
        emt_default_serialize_pointer, emt_default_begin, emt_default_finish, NULL, "",            \
        __VA_ARGS__                                                                                \
    )
#define EMTRACE(str)                                                                               \
    EMT_TRACE_F(                                                                                   \
        EMT_DEFAULT_SEC_ATTR, EMT_NO_FORMAT, EMT_DEFAULT_LENGTH_TYPE, emt_default_out,             \
        emt_default_serialize_pointer, emt_default_begin, emt_default_finish, NULL, "", str        \
    )

#define EMTRACELN_F(...)                                                                           \
    EMT_TRACE_F(                                                                                   \
        EMT_DEFAULT_SEC_ATTR, EMT_PY_FORMAT, EMT_DEFAULT_LENGTH_TYPE, emt_default_out,             \
        emt_default_serialize_pointer, emt_default_begin, emt_default_finish, NULL, "\n",          \
        __VA_ARGS__                                                                                \
    )
#define EMTRACELN(str)                                                                             \
    EMT_TRACE_F(                                                                                   \
        EMT_DEFAULT_SEC_ATTR, EMT_NO_FORMAT, EMT_DEFAULT_LENGTH_TYPE, emt_default_out,             \
        emt_default_serialize_pointer, emt_default_begin, emt_default_finish, NULL, "", str "\n"   \
    )

#ifdef EMT_FILE
#define EMTRACE_MAGIC(emt_ptr_t)                                                                   \
    EMT_FILE(EMT_DEFAULT_SEC_ATTR, emt_ptr_t, EMT_DEFAULT_ALIGNMENT_POWER, stdout)
#endif

#ifdef EMT_FILE_COBS
#define EMTRACE_MAGIC_COBS(emt_ptr_t)                                                              \
    EMT_FILE_COBS(EMT_DEFAULT_SEC_ATTR, emt_ptr_t, EMT_DEFAULT_ALIGNMENT_POWER, stdout)
#endif

#ifdef EMT_FILE_COBS_PASSTHROUGH
#define EMTRACE_MAGIC_COBS_PASSTHROUGH(emt_ptr_t)                                                  \
    EMT_FILE_COBS_PASSTHROUGH(EMT_DEFAULT_SEC_ATTR, emt_ptr_t, EMT_DEFAULT_ALIGNMENT_POWER, stdout)
#endif

#ifndef EMT_NO_LIBC
static inline void emtrace_init(void) {
#ifdef _WIN32
    // this prevents byte `0xa` from being replaced with the sequence `0xd` `0xa` (carriage return)
    _setmode(_fileno(stdout), _O_BINARY);
#endif
    setvbuf(stdout, NULL, _IOFBF, 0);
    EMT_INIT(
        &g_emt_default_magic, emt_default_serialize_pointer, emt_default_out, emt_default_begin,
        emt_default_finish, stdout
    );
}
#endif // EMT_NO_LIBC

#endif // EMT_DEFAULT_SEC_ATTR

#ifdef _MSC_VER
#define EMT_MACRO_ARGS_CAP 127
#endif
