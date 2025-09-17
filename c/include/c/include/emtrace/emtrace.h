#ifndef EMTRACE_EMTRACE_H
#define EMTRACE_EMTRACE_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif
// NOLINTBEGIN(modernize-use-using,modernize-avoid-c-arrays)

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

#if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L) ||                                  \
    (defined(__cplusplus) && __cplusplus >= 201103L)
enum : size_t {
    // In the format info signals whether a piece of dynamically sized data is null-terminated or
    // length-prefixed.
    EMT_NULL_TERMINATED = ///< associated bytes are variable in length and null-terminated
    (((size_t) 1) << (8 * sizeof(size_t) - 1)),
    EMT_LENGTH_PREFIXED = ///< associates bytes are variable in length prefixed by how many there
                          ///< will be
    (((size_t) 1) << (8 * sizeof(size_t) - 2)),

    // In the format info signals what formatter to use.
    EMT_PY_FORMAT = 0, ///< Use python's str.format function for formatting.
    EMT_NO_FORMAT = ///< Do not use any formatter; print the string as-is. All additional arguments
                    ///< are discarded.
    1,
    EMT_C_STYLE_FORMAT = 2, ///< Use python's C-style formatter
};
#else
/// associated bytes are variable in length and null-terminated
#define EMT_NULL_TERMINATED (((size_t) 1) << (8 * sizeof(size_t) - 1))
/// associates bytes are variable in length prefixed by how many there will be
#define EMT_LENGTH_PREFIXED (((size_t) 1) << (8 * sizeof(size_t) - 2))

/// Use python's str.format function for formatting.
#define EMT_PY_FORMAT ((size_t) 0)
/// Do not use any formatter; print the string as-is. All additional arguments are discarded.
#define EMT_NO_FORMAT ((size_t) 1)
/// Use python's C-style formatter
#define EMT_C_STYLE_FORMAT ((size_t) 2)
#endif

typedef struct {
    uint8_t
        main[35]; ///< first 32 bytes are emtrace's magic constant.
                  ///< next byte contains the offset from start of member main to start of member
                  ///< info. final two bytes contain sizeof(size_t), and sizeof(void*) respectively
    size_t info[4];
    // size_t byteorder_id;
    // size_t null_terminated;
    // size_t length_prefixed;
    // size_t no_format;
} emt_magic_t;

static inline void emt_out_file(const void* data, size_t size, FILE* file) {
    fwrite(data, 1, size, file);
}

#define EMT_NTH_ARG(                                                                               \
    a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, x, y, z, aa, bb, cc, dd, ee, \
    ff, gg, hh, ii, ...                                                                            \
)                                                                                                  \
    ii

#define EMT_NUM_ARGS_REST(...)                                                                     \
    EMT_NTH_ARG(                                                                                   \
        __VA_ARGS__, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14,   \
        13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 0                                            \
    )

EMT_STATIC_ASSERT(EMT_NUM_ARGS_REST(a) == 0, "NUM_VA_ARGS is broken.");
EMT_STATIC_ASSERT(EMT_NUM_ARGS_REST(a, b) == 1, "NUM_VA_ARGS is broken.");
EMT_STATIC_ASSERT(EMT_NUM_ARGS_REST(a, b, c) == 2, "NUM_VA_ARGS is broken.");
EMT_STATIC_ASSERT(
    EMT_NUM_ARGS_REST(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o) == 14, "NUM_VA_ARGS is broken."
);

#define EMT_FIRST_ARG(a, ...) a
#define EMT_REST_ARGS(a, ...) __VA_ARGS__

#define EMT_F_0(out_fn, extra_arg, a) ((void) 0)
#define EMT_F_2(out_fn, extra_arg, type_x, x, dummy)                                               \
    do {                                                                                           \
        type_x temp = x;                                                                           \
        EMT_STATIC_ASSERT_INNER(                                                                   \
            sizeof(type_x) != EMT_NULL_TERMINATED, "Size of type_x is too large"                   \
        );                                                                                         \
        EMT_STATIC_ASSERT_INNER(                                                                   \
            sizeof(type_x) != EMT_LENGTH_PREFIXED, "Size of type_x is too large"                   \
        );                                                                                         \
        out_fn((const void*) &temp, sizeof(type_x), extra_arg);                                    \
    } while (0)
#define EMT_F_4(out_fn, extra_arg, type_a, a, type_x, x, dummy)                                    \
    do {                                                                                           \
        EMT_F_2(out_fn, extra_arg, type_a, a, 0);                                                  \
        type_x temp = x;                                                                           \
        EMT_STATIC_ASSERT_INNER(                                                                   \
            sizeof(type_x) != EMT_NULL_TERMINATED, "Size of type_x is too large"                   \
        );                                                                                         \
        EMT_STATIC_ASSERT_INNER(                                                                   \
            sizeof(type_x) != EMT_LENGTH_PREFIXED, "Size of type_x is too large"                   \
        );                                                                                         \
        out_fn((const void*) &temp, sizeof(type_x), extra_arg);                                    \
    } while (0)
#define EMT_F_6(out_fn, extra_arg, type_a, a, type_b, b, type_x, x, dummy)                         \
    do {                                                                                           \
        EMT_F_4(out_fn, extra_arg, type_a, a, type_b, b, 0);                                       \
        type_x temp = x;                                                                           \
        EMT_STATIC_ASSERT_INNER(                                                                   \
            sizeof(type_x) != EMT_NULL_TERMINATED, "Size of type_x is too large"                   \
        );                                                                                         \
        EMT_STATIC_ASSERT_INNER(                                                                   \
            sizeof(type_x) != EMT_LENGTH_PREFIXED, "Size of type_x is too large"                   \
        );                                                                                         \
        out_fn((const void*) &temp, sizeof(type_x), extra_arg);                                    \
    } while (0)
