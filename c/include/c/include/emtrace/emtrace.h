// DO NOT MODIFY DIRECTLY! Header is auto-generated from c/header-generator/c/emtrace_template.h and
// c/header-generator/c/emtrace_common.h using this python script: c/header-generator/build_macro.py
// Script arguments were: max_args=64
#ifndef EMTRACE_EMTRACE_H
#define EMTRACE_EMTRACE_H
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
#define EMT_TRACE_VERSION_MAJOR 3
#define EMT_TRACE_VERSION_MINOR 0

/// Version of the format of the data output at runtime when emitting a trace
#define EMT_RUNTIME_DATA_TRACE_VERSION_MAJOR 3
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

#ifndef EMT_DEFAULT_ALIGNMENT_POWER
#define EMT_DEFAULT_ALIGNMENT_POWER 8
#endif

#define EMT_DEFAULT_ALIGNMENT (1 << (EMT_DEFAULT_ALIGNMENT_POWER))

#ifndef EMT_DEFAULT_ALIGN_ATTRIBUTE

#if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L) ||                                  \
    (defined(__cplusplus) && __cplusplus >= 201103L)
#define EMT_DEFAULT_ALIGN_ATTRIBUTE alignas(EMT_DEFAULT_ALIGNMENT)
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define EMT_DEFAULT_ALIGN_ATTRIBUTE _Alignas(EMT_DEFAULT_ALIGNMENT)
#elif defined(__GNUC__) || defined(__clang__)
#define EMT_DEFAULT_ALIGN_ATTRIBUTE __attribute__((aligned(EMT_DEFAULT_ALIGNMENT)))
#endif

#endif // not EMT_DEFAULT_ALIGN_ATTRIBUTE

#ifndef EMT_DEFAULT_SEC_ATTR

#if defined(__GNUC__) || defined(__clang__)
#define EMT_DEFAULT_SEC_ATTR __attribute__((used, section(".emtrace"))) const
#elif defined(_MSC_VER)
#define EMT_DEFAULT_SEC_ATTR                                                                       \
    __declspec(align(EMT_DEMT_DEFAULT_ALIGNMENT)) __declspec(allocate(".emtrace")) const
#endif

#endif // not EMT_DEFAULT_SEC_ATTR

#if !defined(EMT_DEFAULT_ATTR) && defined(EMT_DEFAULT_ALIGN_ATTRIBUTE) &&                          \
    defined(EMT_DEFAULT_SEC_ATTR)
#define EMT_DEFAULT_ATTR EMT_DEFAULT_ALIGN_ATTRIBUTE EMT_DEFAULT_SEC_ATTR
#endif

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

#ifdef __cplusplus
}
#endif
// NOLINTEND(modernize-*,bugprone-casting-through-void)

#endif // EMTRACE_COMMON
// NOLINTBEGIN(modernize-*,bugprone-casting-through-void)

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
// clang-format off
#define EMT_NORM_REST(                                                                             \
    a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21,    \
    a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, \
    a41, a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54, a55, a56, a57, a58, a59, \
    a60, a61, a62, a63, overflow, ...                                                              \
)                                                                                                  \
    , a2, EMT_ARG_##a3, EMT_ARG_##a4, EMT_ARG_##a5, EMT_ARG_##a6, EMT_ARG_##a7, EMT_ARG_##a8,      \
    EMT_ARG_##a9, EMT_ARG_##a10, EMT_ARG_##a11, EMT_ARG_##a12, EMT_ARG_##a13, EMT_ARG_##a14,       \
    EMT_ARG_##a15, EMT_ARG_##a16, EMT_ARG_##a17, EMT_ARG_##a18, EMT_ARG_##a19, EMT_ARG_##a20,      \
    EMT_ARG_##a21, EMT_ARG_##a22, EMT_ARG_##a23, EMT_ARG_##a24, EMT_ARG_##a25, EMT_ARG_##a26,      \
    EMT_ARG_##a27, EMT_ARG_##a28, EMT_ARG_##a29, EMT_ARG_##a30, EMT_ARG_##a31, EMT_ARG_##a32,      \
    EMT_ARG_##a33, EMT_ARG_##a34, EMT_ARG_##a35, EMT_ARG_##a36, EMT_ARG_##a37, EMT_ARG_##a38,      \
    EMT_ARG_##a39, EMT_ARG_##a40, EMT_ARG_##a41, EMT_ARG_##a42, EMT_ARG_##a43, EMT_ARG_##a44,      \
    EMT_ARG_##a45, EMT_ARG_##a46, EMT_ARG_##a47, EMT_ARG_##a48, EMT_ARG_##a49, EMT_ARG_##a50,      \
    EMT_ARG_##a51, EMT_ARG_##a52, EMT_ARG_##a53, EMT_ARG_##a54, EMT_ARG_##a55, EMT_ARG_##a56,      \
    EMT_ARG_##a57, EMT_ARG_##a58, EMT_ARG_##a59, EMT_ARG_##a60, EMT_ARG_##a61, EMT_ARG_##a62,      \
    EMT_ARG_##a63 EMT_OVERFLOW_##overflow

