#include "emtrace_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/// Serialize `data` as part of the COBS frame that was started when `cobs_state` was last
/// initialized. Serialized output goes through `out_fn()`.
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

/// Finish serialization of COBS frame, started when `cobs_state` was last initialized.
/// Serialized output goes through `out_fn()`.
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

/// Serialize `len` as an LEB128 varint. Serialized output goes through `out_fn()`.
static inline void emt_serialize_varint(
    emt_size_t len, void (*out_fn)(const void*, emt_size_t, void*), void* extra_arg
) {
    unsigned char bytes[((sizeof(emt_size_t) * 8) + 6) / 7];

    emt_size_t remaining = len;
    for (size_t i = 0; i < sizeof(bytes); i++) {
        bytes[i] = (unsigned char) remaining & 0x7F;

        remaining >>= 7;
        if (remaining == 0) {
            out_fn(bytes, i + 1, extra_arg);
            return;
        }
        bytes[i] |= 0x80;
    }
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
#define EMT_VAL(type, x)                    type,        x, sizeof(type),             EMT_TAG_VAL
#define EMT_ARR(type, x)                    type,        x, sizeof(x) / sizeof(type), EMT_TAG_ARR
#define EMT_STS(type, x, len)               type,        x, len,                      EMT_TAG_STS
#define EMT_SLC(type, x, len)               type,        x, len,                      EMT_TAG_SLC
#define EMT_STR(x)                          const char*, x, EMT_STRLEN(x) + 1,        EMT_TAG_STR
// clang-format on

#ifndef EMT_NO_SIMPLE_MACROS
// NOLINTBEGIN(readability-identifier-naming)
#define VAL EMT_VAL
#define ARR EMT_ARR
#define STS EMT_STS
#define SLC EMT_SLC
#define STR EMT_STR
// NOLINTEND(readability-identifier-naming)
#endif

/**
 * @brief Emit a trace.
 *
 * Outputs a sequence of bytes, and creates a static variable (the `format-info`) through the
 * contents of which, the sequence of bytes can be decoded to the intended trace.
 *
 * @param fmt_info_attributes Prepends this to the definition of the format-info variable.
 *                            Can be used to e.g. change section placement or other
 *                            compiler-specific attributes.
 *
 * @param formatter           Formatter id: Specifies the formatter used, i.e. how the parser that
 *                            decodes the emitted trace should combine the format string and format
 *                            arguments to obtain the formatted string. Available formatters are
 *                              - EMT_NO_FORMAT (ignores format arguments, formatted string ==
 *                              format string)
 *                              - EMT_PY_FORMAT (uses python's `string.format` function)
 *                              - EMT_C_STYLE_FORMAT (uses python's C-style
 *                              `<format-string> % <format-arguments>` syntax)
 *
 * @param out_fn              A function or function-like macro through which the serialized trace
 *                            is streamed. Signature should be the same as e.g.
 *                            `emt_out_file_wrapper`. The argument itself is expanded in multiple
 *                            positions, so make sure that the expression has no unintended
 *                            side-effects.
 *
 * @param ptr_serializer      A function or function-like macro through which the pointer to the
 *                            format-info is serialized. Signature should be the same as e.g.
 *                            `emt_default_serialize_pointer`.
 *
 * @param begin               A function or function-like macro that is called once before
 *                            the trace is emitted. Signature:
 *                              - 1st argument: pointer to format-info
 *                              - 2nd argument: estimate of the total size of the emitted trace
 *                              - 3rd argument: `extra_arg` (argument of this macro) passed through
 *
 * @param finish              A function or function-like macro that is called once after the trace
 *                            was emitted. Signature is the same as for `begin`
 *
 * @param extra_arg           This argument is passed through to the invocations of `out_fn`,
 *                            `ptr_serializer`, `begin`, and `finish`. It is therefore expanded in
 *                            multiple positions, so make sure that the expression has no unintended
 *                            side-effects.
 *
 * @param postfix             is expanded after the format string in the format-info.
 *
 * @param ...                 The first vararg is the format string that, together with `postfix`,
 *                            ends up in the format-info, the rest of the varargs must each be an
 *                            invocation of either of the following macros:
 *                            EMT_VAL, EMT_ARR, EMT_STS, EMT_SLC, EMT_STR
 *                            *Passing anything else is not stable across versions of this C header*
 */
#define EMT_TRACE_F(                                                                               \
    fmt_info_attributes, formatter, out_fn, ptr_serializer, begin, finish, extra_arg, postfix, ... \
)                                                                                                  \
    do {                                                                                           \
                                                                                                   \
        struct emt_info_unlikely_to_shadow_t {                                                     \
            unsigned char framing[14];                                                             \
            emt_size_t record_size;                                                                \
            emt_size_t endianness_probe;                                                           \
            emt_size_t layout                                                                      \
                [7 + EMT_F_LAYOUT_SIZE(                                                            \
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
                {EMT_RUNTIME_DATA_TRACE_VERSION_MAJOR, EMT_RUNTIME_DATA_TRACE_VERSION_MINOR,       \
                 EMT_NUM_ARGS_REST(__VA_ARGS__) / 4,                                               \
                 EMT_OFFSETOF(struct emt_info_unlikely_to_shadow_t, fmt),                          \
                 EMT_F_LAYOUT(EMT_NUM_ARGS_REST(__VA_ARGS__), EMT_REST_ARGS(__VA_ARGS__, 0))       \
                     formatter,                                                                    \
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
        EMT_F(out_fn, extra_arg, EMT_NUM_ARGS_REST(__VA_ARGS__), EMT_REST_ARGS(__VA_ARGS__, 0))    \
        finish(                                                                                    \
            (const void*) &emt_info_unlikely_to_shadow,                                            \
            EMT_F_TOTAL_SIZE(EMT_NUM_ARGS_REST(__VA_ARGS__), EMT_REST_ARGS(__VA_ARGS__, 0)),       \
            extra_arg                                                                              \
        );                                                                                         \
    } while (0)

/**
 * @brief Create a MGIC record.
 *
 *
 * More precisely: This evaluates to an expression that can be assigned to variables of type
 * `emt_magic_t`. Defining the variable is what creates the MGIC record.
 *
 * @encoding        Promise of how the serialized traces will be encoded. Either EMT_ENCODING_NONE,
 *                  EMT_ENCODING_COBS, or EMT_ENCODING_COBS_PASSTHROUGH.
 *
 * @emt_ptr_len     Promise of how many bytes all pointers to format-info will have in the
 *                  serialized traces.
 *
 * @alignment_power Promise of how much all pointers to format-info will be right shifted before
 *                  being serialized.
 *
 * NOTE: all arguments are "promises" because by creating the MGIC record none of the described
 * properties are ensured. They are merely signals to the parser, so it knows how to decode the
 * stream. You must make sure independently that all runtime-emitted traces uphold these promises.
 */
#define EMT_MAGIC(encoding, emt_ptr_len, alignment_power)                                          \
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
            (unsigned char) (emt_ptr_len),                                                         \
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

#define EMT_FILE_COBS_PASSTHROUGH(attrs, emt_ptr_len, alignment_power, fp)                         \
    static emt_cobs_state_t emt_cobs_state;                                                        \
    static int emt_record_in_flight;                                                               \
    static unsigned char emt_ptr_bytes[emt_ptr_len];                                               \
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
        emt_ptr_t serialized_ptr = ((emt_ptr_t) ptr >> (alignment_power));                         \
        for (emt_size_t i = 0; i < sizeof(emt_ptr_bytes); i++) {                                   \
            emt_ptr_bytes[i] = (unsigned char) (serialized_ptr >> (i * 8));                        \
        }                                                                                          \
        out(emt_ptr_bytes, sizeof(emt_ptr_bytes), extra_arg);                                      \
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
        EMT_MAGIC(EMT_ENCODING_COBS_PASSTHROUGH, sizeof(emt_ptr_bytes), alignment_power)

#define EMT_FILE_COBS(attrs, emt_ptr_len, alignment_power, fp)                                     \
    static emt_cobs_state_t emt_cobs_state;                                                        \
    static int emt_record_in_flight;                                                               \
    static unsigned char emt_ptr_bytes[emt_ptr_len];                                               \
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
        emt_ptr_t serialized_ptr = ((emt_ptr_t) ptr >> (alignment_power));                         \
        for (emt_size_t i = 0; i < sizeof(emt_ptr_bytes); i++) {                                   \
            emt_ptr_bytes[i] = (unsigned char) (serialized_ptr >> (i * 8));                        \
        }                                                                                          \
        out(emt_ptr_bytes, sizeof(emt_ptr_bytes), extra_arg);                                      \
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
        EMT_MAGIC(EMT_ENCODING_COBS, sizeof(emt_ptr_bytes), alignment_power)

#define EMT_FILE(attrs, emt_ptr_len, alignment_power, fp)                                          \
    static int emt_record_in_flight;                                                               \
    static unsigned char emt_ptr_bytes[emt_ptr_len];                                               \
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
        emt_ptr_t serialized_ptr = ((emt_ptr_t) ptr >> (alignment_power));                         \
        for (emt_size_t i = 0; i < sizeof(emt_ptr_bytes); i++) {                                   \
            emt_ptr_bytes[i] = (unsigned char) (serialized_ptr >> (i * 8));                        \
        }                                                                                          \
        out(emt_ptr_bytes, sizeof(emt_ptr_bytes), extra_arg);                                      \
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
    attrs emt_magic_t g_emt_default_magic =                                                        \
        EMT_MAGIC(EMT_ENCODING_NONE, sizeof(emt_ptr_bytes), alignment_power)

#endif // EMT_FLOCK_FILE && EMT_FUNLOCK_FILE

#endif // not EMT_NO_LIBC

#define EMT_INIT(magic_ptr, serialize_ptr, out_fn, begin, finish, extra_arg)                       \
    do {                                                                                           \
        begin((const void*) (magic_ptr), sizeof(emt_magic_t), extra_arg);                          \
        serialize_ptr((const void*) (magic_ptr), out_fn, extra_arg);                               \
        finish((const void*) (magic_ptr), sizeof(emt_magic_t), extra_arg);                         \
    } while (0)

#if defined(EMT_DEFAULT_SEC_ATTR) && defined(EMT_DEFAULT_ALIGNMENT_POWER)

#define EMTRACE_F(...)                                                                             \
    EMT_TRACE_F(                                                                                   \
        EMT_DEFAULT_SEC_ATTR, EMT_PY_FORMAT, emt_default_out, emt_default_serialize_pointer,       \
        emt_default_begin, emt_default_finish, NULL, "", __VA_ARGS__                               \
    )
#define EMTRACE(str)                                                                               \
    EMT_TRACE_F(                                                                                   \
        EMT_DEFAULT_SEC_ATTR, EMT_NO_FORMAT, emt_default_out, emt_default_serialize_pointer,       \
        emt_default_begin, emt_default_finish, NULL, "", str                                       \
    )

#define EMTRACELN_F(...)                                                                           \
    EMT_TRACE_F(                                                                                   \
        EMT_DEFAULT_SEC_ATTR, EMT_PY_FORMAT, emt_default_out, emt_default_serialize_pointer,       \
        emt_default_begin, emt_default_finish, NULL, "\n", __VA_ARGS__                             \
    )
#define EMTRACELN(str)                                                                             \
    EMT_TRACE_F(                                                                                   \
        EMT_DEFAULT_SEC_ATTR, EMT_NO_FORMAT, emt_default_out, emt_default_serialize_pointer,       \
        emt_default_begin, emt_default_finish, NULL, "", str "\n"                                  \
    )

#ifdef EMT_FILE
#define EMTRACE_MAGIC(emt_ptr_len)                                                                 \
    EMT_FILE(EMT_DEFAULT_SEC_ATTR, emt_ptr_len, EMT_DEFAULT_ALIGNMENT_POWER, stdout)
#endif

#ifdef EMT_FILE_COBS
#define EMTRACE_MAGIC_COBS(emt_ptr_len)                                                            \
    EMT_FILE_COBS(EMT_DEFAULT_SEC_ATTR, emt_ptr_len, EMT_DEFAULT_ALIGNMENT_POWER, stdout)
#endif

#ifdef EMT_FILE_COBS_PASSTHROUGH
#define EMTRACE_MAGIC_COBS_PASSTHROUGH(emt_ptr_len)                                                \
    EMT_FILE_COBS_PASSTHROUGH(                                                                     \
        EMT_DEFAULT_SEC_ATTR, emt_ptr_len, EMT_DEFAULT_ALIGNMENT_POWER, stdout                     \
    )
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
#endif // not EMT_NO_LIBC

#endif // EMT_DEFAULT_SEC_ATTR

#ifdef _MSC_VER
#define EMT_MACRO_ARGS_CAP 127
#endif