#define EMT_F_8(out_fn, extra_arg, type_a, a, type_b, b, type_c, c, type_x, x, dummy)              \
    do {                                                                                           \
        EMT_F_6(out_fn, extra_arg, type_a, a, type_b, b, type_c, c, 0);                            \
        type_x temp = x;                                                                           \
        EMT_STATIC_ASSERT_INNER(                                                                   \
            sizeof(type_x) != EMT_NULL_TERMINATED, "Size of type_x is too large"                   \
        );                                                                                         \
        EMT_STATIC_ASSERT_INNER(                                                                   \
            sizeof(type_x) != EMT_LENGTH_PREFIXED, "Size of type_x is too large"                   \
        );                                                                                         \
        out_fn((const void*) &temp, sizeof(type_x), extra_arg);                                    \
    } while (0)
#define EMT_F_10(out_fn, extra_arg, type_a, a, type_b, b, type_c, c, type_d, d, type_x, x, dummy)  \
    do {                                                                                           \
        EMT_F_8(out_fn, extra_arg, type_a, a, type_b, b, type_c, c, type_d, d, 0);                 \
        type_x temp = x;                                                                           \
        EMT_STATIC_ASSERT_INNER(                                                                   \
            sizeof(type_x) != EMT_NULL_TERMINATED, "Size of type_x is too large"                   \
        );                                                                                         \
        EMT_STATIC_ASSERT_INNER(                                                                   \
            sizeof(type_x) != EMT_LENGTH_PREFIXED, "Size of type_x is too large"                   \
        );                                                                                         \
        out_fn((const void*) &temp, sizeof(type_x), extra_arg);                                    \
    } while (0)
#define EMT_F_12(                                                                                  \
    out_fn, extra_arg, type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_x, x, dummy     \
)                                                                                                  \
    do {                                                                                           \
        EMT_F_10(out_fn, extra_arg, type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, 0);     \
        type_x temp = x;                                                                           \
        EMT_STATIC_ASSERT_INNER(                                                                   \
            sizeof(type_x) != EMT_NULL_TERMINATED, "Size of type_x is too large"                   \
        );                                                                                         \
        EMT_STATIC_ASSERT_INNER(                                                                   \
            sizeof(type_x) != EMT_LENGTH_PREFIXED, "Size of type_x is too large"                   \
        );                                                                                         \
        out_fn((const void*) &temp, sizeof(type_x), extra_arg);                                    \
    } while (0)
#define EMT_F_14(                                                                                  \
    out_fn, extra_arg, type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_x,   \
    x, dummy                                                                                       \
)                                                                                                  \
    do {                                                                                           \
        EMT_F_12(                                                                                  \
            out_fn, extra_arg, type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, 0 \
        );                                                                                         \
        type_x temp = x;                                                                           \
        EMT_STATIC_ASSERT_INNER(                                                                   \
            sizeof(type_x) != EMT_NULL_TERMINATED, "Size of type_x is too large"                   \
        );                                                                                         \
        EMT_STATIC_ASSERT_INNER(                                                                   \
            sizeof(type_x) != EMT_LENGTH_PREFIXED, "Size of type_x is too large"                   \
        );                                                                                         \
        out_fn((const void*) &temp, sizeof(type_x), extra_arg);                                    \
    } while (0)
#define EMT_F_16(                                                                                  \
    out_fn, extra_arg, type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g,   \
    g, type_x, x, dummy                                                                            \
)                                                                                                  \
    do {                                                                                           \
        EMT_F_14(                                                                                  \
            out_fn, extra_arg, type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f,   \
            type_g, g, 0                                                                           \
        );                                                                                         \
        type_x temp = x;                                                                           \
        EMT_STATIC_ASSERT_INNER(                                                                   \
            sizeof(type_x) != EMT_NULL_TERMINATED, "Size of type_x is too large"                   \
        );                                                                                         \
        EMT_STATIC_ASSERT_INNER(                                                                   \
            sizeof(type_x) != EMT_LENGTH_PREFIXED, "Size of type_x is too large"                   \
        );                                                                                         \
        out_fn((const void*) &temp, sizeof(type_x), extra_arg);                                    \
    } while (0)
#define EMT_F_18(                                                                                  \
    out_fn, extra_arg, type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g,   \
    g, type_h, h, type_x, x, dummy                                                                 \
)                                                                                                  \
    do {                                                                                           \
        EMT_F_16(                                                                                  \
            out_fn, extra_arg, type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f,   \
            type_g, g, 0                                                                           \
        );                                                                                         \
        type_x temp = x;                                                                           \
        EMT_STATIC_ASSERT_INNER(                                                                   \
            sizeof(type_x) != EMT_NULL_TERMINATED, "Size of type_x is too large"                   \
        );                                                                                         \
        EMT_STATIC_ASSERT_INNER(                                                                   \
            sizeof(type_x) != EMT_LENGTH_PREFIXED, "Size of type_x is too large"                   \
        );                                                                                         \
        out_fn((const void*) &temp, sizeof(type_x), extra_arg);                                    \
    } while (0)
#define EMT_F_20(                                                                                  \
    out_fn, extra_arg, type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g,   \
    g, type_h, h, type_i, i, type_x, x, dummy                                                      \
)                                                                                                  \
    do {                                                                                           \
        EMT_F_18(                                                                                  \
            out_fn, extra_arg, type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f,   \
            type_g, g, type_h, h, 0                                                                \
        );                                                                                         \
        type_x temp = x;                                                                           \
        EMT_STATIC_ASSERT_INNER(                                                                   \
            sizeof(type_x) != EMT_NULL_TERMINATED, "Size of type_x is too large"                   \
        );                                                                                         \
        EMT_STATIC_ASSERT_INNER(                                                                   \
            sizeof(type_x) != EMT_LENGTH_PREFIXED, "Size of type_x is too large"                   \
        );                                                                                         \
        out_fn((const void*) &temp, sizeof(type_x), extra_arg);                                    \
    } while (0)
#define EMT_F_22(                                                                                  \
    out_fn, extra_arg, type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g,   \
    g, type_h, h, type_i, i, type_j, j, type_x, x, dummy                                           \
)                                                                                                  \
    do {                                                                                           \
        EMT_F_20(                                                                                  \
            out_fn, extra_arg, type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f,   \
            type_g, g, type_h, h, type_i, i, 0                                                     \
        );                                                                                         \
        type_x temp = x;                                                                           \
        EMT_STATIC_ASSERT_INNER(                                                                   \
            sizeof(type_x) != EMT_NULL_TERMINATED, "Size of type_x is too large"                   \
        );                                                                                         \
        EMT_STATIC_ASSERT_INNER(                                                                   \
            sizeof(type_x) != EMT_LENGTH_PREFIXED, "Size of type_x is too large"                   \
        );                                                                                         \
        out_fn((const void*) &temp, sizeof(type_x), extra_arg);                                    \
    } while (0)
