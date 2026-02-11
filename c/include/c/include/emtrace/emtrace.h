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

/// MGIC record: 'EMT' prefix (3) + offset bytes (3) + 'MGIC' type (4).
/// Followed by record_size (size_t), then meta[39], then padding, then size_t_meta[2].
///
/// Byte layout of framing[10]:
///   [0..2]  = 'E', 'M', 'T'
///   [3]     = offset from record start to type string  = 6
///   [4]     = offset from record start to record_size  = 10
///   [5]     = offset from record start to payload      = 10 + sizeof(size_t)
///   [6..9]  = 'M', 'G', 'I', 'C'
///
/// Byte layout of meta[39]:
///   [0..31] = 32-byte magic constant
///   [32]    = version number low byte  (= 0)
///   [33]    = version number high byte (= 0)
///   [34]    = offset from record start to size_t_meta  = offsetof(emt_magic_t, size_t_meta)
///   [35]    = sizeof(size_t)
///   [36]    = sizeof(emt_ptr_t)
///   [37]    = EMT_ALIGNMENT_POWER
///
/// size_t_meta[2]:
///   [0]     = byteorder id: 0x0706050403020100 (or truncated to sizeof(size_t))
///   [1]     = encoding id
typedef struct {
    uint8_t framing[10];   ///< 'E','M','T' + 3 offset bytes + 'M','G','I','C'
    size_t record_size;    ///< total size of this record in bytes
    uint8_t meta[38];      ///< magic(32) + version(2) + size_t_meta_offset(1) + sizeof_size_t(1) +
                           ///<   sizeof_emt_size_t(1) + sizeof_emt_ptr_t(1) + alignment_power(1)
    size_t size_t_meta[2]; ///< byteorder_id, encoding_id
} emt_magic_t;

void emt_default_begin(const void* info, size_t total_size, void* extra_arg);
void emt_default_out(const void* data, size_t size, void* extra_arg);
void emt_default_finish(const void* info, size_t total_size, void* extra_arg);
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
enum : size_t {
    // In the TRCE record format info, per-argument flag indicating whether dynamically sized data
    // is static (fixed size), null-terminated, or length-prefixed.
    EMT_FLAG_STATIC = 0,          ///< associated bytes have a fixed size
    EMT_FLAG_NULL_TERMINATED = 1, ///< associated bytes are variable in length and null-terminated
    EMT_FLAG_LENGTH_PREFIXED = 2, ///< associated bytes are variable in length, prefixed by a
                                  ///< size_t element count in the output stream

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
#define EMT_FLAG_STATIC ((size_t) 0)
/// In TRCE record format info: associated bytes are variable in length and null-terminated.
#define EMT_FLAG_NULL_TERMINATED ((size_t) 1)
/// In TRCE record format info: associated bytes are variable in length, prefixed by a size_t
/// element count in the output stream.
#define EMT_FLAG_LENGTH_PREFIXED ((size_t) 2)

/// Use python's str.format function for formatting.
#define EMT_PY_FORMAT ((size_t) 0)
/// Do not use any formatter; print the string as-is. All additional arguments are discarded.
#define EMT_NO_FORMAT ((size_t) 1)
/// Use python's C-style formatter
#define EMT_C_STYLE_FORMAT ((size_t) 2)

/// Send raw bytestream without any formatting.
#define EMT_ENCODING_NONE ((size_t) 0)
/// Use COBS encoding for the bytestream.
#define EMT_ENCODING_COBS ((size_t) 1)
#define EMT_ENCODING_COBS_PASSTHROUGH ((size_t) 2)
#endif

static inline void emt_out_file(const void* data, size_t size, FILE* file) {
    fwrite(data, 1, size, file);
}

static inline void emt_out_file_wrapper(const void* data, size_t size, void* extra_arg) {
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
    void (*out_fn)(const void*, size_t, void*),
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
    emt_cobs_state_t* cobs_state, void (*out_fn)(const void*, size_t, void*), void* extra_arg
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

// clang-format off
#define EMT_VAL(type, x)                    type,        x, sizeof(type),             ,            EMT_TAG_VAL
#define EMT_ARR(type, x)                    type,        x, sizeof(x) / sizeof(type), ,            EMT_TAG_ARR
#define EMT_STS(type, x, len)               type,        x, len,                      ,            EMT_TAG_STS
#define EMT_SLC(type, x, len)               type,        x, len,                      ,            EMT_TAG_SLC
#define EMT_ESLC(type, x, length_type, len) type,        x, len,                      length_type, EMT_TAG_ESLC
#define EMT_STR(x)                          const char*, x, strlen(x) + 1,            ,            EMT_TAG_STR
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
    fmt_info_attributes, formatter, length_type, out_fn, begin, finish, extra_arg, postfix, ...    \
)                                                                                                  \
    do {                                                                                           \
                                                                                                   \
        struct emt_info_unlikely_to_shadow_t {                                                     \
            uint8_t framing[10];                                                                   \
            size_t record_size;                                                                    \
            size_t layout                                                                          \
                [5 + EMT_F_LAYOUT_SIZE(                                                            \
                         EMT_NUM_ARGS_REST(__VA_ARGS__), EMT_REST_ARGS(__VA_ARGS__, 0)             \
                     )];                                                                           \
            char fmt[sizeof(EMT_FIRST_ARG(__VA_ARGS__, 0) postfix)];                               \
            EMT_F_INFO_MEMBER(EMT_NUM_ARGS_REST(__VA_ARGS__), EMT_REST_ARGS(__VA_ARGS__, 0))       \
            char file[sizeof(__FILE__)];                                                           \
        };                                                                                         \
        static fmt_info_attributes struct emt_info_unlikely_to_shadow_t                            \
            emt_info_unlikely_to_shadow = {                                                        \
                {'E', 'M', 'T', 6,                                                                 \
                 (uint8_t) offsetof(struct emt_info_unlikely_to_shadow_t, record_size),            \
                 (uint8_t) offsetof(struct emt_info_unlikely_to_shadow_t, layout), 'T', 'R', 'C',  \
                 'E'},                                                                             \
                sizeof(struct emt_info_unlikely_to_shadow_t),                                      \
                {EMT_NUM_ARGS_REST(__VA_ARGS__) / 5,                                               \
                 offsetof(struct emt_info_unlikely_to_shadow_t, fmt),                              \
                 EMT_F_LAYOUT(                                                                     \
                     length_type, EMT_NUM_ARGS_REST(__VA_ARGS__), EMT_REST_ARGS(__VA_ARGS__, 0)    \
                 ) formatter,                                                                      \
                 offsetof(struct emt_info_unlikely_to_shadow_t, file), __LINE__},                  \
                EMT_FIRST_ARG(__VA_ARGS__, 0) postfix,                                             \
                EMT_F_INFO(EMT_NUM_ARGS_REST(__VA_ARGS__), EMT_REST_ARGS(__VA_ARGS__, 0))          \
                    __FILE__,                                                                      \
        };                                                                                         \
        begin(                                                                                     \
            (const void*) &emt_info_unlikely_to_shadow,                                            \
            EMT_F_TOTAL_SIZE(EMT_NUM_ARGS_REST(__VA_ARGS__), EMT_REST_ARGS(__VA_ARGS__, 0)),       \
            extra_arg                                                                              \
        );                                                                                         \
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
            6,                                                                                     \
            (uint8_t) offsetof(emt_magic_t, record_size),                                          \
            (uint8_t) offsetof(emt_magic_t, meta),                                                 \
            'M',                                                                                   \
            'G',                                                                                   \
            'I',                                                                                   \
            'C',                                                                                   \
        },                                                                                         \
        sizeof(emt_magic_t),                                                                       \
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
            0,                                                                                     \
            0,                                                                                     \
            (uint8_t) offsetof(emt_magic_t, size_t_meta),                                          \
            (uint8_t) sizeof(size_t),                                                              \
            (uint8_t) sizeof(emt_ptr_t),                                                           \
            alignment_power,                                                                       \
        },                                                                                         \
        {                                                                                          \
            (size_t) 0x0706050403020100,                                                           \
            encoding,                                                                              \
        },                                                                                         \
    }

// for thread safety we want to lock stdout while writing a trace to it so that data from multiple
// traces cannot interleave
#if defined(unix) || defined(__unix) || defined(__unix__) ||                                       \
    (defined(__APPLE__) && defined(__MACH__))

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

#define EMT_FILE_COBS_PASSTHROUGH(attrs, emt_ptr_t, alignment_power, fp)                           \
    attrs emt_magic_t g_emt_default_magic =                                                        \
        EMT_MAGIC(EMT_ENCODING_COBS_PASSTHROUGH, emt_ptr_t, alignment_power);                      \
    static emt_cobs_state_t state;                                                                 \
    void emt_default_begin(const void* info, size_t total_size, void* extra_arg) {                 \
        (void) extra_arg; /* unused */                                                             \
        emt_cobs_init(&state);                                                                     \
        EMT_FLOCK_FILE(info, total_size, (fp));                                                    \
        emt_out_file("", 1, (fp));                                                                 \
        emt_ptr_t ptr = (emt_ptr_t) ((uintptr_t) info >> (alignment_power));                       \
        emt_default_out((const void*) &ptr, sizeof(emt_ptr_t), extra_arg);                         \
    }                                                                                              \
    void emt_default_out(const void* data, size_t total_size, void* extra_arg) {                   \
        (void) extra_arg; /* unused */                                                             \
        emt_cobs_encode(data, total_size, &state, emt_out_file_wrapper, (fp));                     \
    }                                                                                              \
    void emt_default_finish(const void* info, size_t total_size, void* extra_arg) {                \
        (void) extra_arg; /* unused */                                                             \
        emt_cobs_finalize(&state, emt_out_file_wrapper, (fp));                                     \
        EMT_FUNLOCK_FILE(info, total_size, (fp));                                                  \
    }

#define EMT_FILE_COBS(attrs, emt_ptr_t, alignment_power, fp)                                       \
    attrs emt_magic_t g_emt_default_magic =                                                        \
        EMT_MAGIC(EMT_ENCODING_COBS, emt_ptr_t, alignment_power);                                  \
    static emt_cobs_state_t state;                                                                 \
    void emt_default_begin(const void* info, size_t total_size, void* extra_arg) {                 \
        (void) extra_arg; /* unused */                                                             \
        emt_cobs_init(&state);                                                                     \
        EMT_FLOCK_FILE(info, total_size, (fp));                                                    \
        emt_ptr_t ptr = (emt_ptr_t) ((uintptr_t) info >> (alignment_power));                       \
        emt_default_out((const void*) &ptr, sizeof(emt_ptr_t), extra_arg);                         \
    }                                                                                              \
    void emt_default_out(const void* data, size_t total_size, void* extra_arg) {                   \
        (void) extra_arg; /* unused */                                                             \
        emt_cobs_encode(data, total_size, &state, emt_out_file_wrapper, (fp));                     \
    }                                                                                              \
    void emt_default_finish(const void* info, size_t total_size, void* extra_arg) {                \
        (void) extra_arg; /* unused */                                                             \
        emt_cobs_finalize(&state, emt_out_file_wrapper, (fp));                                     \
        EMT_FUNLOCK_FILE(info, total_size, (fp));                                                  \
    }

#define EMT_FILE(attrs, emt_ptr_t, alignment_power, fp)                                            \
    attrs emt_magic_t g_emt_default_magic =                                                        \
        EMT_MAGIC(EMT_ENCODING_NONE, emt_ptr_t, alignment_power);                                  \
    void emt_default_begin(const void* info, size_t total_size, void* extra_arg) {                 \
        (void) extra_arg; /* unused */                                                             \
        EMT_FLOCK_FILE(info, total_size, (fp));                                                    \
        emt_ptr_t ptr = (emt_ptr_t) ((uintptr_t) info >> (alignment_power));                       \
        emt_default_out((const void*) &ptr, sizeof(emt_ptr_t), extra_arg);                         \
    }                                                                                              \
    void emt_default_out(const void* data, size_t total_size, void* extra_arg) {                   \
        (void) extra_arg; /* unused */                                                             \
        emt_out_file(data, total_size, (fp));                                                      \
    }                                                                                              \
    void emt_default_finish(const void* info, size_t total_size, void* extra_arg) {                \
        (void) extra_arg; /* unused */                                                             \
        EMT_FUNLOCK_FILE(info, total_size, (fp));                                                  \
    }

#define EMT_INIT(magic_ptr, begin, finish, extra_arg)                                              \
    do {                                                                                           \
        begin((const void*) (magic_ptr), sizeof(emt_magic_t), extra_arg);                          \
        finish((const void*) (magic_ptr), sizeof(emt_magic_t), extra_arg);                         \
    } while (0)

#if defined(EMT_DEFAULT_SEC_ATTR) && defined(EMT_DEFAULT_ALIGNMENT_POWER)

#define EMTRACE_F(...)                                                                             \
    EMT_TRACE_F(                                                                                   \
        EMT_DEFAULT_SEC_ATTR, EMT_PY_FORMAT, EMT_DEFAULT_LENGTH_TYPE, emt_default_out,             \
        emt_default_begin, emt_default_finish, stdout, "", __VA_ARGS__                             \
    )