#define EMT_NORM_TAIL(a1, ...)                                                                     \
    , a1 EMT_NORM_REST(EMT_ARG_##__VA_ARGS__, EMT_END, EMT_END, EMT_END, EMT_END, EMT_END, EMT_END,\
    EMT_END, EMT_END, EMT_END, EMT_END, EMT_END, EMT_END, EMT_END, EMT_END, EMT_END, EMT_END,      \
    EMT_END, EMT_END, EMT_END, EMT_END, EMT_END, EMT_END, EMT_END, EMT_END, EMT_END, EMT_END,      \
    EMT_END, EMT_END, EMT_END, EMT_END, EMT_END, EMT_END, EMT_END, EMT_END, EMT_END, EMT_END,      \
    EMT_END, EMT_END, EMT_END, EMT_END, EMT_END, EMT_END, EMT_END, EMT_END, EMT_END, EMT_END,      \
    EMT_END, EMT_END, EMT_END, EMT_END, EMT_END, EMT_END, EMT_END, EMT_END, EMT_END, EMT_END,      \
    EMT_END, EMT_END, EMT_END, EMT_END, EMT_END, EMT_END, EMT_END)

#define EMT_F_COUNT(                                                                               \
    a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, \
    a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, \
    a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54, a55, a56, a57, a58, \
    a59, a60, a61, a62, a63                                                                        \
)                                                                                                  \
    (0 EMT_F_COUNT_##a0 EMT_F_COUNT_##a1 EMT_F_COUNT_##a2 EMT_F_COUNT_##a3 EMT_F_COUNT_##a4        \
    EMT_F_COUNT_##a5 EMT_F_COUNT_##a6 EMT_F_COUNT_##a7 EMT_F_COUNT_##a8 EMT_F_COUNT_##a9           \
    EMT_F_COUNT_##a10 EMT_F_COUNT_##a11 EMT_F_COUNT_##a12 EMT_F_COUNT_##a13 EMT_F_COUNT_##a14      \
    EMT_F_COUNT_##a15 EMT_F_COUNT_##a16 EMT_F_COUNT_##a17 EMT_F_COUNT_##a18 EMT_F_COUNT_##a19      \
    EMT_F_COUNT_##a20 EMT_F_COUNT_##a21 EMT_F_COUNT_##a22 EMT_F_COUNT_##a23 EMT_F_COUNT_##a24      \
    EMT_F_COUNT_##a25 EMT_F_COUNT_##a26 EMT_F_COUNT_##a27 EMT_F_COUNT_##a28 EMT_F_COUNT_##a29      \
    EMT_F_COUNT_##a30 EMT_F_COUNT_##a31 EMT_F_COUNT_##a32 EMT_F_COUNT_##a33 EMT_F_COUNT_##a34      \
    EMT_F_COUNT_##a35 EMT_F_COUNT_##a36 EMT_F_COUNT_##a37 EMT_F_COUNT_##a38 EMT_F_COUNT_##a39      \
    EMT_F_COUNT_##a40 EMT_F_COUNT_##a41 EMT_F_COUNT_##a42 EMT_F_COUNT_##a43 EMT_F_COUNT_##a44      \
    EMT_F_COUNT_##a45 EMT_F_COUNT_##a46 EMT_F_COUNT_##a47 EMT_F_COUNT_##a48 EMT_F_COUNT_##a49      \
    EMT_F_COUNT_##a50 EMT_F_COUNT_##a51 EMT_F_COUNT_##a52 EMT_F_COUNT_##a53 EMT_F_COUNT_##a54      \
    EMT_F_COUNT_##a55 EMT_F_COUNT_##a56 EMT_F_COUNT_##a57 EMT_F_COUNT_##a58 EMT_F_COUNT_##a59      \
    EMT_F_COUNT_##a60 EMT_F_COUNT_##a61 EMT_F_COUNT_##a62 EMT_F_COUNT_##a63)

#define EMT_F_LAYOUT_SIZE(                                                                         \
    a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, \
    a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, \
    a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54, a55, a56, a57, a58, \
    a59, a60, a61, a62, a63                                                                        \
)                                                                                                  \
    (0 EMT_F_LAYOUT_SIZE_##a0 EMT_F_LAYOUT_SIZE_##a1 EMT_F_LAYOUT_SIZE_##a2 EMT_F_LAYOUT_SIZE_##a3 \
    EMT_F_LAYOUT_SIZE_##a4 EMT_F_LAYOUT_SIZE_##a5 EMT_F_LAYOUT_SIZE_##a6 EMT_F_LAYOUT_SIZE_##a7    \
    EMT_F_LAYOUT_SIZE_##a8 EMT_F_LAYOUT_SIZE_##a9 EMT_F_LAYOUT_SIZE_##a10 EMT_F_LAYOUT_SIZE_##a11  \
    EMT_F_LAYOUT_SIZE_##a12 EMT_F_LAYOUT_SIZE_##a13 EMT_F_LAYOUT_SIZE_##a14                        \
    EMT_F_LAYOUT_SIZE_##a15 EMT_F_LAYOUT_SIZE_##a16 EMT_F_LAYOUT_SIZE_##a17                        \
    EMT_F_LAYOUT_SIZE_##a18 EMT_F_LAYOUT_SIZE_##a19 EMT_F_LAYOUT_SIZE_##a20                        \
    EMT_F_LAYOUT_SIZE_##a21 EMT_F_LAYOUT_SIZE_##a22 EMT_F_LAYOUT_SIZE_##a23                        \
    EMT_F_LAYOUT_SIZE_##a24 EMT_F_LAYOUT_SIZE_##a25 EMT_F_LAYOUT_SIZE_##a26                        \
    EMT_F_LAYOUT_SIZE_##a27 EMT_F_LAYOUT_SIZE_##a28 EMT_F_LAYOUT_SIZE_##a29                        \
    EMT_F_LAYOUT_SIZE_##a30 EMT_F_LAYOUT_SIZE_##a31 EMT_F_LAYOUT_SIZE_##a32                        \
    EMT_F_LAYOUT_SIZE_##a33 EMT_F_LAYOUT_SIZE_##a34 EMT_F_LAYOUT_SIZE_##a35                        \
    EMT_F_LAYOUT_SIZE_##a36 EMT_F_LAYOUT_SIZE_##a37 EMT_F_LAYOUT_SIZE_##a38                        \
    EMT_F_LAYOUT_SIZE_##a39 EMT_F_LAYOUT_SIZE_##a40 EMT_F_LAYOUT_SIZE_##a41                        \
    EMT_F_LAYOUT_SIZE_##a42 EMT_F_LAYOUT_SIZE_##a43 EMT_F_LAYOUT_SIZE_##a44                        \
    EMT_F_LAYOUT_SIZE_##a45 EMT_F_LAYOUT_SIZE_##a46 EMT_F_LAYOUT_SIZE_##a47                        \
    EMT_F_LAYOUT_SIZE_##a48 EMT_F_LAYOUT_SIZE_##a49 EMT_F_LAYOUT_SIZE_##a50                        \
    EMT_F_LAYOUT_SIZE_##a51 EMT_F_LAYOUT_SIZE_##a52 EMT_F_LAYOUT_SIZE_##a53                        \
    EMT_F_LAYOUT_SIZE_##a54 EMT_F_LAYOUT_SIZE_##a55 EMT_F_LAYOUT_SIZE_##a56                        \
    EMT_F_LAYOUT_SIZE_##a57 EMT_F_LAYOUT_SIZE_##a58 EMT_F_LAYOUT_SIZE_##a59                        \
    EMT_F_LAYOUT_SIZE_##a60 EMT_F_LAYOUT_SIZE_##a61 EMT_F_LAYOUT_SIZE_##a62                        \
    EMT_F_LAYOUT_SIZE_##a63)

#define EMT_F_INFO_MEMBER(                                                                         \
    a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, \
    a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, \
    a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54, a55, a56, a57, a58, \
    a59, a60, a61, a62, a63                                                                        \
)                                                                                                  \
    EMT_F_INFO_MEMBER_DISPATCH(type0, a0) EMT_F_INFO_MEMBER_DISPATCH(type1, a1)                    \
    EMT_F_INFO_MEMBER_DISPATCH(type2, a2) EMT_F_INFO_MEMBER_DISPATCH(type3, a3)                    \
    EMT_F_INFO_MEMBER_DISPATCH(type4, a4) EMT_F_INFO_MEMBER_DISPATCH(type5, a5)                    \
    EMT_F_INFO_MEMBER_DISPATCH(type6, a6) EMT_F_INFO_MEMBER_DISPATCH(type7, a7)                    \
    EMT_F_INFO_MEMBER_DISPATCH(type8, a8) EMT_F_INFO_MEMBER_DISPATCH(type9, a9)                    \
    EMT_F_INFO_MEMBER_DISPATCH(type10, a10) EMT_F_INFO_MEMBER_DISPATCH(type11, a11)                \
    EMT_F_INFO_MEMBER_DISPATCH(type12, a12) EMT_F_INFO_MEMBER_DISPATCH(type13, a13)                \
    EMT_F_INFO_MEMBER_DISPATCH(type14, a14) EMT_F_INFO_MEMBER_DISPATCH(type15, a15)                \
    EMT_F_INFO_MEMBER_DISPATCH(type16, a16) EMT_F_INFO_MEMBER_DISPATCH(type17, a17)                \
    EMT_F_INFO_MEMBER_DISPATCH(type18, a18) EMT_F_INFO_MEMBER_DISPATCH(type19, a19)                \
    EMT_F_INFO_MEMBER_DISPATCH(type20, a20) EMT_F_INFO_MEMBER_DISPATCH(type21, a21)                \
    EMT_F_INFO_MEMBER_DISPATCH(type22, a22) EMT_F_INFO_MEMBER_DISPATCH(type23, a23)                \
    EMT_F_INFO_MEMBER_DISPATCH(type24, a24) EMT_F_INFO_MEMBER_DISPATCH(type25, a25)                \
    EMT_F_INFO_MEMBER_DISPATCH(type26, a26) EMT_F_INFO_MEMBER_DISPATCH(type27, a27)                \
    EMT_F_INFO_MEMBER_DISPATCH(type28, a28) EMT_F_INFO_MEMBER_DISPATCH(type29, a29)                \
    EMT_F_INFO_MEMBER_DISPATCH(type30, a30) EMT_F_INFO_MEMBER_DISPATCH(type31, a31)                \
    EMT_F_INFO_MEMBER_DISPATCH(type32, a32) EMT_F_INFO_MEMBER_DISPATCH(type33, a33)                \
    EMT_F_INFO_MEMBER_DISPATCH(type34, a34) EMT_F_INFO_MEMBER_DISPATCH(type35, a35)                \
    EMT_F_INFO_MEMBER_DISPATCH(type36, a36) EMT_F_INFO_MEMBER_DISPATCH(type37, a37)                \
    EMT_F_INFO_MEMBER_DISPATCH(type38, a38) EMT_F_INFO_MEMBER_DISPATCH(type39, a39)                \
    EMT_F_INFO_MEMBER_DISPATCH(type40, a40) EMT_F_INFO_MEMBER_DISPATCH(type41, a41)                \
    EMT_F_INFO_MEMBER_DISPATCH(type42, a42) EMT_F_INFO_MEMBER_DISPATCH(type43, a43)                \
    EMT_F_INFO_MEMBER_DISPATCH(type44, a44) EMT_F_INFO_MEMBER_DISPATCH(type45, a45)                \
    EMT_F_INFO_MEMBER_DISPATCH(type46, a46) EMT_F_INFO_MEMBER_DISPATCH(type47, a47)                \
    EMT_F_INFO_MEMBER_DISPATCH(type48, a48) EMT_F_INFO_MEMBER_DISPATCH(type49, a49)                \
    EMT_F_INFO_MEMBER_DISPATCH(type50, a50) EMT_F_INFO_MEMBER_DISPATCH(type51, a51)                \
    EMT_F_INFO_MEMBER_DISPATCH(type52, a52) EMT_F_INFO_MEMBER_DISPATCH(type53, a53)                \
    EMT_F_INFO_MEMBER_DISPATCH(type54, a54) EMT_F_INFO_MEMBER_DISPATCH(type55, a55)                \
    EMT_F_INFO_MEMBER_DISPATCH(type56, a56) EMT_F_INFO_MEMBER_DISPATCH(type57, a57)                \
    EMT_F_INFO_MEMBER_DISPATCH(type58, a58) EMT_F_INFO_MEMBER_DISPATCH(type59, a59)                \
    EMT_F_INFO_MEMBER_DISPATCH(type60, a60) EMT_F_INFO_MEMBER_DISPATCH(type61, a61)                \
    EMT_F_INFO_MEMBER_DISPATCH(type62, a62) EMT_F_INFO_MEMBER_DISPATCH(type63, a63)