#define EMT_F_24(                                                                                  \
    out_fn, extra_arg, type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g,   \
    g, type_h, h, type_i, i, type_j, j, type_k, k, type_x, x, dummy                                \
)                                                                                                  \
    do {                                                                                           \
        EMT_F_22(                                                                                  \
            out_fn, extra_arg, type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f,   \
            type_g, g, type_h, h, type_i, i, type_j, j, 0                                          \
        );                                                                                         \
        type_x temp = x;                                                                           \
        EMT_STATIC_ASSERT_INNER(                                                                   \
            sizeof(type_x) != EMT_NULL_TERMINATED, "Size of type_x is too large"                   \
        );                                                                                         \
        EMT_STATIC_ASSERT_INNER(                                                                   \
            sizeof(type_x) != EMT_LENGTH_PREFIXED, "Size of type_x is too large"                   \
        );                                                                                         \
        out_fn((const void*) &temp, sizeof(type_x), extra_arg);                                    \
    } while (0)
#define EMT_F_26(                                                                                  \
    out_fn, extra_arg, type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g,   \
    g, type_h, h, type_i, i, type_j, j, type_k, k, type_l, l, type_x, x, dummy                     \
)                                                                                                  \
    do {                                                                                           \
        EMT_F_24(                                                                                  \
            out_fn, extra_arg, type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f,   \
            type_g, g, type_h, h, type_i, i, type_j, j, type_k, k, 0                               \
        );                                                                                         \
        type_x temp = x;                                                                           \
        EMT_STATIC_ASSERT_INNER(                                                                   \
            sizeof(type_x) != EMT_NULL_TERMINATED, "Size of type_x is too large"                   \
        );                                                                                         \
        EMT_STATIC_ASSERT_INNER(                                                                   \
            sizeof(type_x) != EMT_LENGTH_PREFIXED, "Size of type_x is too large"                   \
        );                                                                                         \
        out_fn((const void*) &temp, sizeof(type_x), extra_arg);                                    \
    } while (0)
#define EMT_F_28(                                                                                  \
    out_fn, extra_arg, type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g,   \
    g, type_h, h, type_i, i, type_j, j, type_k, k, type_l, l, type_m, m, type_x, x, dummy          \
)                                                                                                  \
    do {                                                                                           \
        EMT_F_26(                                                                                  \
            out_fn, extra_arg, type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f,   \
            type_g, g, type_h, h, type_i, i, type_j, j, type_k, k, type_l, l, 0                    \
        );                                                                                         \
        type_x temp = x;                                                                           \
        EMT_STATIC_ASSERT_INNER(                                                                   \
            sizeof(type_x) != EMT_NULL_TERMINATED, "Size of type_x is too large"                   \
        );                                                                                         \
        EMT_STATIC_ASSERT_INNER(                                                                   \
            sizeof(type_x) != EMT_LENGTH_PREFIXED, "Size of type_x is too large"                   \
        );                                                                                         \
        out_fn((const void*) &temp, sizeof(type_x), extra_arg);                                    \
    } while (0)
#define EMT_F_30(                                                                                  \
    out_fn, extra_arg, type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g,   \
    g, type_h, h, type_i, i, type_j, j, type_k, k, type_l, l, type_m, m, type_n, n, type_x, x,     \
    dummy                                                                                          \
)                                                                                                  \
    do {                                                                                           \
        EMT_F_28(                                                                                  \
            out_fn, extra_arg, type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f,   \
            type_g, g, type_h, h, type_i, i, type_j, j, type_k, k, type_l, l, type_m, m, 0         \
        );                                                                                         \
        type_x temp = x;                                                                           \
        EMT_STATIC_ASSERT_INNER(                                                                   \
            sizeof(type_x) != EMT_NULL_TERMINATED, "Size of type_x is too large"                   \
        );                                                                                         \
        EMT_STATIC_ASSERT_INNER(                                                                   \
            sizeof(type_x) != EMT_LENGTH_PREFIXED, "Size of type_x is too large"                   \
        );                                                                                         \
        out_fn((const void*) &temp, sizeof(type_x), extra_arg);                                    \
    } while (0)
#define EMT_F_32(                                                                                  \
    out_fn, extra_arg, type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g,   \
    g, type_h, h, type_i, i, type_j, j, type_k, k, type_l, l, type_m, m, type_n, n, type_o, o,     \
    type_x, x, dummy                                                                               \
)                                                                                                  \
    do {                                                                                           \
        EMT_F_30(                                                                                  \
            out_fn, extra_arg, type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f,   \
            type_g, g, type_h, h, type_i, i, type_j, j, type_k, k, type_l, l, type_m, m, type_n,   \
            n, 0                                                                                   \
        );                                                                                         \
        type_x temp = x;                                                                           \
        EMT_STATIC_ASSERT_INNER(                                                                   \
            sizeof(type_x) != EMT_NULL_TERMINATED, "Size of type_x is too large"                   \
        );                                                                                         \
        EMT_STATIC_ASSERT_INNER(                                                                   \
            sizeof(type_x) != EMT_LENGTH_PREFIXED, "Size of type_x is too large"                   \
        );                                                                                         \
        out_fn((const void*) &temp, sizeof(type_x), extra_arg);                                    \
    } while (0)

#define EMT_F_HELPER(x, ...) EMT_F_HELPER2(x, __VA_ARGS__)
#define EMT_F_HELPER2(x, ...) EMT_F_##x(__VA_ARGS__)

#define EMT_F_TOTAL_SIZE_0(a, dummy)
#define EMT_F_TOTAL_SIZE_2(type_x, x, dummy) sizeof(type_x)
#define EMT_F_TOTAL_SIZE_4(type_a, a, type_x, x, dummy)                                            \
    EMT_F_TOTAL_SIZE_2(type_a, a, 0) + sizeof(type_x)