#define EMTRACE(str)                                                                               \
    EMT_TRACE_F(                                                                                   \
        EMT_DEFAULT_SEC_ATTR, EMT_NO_FORMAT, EMT_DEFAULT_LENGTH_TYPE, emt_default_out,             \
        emt_default_begin, emt_default_finish, stdout, "", str                                     \
    )

#define EMTRACELN_F(...)                                                                           \
    EMT_TRACE_F(                                                                                   \
        EMT_DEFAULT_SEC_ATTR, EMT_PY_FORMAT, EMT_DEFAULT_LENGTH_TYPE, emt_default_out,             \
        emt_default_begin, emt_default_finish, stdout, "\n", __VA_ARGS__                           \
    )
#define EMTRACELN(str)                                                                             \
    EMT_TRACE_F(                                                                                   \
        EMT_DEFAULT_SEC_ATTR, EMT_NO_FORMAT, EMT_DEFAULT_LENGTH_TYPE, emt_default_out,             \
        emt_default_begin, emt_default_finish, stdout, "", str "\n"                                \
    )

#define EMTRACE_MAGIC(emt_ptr_t)                                                                   \
    EMT_FILE(EMT_DEFAULT_SEC_ATTR, emt_ptr_t, EMT_DEFAULT_ALIGNMENT_POWER, stdout)

#define EMTRACE_MAGIC_COBS(emt_ptr_t)                                                              \
    EMT_FILE_COBS(EMT_DEFAULT_SEC_ATTR, emt_ptr_t, EMT_DEFAULT_ALIGNMENT_POWER, stdout)

#define EMTRACE_MAGIC_COBS_PASSTHROUGH(emt_ptr_t)                                                  \
    EMT_FILE_COBS_PASSTHROUGH(EMT_DEFAULT_SEC_ATTR, emt_ptr_t, EMT_DEFAULT_ALIGNMENT_POWER, stdout)

static inline void emtrace_init(void) {
    EMT_INIT(&g_emt_default_magic, emt_default_begin, emt_default_finish, stdout);
}

#endif // EMT_DEFAULT_SEC_ATTR

#ifdef _MSC_VER
#define EMT_MACRO_ARGS_CAP 127
#endif
#define EMT_F_LAYOUT_SIZE_DISPATCH_EMT_TAG_VAL() 4

#define EMT_F_LAYOUT_SIZE_DISPATCH_EMT_TAG_STR() 4

#define EMT_F_LAYOUT_SIZE_DISPATCH_EMT_TAG_ARR() 8

#define EMT_F_LAYOUT_SIZE_DISPATCH_EMT_TAG_SLC() 8

#define EMT_F_LAYOUT_SIZE_DISPATCH_EMT_TAG_ESLC() 8

#define EMT_F_LAYOUT_SIZE_DISPATCH_EMT_TAG_STS() 8

#define EMT_F_LAYOUT_SIZE_DISPATCH_HELPER(_tag) EMT_F_LAYOUT_SIZE_DISPATCH_##_tag()