#define EMT_F_LAYOUT(                                                                              \
    a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, \
    a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, \
    a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54, a55, a56, a57, a58, \
    a59, a60, a61, a62, a63                                                                        \
)                                                                                                  \
    EMT_F_LAYOUT_DISPATCH(type0, a0) EMT_F_LAYOUT_DISPATCH(type1, a1)                              \
    EMT_F_LAYOUT_DISPATCH(type2, a2) EMT_F_LAYOUT_DISPATCH(type3, a3)                              \
    EMT_F_LAYOUT_DISPATCH(type4, a4) EMT_F_LAYOUT_DISPATCH(type5, a5)                              \
    EMT_F_LAYOUT_DISPATCH(type6, a6) EMT_F_LAYOUT_DISPATCH(type7, a7)                              \
    EMT_F_LAYOUT_DISPATCH(type8, a8) EMT_F_LAYOUT_DISPATCH(type9, a9)                              \
    EMT_F_LAYOUT_DISPATCH(type10, a10) EMT_F_LAYOUT_DISPATCH(type11, a11)                          \
    EMT_F_LAYOUT_DISPATCH(type12, a12) EMT_F_LAYOUT_DISPATCH(type13, a13)                          \
    EMT_F_LAYOUT_DISPATCH(type14, a14) EMT_F_LAYOUT_DISPATCH(type15, a15)                          \
    EMT_F_LAYOUT_DISPATCH(type16, a16) EMT_F_LAYOUT_DISPATCH(type17, a17)                          \
    EMT_F_LAYOUT_DISPATCH(type18, a18) EMT_F_LAYOUT_DISPATCH(type19, a19)                          \
    EMT_F_LAYOUT_DISPATCH(type20, a20) EMT_F_LAYOUT_DISPATCH(type21, a21)                          \
    EMT_F_LAYOUT_DISPATCH(type22, a22) EMT_F_LAYOUT_DISPATCH(type23, a23)                          \
    EMT_F_LAYOUT_DISPATCH(type24, a24) EMT_F_LAYOUT_DISPATCH(type25, a25)                          \
    EMT_F_LAYOUT_DISPATCH(type26, a26) EMT_F_LAYOUT_DISPATCH(type27, a27)                          \
    EMT_F_LAYOUT_DISPATCH(type28, a28) EMT_F_LAYOUT_DISPATCH(type29, a29)                          \
    EMT_F_LAYOUT_DISPATCH(type30, a30) EMT_F_LAYOUT_DISPATCH(type31, a31)                          \
    EMT_F_LAYOUT_DISPATCH(type32, a32) EMT_F_LAYOUT_DISPATCH(type33, a33)                          \
    EMT_F_LAYOUT_DISPATCH(type34, a34) EMT_F_LAYOUT_DISPATCH(type35, a35)                          \
    EMT_F_LAYOUT_DISPATCH(type36, a36) EMT_F_LAYOUT_DISPATCH(type37, a37)                          \
    EMT_F_LAYOUT_DISPATCH(type38, a38) EMT_F_LAYOUT_DISPATCH(type39, a39)                          \
    EMT_F_LAYOUT_DISPATCH(type40, a40) EMT_F_LAYOUT_DISPATCH(type41, a41)                          \
    EMT_F_LAYOUT_DISPATCH(type42, a42) EMT_F_LAYOUT_DISPATCH(type43, a43)                          \
    EMT_F_LAYOUT_DISPATCH(type44, a44) EMT_F_LAYOUT_DISPATCH(type45, a45)                          \
    EMT_F_LAYOUT_DISPATCH(type46, a46) EMT_F_LAYOUT_DISPATCH(type47, a47)                          \
    EMT_F_LAYOUT_DISPATCH(type48, a48) EMT_F_LAYOUT_DISPATCH(type49, a49)                          \
    EMT_F_LAYOUT_DISPATCH(type50, a50) EMT_F_LAYOUT_DISPATCH(type51, a51)                          \
    EMT_F_LAYOUT_DISPATCH(type52, a52) EMT_F_LAYOUT_DISPATCH(type53, a53)                          \
    EMT_F_LAYOUT_DISPATCH(type54, a54) EMT_F_LAYOUT_DISPATCH(type55, a55)                          \
    EMT_F_LAYOUT_DISPATCH(type56, a56) EMT_F_LAYOUT_DISPATCH(type57, a57)                          \
    EMT_F_LAYOUT_DISPATCH(type58, a58) EMT_F_LAYOUT_DISPATCH(type59, a59)                          \
    EMT_F_LAYOUT_DISPATCH(type60, a60) EMT_F_LAYOUT_DISPATCH(type61, a61)                          \
    EMT_F_LAYOUT_DISPATCH(type62, a62) EMT_F_LAYOUT_DISPATCH(type63, a63)