#define EMT_F_TOTAL_SIZE_6(type_a, a, type_b, b, type_x, x, dummy)                                 \
    EMT_F_TOTAL_SIZE_4(type_a, a, type_b, b, 0) + sizeof(type_x)
#define EMT_F_TOTAL_SIZE_8(type_a, a, type_b, b, type_c, c, type_x, x, dummy)                      \
    EMT_F_TOTAL_SIZE_6(type_a, a, type_b, b, type_c, c, 0) + sizeof(type_x)
#define EMT_F_TOTAL_SIZE_10(type_a, a, type_b, b, type_c, c, type_d, d, type_x, x, dummy)          \
    EMT_F_TOTAL_SIZE_8(type_a, a, type_b, b, type_c, c, type_d, d, 0) + sizeof(type_x)
#define EMT_F_TOTAL_SIZE_12(                                                                       \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_x, x, dummy                        \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_10(type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, 0) + sizeof(type_x)
#define EMT_F_TOTAL_SIZE_14(                                                                       \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_x, x, dummy             \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_12(type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, 0) +     \
        sizeof(type_x)
#define EMT_F_TOTAL_SIZE_16(                                                                       \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_x, x, dummy  \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_14(                                                                           \
        type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, 0             \
    ) + sizeof(type_x)
#define EMT_F_TOTAL_SIZE_18(                                                                       \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,        \
    type_x, x, dummy                                                                               \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_16(                                                                           \
        type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, 0  \
    ) + sizeof(type_x)
#define EMT_F_TOTAL_SIZE_20(                                                                       \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,        \
    type_i, i, type_x, x, dummy                                                                    \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_18(                                                                           \
        type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,    \
        type_i, i, 0                                                                               \
    ) + sizeof(type_x)
#define EMT_F_TOTAL_SIZE_22(                                                                       \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,        \
    type_i, i, type_j, j, type_x, x, dummy                                                         \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_20(                                                                           \
        type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,    \
        type_i, i, type_j, j, 0                                                                    \
    ) + sizeof(type_x)
#define EMT_F_TOTAL_SIZE_24(                                                                       \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,        \
    type_i, i, type_j, j, type_k, k, type_x, x, dummy                                              \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_22(                                                                           \
        type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,    \
        type_i, i, type_j, j, type_k, k, 0                                                         \
    ) + sizeof(type_x)
#define EMT_F_TOTAL_SIZE_26(                                                                       \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,        \
    type_i, i, type_j, j, type_k, k, type_l, l, type_x, x, dummy                                   \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_24(                                                                           \
        type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,    \
        type_i, i, type_j, j, type_k, k, type_l, l, 0                                              \
    ) + sizeof(type_x)
#define EMT_F_TOTAL_SIZE_28(                                                                       \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,        \
    type_i, i, type_j, j, type_k, k, type_l, l, type_m, m, type_x, x, dummy                        \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_26(                                                                           \
        type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,    \
        type_i, i, type_j, j, type_k, k, type_l, l, type_m, m, 0                                   \
    ) + sizeof(type_x)
#define EMT_F_TOTAL_SIZE_30(                                                                       \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,        \
    type_i, i, type_j, j, type_k, k, type_l, l, type_m, m, type_n, n, type_x, x, dummy             \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_28(                                                                           \
        type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,    \
        type_i, i, type_j, j, type_k, k, type_l, l, type_m, m, type_n, n, 0                        \
    ) + sizeof(type_x)
#define EMT_F_TOTAL_SIZE_32(                                                                       \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,        \
    type_i, i, type_j, j, type_k, k, type_l, l, type_m, m, type_n, n, type_o, o, type_x, x, dummy  \
)                                                                                                  \
    EMT_F_TOTAL_SIZE_30(                                                                           \
        type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,    \
        type_i, i, type_j, j, type_k, k, type_l, l, type_m, m, type_n, n, type_o, o, 0             \
    ) + sizeof(type_x)

#define EMT_F_TOTAL_SIZE_HELPER2(n, ...) EMT_F_TOTAL_SIZE_##n(__VA_ARGS__)
#define EMT_F_TOTAL_SIZE_HELPER(n, ...) EMT_F_TOTAL_SIZE_HELPER2(n, __VA_ARGS__)