#define EMT_F_LAYOUT_SIZE_DISPATCH(_tag) EMT_F_LAYOUT_SIZE_DISPATCH_HELPER(_tag)

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 1
#define EMT_F_LAYOUT_SIZE_0(_dummy) 0
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 6
#define EMT_F_LAYOUT_SIZE_5(type, x, len, length_type, tag, _dummy) EMT_F_LAYOUT_SIZE_DISPATCH(tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 11
#define EMT_F_LAYOUT_SIZE_10(_0, _1, _2, _3, _4, type, x, len, length_type, tag, _dummy)           \
    EMT_F_LAYOUT_SIZE_5(_0, _1, _2, _3, _4, 0) + EMT_F_LAYOUT_SIZE_DISPATCH(tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 16
#define EMT_F_LAYOUT_SIZE_15(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, type, x, len, length_type, tag, _dummy                 \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_10(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, 0) +                              \
        EMT_F_LAYOUT_SIZE_DISPATCH(tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 21
#define EMT_F_LAYOUT_SIZE_20(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, type, x, len, length_type, tag,    \
    _dummy                                                                                         \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_15(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, 0) +          \
        EMT_F_LAYOUT_SIZE_DISPATCH(tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 26
#define EMT_F_LAYOUT_SIZE_25(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, type, x,   \
    len, length_type, tag, _dummy                                                                  \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_20(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, 0      \
    ) + EMT_F_LAYOUT_SIZE_DISPATCH(tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 31
#define EMT_F_LAYOUT_SIZE_30(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, type, x, len, length_type, tag, _dummy                                          \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_25(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, 0                                                                      \
    ) + EMT_F_LAYOUT_SIZE_DISPATCH(tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 36
#define EMT_F_LAYOUT_SIZE_35(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, type, x, len, length_type, tag, _dummy                 \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_30(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, 0                                             \
    ) + EMT_F_LAYOUT_SIZE_DISPATCH(tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 41
#define EMT_F_LAYOUT_SIZE_40(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, type, x, len, length_type,    \
    tag, _dummy                                                                                    \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_35(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, 0                    \
    ) + EMT_F_LAYOUT_SIZE_DISPATCH(tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 46
#define EMT_F_LAYOUT_SIZE_45(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27,      \
    type, x, len, length_type, tag, _dummy                                                         \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_40(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, 0                                                                                     \
    ) + EMT_F_LAYOUT_SIZE_DISPATCH(tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 51
#define EMT_F_LAYOUT_SIZE_50(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, type, x, len, length_type, tag, _dummy                                     \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_45(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, 0                                                            \
    ) + EMT_F_LAYOUT_SIZE_DISPATCH(tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 56
#define EMT_F_LAYOUT_SIZE_55(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, type, x, len, length_type, tag, _dummy            \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_50(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, 0                                   \
    ) + EMT_F_LAYOUT_SIZE_DISPATCH(tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 61
#define EMT_F_LAYOUT_SIZE_60(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, type, x, len,            \
    length_type, tag, _dummy                                                                       \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_55(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, 0          \
    ) + EMT_F_LAYOUT_SIZE_DISPATCH(tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 66
#define EMT_F_LAYOUT_SIZE_65(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    type, x, len, length_type, tag, _dummy                                                         \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_60(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, 0                                                                           \
    ) + EMT_F_LAYOUT_SIZE_DISPATCH(tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 71
#define EMT_F_LAYOUT_SIZE_70(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, type, x, len, length_type, tag, _dummy                                \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_65(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, 0                                                  \
    ) + EMT_F_LAYOUT_SIZE_DISPATCH(tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 76
#define EMT_F_LAYOUT_SIZE_75(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, type, x, len, length_type, tag, _dummy       \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_70(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, 0                         \
    ) + EMT_F_LAYOUT_SIZE_DISPATCH(tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 81
#define EMT_F_LAYOUT_SIZE_80(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, type, x, len,       \
    length_type, tag, _dummy                                                                       \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_75(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        0                                                                                          \
    ) + EMT_F_LAYOUT_SIZE_DISPATCH(tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 86
#define EMT_F_LAYOUT_SIZE_85(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, type, x, len, length_type, tag, _dummy                                                    \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_80(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, 0                                                                 \
    ) + EMT_F_LAYOUT_SIZE_DISPATCH(tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 91
#define EMT_F_LAYOUT_SIZE_90(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, type, x, len, length_type, tag, _dummy                           \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_85(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, 0                                        \
    ) + EMT_F_LAYOUT_SIZE_DISPATCH(tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 96
#define EMT_F_LAYOUT_SIZE_95(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, type, x, len, length_type, tag, _dummy  \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_90(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, 0               \
    ) + EMT_F_LAYOUT_SIZE_DISPATCH(tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 101
#define EMT_F_LAYOUT_SIZE_100(                                                                     \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, type, x, len,  \
    length_type, tag, _dummy                                                                       \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_95(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, 0                                                                                \
    ) + EMT_F_LAYOUT_SIZE_DISPATCH(tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 106
#define EMT_F_LAYOUT_SIZE_105(                                                                     \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, type, x, len, length_type, tag, _dummy                                               \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_100(                                                                         \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, 0                                                       \
    ) + EMT_F_LAYOUT_SIZE_DISPATCH(tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 111
#define EMT_F_LAYOUT_SIZE_110(                                                                     \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, type, x, len, length_type, tag, _dummy                      \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_105(                                                                         \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, 0                              \
    ) + EMT_F_LAYOUT_SIZE_DISPATCH(tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 116
#define EMT_F_LAYOUT_SIZE_115(                                                                     \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, type, x, len, length_type, tag,    \
    _dummy                                                                                         \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_110(                                                                         \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, 0     \
    ) + EMT_F_LAYOUT_SIZE_DISPATCH(tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 121
#define EMT_F_LAYOUT_SIZE_120(                                                                     \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, type, x,  \
    len, length_type, tag, _dummy                                                                  \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_115(                                                                         \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e,  \
        _6f, _70, _71, _72, 0                                                                      \
    ) + EMT_F_LAYOUT_SIZE_DISPATCH(tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 126
#define EMT_F_LAYOUT_SIZE_125(                                                                     \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, \
    _75, _76, _77, type, x, len, length_type, tag, _dummy                                          \
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
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 131
#define EMT_F_LAYOUT_SIZE_130(                                                                     \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, \
    _75, _76, _77, _78, _79, _7a, _7b, _7c, type, x, len, length_type, tag, _dummy                 \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_125(                                                                         \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e,  \
        _6f, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _7a, _7b, _7c, 0                    \
    ) + EMT_F_LAYOUT_SIZE_DISPATCH(tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 136
#define EMT_F_LAYOUT_SIZE_135(                                                                     \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, \
    _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, _7f, _80, _81, type, x, len, length_type,    \
    tag, _dummy                                                                                    \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_130(                                                                         \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e,  \
        _6f, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, _7f, _80,  \
        _81, 0                                                                                     \
    ) + EMT_F_LAYOUT_SIZE_DISPATCH(tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 141
#define EMT_F_LAYOUT_SIZE_140(                                                                     \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, \
    _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, _7f, _80, _81, _82, _83, _84, _85, _86,      \
    type, x, len, length_type, tag, _dummy                                                         \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_135(                                                                         \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e,  \
        _6f, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, _7f, _80,  \
        _81, _82, _83, _84, _85, _86, 0                                                            \
    ) + EMT_F_LAYOUT_SIZE_DISPATCH(tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 146
#define EMT_F_LAYOUT_SIZE_145(                                                                     \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, \
    _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, _7f, _80, _81, _82, _83, _84, _85, _86, _87, \
    _88, _89, _8a, _8b, type, x, len, length_type, tag, _dummy                                     \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_140(                                                                         \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e,  \
        _6f, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, _7f, _80,  \
        _81, _82, _83, _84, _85, _86, _87, _88, _89, _8a, _8b, 0                                   \
    ) + EMT_F_LAYOUT_SIZE_DISPATCH(tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 151
#define EMT_F_LAYOUT_SIZE_150(                                                                     \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, \
    _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, _7f, _80, _81, _82, _83, _84, _85, _86, _87, \
    _88, _89, _8a, _8b, _8c, _8d, _8e, _8f, _90, type, x, len, length_type, tag, _dummy            \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_145(                                                                         \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e,  \
        _6f, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, _7f, _80,  \
        _81, _82, _83, _84, _85, _86, _87, _88, _89, _8a, _8b, _8c, _8d, _8e, _8f, _90, 0          \
    ) + EMT_F_LAYOUT_SIZE_DISPATCH(tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 156
#define EMT_F_LAYOUT_SIZE_155(                                                                     \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, \
    _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, _7f, _80, _81, _82, _83, _84, _85, _86, _87, \
    _88, _89, _8a, _8b, _8c, _8d, _8e, _8f, _90, _91, _92, _93, _94, _95, type, x, len,            \
    length_type, tag, _dummy                                                                       \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_150(                                                                         \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e,  \
        _6f, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, _7f, _80,  \
        _81, _82, _83, _84, _85, _86, _87, _88, _89, _8a, _8b, _8c, _8d, _8e, _8f, _90, _91, _92,  \
        _93, _94, _95, 0                                                                           \
    ) + EMT_F_LAYOUT_SIZE_DISPATCH(tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 161
#define EMT_F_LAYOUT_SIZE_160(                                                                     \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, \
    _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, _7f, _80, _81, _82, _83, _84, _85, _86, _87, \
    _88, _89, _8a, _8b, _8c, _8d, _8e, _8f, _90, _91, _92, _93, _94, _95, _96, _97, _98, _99, _9a, \
    type, x, len, length_type, tag, _dummy                                                         \
)                                                                                                  \
    EMT_F_LAYOUT_SIZE_155(                                                                         \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e,  \
        _6f, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, _7f, _80,  \
        _81, _82, _83, _84, _85, _86, _87, _88, _89, _8a, _8b, _8c, _8d, _8e, _8f, _90, _91, _92,  \
        _93, _94, _95, _96, _97, _98, _99, _9a, 0                                                  \
    ) + EMT_F_LAYOUT_SIZE_DISPATCH(tag)
#endif

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

#define EMT_F_INFO_MEMBER_DISPATCH_EMT_TAG_ESLC(type, name)                                        \
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

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 1
#define EMT_F_INFO_MEMBER_0(_dummy)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 6
#define EMT_F_INFO_MEMBER_5(type, x, len, length_type, tag, _dummy)                                \
    EMT_F_INFO_MEMBER_DISPATCH(type, type0, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 11
#define EMT_F_INFO_MEMBER_10(_0, _1, _2, _3, _4, type, x, len, length_type, tag, _dummy)           \
    EMT_F_INFO_MEMBER_5(_0, _1, _2, _3, _4, 0) EMT_F_INFO_MEMBER_DISPATCH(type, type1, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 16
#define EMT_F_INFO_MEMBER_15(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, type, x, len, length_type, tag, _dummy                 \
)                                                                                                  \
    EMT_F_INFO_MEMBER_10(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, 0)                                \
    EMT_F_INFO_MEMBER_DISPATCH(type, type2, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 21
#define EMT_F_INFO_MEMBER_20(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, type, x, len, length_type, tag,    \
    _dummy                                                                                         \
)                                                                                                  \
    EMT_F_INFO_MEMBER_15(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, 0)            \
    EMT_F_INFO_MEMBER_DISPATCH(type, type3, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 26
#define EMT_F_INFO_MEMBER_25(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, type, x,   \
    len, length_type, tag, _dummy                                                                  \
)                                                                                                  \
    EMT_F_INFO_MEMBER_20(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, 0      \
    )                                                                                              \
    EMT_F_INFO_MEMBER_DISPATCH(type, type4, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 31
#define EMT_F_INFO_MEMBER_30(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, type, x, len, length_type, tag, _dummy                                          \
)                                                                                                  \
    EMT_F_INFO_MEMBER_25(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, 0                                                                      \
    )                                                                                              \
    EMT_F_INFO_MEMBER_DISPATCH(type, type5, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 36
#define EMT_F_INFO_MEMBER_35(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, type, x, len, length_type, tag, _dummy                 \
)                                                                                                  \
    EMT_F_INFO_MEMBER_30(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, 0                                             \
    )                                                                                              \
    EMT_F_INFO_MEMBER_DISPATCH(type, type6, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 41
#define EMT_F_INFO_MEMBER_40(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, type, x, len, length_type,    \
    tag, _dummy                                                                                    \
)                                                                                                  \
    EMT_F_INFO_MEMBER_35(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, 0                    \
    )                                                                                              \
    EMT_F_INFO_MEMBER_DISPATCH(type, type7, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 46
#define EMT_F_INFO_MEMBER_45(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27,      \
    type, x, len, length_type, tag, _dummy                                                         \
)                                                                                                  \
    EMT_F_INFO_MEMBER_40(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, 0                                                                                     \
    )                                                                                              \
    EMT_F_INFO_MEMBER_DISPATCH(type, type8, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 51
#define EMT_F_INFO_MEMBER_50(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, type, x, len, length_type, tag, _dummy                                     \
)                                                                                                  \
    EMT_F_INFO_MEMBER_45(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, 0                                                            \
    )                                                                                              \
    EMT_F_INFO_MEMBER_DISPATCH(type, type9, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 56
#define EMT_F_INFO_MEMBER_55(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, type, x, len, length_type, tag, _dummy            \
)                                                                                                  \
    EMT_F_INFO_MEMBER_50(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, 0                                   \
    )                                                                                              \
    EMT_F_INFO_MEMBER_DISPATCH(type, type10, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 61
#define EMT_F_INFO_MEMBER_60(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, type, x, len,            \
    length_type, tag, _dummy                                                                       \
)                                                                                                  \
    EMT_F_INFO_MEMBER_55(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, 0          \
    )                                                                                              \
    EMT_F_INFO_MEMBER_DISPATCH(type, type11, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 66
#define EMT_F_INFO_MEMBER_65(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    type, x, len, length_type, tag, _dummy                                                         \
)                                                                                                  \
    EMT_F_INFO_MEMBER_60(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, 0                                                                           \
    )                                                                                              \
    EMT_F_INFO_MEMBER_DISPATCH(type, type12, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 71
#define EMT_F_INFO_MEMBER_70(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, type, x, len, length_type, tag, _dummy                                \
)                                                                                                  \
    EMT_F_INFO_MEMBER_65(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, 0                                                  \
    )                                                                                              \
    EMT_F_INFO_MEMBER_DISPATCH(type, type13, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 76
#define EMT_F_INFO_MEMBER_75(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, type, x, len, length_type, tag, _dummy       \
)                                                                                                  \
    EMT_F_INFO_MEMBER_70(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, 0                         \
    )                                                                                              \
    EMT_F_INFO_MEMBER_DISPATCH(type, type14, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 81
#define EMT_F_INFO_MEMBER_80(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, type, x, len,       \
    length_type, tag, _dummy                                                                       \
)                                                                                                  \
    EMT_F_INFO_MEMBER_75(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        0                                                                                          \
    )                                                                                              \
    EMT_F_INFO_MEMBER_DISPATCH(type, type15, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 86
#define EMT_F_INFO_MEMBER_85(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, type, x, len, length_type, tag, _dummy                                                    \
)                                                                                                  \
    EMT_F_INFO_MEMBER_80(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, 0                                                                 \
    )                                                                                              \
    EMT_F_INFO_MEMBER_DISPATCH(type, type16, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 91
#define EMT_F_INFO_MEMBER_90(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, type, x, len, length_type, tag, _dummy                           \
)                                                                                                  \
    EMT_F_INFO_MEMBER_85(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, 0                                        \
    )                                                                                              \
    EMT_F_INFO_MEMBER_DISPATCH(type, type17, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 96
#define EMT_F_INFO_MEMBER_95(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, type, x, len, length_type, tag, _dummy  \
)                                                                                                  \
    EMT_F_INFO_MEMBER_90(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, 0               \
    )                                                                                              \
    EMT_F_INFO_MEMBER_DISPATCH(type, type18, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 101
#define EMT_F_INFO_MEMBER_100(                                                                     \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, type, x, len,  \
    length_type, tag, _dummy                                                                       \
)                                                                                                  \
    EMT_F_INFO_MEMBER_95(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, 0                                                                                \
    )                                                                                              \
    EMT_F_INFO_MEMBER_DISPATCH(type, type19, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 106
#define EMT_F_INFO_MEMBER_105(                                                                     \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, type, x, len, length_type, tag, _dummy                                               \
)                                                                                                  \
    EMT_F_INFO_MEMBER_100(                                                                         \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, 0                                                       \
    )                                                                                              \
    EMT_F_INFO_MEMBER_DISPATCH(type, type20, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 111
#define EMT_F_INFO_MEMBER_110(                                                                     \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, type, x, len, length_type, tag, _dummy                      \
)                                                                                                  \
    EMT_F_INFO_MEMBER_105(                                                                         \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, 0                              \
    )                                                                                              \
    EMT_F_INFO_MEMBER_DISPATCH(type, type21, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 116
#define EMT_F_INFO_MEMBER_115(                                                                     \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, type, x, len, length_type, tag,    \
    _dummy                                                                                         \
)                                                                                                  \
    EMT_F_INFO_MEMBER_110(                                                                         \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, 0     \
    )                                                                                              \
    EMT_F_INFO_MEMBER_DISPATCH(type, type22, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 121
#define EMT_F_INFO_MEMBER_120(                                                                     \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, type, x,  \
    len, length_type, tag, _dummy                                                                  \
)                                                                                                  \
    EMT_F_INFO_MEMBER_115(                                                                         \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e,  \
        _6f, _70, _71, _72, 0                                                                      \
    )                                                                                              \
    EMT_F_INFO_MEMBER_DISPATCH(type, type23, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 126
#define EMT_F_INFO_MEMBER_125(                                                                     \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, \
    _75, _76, _77, type, x, len, length_type, tag, _dummy                                          \
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
    EMT_F_INFO_MEMBER_DISPATCH(type, type24, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 131
#define EMT_F_INFO_MEMBER_130(                                                                     \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, \
    _75, _76, _77, _78, _79, _7a, _7b, _7c, type, x, len, length_type, tag, _dummy                 \
)                                                                                                  \
    EMT_F_INFO_MEMBER_125(                                                                         \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e,  \
        _6f, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _7a, _7b, _7c, 0                    \
    )                                                                                              \
    EMT_F_INFO_MEMBER_DISPATCH(type, type25, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 136
#define EMT_F_INFO_MEMBER_135(                                                                     \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, \
    _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, _7f, _80, _81, type, x, len, length_type,    \
    tag, _dummy                                                                                    \
)                                                                                                  \
    EMT_F_INFO_MEMBER_130(                                                                         \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e,  \
        _6f, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, _7f, _80,  \
        _81, 0                                                                                     \
    )                                                                                              \
    EMT_F_INFO_MEMBER_DISPATCH(type, type26, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 141
#define EMT_F_INFO_MEMBER_140(                                                                     \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, \
    _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, _7f, _80, _81, _82, _83, _84, _85, _86,      \
    type, x, len, length_type, tag, _dummy                                                         \
)                                                                                                  \
    EMT_F_INFO_MEMBER_135(                                                                         \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e,  \
        _6f, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, _7f, _80,  \
        _81, _82, _83, _84, _85, _86, 0                                                            \
    )                                                                                              \
    EMT_F_INFO_MEMBER_DISPATCH(type, type27, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 146
#define EMT_F_INFO_MEMBER_145(                                                                     \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, \
    _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, _7f, _80, _81, _82, _83, _84, _85, _86, _87, \
    _88, _89, _8a, _8b, type, x, len, length_type, tag, _dummy                                     \
)                                                                                                  \
    EMT_F_INFO_MEMBER_140(                                                                         \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e,  \
        _6f, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, _7f, _80,  \
        _81, _82, _83, _84, _85, _86, _87, _88, _89, _8a, _8b, 0                                   \
    )                                                                                              \
    EMT_F_INFO_MEMBER_DISPATCH(type, type28, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 151
#define EMT_F_INFO_MEMBER_150(                                                                     \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, \
    _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, _7f, _80, _81, _82, _83, _84, _85, _86, _87, \
    _88, _89, _8a, _8b, _8c, _8d, _8e, _8f, _90, type, x, len, length_type, tag, _dummy            \
)                                                                                                  \
    EMT_F_INFO_MEMBER_145(                                                                         \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e,  \
        _6f, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, _7f, _80,  \
        _81, _82, _83, _84, _85, _86, _87, _88, _89, _8a, _8b, _8c, _8d, _8e, _8f, _90, 0          \
    )                                                                                              \
    EMT_F_INFO_MEMBER_DISPATCH(type, type29, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 156
#define EMT_F_INFO_MEMBER_155(                                                                     \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, \
    _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, _7f, _80, _81, _82, _83, _84, _85, _86, _87, \
    _88, _89, _8a, _8b, _8c, _8d, _8e, _8f, _90, _91, _92, _93, _94, _95, type, x, len,            \
    length_type, tag, _dummy                                                                       \
)                                                                                                  \
    EMT_F_INFO_MEMBER_150(                                                                         \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e,  \
        _6f, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, _7f, _80,  \
        _81, _82, _83, _84, _85, _86, _87, _88, _89, _8a, _8b, _8c, _8d, _8e, _8f, _90, _91, _92,  \
        _93, _94, _95, 0                                                                           \
    )                                                                                              \
    EMT_F_INFO_MEMBER_DISPATCH(type, type30, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 161
#define EMT_F_INFO_MEMBER_160(                                                                     \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, \
    _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, _7f, _80, _81, _82, _83, _84, _85, _86, _87, \
    _88, _89, _8a, _8b, _8c, _8d, _8e, _8f, _90, _91, _92, _93, _94, _95, _96, _97, _98, _99, _9a, \
    type, x, len, length_type, tag, _dummy                                                         \
)                                                                                                  \
    EMT_F_INFO_MEMBER_155(                                                                         \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e,  \
        _6f, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, _7f, _80,  \
        _81, _82, _83, _84, _85, _86, _87, _88, _89, _8a, _8b, _8c, _8d, _8e, _8f, _90, _91, _92,  \
        _93, _94, _95, _96, _97, _98, _99, _9a, 0                                                  \
    )                                                                                              \
    EMT_F_INFO_MEMBER_DISPATCH(type, type31, tag)
#endif

#define EMT_F_INFO_MEMBER(x, ...) EMT_F_INFO_MEMBER_HELPER(x, __VA_ARGS__)
#define EMT_F_INFO_MEMBER_HELPER(x, ...) EMT_F_INFO_MEMBER_##x(__VA_ARGS__)
#define EMT_F_LAYOUT_DISPATCH_EMT_TAG_VAL(name, len, type, length_type, default_length_type)       \
    offsetof(struct emt_info_unlikely_to_shadow_t, name), sizeof(type), EMT_FLAG_STATIC, 0,

#define EMT_F_LAYOUT_DISPATCH_EMT_TAG_STR(name, len, type, length_type, default_length_type)       \
    offsetof(struct emt_info_unlikely_to_shadow_t, name), (size_t) 1, EMT_FLAG_NULL_TERMINATED, 0,

#define EMT_F_LAYOUT_DISPATCH_EMT_TAG_ARR(name, len, type, length_type, default_length_type)       \
    offsetof(struct emt_info_unlikely_to_shadow_t, name), (size_t) 1 * (len), EMT_FLAG_STATIC, 1,  \
        offsetof(struct emt_info_unlikely_to_shadow_t, name##_child_name), sizeof(type),           \
        EMT_FLAG_STATIC, offsetof(struct emt_info_unlikely_to_shadow_t, name##_child_type_id),

#define EMT_F_LAYOUT_DISPATCH_EMT_TAG_STS(name, len, type, length_type, default_length_type)       \
    offsetof(struct emt_info_unlikely_to_shadow_t, name), (size_t) 1 * (len), EMT_FLAG_STATIC, 1,  \
        offsetof(struct emt_info_unlikely_to_shadow_t, name##_child_name), sizeof(type),           \
        EMT_FLAG_STATIC, offsetof(struct emt_info_unlikely_to_shadow_t, name##_child_type_id),

#define EMT_F_LAYOUT_DISPATCH_EMT_TAG_SLC(name, len, type, length_type, default_length_type)       \
    offsetof(struct emt_info_unlikely_to_shadow_t, name), sizeof(default_length_type),             \
        EMT_FLAG_LENGTH_PREFIXED, 1,                                                               \
        offsetof(struct emt_info_unlikely_to_shadow_t, name##_child_name), sizeof(type),           \
        EMT_FLAG_STATIC, offsetof(struct emt_info_unlikely_to_shadow_t, name##_child_type_id),

#define EMT_F_LAYOUT_DISPATCH_EMT_TAG_ESLC(name, len, type, length_type, default_length_type)      \
    offsetof(struct emt_info_unlikely_to_shadow_t, name), sizeof(length_type),                     \
        EMT_FLAG_LENGTH_PREFIXED, 1,                                                               \
        offsetof(struct emt_info_unlikely_to_shadow_t, name##_child_name), sizeof(type),           \
        EMT_FLAG_STATIC, offsetof(struct emt_info_unlikely_to_shadow_t, name##_child_type_id),

#define EMT_F_LAYOUT_DISPATCH_HELPER(name, len, type, length_type, default_length_type, _tag)      \
    EMT_F_LAYOUT_DISPATCH_##_tag(name, len, type, length_type, default_length_type)

#define EMT_F_LAYOUT_DISPATCH(name, len, type, length_type, default_length_type, _tag)             \
    EMT_F_LAYOUT_DISPATCH_HELPER(name, len, type, length_type, default_length_type, _tag)

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 2
#define EMT_F_LAYOUT_0(default_length_type, _dummy)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 7
#define EMT_F_LAYOUT_5(default_length_type, type, x, len, length_type, tag, _dummy)                \
    EMT_F_LAYOUT_DISPATCH(type0, len, type, length_type, default_length_type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 12
#define EMT_F_LAYOUT_10(                                                                           \
    default_length_type, _0, _1, _2, _3, _4, type, x, len, length_type, tag, _dummy                \
)                                                                                                  \
    EMT_F_LAYOUT_5(default_length_type, _0, _1, _2, _3, _4, 0)                                     \
    EMT_F_LAYOUT_DISPATCH(type1, len, type, length_type, default_length_type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 17
#define EMT_F_LAYOUT_15(                                                                           \
    default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, type, x, len, length_type, tag,   \
    _dummy                                                                                         \
)                                                                                                  \
    EMT_F_LAYOUT_10(default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, 0)                \
    EMT_F_LAYOUT_DISPATCH(type2, len, type, length_type, default_length_type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 22
#define EMT_F_LAYOUT_20(                                                                           \
    default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, type, x, len, \
    length_type, tag, _dummy                                                                       \
)                                                                                                  \
    EMT_F_LAYOUT_15(                                                                               \
        default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, 0         \
    )                                                                                              \
    EMT_F_LAYOUT_DISPATCH(type3, len, type, length_type, default_length_type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 27
#define EMT_F_LAYOUT_25(                                                                           \
    default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, \
    _12, _13, type, x, len, length_type, tag, _dummy                                               \
)                                                                                                  \
    EMT_F_LAYOUT_20(                                                                               \
        default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10,  \
        _11, _12, _13, 0                                                                           \
    )                                                                                              \
    EMT_F_LAYOUT_DISPATCH(type4, len, type, length_type, default_length_type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 32
#define EMT_F_LAYOUT_30(                                                                           \
    default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, \
    _12, _13, _14, _15, _16, _17, _18, type, x, len, length_type, tag, _dummy                      \
)                                                                                                  \
    EMT_F_LAYOUT_25(                                                                               \
        default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10,  \
        _11, _12, _13, _14, _15, _16, _17, _18, 0                                                  \
    )                                                                                              \
    EMT_F_LAYOUT_DISPATCH(type5, len, type, length_type, default_length_type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 37
#define EMT_F_LAYOUT_35(                                                                           \
    default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, \
    _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, type, x, len, length_type, tag,    \
    _dummy                                                                                         \
)                                                                                                  \
    EMT_F_LAYOUT_30(                                                                               \
        default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10,  \
        _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, 0                         \
    )                                                                                              \
    EMT_F_LAYOUT_DISPATCH(type6, len, type, length_type, default_length_type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 42
#define EMT_F_LAYOUT_40(                                                                           \
    default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, \
    _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, type, x,  \
    len, length_type, tag, _dummy                                                                  \
)                                                                                                  \
    EMT_F_LAYOUT_35(                                                                               \
        default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10,  \
        _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22,  \
        0                                                                                          \
    )                                                                                              \
    EMT_F_LAYOUT_DISPATCH(type7, len, type, length_type, default_length_type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 47
#define EMT_F_LAYOUT_45(                                                                           \
    default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, \
    _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, \
    _25, _26, _27, type, x, len, length_type, tag, _dummy                                          \
)                                                                                                  \
    EMT_F_LAYOUT_40(                                                                               \
        default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10,  \
        _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22,  \
        _23, _24, _25, _26, _27, 0                                                                 \
    )                                                                                              \
    EMT_F_LAYOUT_DISPATCH(type8, len, type, length_type, default_length_type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 52
#define EMT_F_LAYOUT_50(                                                                           \
    default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, \
    _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, \
    _25, _26, _27, _28, _29, _2a, _2b, _2c, type, x, len, length_type, tag, _dummy                 \
)                                                                                                  \
    EMT_F_LAYOUT_45(                                                                               \
        default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10,  \
        _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22,  \
        _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, 0                                        \
    )                                                                                              \
    EMT_F_LAYOUT_DISPATCH(type9, len, type, length_type, default_length_type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 57
#define EMT_F_LAYOUT_55(                                                                           \
    default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, \
    _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, \
    _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, type, x, len, length_type,    \
    tag, _dummy                                                                                    \
)                                                                                                  \
    EMT_F_LAYOUT_50(                                                                               \
        default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10,  \
        _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22,  \
        _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, 0               \
    )                                                                                              \
    EMT_F_LAYOUT_DISPATCH(type10, len, type, length_type, default_length_type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 62
#define EMT_F_LAYOUT_60(                                                                           \
    default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, \
    _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, \
    _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36,      \
    type, x, len, length_type, tag, _dummy                                                         \
)                                                                                                  \
    EMT_F_LAYOUT_55(                                                                               \
        default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10,  \
        _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22,  \
        _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34,  \
        _35, _36, 0                                                                                \
    )                                                                                              \
    EMT_F_LAYOUT_DISPATCH(type11, len, type, length_type, default_length_type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 67
#define EMT_F_LAYOUT_65(                                                                           \
    default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, \
    _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, \
    _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, \
    _38, _39, _3a, _3b, type, x, len, length_type, tag, _dummy                                     \
)                                                                                                  \
    EMT_F_LAYOUT_60(                                                                               \
        default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10,  \
        _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22,  \
        _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34,  \
        _35, _36, _37, _38, _39, _3a, _3b, 0                                                       \
    )                                                                                              \
    EMT_F_LAYOUT_DISPATCH(type12, len, type, length_type, default_length_type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 72
#define EMT_F_LAYOUT_70(                                                                           \
    default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, \
    _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, \
    _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, \
    _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, type, x, len, length_type, tag, _dummy            \
)                                                                                                  \
    EMT_F_LAYOUT_65(                                                                               \
        default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10,  \
        _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22,  \
        _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34,  \
        _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, 0                              \
    )                                                                                              \
    EMT_F_LAYOUT_DISPATCH(type13, len, type, length_type, default_length_type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 77
#define EMT_F_LAYOUT_75(                                                                           \
    default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, \
    _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, \
    _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, \
    _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, type, x, len,            \
    length_type, tag, _dummy                                                                       \
)                                                                                                  \
    EMT_F_LAYOUT_70(                                                                               \
        default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10,  \
        _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22,  \
        _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34,  \
        _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, 0     \
    )                                                                                              \
    EMT_F_LAYOUT_DISPATCH(type14, len, type, length_type, default_length_type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 82
#define EMT_F_LAYOUT_80(                                                                           \
    default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, \
    _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, \
    _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, \
    _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, \
    type, x, len, length_type, tag, _dummy                                                         \
)                                                                                                  \
    EMT_F_LAYOUT_75(                                                                               \
        default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10,  \
        _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22,  \
        _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34,  \
        _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46,  \
        _47, _48, _49, _4a, 0                                                                      \
    )                                                                                              \
    EMT_F_LAYOUT_DISPATCH(type15, len, type, length_type, default_length_type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 87
#define EMT_F_LAYOUT_85(                                                                           \
    default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, \
    _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, \
    _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, \
    _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, \
    _4b, _4c, _4d, _4e, _4f, type, x, len, length_type, tag, _dummy                                \
)                                                                                                  \
    EMT_F_LAYOUT_80(                                                                               \
        default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10,  \
        _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22,  \
        _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34,  \
        _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46,  \
        _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, 0                                             \
    )                                                                                              \
    EMT_F_LAYOUT_DISPATCH(type16, len, type, length_type, default_length_type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 92
#define EMT_F_LAYOUT_90(                                                                           \
    default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, \
    _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, \
    _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, \
    _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, \
    _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, type, x, len, length_type, tag, _dummy       \
)                                                                                                  \
    EMT_F_LAYOUT_85(                                                                               \
        default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10,  \
        _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22,  \
        _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34,  \
        _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46,  \
        _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, 0                    \
    )                                                                                              \
    EMT_F_LAYOUT_DISPATCH(type17, len, type, length_type, default_length_type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 97
#define EMT_F_LAYOUT_95(                                                                           \
    default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, \
    _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, \
    _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, \
    _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, \
    _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, type, x, len,       \
    length_type, tag, _dummy                                                                       \
)                                                                                                  \
    EMT_F_LAYOUT_90(                                                                               \
        default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10,  \
        _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22,  \
        _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34,  \
        _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46,  \
        _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58,  \
        _59, 0                                                                                     \
    )                                                                                              \
    EMT_F_LAYOUT_DISPATCH(type18, len, type, length_type, default_length_type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 102
#define EMT_F_LAYOUT_100(                                                                          \
    default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, \
    _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, \
    _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, \
    _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, \
    _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, \
    _5e, type, x, len, length_type, tag, _dummy                                                    \
)                                                                                                  \
    EMT_F_LAYOUT_95(                                                                               \
        default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10,  \
        _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22,  \
        _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34,  \
        _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46,  \
        _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58,  \
        _59, _5a, _5b, _5c, _5d, _5e, 0                                                            \
    )                                                                                              \
    EMT_F_LAYOUT_DISPATCH(type19, len, type, length_type, default_length_type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 107
#define EMT_F_LAYOUT_105(                                                                          \
    default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, \
    _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, \
    _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, \
    _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, \
    _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, \
    _5e, _5f, _60, _61, _62, _63, type, x, len, length_type, tag, _dummy                           \
)                                                                                                  \
    EMT_F_LAYOUT_100(                                                                              \
        default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10,  \
        _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22,  \
        _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34,  \
        _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46,  \
        _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58,  \
        _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, _62, _63, 0                                   \
    )                                                                                              \
    EMT_F_LAYOUT_DISPATCH(type20, len, type, length_type, default_length_type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 112
#define EMT_F_LAYOUT_110(                                                                          \
    default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, \
    _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, \
    _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, \
    _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, \
    _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, \
    _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, type, x, len, length_type, tag, _dummy  \
)                                                                                                  \
    EMT_F_LAYOUT_105(                                                                              \
        default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10,  \
        _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22,  \
        _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34,  \
        _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46,  \
        _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58,  \
        _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, 0          \
    )                                                                                              \
    EMT_F_LAYOUT_DISPATCH(type21, len, type, length_type, default_length_type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 117
#define EMT_F_LAYOUT_115(                                                                          \
    default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, \
    _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, \
    _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, \
    _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, \
    _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, \
    _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, type, x, len,  \
    length_type, tag, _dummy                                                                       \
)                                                                                                  \
    EMT_F_LAYOUT_110(                                                                              \
        default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10,  \
        _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22,  \
        _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34,  \
        _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46,  \
        _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58,  \
        _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a,  \
        _6b, _6c, _6d, 0                                                                           \
    )                                                                                              \
    EMT_F_LAYOUT_DISPATCH(type22, len, type, length_type, default_length_type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 122
#define EMT_F_LAYOUT_120(                                                                          \
    default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, \
    _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, \
    _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, \
    _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, \
    _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, \
    _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, \
    _71, _72, type, x, len, length_type, tag, _dummy                                               \
)                                                                                                  \
    EMT_F_LAYOUT_115(                                                                              \
        default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10,  \
        _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22,  \
        _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34,  \
        _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46,  \
        _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58,  \
        _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a,  \
        _6b, _6c, _6d, _6e, _6f, _70, _71, _72, 0                                                  \
    )                                                                                              \
    EMT_F_LAYOUT_DISPATCH(type23, len, type, length_type, default_length_type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 127
#define EMT_F_LAYOUT_125(                                                                          \
    default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, \
    _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, \
    _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, \
    _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, \
    _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, \
    _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, \
    _71, _72, _73, _74, _75, _76, _77, type, x, len, length_type, tag, _dummy                      \
)                                                                                                  \
    EMT_F_LAYOUT_120(                                                                              \
        default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10,  \
        _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22,  \
        _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34,  \
        _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46,  \
        _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58,  \
        _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a,  \
        _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, _75, _76, _77, 0                         \
    )                                                                                              \
    EMT_F_LAYOUT_DISPATCH(type24, len, type, length_type, default_length_type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 132
#define EMT_F_LAYOUT_130(                                                                          \
    default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, \
    _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, \
    _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, \
    _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, \
    _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, \
    _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, \
    _71, _72, _73, _74, _75, _76, _77, _78, _79, _7a, _7b, _7c, type, x, len, length_type, tag,    \
    _dummy                                                                                         \
)                                                                                                  \
    EMT_F_LAYOUT_125(                                                                              \
        default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10,  \
        _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22,  \
        _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34,  \
        _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46,  \
        _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58,  \
        _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a,  \
        _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _7a, _7b, _7c,  \
        0                                                                                          \
    )                                                                                              \
    EMT_F_LAYOUT_DISPATCH(type25, len, type, length_type, default_length_type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 137
#define EMT_F_LAYOUT_135(                                                                          \
    default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, \
    _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, \
    _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, \
    _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, \
    _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, \
    _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, \
    _71, _72, _73, _74, _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, _7f, _80, _81, type, x,  \
    len, length_type, tag, _dummy                                                                  \
)                                                                                                  \
    EMT_F_LAYOUT_130(                                                                              \
        default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10,  \
        _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22,  \
        _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34,  \
        _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46,  \
        _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58,  \
        _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a,  \
        _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _7a, _7b, _7c,  \
        _7d, _7e, _7f, _80, _81, 0                                                                 \
    )                                                                                              \
    EMT_F_LAYOUT_DISPATCH(type26, len, type, length_type, default_length_type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 142
#define EMT_F_LAYOUT_140(                                                                          \
    default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, \
    _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, \
    _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, \
    _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, \
    _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, \
    _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, \
    _71, _72, _73, _74, _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, _7f, _80, _81, _82, _83, \
    _84, _85, _86, type, x, len, length_type, tag, _dummy                                          \
)                                                                                                  \
    EMT_F_LAYOUT_135(                                                                              \
        default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10,  \
        _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22,  \
        _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34,  \
        _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46,  \
        _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58,  \
        _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a,  \
        _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _7a, _7b, _7c,  \
        _7d, _7e, _7f, _80, _81, _82, _83, _84, _85, _86, 0                                        \
    )                                                                                              \
    EMT_F_LAYOUT_DISPATCH(type27, len, type, length_type, default_length_type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 147
#define EMT_F_LAYOUT_145(                                                                          \
    default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, \
    _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, \
    _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, \
    _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, \
    _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, \
    _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, \
    _71, _72, _73, _74, _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, _7f, _80, _81, _82, _83, \
    _84, _85, _86, _87, _88, _89, _8a, _8b, type, x, len, length_type, tag, _dummy                 \
)                                                                                                  \
    EMT_F_LAYOUT_140(                                                                              \
        default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10,  \
        _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22,  \
        _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34,  \
        _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46,  \
        _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58,  \
        _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a,  \
        _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _7a, _7b, _7c,  \
        _7d, _7e, _7f, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _8a, _8b, 0               \
    )                                                                                              \
    EMT_F_LAYOUT_DISPATCH(type28, len, type, length_type, default_length_type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 152
#define EMT_F_LAYOUT_150(                                                                          \
    default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, \
    _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, \
    _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, \
    _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, \
    _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, \
    _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, \
    _71, _72, _73, _74, _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, _7f, _80, _81, _82, _83, \
    _84, _85, _86, _87, _88, _89, _8a, _8b, _8c, _8d, _8e, _8f, _90, type, x, len, length_type,    \
    tag, _dummy                                                                                    \
)                                                                                                  \
    EMT_F_LAYOUT_145(                                                                              \
        default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10,  \
        _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22,  \
        _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34,  \
        _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46,  \
        _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58,  \
        _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a,  \
        _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _7a, _7b, _7c,  \
        _7d, _7e, _7f, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _8a, _8b, _8c, _8d, _8e,  \
        _8f, _90, 0                                                                                \
    )                                                                                              \
    EMT_F_LAYOUT_DISPATCH(type29, len, type, length_type, default_length_type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 157
#define EMT_F_LAYOUT_155(                                                                          \
    default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, \
    _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, \
    _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, \
    _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, \
    _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, \
    _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, \
    _71, _72, _73, _74, _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, _7f, _80, _81, _82, _83, \
    _84, _85, _86, _87, _88, _89, _8a, _8b, _8c, _8d, _8e, _8f, _90, _91, _92, _93, _94, _95,      \
    type, x, len, length_type, tag, _dummy                                                         \
)                                                                                                  \
    EMT_F_LAYOUT_150(                                                                              \
        default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10,  \
        _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22,  \
        _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34,  \
        _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46,  \
        _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58,  \
        _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a,  \
        _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _7a, _7b, _7c,  \
        _7d, _7e, _7f, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _8a, _8b, _8c, _8d, _8e,  \
        _8f, _90, _91, _92, _93, _94, _95, 0                                                       \
    )                                                                                              \
    EMT_F_LAYOUT_DISPATCH(type30, len, type, length_type, default_length_type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 162
#define EMT_F_LAYOUT_160(                                                                          \
    default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, \
    _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, \
    _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, \
    _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, \
    _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, \
    _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, \
    _71, _72, _73, _74, _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, _7f, _80, _81, _82, _83, \
    _84, _85, _86, _87, _88, _89, _8a, _8b, _8c, _8d, _8e, _8f, _90, _91, _92, _93, _94, _95, _96, \
    _97, _98, _99, _9a, type, x, len, length_type, tag, _dummy                                     \
)                                                                                                  \
    EMT_F_LAYOUT_155(                                                                              \
        default_length_type, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10,  \
        _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22,  \
        _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34,  \
        _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46,  \
        _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58,  \
        _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a,  \
        _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _7a, _7b, _7c,  \
        _7d, _7e, _7f, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _8a, _8b, _8c, _8d, _8e,  \
        _8f, _90, _91, _92, _93, _94, _95, _96, _97, _98, _99, _9a, 0                              \
    )                                                                                              \
    EMT_F_LAYOUT_DISPATCH(type31, len, type, length_type, default_length_type, tag)
#endif

#define EMT_F_LAYOUT(default_length_type, x, ...)                                                  \
    EMT_F_LAYOUT_HELPER(default_length_type, x, __VA_ARGS__)
#define EMT_F_LAYOUT_HELPER(default_length_type, x, ...)                                           \
    EMT_F_LAYOUT_##x(default_length_type, __VA_ARGS__)
#define EMT_F_INFO_DISPATCH_EMT_TAG_VAL(type) #type,

#define EMT_F_INFO_DISPATCH_EMT_TAG_STR(type) "string",

#define EMT_F_INFO_DISPATCH_EMT_TAG_ARR(type) "list", "", #type,

#define EMT_F_INFO_DISPATCH_EMT_TAG_SLC(type) "list", "", #type,

#define EMT_F_INFO_DISPATCH_EMT_TAG_ESLC(type) "list", "", #type,

#define EMT_F_INFO_DISPATCH_EMT_TAG_STS(type) "list", "", #type,

#define EMT_F_INFO_DISPATCH_HELPER(type, _tag) EMT_F_INFO_DISPATCH_##_tag(type)

#define EMT_F_INFO_DISPATCH(type, _tag) EMT_F_INFO_DISPATCH_HELPER(type, _tag)

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 1
#define EMT_F_INFO_0(_dummy)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 6
#define EMT_F_INFO_5(type, x, len, length_type, tag, _dummy) EMT_F_INFO_DISPATCH(type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 11
#define EMT_F_INFO_10(_0, _1, _2, _3, _4, type, x, len, length_type, tag, _dummy)                  \
    EMT_F_INFO_5(_0, _1, _2, _3, _4, 0) EMT_F_INFO_DISPATCH(type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 16
#define EMT_F_INFO_15(                                                                             \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, type, x, len, length_type, tag, _dummy                 \
)                                                                                                  \
    EMT_F_INFO_10(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, 0) EMT_F_INFO_DISPATCH(type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 21
#define EMT_F_INFO_20(                                                                             \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, type, x, len, length_type, tag,    \
    _dummy                                                                                         \
)                                                                                                  \
    EMT_F_INFO_15(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, 0)                   \
    EMT_F_INFO_DISPATCH(type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 26
#define EMT_F_INFO_25(                                                                             \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, type, x,   \
    len, length_type, tag, _dummy                                                                  \
)                                                                                                  \
    EMT_F_INFO_20(                                                                                 \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, 0      \
    )                                                                                              \
    EMT_F_INFO_DISPATCH(type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 31
#define EMT_F_INFO_30(                                                                             \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, type, x, len, length_type, tag, _dummy                                          \
)                                                                                                  \
    EMT_F_INFO_25(                                                                                 \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, 0                                                                      \
    )                                                                                              \
    EMT_F_INFO_DISPATCH(type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 36
#define EMT_F_INFO_35(                                                                             \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, type, x, len, length_type, tag, _dummy                 \
)                                                                                                  \
    EMT_F_INFO_30(                                                                                 \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, 0                                             \
    )                                                                                              \
    EMT_F_INFO_DISPATCH(type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 41
#define EMT_F_INFO_40(                                                                             \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, type, x, len, length_type,    \
    tag, _dummy                                                                                    \
)                                                                                                  \
    EMT_F_INFO_35(                                                                                 \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, 0                    \
    )                                                                                              \
    EMT_F_INFO_DISPATCH(type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 46
#define EMT_F_INFO_45(                                                                             \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27,      \
    type, x, len, length_type, tag, _dummy                                                         \
)                                                                                                  \
    EMT_F_INFO_40(                                                                                 \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, 0                                                                                     \
    )                                                                                              \
    EMT_F_INFO_DISPATCH(type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 51
#define EMT_F_INFO_50(                                                                             \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, type, x, len, length_type, tag, _dummy                                     \
)                                                                                                  \
    EMT_F_INFO_45(                                                                                 \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, 0                                                            \
    )                                                                                              \
    EMT_F_INFO_DISPATCH(type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 56
#define EMT_F_INFO_55(                                                                             \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, type, x, len, length_type, tag, _dummy            \
)                                                                                                  \
    EMT_F_INFO_50(                                                                                 \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, 0                                   \
    )                                                                                              \
    EMT_F_INFO_DISPATCH(type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 61
#define EMT_F_INFO_60(                                                                             \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, type, x, len,            \
    length_type, tag, _dummy                                                                       \
)                                                                                                  \
    EMT_F_INFO_55(                                                                                 \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, 0          \
    )                                                                                              \
    EMT_F_INFO_DISPATCH(type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 66
#define EMT_F_INFO_65(                                                                             \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    type, x, len, length_type, tag, _dummy                                                         \
)                                                                                                  \
    EMT_F_INFO_60(                                                                                 \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, 0                                                                           \
    )                                                                                              \
    EMT_F_INFO_DISPATCH(type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 71
#define EMT_F_INFO_70(                                                                             \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, type, x, len, length_type, tag, _dummy                                \
)                                                                                                  \
    EMT_F_INFO_65(                                                                                 \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, 0                                                  \
    )                                                                                              \
    EMT_F_INFO_DISPATCH(type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 76
#define EMT_F_INFO_75(                                                                             \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, type, x, len, length_type, tag, _dummy       \
)                                                                                                  \
    EMT_F_INFO_70(                                                                                 \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, 0                         \
    )                                                                                              \
    EMT_F_INFO_DISPATCH(type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 81
#define EMT_F_INFO_80(                                                                             \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, type, x, len,       \
    length_type, tag, _dummy                                                                       \
)                                                                                                  \
    EMT_F_INFO_75(                                                                                 \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        0                                                                                          \
    )                                                                                              \
    EMT_F_INFO_DISPATCH(type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 86
#define EMT_F_INFO_85(                                                                             \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, type, x, len, length_type, tag, _dummy                                                    \
)                                                                                                  \
    EMT_F_INFO_80(                                                                                 \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, 0                                                                 \
    )                                                                                              \
    EMT_F_INFO_DISPATCH(type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 91
#define EMT_F_INFO_90(                                                                             \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, type, x, len, length_type, tag, _dummy                           \
)                                                                                                  \
    EMT_F_INFO_85(                                                                                 \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, 0                                        \
    )                                                                                              \
    EMT_F_INFO_DISPATCH(type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 96
#define EMT_F_INFO_95(                                                                             \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, type, x, len, length_type, tag, _dummy  \
)                                                                                                  \
    EMT_F_INFO_90(                                                                                 \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, 0               \
    )                                                                                              \
    EMT_F_INFO_DISPATCH(type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 101
#define EMT_F_INFO_100(                                                                            \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, type, x, len,  \
    length_type, tag, _dummy                                                                       \
)                                                                                                  \
    EMT_F_INFO_95(                                                                                 \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, 0                                                                                \
    )                                                                                              \
    EMT_F_INFO_DISPATCH(type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 106
#define EMT_F_INFO_105(                                                                            \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, type, x, len, length_type, tag, _dummy                                               \
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
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 111
#define EMT_F_INFO_110(                                                                            \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, type, x, len, length_type, tag, _dummy                      \
)                                                                                                  \
    EMT_F_INFO_105(                                                                                \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, 0                              \
    )                                                                                              \
    EMT_F_INFO_DISPATCH(type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 116
#define EMT_F_INFO_115(                                                                            \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, type, x, len, length_type, tag,    \
    _dummy                                                                                         \
)                                                                                                  \
    EMT_F_INFO_110(                                                                                \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, 0     \
    )                                                                                              \
    EMT_F_INFO_DISPATCH(type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 121
#define EMT_F_INFO_120(                                                                            \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, type, x,  \
    len, length_type, tag, _dummy                                                                  \
)                                                                                                  \
    EMT_F_INFO_115(                                                                                \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e,  \
        _6f, _70, _71, _72, 0                                                                      \
    )                                                                                              \
    EMT_F_INFO_DISPATCH(type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 126
#define EMT_F_INFO_125(                                                                            \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, \
    _75, _76, _77, type, x, len, length_type, tag, _dummy                                          \
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
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 131
#define EMT_F_INFO_130(                                                                            \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, \
    _75, _76, _77, _78, _79, _7a, _7b, _7c, type, x, len, length_type, tag, _dummy                 \
)                                                                                                  \
    EMT_F_INFO_125(                                                                                \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e,  \
        _6f, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _7a, _7b, _7c, 0                    \
    )                                                                                              \
    EMT_F_INFO_DISPATCH(type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 136
#define EMT_F_INFO_135(                                                                            \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, \
    _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, _7f, _80, _81, type, x, len, length_type,    \
    tag, _dummy                                                                                    \
)                                                                                                  \
    EMT_F_INFO_130(                                                                                \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e,  \
        _6f, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, _7f, _80,  \
        _81, 0                                                                                     \
    )                                                                                              \
    EMT_F_INFO_DISPATCH(type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 141
#define EMT_F_INFO_140(                                                                            \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, \
    _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, _7f, _80, _81, _82, _83, _84, _85, _86,      \
    type, x, len, length_type, tag, _dummy                                                         \
)                                                                                                  \
    EMT_F_INFO_135(                                                                                \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e,  \
        _6f, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, _7f, _80,  \
        _81, _82, _83, _84, _85, _86, 0                                                            \
    )                                                                                              \
    EMT_F_INFO_DISPATCH(type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 146
#define EMT_F_INFO_145(                                                                            \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, \
    _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, _7f, _80, _81, _82, _83, _84, _85, _86, _87, \
    _88, _89, _8a, _8b, type, x, len, length_type, tag, _dummy                                     \
)                                                                                                  \
    EMT_F_INFO_140(                                                                                \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e,  \
        _6f, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, _7f, _80,  \
        _81, _82, _83, _84, _85, _86, _87, _88, _89, _8a, _8b, 0                                   \
    )                                                                                              \
    EMT_F_INFO_DISPATCH(type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 151
#define EMT_F_INFO_150(                                                                            \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, \
    _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, _7f, _80, _81, _82, _83, _84, _85, _86, _87, \
    _88, _89, _8a, _8b, _8c, _8d, _8e, _8f, _90, type, x, len, length_type, tag, _dummy            \
)                                                                                                  \
    EMT_F_INFO_145(                                                                                \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e,  \
        _6f, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, _7f, _80,  \
        _81, _82, _83, _84, _85, _86, _87, _88, _89, _8a, _8b, _8c, _8d, _8e, _8f, _90, 0          \
    )                                                                                              \
    EMT_F_INFO_DISPATCH(type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 156
#define EMT_F_INFO_155(                                                                            \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, \
    _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, _7f, _80, _81, _82, _83, _84, _85, _86, _87, \
    _88, _89, _8a, _8b, _8c, _8d, _8e, _8f, _90, _91, _92, _93, _94, _95, type, x, len,            \
    length_type, tag, _dummy                                                                       \
)                                                                                                  \
    EMT_F_INFO_150(                                                                                \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e,  \
        _6f, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, _7f, _80,  \
        _81, _82, _83, _84, _85, _86, _87, _88, _89, _8a, _8b, _8c, _8d, _8e, _8f, _90, _91, _92,  \
        _93, _94, _95, 0                                                                           \
    )                                                                                              \
    EMT_F_INFO_DISPATCH(type, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 161
#define EMT_F_INFO_160(                                                                            \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, \
    _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, _7f, _80, _81, _82, _83, _84, _85, _86, _87, \
    _88, _89, _8a, _8b, _8c, _8d, _8e, _8f, _90, _91, _92, _93, _94, _95, _96, _97, _98, _99, _9a, \
    type, x, len, length_type, tag, _dummy                                                         \
)                                                                                                  \
    EMT_F_INFO_155(                                                                                \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e,  \
        _6f, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, _7f, _80,  \
        _81, _82, _83, _84, _85, _86, _87, _88, _89, _8a, _8b, _8c, _8d, _8e, _8f, _90, _91, _92,  \
        _93, _94, _95, _96, _97, _98, _99, _9a, 0                                                  \
    )                                                                                              \
    EMT_F_INFO_DISPATCH(type, tag)
#endif

#define EMT_F_INFO(x, ...) EMT_F_INFO_HELPER(x, __VA_ARGS__)
#define EMT_F_INFO_HELPER(x, ...) EMT_F_INFO_##x(__VA_ARGS__)
#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 1
#define EMT_F_TOTAL_SIZE_0(_dummy) 0
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 6
#define EMT_F_TOTAL_SIZE_5(type, x, len, length_type, tag, _dummy) (len)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 11
#define EMT_F_TOTAL_SIZE_10(_0, _1, _2, _3, _4, type, x, len, length_type, tag, _dummy)            \
    EMT_F_TOTAL_SIZE_5(_0, _1, _2, _3, _4, 0) + (len)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 16
#define EMT_F_TOTAL_SIZE_15(                                                                       \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, type, x, len, length_type, tag, _dummy                 \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_10(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, 0) + (len)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 21
#define EMT_F_TOTAL_SIZE_20(                                                                       \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, type, x, len, length_type, tag,    \
    _dummy                                                                                         \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_15(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, 0) + (len)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 26
#define EMT_F_TOTAL_SIZE_25(                                                                       \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, type, x,   \
    len, length_type, tag, _dummy                                                                  \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_20(                                                                           \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, 0      \
    ) + (len)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 31
#define EMT_F_TOTAL_SIZE_30(                                                                       \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, type, x, len, length_type, tag, _dummy                                          \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_25(                                                                           \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, 0                                                                      \
    ) + (len)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 36
#define EMT_F_TOTAL_SIZE_35(                                                                       \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, type, x, len, length_type, tag, _dummy                 \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_30(                                                                           \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, 0                                             \
    ) + (len)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 41
#define EMT_F_TOTAL_SIZE_40(                                                                       \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, type, x, len, length_type,    \
    tag, _dummy                                                                                    \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_35(                                                                           \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, 0                    \
    ) + (len)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 46
#define EMT_F_TOTAL_SIZE_45(                                                                       \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27,      \
    type, x, len, length_type, tag, _dummy                                                         \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_40(                                                                           \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, 0                                                                                     \
    ) + (len)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 51
#define EMT_F_TOTAL_SIZE_50(                                                                       \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, type, x, len, length_type, tag, _dummy                                     \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_45(                                                                           \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, 0                                                            \
    ) + (len)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 56
#define EMT_F_TOTAL_SIZE_55(                                                                       \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, type, x, len, length_type, tag, _dummy            \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_50(                                                                           \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, 0                                   \
    ) + (len)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 61
#define EMT_F_TOTAL_SIZE_60(                                                                       \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, type, x, len,            \
    length_type, tag, _dummy                                                                       \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_55(                                                                           \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, 0          \
    ) + (len)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 66
#define EMT_F_TOTAL_SIZE_65(                                                                       \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    type, x, len, length_type, tag, _dummy                                                         \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_60(                                                                           \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, 0                                                                           \
    ) + (len)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 71
#define EMT_F_TOTAL_SIZE_70(                                                                       \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, type, x, len, length_type, tag, _dummy                                \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_65(                                                                           \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, 0                                                  \
    ) + (len)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 76
#define EMT_F_TOTAL_SIZE_75(                                                                       \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, type, x, len, length_type, tag, _dummy       \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_70(                                                                           \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, 0                         \
    ) + (len)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 81
#define EMT_F_TOTAL_SIZE_80(                                                                       \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, type, x, len,       \
    length_type, tag, _dummy                                                                       \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_75(                                                                           \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        0                                                                                          \
    ) + (len)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 86
#define EMT_F_TOTAL_SIZE_85(                                                                       \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, type, x, len, length_type, tag, _dummy                                                    \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_80(                                                                           \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, 0                                                                 \
    ) + (len)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 91
#define EMT_F_TOTAL_SIZE_90(                                                                       \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, type, x, len, length_type, tag, _dummy                           \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_85(                                                                           \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, 0                                        \
    ) + (len)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 96
#define EMT_F_TOTAL_SIZE_95(                                                                       \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, type, x, len, length_type, tag, _dummy  \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_90(                                                                           \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, 0               \
    ) + (len)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 101
#define EMT_F_TOTAL_SIZE_100(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, type, x, len,  \
    length_type, tag, _dummy                                                                       \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_95(                                                                           \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, 0                                                                                \
    ) + (len)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 106
#define EMT_F_TOTAL_SIZE_105(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, type, x, len, length_type, tag, _dummy                                               \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_100(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, 0                                                       \
    ) + (len)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 111
#define EMT_F_TOTAL_SIZE_110(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, type, x, len, length_type, tag, _dummy                      \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_105(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, 0                              \
    ) + (len)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 116
#define EMT_F_TOTAL_SIZE_115(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, type, x, len, length_type, tag,    \
    _dummy                                                                                         \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_110(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, 0     \
    ) + (len)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 121
#define EMT_F_TOTAL_SIZE_120(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, type, x,  \
    len, length_type, tag, _dummy                                                                  \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_115(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e,  \
        _6f, _70, _71, _72, 0                                                                      \
    ) + (len)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 126
#define EMT_F_TOTAL_SIZE_125(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, \
    _75, _76, _77, type, x, len, length_type, tag, _dummy                                          \
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
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 131
#define EMT_F_TOTAL_SIZE_130(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, \
    _75, _76, _77, _78, _79, _7a, _7b, _7c, type, x, len, length_type, tag, _dummy                 \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_125(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e,  \
        _6f, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _7a, _7b, _7c, 0                    \
    ) + (len)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 136
#define EMT_F_TOTAL_SIZE_135(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, \
    _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, _7f, _80, _81, type, x, len, length_type,    \
    tag, _dummy                                                                                    \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_130(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e,  \
        _6f, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, _7f, _80,  \
        _81, 0                                                                                     \
    ) + (len)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 141
#define EMT_F_TOTAL_SIZE_140(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, \
    _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, _7f, _80, _81, _82, _83, _84, _85, _86,      \
    type, x, len, length_type, tag, _dummy                                                         \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_135(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e,  \
        _6f, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, _7f, _80,  \
        _81, _82, _83, _84, _85, _86, 0                                                            \
    ) + (len)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 146
#define EMT_F_TOTAL_SIZE_145(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, \
    _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, _7f, _80, _81, _82, _83, _84, _85, _86, _87, \
    _88, _89, _8a, _8b, type, x, len, length_type, tag, _dummy                                     \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_140(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e,  \
        _6f, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, _7f, _80,  \
        _81, _82, _83, _84, _85, _86, _87, _88, _89, _8a, _8b, 0                                   \
    ) + (len)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 151
#define EMT_F_TOTAL_SIZE_150(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, \
    _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, _7f, _80, _81, _82, _83, _84, _85, _86, _87, \
    _88, _89, _8a, _8b, _8c, _8d, _8e, _8f, _90, type, x, len, length_type, tag, _dummy            \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_145(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e,  \
        _6f, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, _7f, _80,  \
        _81, _82, _83, _84, _85, _86, _87, _88, _89, _8a, _8b, _8c, _8d, _8e, _8f, _90, 0          \
    ) + (len)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 156
#define EMT_F_TOTAL_SIZE_155(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, \
    _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, _7f, _80, _81, _82, _83, _84, _85, _86, _87, \
    _88, _89, _8a, _8b, _8c, _8d, _8e, _8f, _90, _91, _92, _93, _94, _95, type, x, len,            \
    length_type, tag, _dummy                                                                       \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_150(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e,  \
        _6f, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, _7f, _80,  \
        _81, _82, _83, _84, _85, _86, _87, _88, _89, _8a, _8b, _8c, _8d, _8e, _8f, _90, _91, _92,  \
        _93, _94, _95, 0                                                                           \
    ) + (len)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 161
#define EMT_F_TOTAL_SIZE_160(                                                                      \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14, _15,  \
    _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
    _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, \
    _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, \
    _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, \
    _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, _7f, _80, _81, _82, _83, _84, _85, _86, _87, \
    _88, _89, _8a, _8b, _8c, _8d, _8e, _8f, _90, _91, _92, _93, _94, _95, _96, _97, _98, _99, _9a, \
    type, x, len, length_type, tag, _dummy                                                         \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_155(                                                                          \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c, _d, _e, _f, _10, _11, _12, _13, _14,   \
        _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f, _20, _21, _22, _23, _24, _25, _26,  \
        _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, _33, _34, _35, _36, _37, _38,  \
        _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _4a,  \
        _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _5a, _5b, _5c,  \
        _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, _6c, _6d, _6e,  \
        _6f, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, _7f, _80,  \
        _81, _82, _83, _84, _85, _86, _87, _88, _89, _8a, _8b, _8c, _8d, _8e, _8f, _90, _91, _92,  \
        _93, _94, _95, _96, _97, _98, _99, _9a, 0                                                  \
    ) + (len)
#endif

#define EMT_F_TOTAL_SIZE(x, ...) EMT_F_TOTAL_SIZE_HELPER(x, __VA_ARGS__)
#define EMT_F_TOTAL_SIZE_HELPER(x, ...) EMT_F_TOTAL_SIZE_##x(__VA_ARGS__)
#define EMT_F_DISPATCH_EMT_TAG_VAL(                                                                \
    type, x, len, out_fn, length_type, default_length_type, extra_arg                              \
)                                                                                                  \
    {                                                                                              \
        type temp = x;                                                                             \
        out_fn((const void*) &temp, sizeof(type), extra_arg);                                      \
    }

#define EMT_F_DISPATCH_EMT_TAG_STR(                                                                \
    type, x, len, out_fn, length_type, default_length_type, extra_arg                              \
)                                                                                                  \
    {                                                                                              \
        const char* temp = x;                                                                      \
        out_fn(temp, strlen(temp) + 1, extra_arg);                                                 \
    }

#define EMT_F_DISPATCH_EMT_TAG_ARR(                                                                \
    type, x, len, out_fn, length_type, default_length_type, extra_arg                              \
)                                                                                                  \
    {                                                                                              \
        const type(*temp)[] = &(x);                                                                \
        out_fn(temp, sizeof(x), extra_arg);                                                        \
    }

#define EMT_F_DISPATCH_EMT_TAG_SLC(                                                                \
    type, x, len, out_fn, length_type, default_length_type, extra_arg                              \
)                                                                                                  \
    {                                                                                              \
        const type* emt_temp_unlikely_to_shadow = x;                                               \
        default_length_type size = (default_length_type) (len);                                    \
        out_fn(&size, sizeof(size), extra_arg);                                                    \
        out_fn(emt_temp_unlikely_to_shadow, size * sizeof(type), extra_arg);                       \
    }

#define EMT_F_DISPATCH_EMT_TAG_ESLC(                                                               \
    type, x, len, out_fn, length_type, default_length_type, extra_arg                              \
)                                                                                                  \
    {                                                                                              \
        const type* emt_temp_unlikely_to_shadow = x;                                               \
        length_type size = (length_type) (len);                                                    \
        out_fn(&size, sizeof(size), extra_arg);                                                    \
        out_fn(emt_temp_unlikely_to_shadow, size * sizeof(type), extra_arg);                       \
    }

#define EMT_F_DISPATCH_EMT_TAG_STS(                                                                \
    type, x, len, out_fn, length_type, default_length_type, extra_arg                              \
)                                                                                                  \
    {                                                                                              \
        const type* temp = x;                                                                      \
        out_fn(temp, sizeof(type) * (len), extra_arg);                                             \
    }

#define EMT_F_DISPATCH_HELPER(                                                                     \
    type, x, len, out_fn, length_type, default_length_type, extra_arg, _tag                        \
)                                                                                                  \
    EMT_F_DISPATCH_##_tag(type, x, len, out_fn, length_type, default_length_type, extra_arg)

#define EMT_F_DISPATCH(type, x, len, out_fn, length_type, default_length_type, extra_arg, _tag)    \
    EMT_F_DISPATCH_HELPER(type, x, len, out_fn, length_type, default_length_type, extra_arg, _tag)

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 4
#define EMT_F_0(out_fn, default_length_type, extra_arg, _dummy)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 9
#define EMT_F_5(out_fn, default_length_type, extra_arg, type, x, len, length_type, tag, _dummy)    \
    EMT_F_DISPATCH(type, x, len, out_fn, length_type, default_length_type, extra_arg, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 14
#define EMT_F_10(                                                                                  \
    out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, type, x, len, length_type, tag,    \
    _dummy                                                                                         \
)                                                                                                  \
    EMT_F_5(out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, 0)                         \
    EMT_F_DISPATCH(type, x, len, out_fn, length_type, default_length_type, extra_arg, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 19
#define EMT_F_15(                                                                                  \
    out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, type, x, len,  \
    length_type, tag, _dummy                                                                       \
)                                                                                                  \
    EMT_F_10(out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, 0)    \
    EMT_F_DISPATCH(type, x, len, out_fn, length_type, default_length_type, extra_arg, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 24
#define EMT_F_20(                                                                                  \
    out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c,    \
    _d, _e, type, x, len, length_type, tag, _dummy                                                 \
)                                                                                                  \
    EMT_F_15(                                                                                      \
        out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b,    \
        _c, _d, _e, 0                                                                              \
    )                                                                                              \
    EMT_F_DISPATCH(type, x, len, out_fn, length_type, default_length_type, extra_arg, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 29
#define EMT_F_25(                                                                                  \
    out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c,    \
    _d, _e, _f, _10, _11, _12, _13, type, x, len, length_type, tag, _dummy                         \
)                                                                                                  \
    EMT_F_20(                                                                                      \
        out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b,    \
        _c, _d, _e, _f, _10, _11, _12, _13, 0                                                      \
    )                                                                                              \
    EMT_F_DISPATCH(type, x, len, out_fn, length_type, default_length_type, extra_arg, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 34
#define EMT_F_30(                                                                                  \
    out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c,    \
    _d, _e, _f, _10, _11, _12, _13, _14, _15, _16, _17, _18, type, x, len, length_type, tag,       \
    _dummy                                                                                         \
)                                                                                                  \
    EMT_F_25(                                                                                      \
        out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b,    \
        _c, _d, _e, _f, _10, _11, _12, _13, _14, _15, _16, _17, _18, 0                             \
    )                                                                                              \
    EMT_F_DISPATCH(type, x, len, out_fn, length_type, default_length_type, extra_arg, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 39
#define EMT_F_35(                                                                                  \
    out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c,    \
    _d, _e, _f, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, type, x,     \
    len, length_type, tag, _dummy                                                                  \
)                                                                                                  \
    EMT_F_30(                                                                                      \
        out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b,    \
        _c, _d, _e, _f, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, 0    \
    )                                                                                              \
    EMT_F_DISPATCH(type, x, len, out_fn, length_type, default_length_type, extra_arg, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 44
#define EMT_F_40(                                                                                  \
    out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c,    \
    _d, _e, _f, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f,    \
    _20, _21, _22, type, x, len, length_type, tag, _dummy                                          \
)                                                                                                  \
    EMT_F_35(                                                                                      \
        out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b,    \
        _c, _d, _e, _f, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, \
        _1f, _20, _21, _22, 0                                                                      \
    )                                                                                              \
    EMT_F_DISPATCH(type, x, len, out_fn, length_type, default_length_type, extra_arg, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 49
#define EMT_F_45(                                                                                  \
    out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c,    \
    _d, _e, _f, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f,    \
    _20, _21, _22, _23, _24, _25, _26, _27, type, x, len, length_type, tag, _dummy                 \
)                                                                                                  \
    EMT_F_40(                                                                                      \
        out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b,    \
        _c, _d, _e, _f, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, \
        _1f, _20, _21, _22, _23, _24, _25, _26, _27, 0                                             \
    )                                                                                              \
    EMT_F_DISPATCH(type, x, len, out_fn, length_type, default_length_type, extra_arg, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 54
#define EMT_F_50(                                                                                  \
    out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c,    \
    _d, _e, _f, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f,    \
    _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, type, x, len, length_type,    \
    tag, _dummy                                                                                    \
)                                                                                                  \
    EMT_F_45(                                                                                      \
        out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b,    \
        _c, _d, _e, _f, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, \
        _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, 0                    \
    )                                                                                              \
    EMT_F_DISPATCH(type, x, len, out_fn, length_type, default_length_type, extra_arg, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 59
#define EMT_F_55(                                                                                  \
    out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c,    \
    _d, _e, _f, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f,    \
    _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31,      \
    type, x, len, length_type, tag, _dummy                                                         \
)                                                                                                  \
    EMT_F_50(                                                                                      \
        out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b,    \
        _c, _d, _e, _f, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, \
        _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30,  \
        _31, 0                                                                                     \
    )                                                                                              \
    EMT_F_DISPATCH(type, x, len, out_fn, length_type, default_length_type, extra_arg, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 64
#define EMT_F_60(                                                                                  \
    out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c,    \
    _d, _e, _f, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f,    \
    _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, \
    _33, _34, _35, _36, type, x, len, length_type, tag, _dummy                                     \
)                                                                                                  \
    EMT_F_55(                                                                                      \
        out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b,    \
        _c, _d, _e, _f, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, \
        _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30,  \
        _31, _32, _33, _34, _35, _36, 0                                                            \
    )                                                                                              \
    EMT_F_DISPATCH(type, x, len, out_fn, length_type, default_length_type, extra_arg, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 69
#define EMT_F_65(                                                                                  \
    out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c,    \
    _d, _e, _f, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f,    \
    _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, \
    _33, _34, _35, _36, _37, _38, _39, _3a, _3b, type, x, len, length_type, tag, _dummy            \
)                                                                                                  \
    EMT_F_60(                                                                                      \
        out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b,    \
        _c, _d, _e, _f, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, \
        _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30,  \
        _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, 0                                   \
    )                                                                                              \
    EMT_F_DISPATCH(type, x, len, out_fn, length_type, default_length_type, extra_arg, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 74
#define EMT_F_70(                                                                                  \
    out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c,    \
    _d, _e, _f, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f,    \
    _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, \
    _33, _34, _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, type, x, len,            \
    length_type, tag, _dummy                                                                       \
)                                                                                                  \
    EMT_F_65(                                                                                      \
        out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b,    \
        _c, _d, _e, _f, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, \
        _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30,  \
        _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, 0          \
    )                                                                                              \
    EMT_F_DISPATCH(type, x, len, out_fn, length_type, default_length_type, extra_arg, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 79
#define EMT_F_75(                                                                                  \
    out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c,    \
    _d, _e, _f, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f,    \
    _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, \
    _33, _34, _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, \
    type, x, len, length_type, tag, _dummy                                                         \
)                                                                                                  \
    EMT_F_70(                                                                                      \
        out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b,    \
        _c, _d, _e, _f, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, \
        _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30,  \
        _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42,  \
        _43, _44, _45, 0                                                                           \
    )                                                                                              \
    EMT_F_DISPATCH(type, x, len, out_fn, length_type, default_length_type, extra_arg, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 84
#define EMT_F_80(                                                                                  \
    out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c,    \
    _d, _e, _f, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f,    \
    _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, \
    _33, _34, _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _4a, type, x, len, length_type, tag, _dummy                                \
)                                                                                                  \
    EMT_F_75(                                                                                      \
        out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b,    \
        _c, _d, _e, _f, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, \
        _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30,  \
        _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42,  \
        _43, _44, _45, _46, _47, _48, _49, _4a, 0                                                  \
    )                                                                                              \
    EMT_F_DISPATCH(type, x, len, out_fn, length_type, default_length_type, extra_arg, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 89
#define EMT_F_85(                                                                                  \
    out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c,    \
    _d, _e, _f, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f,    \
    _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, \
    _33, _34, _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, type, x, len, length_type, tag, _dummy       \
)                                                                                                  \
    EMT_F_80(                                                                                      \
        out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b,    \
        _c, _d, _e, _f, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, \
        _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30,  \
        _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42,  \
        _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, 0                         \
    )                                                                                              \
    EMT_F_DISPATCH(type, x, len, out_fn, length_type, default_length_type, extra_arg, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 94
#define EMT_F_90(                                                                                  \
    out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c,    \
    _d, _e, _f, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f,    \
    _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, \
    _33, _34, _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, type, x, len,       \
    length_type, tag, _dummy                                                                       \
)                                                                                                  \
    EMT_F_85(                                                                                      \
        out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b,    \
        _c, _d, _e, _f, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, \
        _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30,  \
        _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42,  \
        _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54,  \
        0                                                                                          \
    )                                                                                              \
    EMT_F_DISPATCH(type, x, len, out_fn, length_type, default_length_type, extra_arg, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 99
#define EMT_F_95(                                                                                  \
    out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c,    \
    _d, _e, _f, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f,    \
    _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, \
    _33, _34, _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, \
    _59, type, x, len, length_type, tag, _dummy                                                    \
)                                                                                                  \
    EMT_F_90(                                                                                      \
        out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b,    \
        _c, _d, _e, _f, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, \
        _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30,  \
        _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42,  \
        _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54,  \
        _55, _56, _57, _58, _59, 0                                                                 \
    )                                                                                              \
    EMT_F_DISPATCH(type, x, len, out_fn, length_type, default_length_type, extra_arg, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 104
#define EMT_F_100(                                                                                 \
    out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c,    \
    _d, _e, _f, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f,    \
    _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, \
    _33, _34, _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, \
    _59, _5a, _5b, _5c, _5d, _5e, type, x, len, length_type, tag, _dummy                           \
)                                                                                                  \
    EMT_F_95(                                                                                      \
        out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b,    \
        _c, _d, _e, _f, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, \
        _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30,  \
        _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42,  \
        _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54,  \
        _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, 0                                        \
    )                                                                                              \
    EMT_F_DISPATCH(type, x, len, out_fn, length_type, default_length_type, extra_arg, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 109
#define EMT_F_105(                                                                                 \
    out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c,    \
    _d, _e, _f, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f,    \
    _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, \
    _33, _34, _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, \
    _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, _62, _63, type, x, len, length_type, tag, _dummy  \
)                                                                                                  \
    EMT_F_100(                                                                                     \
        out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b,    \
        _c, _d, _e, _f, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, \
        _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30,  \
        _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42,  \
        _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54,  \
        _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, _62, _63, 0               \
    )                                                                                              \
    EMT_F_DISPATCH(type, x, len, out_fn, length_type, default_length_type, extra_arg, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 114
#define EMT_F_110(                                                                                 \
    out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c,    \
    _d, _e, _f, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f,    \
    _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, \
    _33, _34, _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, \
    _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, type, x, len,  \
    length_type, tag, _dummy                                                                       \
)                                                                                                  \
    EMT_F_105(                                                                                     \
        out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b,    \
        _c, _d, _e, _f, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, \
        _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30,  \
        _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42,  \
        _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54,  \
        _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66,  \
        _67, _68, 0                                                                                \
    )                                                                                              \
    EMT_F_DISPATCH(type, x, len, out_fn, length_type, default_length_type, extra_arg, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 119
#define EMT_F_115(                                                                                 \
    out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c,    \
    _d, _e, _f, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f,    \
    _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, \
    _33, _34, _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, \
    _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, \
    _6c, _6d, type, x, len, length_type, tag, _dummy                                               \
)                                                                                                  \
    EMT_F_110(                                                                                     \
        out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b,    \
        _c, _d, _e, _f, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, \
        _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30,  \
        _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42,  \
        _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54,  \
        _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66,  \
        _67, _68, _69, _6a, _6b, _6c, _6d, 0                                                       \
    )                                                                                              \
    EMT_F_DISPATCH(type, x, len, out_fn, length_type, default_length_type, extra_arg, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 124
#define EMT_F_120(                                                                                 \
    out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c,    \
    _d, _e, _f, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f,    \
    _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, \
    _33, _34, _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, \
    _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, \
    _6c, _6d, _6e, _6f, _70, _71, _72, type, x, len, length_type, tag, _dummy                      \
)                                                                                                  \
    EMT_F_115(                                                                                     \
        out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b,    \
        _c, _d, _e, _f, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, \
        _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30,  \
        _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42,  \
        _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54,  \
        _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66,  \
        _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, 0                              \
    )                                                                                              \
    EMT_F_DISPATCH(type, x, len, out_fn, length_type, default_length_type, extra_arg, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 129
#define EMT_F_125(                                                                                 \
    out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c,    \
    _d, _e, _f, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f,    \
    _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, \
    _33, _34, _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, \
    _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, \
    _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, _75, _76, _77, type, x, len, length_type, tag,    \
    _dummy                                                                                         \
)                                                                                                  \
    EMT_F_120(                                                                                     \
        out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b,    \
        _c, _d, _e, _f, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, \
        _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30,  \
        _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42,  \
        _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54,  \
        _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66,  \
        _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, _75, _76, _77, 0     \
    )                                                                                              \
    EMT_F_DISPATCH(type, x, len, out_fn, length_type, default_length_type, extra_arg, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 134
#define EMT_F_130(                                                                                 \
    out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c,    \
    _d, _e, _f, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f,    \
    _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, \
    _33, _34, _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, \
    _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, \
    _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _7a, _7b, _7c, type, x,  \
    len, length_type, tag, _dummy                                                                  \
)                                                                                                  \
    EMT_F_125(                                                                                     \
        out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b,    \
        _c, _d, _e, _f, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, \
        _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30,  \
        _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42,  \
        _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54,  \
        _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66,  \
        _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, _75, _76, _77, _78,  \
        _79, _7a, _7b, _7c, 0                                                                      \
    )                                                                                              \
    EMT_F_DISPATCH(type, x, len, out_fn, length_type, default_length_type, extra_arg, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 139
#define EMT_F_135(                                                                                 \
    out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c,    \
    _d, _e, _f, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f,    \
    _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, \
    _33, _34, _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, \
    _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, \
    _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, \
    _7f, _80, _81, type, x, len, length_type, tag, _dummy                                          \
)                                                                                                  \
    EMT_F_130(                                                                                     \
        out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b,    \
        _c, _d, _e, _f, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, \
        _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30,  \
        _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42,  \
        _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54,  \
        _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66,  \
        _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, _75, _76, _77, _78,  \
        _79, _7a, _7b, _7c, _7d, _7e, _7f, _80, _81, 0                                             \
    )                                                                                              \
    EMT_F_DISPATCH(type, x, len, out_fn, length_type, default_length_type, extra_arg, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 144
#define EMT_F_140(                                                                                 \
    out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c,    \
    _d, _e, _f, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f,    \
    _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, \
    _33, _34, _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, \
    _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, \
    _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, \
    _7f, _80, _81, _82, _83, _84, _85, _86, type, x, len, length_type, tag, _dummy                 \
)                                                                                                  \
    EMT_F_135(                                                                                     \
        out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b,    \
        _c, _d, _e, _f, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, \
        _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30,  \
        _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42,  \
        _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54,  \
        _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66,  \
        _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, _75, _76, _77, _78,  \
        _79, _7a, _7b, _7c, _7d, _7e, _7f, _80, _81, _82, _83, _84, _85, _86, 0                    \
    )                                                                                              \
    EMT_F_DISPATCH(type, x, len, out_fn, length_type, default_length_type, extra_arg, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 149
#define EMT_F_145(                                                                                 \
    out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c,    \
    _d, _e, _f, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f,    \
    _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, \
    _33, _34, _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, \
    _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, \
    _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, \
    _7f, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _8a, _8b, type, x, len, length_type,    \
    tag, _dummy                                                                                    \
)                                                                                                  \
    EMT_F_140(                                                                                     \
        out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b,    \
        _c, _d, _e, _f, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, \
        _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30,  \
        _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42,  \
        _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54,  \
        _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66,  \
        _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, _75, _76, _77, _78,  \
        _79, _7a, _7b, _7c, _7d, _7e, _7f, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _8a,  \
        _8b, 0                                                                                     \
    )                                                                                              \
    EMT_F_DISPATCH(type, x, len, out_fn, length_type, default_length_type, extra_arg, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 154
#define EMT_F_150(                                                                                 \
    out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c,    \
    _d, _e, _f, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f,    \
    _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, \
    _33, _34, _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, \
    _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, \
    _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, \
    _7f, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _8a, _8b, _8c, _8d, _8e, _8f, _90,      \
    type, x, len, length_type, tag, _dummy                                                         \
)                                                                                                  \
    EMT_F_145(                                                                                     \
        out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b,    \
        _c, _d, _e, _f, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, \
        _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30,  \
        _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42,  \
        _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54,  \
        _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66,  \
        _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, _75, _76, _77, _78,  \
        _79, _7a, _7b, _7c, _7d, _7e, _7f, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _8a,  \
        _8b, _8c, _8d, _8e, _8f, _90, 0                                                            \
    )                                                                                              \
    EMT_F_DISPATCH(type, x, len, out_fn, length_type, default_length_type, extra_arg, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 159
#define EMT_F_155(                                                                                 \
    out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c,    \
    _d, _e, _f, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f,    \
    _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, \
    _33, _34, _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, \
    _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, \
    _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, \
    _7f, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _8a, _8b, _8c, _8d, _8e, _8f, _90, _91, \
    _92, _93, _94, _95, type, x, len, length_type, tag, _dummy                                     \
)                                                                                                  \
    EMT_F_150(                                                                                     \
        out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b,    \
        _c, _d, _e, _f, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, \
        _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30,  \
        _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42,  \
        _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54,  \
        _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66,  \
        _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, _75, _76, _77, _78,  \
        _79, _7a, _7b, _7c, _7d, _7e, _7f, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _8a,  \
        _8b, _8c, _8d, _8e, _8f, _90, _91, _92, _93, _94, _95, 0                                   \
    )                                                                                              \
    EMT_F_DISPATCH(type, x, len, out_fn, length_type, default_length_type, extra_arg, tag)
#endif

#if !defined(EMT_MACRO_ARGS_CAP) || EMT_MACRO_ARGS_CAP >= 164
#define EMT_F_160(                                                                                 \
    out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b, _c,    \
    _d, _e, _f, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, _1f,    \
    _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30, _31, _32, \
    _33, _34, _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42, _43, _44, _45, \
    _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54, _55, _56, _57, _58, \
    _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _6a, _6b, \
    _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, _75, _76, _77, _78, _79, _7a, _7b, _7c, _7d, _7e, \
    _7f, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _8a, _8b, _8c, _8d, _8e, _8f, _90, _91, \
    _92, _93, _94, _95, _96, _97, _98, _99, _9a, type, x, len, length_type, tag, _dummy            \
)                                                                                                  \
    EMT_F_155(                                                                                     \
        out_fn, default_length_type, extra_arg, _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _a, _b,    \
        _c, _d, _e, _f, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _1a, _1b, _1c, _1d, _1e, \
        _1f, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _2a, _2b, _2c, _2d, _2e, _2f, _30,  \
        _31, _32, _33, _34, _35, _36, _37, _38, _39, _3a, _3b, _3c, _3d, _3e, _3f, _40, _41, _42,  \
        _43, _44, _45, _46, _47, _48, _49, _4a, _4b, _4c, _4d, _4e, _4f, _50, _51, _52, _53, _54,  \
        _55, _56, _57, _58, _59, _5a, _5b, _5c, _5d, _5e, _5f, _60, _61, _62, _63, _64, _65, _66,  \
        _67, _68, _69, _6a, _6b, _6c, _6d, _6e, _6f, _70, _71, _72, _73, _74, _75, _76, _77, _78,  \
        _79, _7a, _7b, _7c, _7d, _7e, _7f, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _8a,  \
        _8b, _8c, _8d, _8e, _8f, _90, _91, _92, _93, _94, _95, _96, _97, _98, _99, _9a, 0          \
    )                                                                                              \
    EMT_F_DISPATCH(type, x, len, out_fn, length_type, default_length_type, extra_arg, tag)
#endif

#define EMT_F(out_fn, default_length_type, extra_arg, x, ...)                                      \
    EMT_F_HELPER(out_fn, default_length_type, extra_arg, x, __VA_ARGS__)
#define EMT_F_HELPER(out_fn, default_length_type, extra_arg, x, ...)                               \
    EMT_F_##x(out_fn, default_length_type, extra_arg, __VA_ARGS__)
// NOLINTEND(modernize-avoid-c-arrays)

#endif // EMTRACE_EMTRACE_H