#define EMT_F_INFO(                                                                                \
    a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, \
    a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, \
    a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54, a55, a56, a57, a58, \
    a59, a60, a61, a62, a63                                                                        \
)                                                                                                  \
    EMT_F_INFO_##a0 EMT_F_INFO_##a1 EMT_F_INFO_##a2 EMT_F_INFO_##a3 EMT_F_INFO_##a4                \
    EMT_F_INFO_##a5 EMT_F_INFO_##a6 EMT_F_INFO_##a7 EMT_F_INFO_##a8 EMT_F_INFO_##a9                \
    EMT_F_INFO_##a10 EMT_F_INFO_##a11 EMT_F_INFO_##a12 EMT_F_INFO_##a13 EMT_F_INFO_##a14           \
    EMT_F_INFO_##a15 EMT_F_INFO_##a16 EMT_F_INFO_##a17 EMT_F_INFO_##a18 EMT_F_INFO_##a19           \
    EMT_F_INFO_##a20 EMT_F_INFO_##a21 EMT_F_INFO_##a22 EMT_F_INFO_##a23 EMT_F_INFO_##a24           \
    EMT_F_INFO_##a25 EMT_F_INFO_##a26 EMT_F_INFO_##a27 EMT_F_INFO_##a28 EMT_F_INFO_##a29           \
    EMT_F_INFO_##a30 EMT_F_INFO_##a31 EMT_F_INFO_##a32 EMT_F_INFO_##a33 EMT_F_INFO_##a34           \
    EMT_F_INFO_##a35 EMT_F_INFO_##a36 EMT_F_INFO_##a37 EMT_F_INFO_##a38 EMT_F_INFO_##a39           \
    EMT_F_INFO_##a40 EMT_F_INFO_##a41 EMT_F_INFO_##a42 EMT_F_INFO_##a43 EMT_F_INFO_##a44           \
    EMT_F_INFO_##a45 EMT_F_INFO_##a46 EMT_F_INFO_##a47 EMT_F_INFO_##a48 EMT_F_INFO_##a49           \
    EMT_F_INFO_##a50 EMT_F_INFO_##a51 EMT_F_INFO_##a52 EMT_F_INFO_##a53 EMT_F_INFO_##a54           \
    EMT_F_INFO_##a55 EMT_F_INFO_##a56 EMT_F_INFO_##a57 EMT_F_INFO_##a58 EMT_F_INFO_##a59           \
    EMT_F_INFO_##a60 EMT_F_INFO_##a61 EMT_F_INFO_##a62 EMT_F_INFO_##a63