#define EMT_F_INFO_MEMBER_0(a)
#define EMT_F_INFO_MEMBER_2(type_x, x, dummy) char type_1[sizeof(#type_x)];
#define EMT_F_INFO_MEMBER_4(type_a, a, type_x, x, dummy)                                           \
    EMT_F_INFO_MEMBER_2(type_a, a, 0)                                                              \
    char type_2[sizeof(#type_x)];
#define EMT_F_INFO_MEMBER_6(type_a, a, type_b, b, type_x, x, dummy)                                \
    EMT_F_INFO_MEMBER_4(type_a, a, type_b, b, 0)                                                   \
    char type_3[sizeof(#type_x)];
#define EMT_F_INFO_MEMBER_8(type_a, a, type_b, b, type_c, c, type_x, x, dummy)                     \
    EMT_F_INFO_MEMBER_6(type_a, a, type_b, b, type_c, c, 0)                                        \
    char type_4[sizeof(#type_x)];
#define EMT_F_INFO_MEMBER_10(type_a, a, type_b, b, type_c, c, type_d, d, type_x, x, dummy)         \
    EMT_F_INFO_MEMBER_8(type_a, a, type_b, b, type_c, c, type_d, d, 0)                             \
    char type_5[sizeof(#type_x)];
#define EMT_F_INFO_MEMBER_12(                                                                      \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_x, x, dummy                        \
)                                                                                                  \
    EMT_F_INFO_MEMBER_10(type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, 0)                 \
    char type_6[sizeof(#type_x)];
#define EMT_F_INFO_MEMBER_14(                                                                      \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_x, x, dummy             \
)                                                                                                  \
    EMT_F_INFO_MEMBER_12(type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, 0)      \
    char type_7[sizeof(#type_x)];
#define EMT_F_INFO_MEMBER_16(                                                                      \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_x, x, dummy  \
)                                                                                                  \
    EMT_F_INFO_MEMBER_14(                                                                          \
        type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, 0             \
    )                                                                                              \
    char type_8[sizeof(#type_x)];
#define EMT_F_INFO_MEMBER_18(                                                                      \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,        \
    type_x, x, dummy                                                                               \
)                                                                                                  \
    EMT_F_INFO_MEMBER_16(                                                                          \
        type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, 0  \
    )                                                                                              \
    char type_9[sizeof(#type_x)];
#define EMT_F_INFO_MEMBER_20(                                                                      \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,        \
    type_i, i, type_x, x, dummy                                                                    \
)                                                                                                  \
    EMT_F_INFO_MEMBER_18(                                                                          \
        type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,    \
        type_i, i, 0                                                                               \
    )                                                                                              \
    char type_10[sizeof(#type_x)];
#define EMT_F_INFO_MEMBER_22(                                                                      \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,        \
    type_i, i, type_j, j, type_x, x, dummy                                                         \
)                                                                                                  \
    EMT_F_INFO_MEMBER_20(                                                                          \
        type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,    \
        type_i, i, type_j, j, 0                                                                    \
    )                                                                                              \
    char type_11[sizeof(#type_x)];
#define EMT_F_INFO_MEMBER_24(                                                                      \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,        \
    type_i, i, type_j, j, type_k, k, type_x, x, dummy                                              \
)                                                                                                  \
    EMT_F_INFO_MEMBER_22(                                                                          \
        type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,    \
        type_i, i, type_j, j, type_k, k, 0                                                         \
    )                                                                                              \
    char type_12[sizeof(#type_x)];
#define EMT_F_INFO_MEMBER_26(                                                                      \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,        \
    type_i, i, type_j, j, type_k, k, type_l, l, type_x, x, dummy                                   \
)                                                                                                  \
    EMT_F_INFO_MEMBER_24(                                                                          \
        type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,    \
        type_i, i, type_j, j, type_k, k, type_l, l, 0                                              \
    )                                                                                              \
    char type_13[sizeof(#type_x)];
#define EMT_F_INFO_MEMBER_28(                                                                      \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,        \
    type_i, i, type_j, j, type_k, k, type_l, l, type_m, m, type_x, x, dummy                        \
)                                                                                                  \
    EMT_F_INFO_MEMBER_26(                                                                          \
        type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,    \
        type_i, i, type_j, j, type_k, k, type_l, l, type_m, m, 0                                   \
    )                                                                                              \
    char type_14[sizeof(#type_x)];
#define EMT_F_INFO_MEMBER_30(                                                                      \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,        \
    type_i, i, type_j, j, type_k, k, type_l, l, type_m, m, type_n, n, type_x, x, dummy             \
)                                                                                                  \
    EMT_F_INFO_MEMBER_28(                                                                          \
        type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,    \
        type_i, i, type_j, j, type_k, k, type_l, l, type_m, m, type_n, n, 0                        \
    )                                                                                              \
    char type_15[sizeof(#type_x)];
#define EMT_F_INFO_MEMBER_32(                                                                      \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,        \
    type_i, i, type_j, j, type_k, k, type_l, l, type_m, m, type_n, n, type_o, o, type_x, x, dummy  \
)                                                                                                  \
    EMT_F_INFO_MEMBER_30(                                                                          \
        type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,    \
        type_i, i, type_j, j, type_k, k, type_l, l, type_m, m, type_n, n, type_o, o, 0             \
    )                                                                                              \
    char type_16[sizeof(#type_x)];

#define EMT_F_INFO_MEMBER_HELPER2(n, ...) EMT_F_INFO_MEMBER_##n(__VA_ARGS__)
#define EMT_F_INFO_MEMBER_HELPER(n, ...) EMT_F_INFO_MEMBER_HELPER2(n, __VA_ARGS__)

#define EMT_F_INFO_0(a)
#define EMT_F_INFO_2(type_x, x, dummy) #type_x,
#define EMT_F_INFO_4(type_a, a, type_x, x, dummy) EMT_F_INFO_2(type_a, a, 0) #type_x,
#define EMT_F_INFO_6(type_a, a, type_b, b, type_x, x, dummy)                                       \
    EMT_F_INFO_4(type_a, a, type_b, b, 0) #type_x,
#define EMT_F_INFO_8(type_a, a, type_b, b, type_c, c, type_x, x, dummy)                            \
    EMT_F_INFO_6(type_a, a, type_b, b, type_c, c, 0) #type_x,
#define EMT_F_INFO_10(type_a, a, type_b, b, type_c, c, type_d, d, type_x, x, dummy)                \
    EMT_F_INFO_8(type_a, a, type_b, b, type_c, c, type_d, d, 0) #type_x,
#define EMT_F_INFO_12(type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_x, x, dummy)     \
    EMT_F_INFO_10(type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, 0)                        \
    #type_x,
#define EMT_F_INFO_14(                                                                             \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_x, x, dummy             \
)                                                                                                  \
    EMT_F_INFO_12(type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, 0)             \
    #type_x,
#define EMT_F_INFO_16(                                                                             \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_x, x, dummy  \
)                                                                                                  \
    EMT_F_INFO_14(type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, 0)  \
    #type_x,
#define EMT_F_INFO_18(                                                                             \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,        \
    type_x, x, dummy                                                                               \
)                                                                                                  \
    EMT_F_INFO_16(                                                                                 \
        type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, 0  \
    )                                                                                              \
    #type_x,
#define EMT_F_INFO_20(                                                                             \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,        \
    type_i, i, type_x, x, dummy                                                                    \
)                                                                                                  \
    EMT_F_INFO_18(                                                                                 \
        type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,    \
        type_i, i, 0                                                                               \
    )                                                                                              \
    #type_x,
#define EMT_F_INFO_22(                                                                             \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,        \
    type_i, i, type_j, j, type_x, x, dummy                                                         \
)                                                                                                  \
    EMT_F_INFO_20(                                                                                 \
        type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,    \
        type_i, i, type_j, j, 0                                                                    \
    )                                                                                              \
    #type_x,
#define EMT_F_INFO_24(                                                                             \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,        \
    type_i, i, type_j, j, type_k, k, type_x, x, dummy                                              \
)                                                                                                  \
    EMT_F_INFO_22(                                                                                 \
        type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,    \
        type_i, i, type_j, j, type_k, k, 0                                                         \
    )                                                                                              \
    #type_x,
#define EMT_F_INFO_26(                                                                             \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,        \
    type_i, i, type_j, j, type_k, k, type_l, l, type_x, x, dummy                                   \
)                                                                                                  \
    EMT_F_INFO_24(                                                                                 \
        type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,    \
        type_i, i, type_j, j, type_k, k, type_l, l, 0                                              \
    )                                                                                              \
    #type_x,
#define EMT_F_INFO_28(                                                                             \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,        \
    type_i, i, type_j, j, type_k, k, type_l, l, type_m, m, type_x, x, dummy                        \
)                                                                                                  \
    EMT_F_INFO_26(                                                                                 \
        type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,    \
        type_i, i, type_j, j, type_k, k, type_l, l, type_m, m, 0                                   \
    )                                                                                              \
    #type_x,
#define EMT_F_INFO_30(                                                                             \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,        \
    type_i, i, type_j, j, type_k, k, type_l, l, type_m, m, type_n, n, type_x, x, dummy             \
)                                                                                                  \
    EMT_F_INFO_28(                                                                                 \
        type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,    \
        type_i, i, type_j, j, type_k, k, type_l, l, type_m, m, type_n, n, 0                        \
    )                                                                                              \
    #type_x,
#define EMT_F_INFO_32(                                                                             \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,        \
    type_i, i, type_j, j, type_k, k, type_l, l, type_m, m, type_n, n, type_o, o, type_x, x, dummy  \
)                                                                                                  \
    EMT_F_INFO_30(                                                                                 \
        type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,    \
        type_i, i, type_j, j, type_k, k, type_l, l, type_m, m, type_n, n, type_o, o, 0             \
    )                                                                                              \
    #type_x,

#define EMT_F_INFO_HELPER2(n, ...) EMT_F_INFO_##n(__VA_ARGS__)
#define EMT_F_INFO_HELPER(n, ...) EMT_F_INFO_HELPER2(n, __VA_ARGS__)

#define EMT_F_LAYOUT_0(a)
#define EMT_F_LAYOUT_2(type_x, x, dummy) , offsetof(info_t, type_1), sizeof(type_x)
#define EMT_F_LAYOUT_4(type_a, a, type_x, x, dummy)                                                \
    EMT_F_LAYOUT_2(type_a, a, 0), offsetof(info_t, type_2), sizeof(type_x)
#define EMT_F_LAYOUT_6(type_a, a, type_b, b, type_x, x, dummy)                                     \
    EMT_F_LAYOUT_4(type_a, a, type_b, b, 0), offsetof(info_t, type_3), sizeof(type_x)
#define EMT_F_LAYOUT_8(type_a, a, type_b, b, type_c, c, type_x, x, dummy)                          \
    EMT_F_LAYOUT_6(type_a, a, type_b, b, type_c, c, 0), offsetof(info_t, type_4), sizeof(type_x)
#define EMT_F_LAYOUT_10(type_a, a, type_b, b, type_c, c, type_d, d, type_x, x, dummy)              \
    EMT_F_LAYOUT_8(type_a, a, type_b, b, type_c, c, type_d, d, 0), offsetof(info_t, type_5),       \
        sizeof(type_x)
#define EMT_F_LAYOUT_12(type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_x, x, dummy)   \
    EMT_F_LAYOUT_10(type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, 0),                     \
        offsetof(info_t, type_6), sizeof(type_x)
#define EMT_F_LAYOUT_14(                                                                           \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_x, x, dummy             \
)                                                                                                  \
    EMT_F_LAYOUT_12(type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, 0),          \
        offsetof(info_t, type_7), sizeof(type_x)
#define EMT_F_LAYOUT_16(                                                                           \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_x, x, dummy  \
)                                                                                                  \
    EMT_F_LAYOUT_14(                                                                               \
        type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, 0             \
    ),                                                                                             \
        offsetof(info_t, type_8), sizeof(type_x)
#define EMT_F_LAYOUT_18(                                                                           \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,        \
    type_x, x, dummy                                                                               \
)                                                                                                  \
    EMT_F_LAYOUT_16(                                                                               \
        type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, 0  \
    ),                                                                                             \
        offsetof(info_t, type_9), sizeof(type_x)
#define EMT_F_LAYOUT_20(                                                                           \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,        \
    type_i, i, type_x, x, dummy                                                                    \
)                                                                                                  \
    EMT_F_LAYOUT_18(                                                                               \
        type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,    \
        type_i, i, 0                                                                               \
    ),                                                                                             \
        offsetof(info_t, type_10), sizeof(type_x)
#define EMT_F_LAYOUT_22(                                                                           \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,        \
    type_i, i, type_j, j, type_x, x, dummy                                                         \
)                                                                                                  \
    EMT_F_LAYOUT_20(                                                                               \
        type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,    \
        type_i, i, type_j, j, 0                                                                    \
    ),                                                                                             \
        offsetof(info_t, type_11), sizeof(type_x)
#define EMT_F_LAYOUT_24(                                                                           \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,        \
    type_i, i, type_j, j, type_k, k, type_x, x, dummy                                              \
)                                                                                                  \
    EMT_F_LAYOUT_22(                                                                               \
        type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,    \
        type_i, i, type_j, j, type_k, k, 0                                                         \
    ),                                                                                             \
        offsetof(info_t, type_12), sizeof(type_x)
#define EMT_F_LAYOUT_26(                                                                           \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,        \
    type_i, i, type_j, j, type_k, k, type_l, l, type_x, x, dummy                                   \
)                                                                                                  \
    EMT_F_LAYOUT_24(                                                                               \
        type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,    \
        type_i, i, type_j, j, type_k, k, type_l, l, 0                                              \
    ),                                                                                             \
        offsetof(info_t, type_13), sizeof(type_x)
#define EMT_F_LAYOUT_28(                                                                           \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,        \
    type_i, i, type_j, j, type_k, k, type_l, l, type_m, m, type_x, x, dummy                        \
)                                                                                                  \
    EMT_F_LAYOUT_26(                                                                               \
        type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,    \
        type_i, i, type_j, j, type_k, k, type_l, l, type_m, m, 0                                   \
    ),                                                                                             \
        offsetof(info_t, type_14), sizeof(type_x)
#define EMT_F_LAYOUT_30(                                                                           \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,        \
    type_i, i, type_j, j, type_k, k, type_l, l, type_m, m, type_n, n, type_x, x, dummy             \
)                                                                                                  \
    EMT_F_LAYOUT_28(                                                                               \
        type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,    \
        type_i, i, type_j, j, type_k, k, type_l, l, type_m, m, type_n, n, 0                        \
    ),                                                                                             \
        offsetof(info_t, type_15), sizeof(type_x)
