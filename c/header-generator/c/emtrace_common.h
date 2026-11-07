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
