#include "emtrace_common.h"

#ifdef __cplusplus
extern "C" {
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
        // A full block is written with the code 0xff, which means that it is not followed by an
        // (implicit) zero. So it has to be written before looking at the next byte, even if that
        // is a zero.
        if (cobs_state->pos == 254) {
            unsigned char ptr = 0xff;
            out_fn(&ptr, 1, extra_arg);
            out_fn(cobs_state->buffer, 254, extra_arg);
            cobs_state->pos = 0;
        }
        if (byte == 0) {
            unsigned char ptr = cobs_state->pos + 1;
            out_fn(&ptr, 1, extra_arg);
            out_fn(cobs_state->buffer, cobs_state->pos, extra_arg);
            cobs_state->pos = 0;
            continue;
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

// clang-format off
#define EMT_ARG_VAL(type, x)      EMT_TAG_VAL(type, x)
#define EMT_ARG_ARR(type, x)      EMT_TAG_ARR(type, x)
#define EMT_ARG_STS(type, x, len) EMT_TAG_STS(type, x, len)
#define EMT_ARG_SLC(type, x, len) EMT_TAG_SLC(type, x, len)
#define EMT_ARG_STR(x)            EMT_TAG_STR(x)
// clang-format on

// NOLINTNEXTLINE(readability-identifier-naming) -- produced by EMT_ARG_## when __VA_ARGS__ is empty
#define EMT_ARG_ EMT_TAG_END
#define EMT_ARG_EMT_END EMT_TAG_END
#define EMT_OVERFLOW_EMT_END
#define EMT_OVERFLOW_EMT_ARG_EMT_END

#define EMT_APPLY_1_CALL(a, fn, ...) fn(a, __VA_ARGS__)
#define EMT_APPLY_1(a, ...) EMT_APPLY_1_CALL(a, __VA_ARGS__)
#define EMT_APPLY_2_CALL(a, b, fn, ...) fn(a, b, __VA_ARGS__)
#define EMT_APPLY_2(a, b, ...) EMT_APPLY_2_CALL(a, b, __VA_ARGS__)

// NOLINTBEGIN(bugprone-macro-parentheses) -- additive fragments expanded inside `(0 ...)`
// Argument count per tag
#define EMT_F_COUNT_EMT_TAG_VAL(type, x) +1
#define EMT_F_COUNT_EMT_TAG_ARR(type, x) +1
#define EMT_F_COUNT_EMT_TAG_STS(type, x, len) +1
#define EMT_F_COUNT_EMT_TAG_SLC(type, x, len) +1
#define EMT_F_COUNT_EMT_TAG_STR(x) +1
#define EMT_F_COUNT_EMT_TAG_END

// Layout entry count per tag: top-level type entry (5), plus child entry (6) for lists
// (see TRACE_FORMAT.md)
#define EMT_F_LAYOUT_SIZE_EMT_TAG_VAL(type, x) +5
#define EMT_F_LAYOUT_SIZE_EMT_TAG_STR(x) +5
#define EMT_F_LAYOUT_SIZE_EMT_TAG_ARR(type, x) +11
#define EMT_F_LAYOUT_SIZE_EMT_TAG_SLC(type, x, len) +11
#define EMT_F_LAYOUT_SIZE_EMT_TAG_STS(type, x, len) +11
#define EMT_F_LAYOUT_SIZE_EMT_TAG_END
// NOLINTEND(bugprone-macro-parentheses)

// Format-info struct member declarations per tag
#define EMT_F_INFO_MEMBER_VAL(name, type) char name[sizeof(#type)];
#define EMT_F_INFO_MEMBER_STR(name, _unused) char name[sizeof("string")];
#define EMT_F_INFO_MEMBER_LIST(name, type)                                                         \
    char name[sizeof("list")];                                                                     \
    char name##_child_name[sizeof("")];                                                            \
    char name##_child_type_id[sizeof(#type)];
#define EMT_F_INFO_MEMBER_END(name, _unused)

#define EMT_F_INFO_MEMBER_EMT_TAG_VAL(type, x) EMT_F_INFO_MEMBER_VAL, type
#define EMT_F_INFO_MEMBER_EMT_TAG_STR(x) EMT_F_INFO_MEMBER_STR, 0
#define EMT_F_INFO_MEMBER_EMT_TAG_ARR(type, x) EMT_F_INFO_MEMBER_LIST, type
#define EMT_F_INFO_MEMBER_EMT_TAG_SLC(type, x, len) EMT_F_INFO_MEMBER_LIST, type
#define EMT_F_INFO_MEMBER_EMT_TAG_STS(type, x, len) EMT_F_INFO_MEMBER_LIST, type
#define EMT_F_INFO_MEMBER_EMT_TAG_END EMT_F_INFO_MEMBER_END, 0
#define EMT_F_INFO_MEMBER_DISPATCH(name, arg) EMT_APPLY_1(name, EMT_F_INFO_MEMBER_##arg)

// Format-info layout entries per tag:
// Top-level type entry: [type_id_offset, size, flag, padding, num_children].
// Child entry: [name_offset, type_id_offset, size, flag, padding, num_children=0].
#define EMT_F_LAYOUT_VAL(name, type)                                                               \
    EMT_OFFSETOF(struct emt_info_unlikely_to_shadow_t, name), sizeof(type), EMT_FLAG_STATIC, 0, 0,
#define EMT_F_LAYOUT_STR(name, _unused)                                                            \
    EMT_OFFSETOF(struct emt_info_unlikely_to_shadow_t, name), (emt_size_t) 1,                      \
        EMT_FLAG_NULL_TERMINATED, 0, 0,
#define EMT_F_LAYOUT_STATIC_LIST(name, type, len)                                                  \
    EMT_OFFSETOF(struct emt_info_unlikely_to_shadow_t, name), (emt_size_t) 1 * (len),              \
        EMT_FLAG_STATIC, 0, 1,                                                                     \
        EMT_OFFSETOF(struct emt_info_unlikely_to_shadow_t, name##_child_name),                     \
        EMT_OFFSETOF(struct emt_info_unlikely_to_shadow_t, name##_child_type_id), sizeof(type),    \
        EMT_FLAG_STATIC, 0, 0,
#define EMT_F_LAYOUT_SLC(name, type)                                                               \
    EMT_OFFSETOF(struct emt_info_unlikely_to_shadow_t, name), 0, EMT_FLAG_LENGTH_PREFIXED, 0, 1,   \
        EMT_OFFSETOF(struct emt_info_unlikely_to_shadow_t, name##_child_name),                     \
        EMT_OFFSETOF(struct emt_info_unlikely_to_shadow_t, name##_child_type_id), sizeof(type),    \
        EMT_FLAG_STATIC, 0, 0,
#define EMT_F_LAYOUT_END(name, _unused)

#define EMT_F_LAYOUT_EMT_TAG_VAL(type, x) EMT_F_LAYOUT_VAL, type
#define EMT_F_LAYOUT_EMT_TAG_STR(x) EMT_F_LAYOUT_STR, 0
#define EMT_F_LAYOUT_EMT_TAG_ARR(type, x) EMT_F_LAYOUT_STATIC_LIST, type, sizeof(x) / sizeof(type)
#define EMT_F_LAYOUT_EMT_TAG_STS(type, x, len) EMT_F_LAYOUT_STATIC_LIST, type, len
#define EMT_F_LAYOUT_EMT_TAG_SLC(type, x, len) EMT_F_LAYOUT_SLC, type
#define EMT_F_LAYOUT_EMT_TAG_END EMT_F_LAYOUT_END, 0
#define EMT_F_LAYOUT_DISPATCH(name, arg) EMT_APPLY_1(name, EMT_F_LAYOUT_##arg)

// Format-info type-id string initializers per tag
#define EMT_F_INFO_EMT_TAG_VAL(type, x) #type,
#define EMT_F_INFO_EMT_TAG_STR(x) "string",
#define EMT_F_INFO_EMT_TAG_ARR(type, x) "list", "", #type,
#define EMT_F_INFO_EMT_TAG_SLC(type, x, len) "list", "", #type,
#define EMT_F_INFO_EMT_TAG_STS(type, x, len) "list", "", #type,
#define EMT_F_INFO_EMT_TAG_END

// NOLINTBEGIN(bugprone-macro-parentheses) -- additive fragments expanded inside `(0 ...)`
// Estimated serialized size contribution per tag
#define EMT_F_TOTAL_SIZE_EMT_TAG_VAL(type, x) +sizeof(type)
#define EMT_F_TOTAL_SIZE_EMT_TAG_STR(x) +(EMT_STRLEN(x) + 1)
#define EMT_F_TOTAL_SIZE_EMT_TAG_ARR(type, x) +(sizeof(x) / sizeof(type))
#define EMT_F_TOTAL_SIZE_EMT_TAG_SLC(type, x, len) +(len)
#define EMT_F_TOTAL_SIZE_EMT_TAG_STS(type, x, len) +(len)
#define EMT_F_TOTAL_SIZE_EMT_TAG_END
// NOLINTEND(bugprone-macro-parentheses)

// Runtime serialization per tag
#define EMT_F_VAL(out_fn, extra_arg, type, x)                                                      \
    {                                                                                              \
        type temp = (x);                                                                           \
        out_fn((const void*) &temp, sizeof(type), extra_arg);                                      \
    }
#define EMT_F_STR(out_fn, extra_arg, x)                                                            \
    {                                                                                              \
        const char* temp = (x);                                                                    \
        out_fn(temp, EMT_STRLEN(temp) + 1, extra_arg);                                             \
    }
#define EMT_F_ARR(out_fn, extra_arg, type, x)                                                      \
    {                                                                                              \
        const type(*temp)[] = &(x);                                                                \
        out_fn(temp, sizeof(x), extra_arg);                                                        \
    }
#define EMT_F_SLC(out_fn, extra_arg, type, x, len)                                                 \
    {                                                                                              \
        const type* emt_temp_unlikely_to_shadow = (x);                                             \
        emt_size_t size = (len);                                                                   \
        emt_serialize_varint(size, out_fn, extra_arg);                                             \
        out_fn(emt_temp_unlikely_to_shadow, size * sizeof(type), extra_arg);                       \
    }
#define EMT_F_STS(out_fn, extra_arg, type, x, len)                                                 \
    {                                                                                              \
        const type* temp = (x);                                                                    \
        out_fn(temp, sizeof(type) * (len), extra_arg);                                             \
    }
#define EMT_F_END(out_fn, extra_arg, _unused)

#define EMT_F_EMT_TAG_VAL(type, x) EMT_F_VAL, type, x
#define EMT_F_EMT_TAG_STR(x) EMT_F_STR, x
#define EMT_F_EMT_TAG_ARR(type, x) EMT_F_ARR, type, x
#define EMT_F_EMT_TAG_SLC(type, x, len) EMT_F_SLC, type, x, len
#define EMT_F_EMT_TAG_STS(type, x, len) EMT_F_STS, type, x, len
#define EMT_F_EMT_TAG_END EMT_F_END, 0
#define EMT_F_DISPATCH(out_fn, extra_arg, arg) EMT_APPLY_2(out_fn, extra_arg, EMT_F_##arg)

#define EMT_TRACE_F_IMPL(                                                                          \
    fmt_info_attributes, formatter, out_fn, ptr_serializer, begin, finish, extra_arg, postfix,     \
    fmt_str, ...                                                                                   \
)                                                                                                  \
    do {                                                                                           \
                                                                                                   \
        struct emt_info_unlikely_to_shadow_t {                                                     \
            unsigned char framing[14];                                                             \
            emt_size_t record_size;                                                                \
            emt_size_t endianness_probe;                                                           \
            emt_size_t layout[7 + EMT_F_LAYOUT_SIZE(__VA_ARGS__)];                                 \
            char fmt[sizeof(fmt_str postfix)];                                                     \
            EMT_F_INFO_MEMBER(__VA_ARGS__)                                                         \
            char file[sizeof(__FILE__)];                                                           \
        };                                                                                         \
        fmt_info_attributes static struct emt_info_unlikely_to_shadow_t                            \
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
                 EMT_F_COUNT(__VA_ARGS__),                                                         \
                 EMT_OFFSETOF(struct emt_info_unlikely_to_shadow_t, fmt),                          \
                 EMT_F_LAYOUT(__VA_ARGS__) formatter,                                              \
                 EMT_OFFSETOF(struct emt_info_unlikely_to_shadow_t, file), __LINE__},              \
                fmt_str postfix,                                                                   \
                EMT_F_INFO(__VA_ARGS__) __FILE__,                                                  \
        };                                                                                         \
        begin(                                                                                     \
            (const void*) &emt_info_unlikely_to_shadow, EMT_F_TOTAL_SIZE(__VA_ARGS__), extra_arg   \
        );                                                                                         \
        ptr_serializer(&emt_info_unlikely_to_shadow, out_fn, extra_arg);                           \
                                                                                                   \
        EMT_F(out_fn, extra_arg, __VA_ARGS__)                                                      \
        finish(                                                                                    \
            (const void*) &emt_info_unlikely_to_shadow, EMT_F_TOTAL_SIZE(__VA_ARGS__), extra_arg   \
        );                                                                                         \
    } while (0)

/**
 * @brief Emit a trace with format arguments.
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
 * @param fmt                 The format string that, together with `postfix`, ends up in the
 *                            format-info.
 *
 * @param ...                 One or more format arguments, each written using one of the tagged
 *                            forms: `VAL(type, x)`, `ARR(type, x)`, `STS(type, x, len)`,
 *                            `SLC(type, x, len)`, or `STR(x)`. These tags are recognized via token
 *                            pasting before macro expansion and are not defined in the global
 *                            namespace, so colliding user identifiers or macros do not interfere.
 *                            For traces without format arguments, use `EMT_TRACE`.
 *                            *Passing anything else is not stable across versions of this C header*
 */
#define EMT_TRACE_F(                                                                               \
    fmt_info_attributes, formatter, out_fn, ptr_serializer, begin, finish, extra_arg, postfix,     \
    fmt, ...                                                                                       \
)                                                                                                  \
    EMT_TRACE_F_FORWARD(                                                                           \
        fmt_info_attributes, formatter, out_fn, ptr_serializer, begin, finish, extra_arg, postfix, \
        fmt, EMT_ARG_##__VA_ARGS__, EMT_END                                                        \
    )

/**
 * @brief Forwarding helper for custom wrapper macros around `EMT_TRACE_F`.
 *
 * When defining a custom variadic wrapper macro around `EMT_TRACE_F`, a plain forwarding call
 * (`EMT_TRACE_F(..., fmt, __VA_ARGS__)`) *can* work, but only if, in the preprocessor context of
 * the caller, the identifiers VAL, ARR, SLC, STS, and STR aren't defined as some other macro. 
 *
 * Instead one should use this macro, which prevents these identifiers from being expanded to
 * possibly existing macro definitions:
 *
 * ```
 * #define MY_TRACE_F(ctx, fmt, ...)                                                              \
 *     EMT_TRACE_F_FORWARD(                                                                       \
 *         EMT_DEFAULT_ATTR, EMT_PY_FORMAT, my_out, my_ptr_ser, my_begin, my_finish, ctx, "",     \
 *         fmt, EMT_ARG_##__VA_ARGS__, EMT_END                                                    \
 *     )
 * ```
 *
 * @param a0  The first trace argument, already token-pasted with `EMT_ARG_`.
 * @param ... Remaining unexpanded trace arguments followed by a trailing `EMT_END`.
 * 
 * For macro arguments preceding `a0` see documentation of EMT_TRACE_F
 */
#define EMT_TRACE_F_FORWARD(                                                                       \
    fmt_info_attributes, formatter, out_fn, ptr_serializer, begin, finish, extra_arg, postfix,     \
    fmt, a0, ...                                                                                   \
)                                                                                                  \
    EMT_TRACE_F_IMPL(                                                                              \
        fmt_info_attributes, formatter, out_fn, ptr_serializer, begin, finish, extra_arg, postfix, \
        fmt, a0 EMT_NORM_TAIL(EMT_ARG_##__VA_ARGS__, EMT_END)                                      \
    )

/**
 * @brief Emit a trace without format arguments.
 *
 * Companion to `EMT_TRACE_F` for emitting a static string trace with no format arguments (strict
 * C99 compliant without relying on empty `__VA_ARGS__`). All parameters have the same meaning as
 * in `EMT_TRACE_F`, with `str` being the string stored in the format-info alongside `postfix`.
 */
#define EMT_TRACE(                                                                                 \
    fmt_info_attributes, formatter, out_fn, ptr_serializer, begin, finish, extra_arg, postfix, str \
)                                                                                                  \
    EMT_TRACE_F_FORWARD(                                                                           \
        fmt_info_attributes, formatter, out_fn, ptr_serializer, begin, finish, extra_arg, postfix, \
        str, EMT_TAG_END, EMT_END                                                                  \
    )

/**
 * @brief Create a MGIC record.
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

#if defined(EMT_DEFAULT_ATTR) && defined(EMT_DEFAULT_ALIGNMENT_POWER)

#define EMTRACE_F(fmt, ...)                                                                        \
    EMT_TRACE_F_FORWARD(                                                                           \
        EMT_DEFAULT_ATTR, EMT_PY_FORMAT, emt_default_out, emt_default_serialize_pointer,           \
        emt_default_begin, emt_default_finish, NULL, "", fmt, EMT_ARG_##__VA_ARGS__, EMT_END       \
    )
#define EMTRACE(str)                                                                               \
    EMT_TRACE(                                                                                     \
        EMT_DEFAULT_ATTR, EMT_NO_FORMAT, emt_default_out, emt_default_serialize_pointer,           \
        emt_default_begin, emt_default_finish, NULL, "", str                                       \
    )

#define EMTRACELN_F(fmt, ...)                                                                      \
    EMT_TRACE_F_FORWARD(                                                                           \
        EMT_DEFAULT_ATTR, EMT_PY_FORMAT, emt_default_out, emt_default_serialize_pointer,           \
        emt_default_begin, emt_default_finish, NULL, "\n", fmt, EMT_ARG_##__VA_ARGS__, EMT_END     \
    )
#define EMTRACELN(str)                                                                             \
    EMT_TRACE(                                                                                     \
        EMT_DEFAULT_ATTR, EMT_NO_FORMAT, emt_default_out, emt_default_serialize_pointer,           \
        emt_default_begin, emt_default_finish, NULL, "", str "\n"                                  \
    )

#ifdef EMT_FILE
#define EMTRACE_MAGIC(emt_ptr_len)                                                                 \
    EMT_FILE(EMT_DEFAULT_ATTR, emt_ptr_len, EMT_DEFAULT_ALIGNMENT_POWER, stdout)
#endif

#ifdef EMT_FILE_COBS
#define EMTRACE_MAGIC_COBS(emt_ptr_len)                                                            \
    EMT_FILE_COBS(EMT_DEFAULT_ATTR, emt_ptr_len, EMT_DEFAULT_ALIGNMENT_POWER, stdout)
#endif

#ifdef EMT_FILE_COBS_PASSTHROUGH
#define EMTRACE_MAGIC_COBS_PASSTHROUGH(emt_ptr_len)                                                \
    EMT_FILE_COBS_PASSTHROUGH(EMT_DEFAULT_ATTR, emt_ptr_len, EMT_DEFAULT_ALIGNMENT_POWER, stdout)
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

#endif // EMT_DEFAULT_ATTR

#ifdef _MSC_VER
#define EMT_MACRO_ARGS_CAP 127
#endif