#define EMT_F_LAYOUT_32(                                                                           \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,        \
    type_i, i, type_j, j, type_k, k, type_l, l, type_m, m, type_n, n, type_o, o, type_x, x, dummy  \
)                                                                                                  \
    EMT_F_LAYOUT_30(                                                                               \
        type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h,    \
        type_i, i, type_j, j, type_k, k, type_l, l, type_m, m, type_n, n, type_o, o, 0             \
    ),                                                                                             \
        offsetof(info_t, type_16), sizeof(type_x)

#define EMT_F_LAYOUT_HELPER2(n, ...) EMT_F_LAYOUT_##n(__VA_ARGS__)
#define EMT_F_LAYOUT_HELPER(n, ...) EMT_F_LAYOUT_HELPER2(n, __VA_ARGS__)

/**
 * @brief Emit a trace.
 *
 * Creates a variable `info`, with modifieable attributes (via `fmt_info_attributes`, should
 * probably always be at least const static). This variable contains the first of the variable
 * arguments (which has to be a string literal), and information about the sizes of the rest of the
 * variable arguments. Then calls `out_fn` for each of the other variable arguments, along with
 * their size.
 *
 * @param fmt_info_attributes - is prefixed to the definition of the `info` variable, which contains
 * all format info that is known at compile time. Can be any set of additional (possibly
 * compiler-specific) attributes to apply to said variable. E.g. `__attribute__((used))
 * __attribute__((section(".emtrace"))) static const` on gcc.
 * @param out_fn - Should evaluate to a function, or function like macro that takes three arguments.
 * Is evaluated and called:
 *              - once in the beginning with the address of a pointer to the `info` variable, the
 * size of a pointer, and the passed-through `extra_arg` parameter
 *              - once more for every format argument with its address, its size,
 *              and the passed-through `extra_arg` parameter.
 * @param lock - Should evaluate to a function or function like macro that takes three arguments. Is
 * evaluated once in the beginning just before any of the evaluations of `out_fn` with a pointer to
 * the `info` variable, the total size of all bytes that out_fn is about to be called with, and the
 * passed-through `extra_arg` parameter.
 * @param unlock - Should evaluate to a function or function like macro that takes three arguments.
 * Is evaluated once in the end just after any of the evaluations of `out_fn` with a pointer to the
 * `info` variable, the total size of all bytes that out_fn is about to be called with, and the
 * passed-through `extra_arg` parameter.
 * @param extra_arg - An additional parameter that is passed through to all invocations of `out_fn`,
 * `lock`, and `unlock`
 * @param postfix - An optional extra string literal that is appended to the format string in the
 * `info` variable
 */