#define EMT_F_TOTAL_SIZE(                                                                          \
    a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, \
    a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, \
    a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54, a55, a56, a57, a58, \
    a59, a60, a61, a62, a63                                                                        \
)                                                                                                  \
    (0 EMT_F_TOTAL_SIZE_##a0 EMT_F_TOTAL_SIZE_##a1 EMT_F_TOTAL_SIZE_##a2 EMT_F_TOTAL_SIZE_##a3     \
    EMT_F_TOTAL_SIZE_##a4 EMT_F_TOTAL_SIZE_##a5 EMT_F_TOTAL_SIZE_##a6 EMT_F_TOTAL_SIZE_##a7        \
    EMT_F_TOTAL_SIZE_##a8 EMT_F_TOTAL_SIZE_##a9 EMT_F_TOTAL_SIZE_##a10 EMT_F_TOTAL_SIZE_##a11      \
    EMT_F_TOTAL_SIZE_##a12 EMT_F_TOTAL_SIZE_##a13 EMT_F_TOTAL_SIZE_##a14 EMT_F_TOTAL_SIZE_##a15    \
    EMT_F_TOTAL_SIZE_##a16 EMT_F_TOTAL_SIZE_##a17 EMT_F_TOTAL_SIZE_##a18 EMT_F_TOTAL_SIZE_##a19    \
    EMT_F_TOTAL_SIZE_##a20 EMT_F_TOTAL_SIZE_##a21 EMT_F_TOTAL_SIZE_##a22 EMT_F_TOTAL_SIZE_##a23    \
    EMT_F_TOTAL_SIZE_##a24 EMT_F_TOTAL_SIZE_##a25 EMT_F_TOTAL_SIZE_##a26 EMT_F_TOTAL_SIZE_##a27    \
    EMT_F_TOTAL_SIZE_##a28 EMT_F_TOTAL_SIZE_##a29 EMT_F_TOTAL_SIZE_##a30 EMT_F_TOTAL_SIZE_##a31    \
    EMT_F_TOTAL_SIZE_##a32 EMT_F_TOTAL_SIZE_##a33 EMT_F_TOTAL_SIZE_##a34 EMT_F_TOTAL_SIZE_##a35    \
    EMT_F_TOTAL_SIZE_##a36 EMT_F_TOTAL_SIZE_##a37 EMT_F_TOTAL_SIZE_##a38 EMT_F_TOTAL_SIZE_##a39    \
    EMT_F_TOTAL_SIZE_##a40 EMT_F_TOTAL_SIZE_##a41 EMT_F_TOTAL_SIZE_##a42 EMT_F_TOTAL_SIZE_##a43    \
    EMT_F_TOTAL_SIZE_##a44 EMT_F_TOTAL_SIZE_##a45 EMT_F_TOTAL_SIZE_##a46 EMT_F_TOTAL_SIZE_##a47    \
    EMT_F_TOTAL_SIZE_##a48 EMT_F_TOTAL_SIZE_##a49 EMT_F_TOTAL_SIZE_##a50 EMT_F_TOTAL_SIZE_##a51    \
    EMT_F_TOTAL_SIZE_##a52 EMT_F_TOTAL_SIZE_##a53 EMT_F_TOTAL_SIZE_##a54 EMT_F_TOTAL_SIZE_##a55    \
    EMT_F_TOTAL_SIZE_##a56 EMT_F_TOTAL_SIZE_##a57 EMT_F_TOTAL_SIZE_##a58 EMT_F_TOTAL_SIZE_##a59    \
    EMT_F_TOTAL_SIZE_##a60 EMT_F_TOTAL_SIZE_##a61 EMT_F_TOTAL_SIZE_##a62 EMT_F_TOTAL_SIZE_##a63)

#define EMT_F(                                                                                     \
    out_fn, extra_arg, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16,  \
    a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, \
    a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54, \
    a55, a56, a57, a58, a59, a60, a61, a62, a63                                                    \
)                                                                                                  \
    EMT_F_DISPATCH(out_fn, extra_arg, a0) EMT_F_DISPATCH(out_fn, extra_arg, a1)                    \
    EMT_F_DISPATCH(out_fn, extra_arg, a2) EMT_F_DISPATCH(out_fn, extra_arg, a3)                    \
    EMT_F_DISPATCH(out_fn, extra_arg, a4) EMT_F_DISPATCH(out_fn, extra_arg, a5)                    \
    EMT_F_DISPATCH(out_fn, extra_arg, a6) EMT_F_DISPATCH(out_fn, extra_arg, a7)                    \
    EMT_F_DISPATCH(out_fn, extra_arg, a8) EMT_F_DISPATCH(out_fn, extra_arg, a9)                    \
    EMT_F_DISPATCH(out_fn, extra_arg, a10) EMT_F_DISPATCH(out_fn, extra_arg, a11)                  \
    EMT_F_DISPATCH(out_fn, extra_arg, a12) EMT_F_DISPATCH(out_fn, extra_arg, a13)                  \
    EMT_F_DISPATCH(out_fn, extra_arg, a14) EMT_F_DISPATCH(out_fn, extra_arg, a15)                  \
    EMT_F_DISPATCH(out_fn, extra_arg, a16) EMT_F_DISPATCH(out_fn, extra_arg, a17)                  \
    EMT_F_DISPATCH(out_fn, extra_arg, a18) EMT_F_DISPATCH(out_fn, extra_arg, a19)                  \
    EMT_F_DISPATCH(out_fn, extra_arg, a20) EMT_F_DISPATCH(out_fn, extra_arg, a21)                  \
    EMT_F_DISPATCH(out_fn, extra_arg, a22) EMT_F_DISPATCH(out_fn, extra_arg, a23)                  \
    EMT_F_DISPATCH(out_fn, extra_arg, a24) EMT_F_DISPATCH(out_fn, extra_arg, a25)                  \
    EMT_F_DISPATCH(out_fn, extra_arg, a26) EMT_F_DISPATCH(out_fn, extra_arg, a27)                  \
    EMT_F_DISPATCH(out_fn, extra_arg, a28) EMT_F_DISPATCH(out_fn, extra_arg, a29)                  \
    EMT_F_DISPATCH(out_fn, extra_arg, a30) EMT_F_DISPATCH(out_fn, extra_arg, a31)                  \
    EMT_F_DISPATCH(out_fn, extra_arg, a32) EMT_F_DISPATCH(out_fn, extra_arg, a33)                  \
    EMT_F_DISPATCH(out_fn, extra_arg, a34) EMT_F_DISPATCH(out_fn, extra_arg, a35)                  \
    EMT_F_DISPATCH(out_fn, extra_arg, a36) EMT_F_DISPATCH(out_fn, extra_arg, a37)                  \
    EMT_F_DISPATCH(out_fn, extra_arg, a38) EMT_F_DISPATCH(out_fn, extra_arg, a39)                  \
    EMT_F_DISPATCH(out_fn, extra_arg, a40) EMT_F_DISPATCH(out_fn, extra_arg, a41)                  \
    EMT_F_DISPATCH(out_fn, extra_arg, a42) EMT_F_DISPATCH(out_fn, extra_arg, a43)                  \
    EMT_F_DISPATCH(out_fn, extra_arg, a44) EMT_F_DISPATCH(out_fn, extra_arg, a45)                  \
    EMT_F_DISPATCH(out_fn, extra_arg, a46) EMT_F_DISPATCH(out_fn, extra_arg, a47)                  \
    EMT_F_DISPATCH(out_fn, extra_arg, a48) EMT_F_DISPATCH(out_fn, extra_arg, a49)                  \
    EMT_F_DISPATCH(out_fn, extra_arg, a50) EMT_F_DISPATCH(out_fn, extra_arg, a51)                  \
    EMT_F_DISPATCH(out_fn, extra_arg, a52) EMT_F_DISPATCH(out_fn, extra_arg, a53)                  \
    EMT_F_DISPATCH(out_fn, extra_arg, a54) EMT_F_DISPATCH(out_fn, extra_arg, a55)                  \
    EMT_F_DISPATCH(out_fn, extra_arg, a56) EMT_F_DISPATCH(out_fn, extra_arg, a57)                  \
    EMT_F_DISPATCH(out_fn, extra_arg, a58) EMT_F_DISPATCH(out_fn, extra_arg, a59)                  \
    EMT_F_DISPATCH(out_fn, extra_arg, a60) EMT_F_DISPATCH(out_fn, extra_arg, a61)                  \
    EMT_F_DISPATCH(out_fn, extra_arg, a62) EMT_F_DISPATCH(out_fn, extra_arg, a63)
// clang-format on

// NOLINTEND(modernize-*,bugprone-casting-through-void)

#endif // EMTRACE_EMTRACE_H