#define EMT_TRACE_F(fmt_info_attributes, formatter, out_fn, lock, unlock, extra_arg, postfix, ...) \
    do {                                                                                           \
        typedef struct {                                                                           \
            size_t layout[EMT_NUM_ARGS_REST(__VA_ARGS__) + 5];                                     \
            char fmt[sizeof(EMT_FIRST_ARG(__VA_ARGS__, 0) postfix)];                               \
            EMT_F_INFO_MEMBER_HELPER(                                                              \
                EMT_NUM_ARGS_REST(__VA_ARGS__), EMT_REST_ARGS(__VA_ARGS__, 0)                      \
            )                                                                                      \
            char file[sizeof(__FILE__)];                                                           \
        } info_t;                                                                                  \
        EMT_STATIC_ASSERT_INNER(                                                                   \
            offsetof(info_t, layout) == 0, "layout member in info struct must have offset 0"       \
        );                                                                                         \
        fmt_info_attributes info_t info = {                                                        \
            {EMT_NUM_ARGS_REST(__VA_ARGS__) / 2,                                                   \
             offsetof(info_t, fmt) EMT_F_LAYOUT_HELPER(                                            \
                 EMT_NUM_ARGS_REST(__VA_ARGS__), EMT_REST_ARGS(__VA_ARGS__, 0)                     \
             ),                                                                                    \
             formatter, offsetof(info_t, file), __LINE__},                                         \
            EMT_FIRST_ARG(__VA_ARGS__, 0) postfix,                                                 \
            EMT_F_INFO_HELPER(EMT_NUM_ARGS_REST(__VA_ARGS__), EMT_REST_ARGS(__VA_ARGS__, 0))       \
                __FILE__,                                                                          \
        };                                                                                         \
        const void* info_ptr = &info;                                                              \
        lock(                                                                                      \
            (const void*) &info_ptr,                                                               \
            EMT_F_TOTAL_SIZE_HELPER(                                                               \
                EMT_NUM_ARGS_REST(__VA_ARGS__), EMT_REST_ARGS(__VA_ARGS__, 0)                      \
            ) + sizeof(info_ptr),                                                                  \
            extra_arg                                                                              \
        );                                                                                         \
        out_fn((const void*) &info_ptr, sizeof(info_ptr), extra_arg);                              \
        EMT_F_HELPER(                                                                              \
            EMT_NUM_ARGS_REST(__VA_ARGS__), out_fn, extra_arg, EMT_REST_ARGS(__VA_ARGS__, 0)       \
        );                                                                                         \
        unlock(                                                                                    \
            (const void*) &info_ptr,                                                               \
            EMT_F_TOTAL_SIZE_HELPER(                                                               \
                EMT_NUM_ARGS_REST(__VA_ARGS__), EMT_REST_ARGS(__VA_ARGS__, 0)                      \
            ) + sizeof(info_ptr),                                                                  \
            extra_arg                                                                              \
        );                                                                                         \
    } while (0)

#define EMT_TRACE(fmt_info_attributes, out_fn, lock, unlock, extra_arg, string)                    \
    EMT_TRACE_F(fmt_info_attributes, EMT_NO_FORMAT, out_fn, lock, unlock, extra_arg, "", string)

#define EMT_TRACE_S(fmt_info_attributes, out_fn, lock, unlock, extra_arg, postfix, str)            \
    do {                                                                                           \
        typedef struct {                                                                           \
            size_t layout[7];                                                                      \
            char fmt[sizeof("{}" postfix)];                                                        \
            char type_1[sizeof("string")];                                                         \
            char file[sizeof(__FILE__)];                                                           \
        } info_t;                                                                                  \
        fmt_info_attributes info_t info = {                                                        \
            {1, offsetof(info_t, fmt), offsetof(info_t, type_1), EMT_NULL_TERMINATED,              \
             EMT_PY_FORMAT, offsetof(info_t, file), __LINE__},                                     \
            "{}" postfix,                                                                          \
            "string",                                                                              \
            __FILE__,                                                                              \
                                                                                                   \
        };                                                                                         \
        const void* info_ptr = &info;                                                              \
        lock((const void*) &info_ptr, sizeof(info_ptr) | EMT_NULL_TERMINATED, extra_arg);          \
        out_fn((const void*) &info_ptr, sizeof(info_ptr), extra_arg);                              \
        const char* ptr = str;                                                                     \
        while (1) {                                                                                \
            out_fn(ptr, sizeof(char), extra_arg);                                                  \
            if (*ptr == 0)                                                                         \
                break;                                                                             \
            ptr++;                                                                                 \
        }                                                                                          \
        unlock((const void*) &info_ptr, sizeof(info_ptr) | EMT_NULL_TERMINATED, extra_arg);        \
    } while (0)

#define EMT_INIT(attrs, out, extra_arg)                                                            \
    do {                                                                                           \
        attrs emt_magic_t magic = {                                                                \
            .main = {0xd1,                                                                         \
                     0x97,                                                                         \
                     0xf5,                                                                         \
                     0x22,                                                                         \
                     0xd9,                                                                         \
                     0x26,                                                                         \
                     0x9f,                                                                         \
                     0xd1,                                                                         \
                     0xad,                                                                         \
                     0x70,                                                                         \
                     0x33,                                                                         \
                     0x92,                                                                         \
                     0xf6,                                                                         \
                     0x59,                                                                         \
                     0xdf,                                                                         \
                     0xd0,                                                                         \
                     0xfb,                                                                         \
                     0xec,                                                                         \
                     0xbd,                                                                         \
                     0x60,                                                                         \
                     0x97,                                                                         \
                     0x13,                                                                         \
                     0x25,                                                                         \
                     0xe8,                                                                         \
                     0x92,                                                                         \
                     0x01,                                                                         \
                     0xb2,                                                                         \
                     0x5a,                                                                         \
                     0x38,                                                                         \
                     0x5d,                                                                         \
                     0x9e,                                                                         \
                     0xc7,                                                                         \
                     offsetof(emt_magic_t, info) - offsetof(emt_magic_t, main),                    \
                     sizeof(size_t),                                                               \
                     sizeof(void*)},                                                               \
            .info = {                                                                              \
                (size_t) 0x0706050403020100,                                                       \
                EMT_NULL_TERMINATED,                                                               \
                EMT_LENGTH_PREFIXED,                                                               \
            }                                                                                      \
        };                                                                                         \
        const void* magic_ptr = &magic;                                                            \
        out((const void*) &magic_ptr, sizeof(magic_ptr), extra_arg);                               \
    } while (0)

#ifdef __GNUC__
#define EMT_DEFAULT_SEC_ATTR __attribute__((used)) __attribute__((section(".emtrace"))) static const
#elif defined(__TINYC__)
#define EMT_DEFAULT_SEC_ATTR __attribute__((section(".emtrace"))) static const
#endif

#define EMT_FLOCK_FILE(x, y, file) flockfile(file)
#define EMT_FUNLOCK_FILE(x, y, file) funlockfile(file)

#ifdef EMT_DEFAULT_SEC_ATTR
#define EMTRACE_F(...)                                                                             \
    EMT_TRACE_F(                                                                                   \
        EMT_DEFAULT_SEC_ATTR, EMT_PY_FORMAT, emt_out_file, EMT_FLOCK_FILE, EMT_FUNLOCK_FILE,       \
        stdout, "", __VA_ARGS__                                                                    \
    )
#define EMTRACE(str)                                                                               \
    EMT_TRACE(EMT_DEFAULT_SEC_ATTR, emt_out_file, EMT_FLOCK_FILE, EMT_FUNLOCK_FILE, stdout, str)
#define EMTRACE_S(str)                                                                             \
    EMT_TRACE_S(                                                                                   \
        EMT_DEFAULT_SEC_ATTR, emt_out_file, EMT_FLOCK_FILE, EMT_FUNLOCK_FILE, stdout, "", str      \
    )

#define EMTRACELN_F(...)                                                                           \
    EMT_TRACE_F(                                                                                   \
        EMT_DEFAULT_SEC_ATTR, EMT_PY_FORMAT, emt_out_file, EMT_FLOCK_FILE, EMT_FUNLOCK_FILE,       \
        stdout, "\n", __VA_ARGS__                                                                  \
    )
#define EMTRACELN(str)                                                                             \
    EMT_TRACE(                                                                                     \
        EMT_DEFAULT_SEC_ATTR, emt_out_file, EMT_FLOCK_FILE, EMT_FUNLOCK_FILE, stdout, str "\n"     \
    )
#define EMTRACELN_S(str)                                                                           \
    EMT_TRACE_S(                                                                                   \
        EMT_DEFAULT_SEC_ATTR, emt_out_file, EMT_FLOCK_FILE, EMT_FUNLOCK_FILE, stdout, "\n", str    \
    )
#define EMTRACE_INIT() EMT_INIT(EMT_DEFAULT_SEC_ATTR, emt_out_file, stdout)
#endif // EMT_DEFAULT_SEC_ATTR

// NOLINTEND(modernize-use-using,modernize-avoid-c-arrays)
#ifdef __cplusplus
}
#endif

#endif // EMTRACE_EMTRACE_H
