#ifndef EMTRACE_F_EMTRACE_F_H
#define EMTRACE_F_EMTRACE_F_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#if __STDC_VERSION__ >= 201112L
#define EMTRACE_STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)
#elif __STDC_VERSION__ >= 202311L || (defined(__cplusplus) && __cplusplus >= 201103L)
#define EMTRACE_STATIC_ASSERT(cond, msg) static_assert(cond, msg)
#else
#define EMTRACE_STATIC_ASSERT(cond, msg) typedef char static_assertion_failed[(cond) ? 1 : -1]
#endif

enum emtrace_flags {
    // in the format info controls whether a piece of dynamically sized data is
    // null-terminated or length-prefixed
    EMTRACE_NULL_TERMINATED = (SIZE_MAX | 1),
    EMTRACE_LENGTH_PREFIXED = (SIZE_MAX & ~((size_t)1)),

    // in the format info, setting the first layout array element to this means
    // the 'fmt' string is supposed to be
    // printed as-is without formatting
    EMTRACE_NO_FORMAT = (SIZE_MAX | 1),
};

static inline void emtrace_out_file(const void* data, size_t size, FILE* file) { fwrite(data, 1, size, file); }

#define NTH_ARG(                                                                                                       \
    a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, x, y, z, aa, bb, cc, dd, ee, ff, gg, hh, ii, ... \
)                                                                                                                      \
    ii

#define NUM_ARGS_REST(...)                                                                                             \
    NTH_ARG(                                                                                                           \
        __VA_ARGS__, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, \
        7, 6, 5, 4, 3, 2, 1, 0                                                                                         \
    )

EMTRACE_STATIC_ASSERT(NUM_ARGS_REST(a) == 0, "NUM_VA_ARGS is broken.");
EMTRACE_STATIC_ASSERT(NUM_ARGS_REST(a, b) == 1, "NUM_VA_ARGS is broken.");
EMTRACE_STATIC_ASSERT(NUM_ARGS_REST(a, b, c) == 2, "NUM_VA_ARGS is broken.");
EMTRACE_STATIC_ASSERT(NUM_ARGS_REST(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o) == 14, "NUM_VA_ARGS is broken.");

#define FIRST_ARG(a, ...) a
#define REST_ARGS(a, ...) __VA_ARGS__

#define EMTRACE_F_0(out_fn, extra_arg, a) ((void)0)
#define EMTRACE_F_2(out_fn, extra_arg, type_x, x)                                                                      \
    do {                                                                                                               \
        type_x temp = x;                                                                                               \
        EMTRACE_STATIC_ASSERT(sizeof(type_x) != EMTRACE_NULL_TERMINATED, "Size of type_x is too large");               \
        EMTRACE_STATIC_ASSERT(sizeof(type_x) != EMTRACE_LENGTH_PREFIXED, "Size of type_x is too large");               \
        out_fn(&temp, sizeof(type_x), extra_arg);                                                                      \
    } while (0)
#define EMTRACE_F_4(out_fn, extra_arg, type_a, a, type_x, x)                                                           \
    do {                                                                                                               \
        EMTRACE_F_2(out_fn, extra_arg, type_a, a);                                                                     \
        type_x temp = x;                                                                                               \
        EMTRACE_STATIC_ASSERT(sizeof(type_x) != EMTRACE_NULL_TERMINATED, "Size of type_x is too large");               \
        EMTRACE_STATIC_ASSERT(sizeof(type_x) != EMTRACE_LENGTH_PREFIXED, "Size of type_x is too large");               \
        out_fn(&temp, sizeof(type_x), extra_arg);                                                                      \
    } while (0)
#define EMTRACE_F_6(out_fn, extra_arg, type_a, a, type_b, b, type_x, x)                                                \
    do {                                                                                                               \
        EMTRACE_F_4(out_fn, extra_arg, type_a, a, type_b, b);                                                          \
        type_x temp = x;                                                                                               \
        EMTRACE_STATIC_ASSERT(sizeof(type_x) != EMTRACE_NULL_TERMINATED, "Size of type_x is too large");               \
        EMTRACE_STATIC_ASSERT(sizeof(type_x) != EMTRACE_LENGTH_PREFIXED, "Size of type_x is too large");               \
        out_fn(&temp, sizeof(type_x), extra_arg);                                                                      \
    } while (0)
#define EMTRACE_F_8(out_fn, extra_arg, type_a, a, type_b, b, type_c, c, type_x, x)                                     \
    do {                                                                                                               \
        EMTRACE_F_6(out_fn, extra_arg, type_a, a, type_b, b, type_c, c);                                               \
        type_x temp = x;                                                                                               \
        EMTRACE_STATIC_ASSERT(sizeof(type_x) != EMTRACE_NULL_TERMINATED, "Size of type_x is too large");               \
        EMTRACE_STATIC_ASSERT(sizeof(type_x) != EMTRACE_LENGTH_PREFIXED, "Size of type_x is too large");               \
        out_fn(&temp, sizeof(type_x), extra_arg);                                                                      \
    } while (0)
#define EMTRACE_F_10(out_fn, extra_arg, type_a, a, type_b, b, type_c, c, type_d, d, type_x, x)                         \
    do {                                                                                                               \
        EMTRACE_F_8(out_fn, extra_arg, type_a, a, type_b, b, type_c, c, type_d, d);                                    \
        type_x temp = x;                                                                                               \
        EMTRACE_STATIC_ASSERT(sizeof(type_x) != EMTRACE_NULL_TERMINATED, "Size of type_x is too large");               \
        EMTRACE_STATIC_ASSERT(sizeof(type_x) != EMTRACE_LENGTH_PREFIXED, "Size of type_x is too large");               \
        out_fn(&temp, sizeof(type_x), extra_arg);                                                                      \
    } while (0)
#define EMTRACE_F_12(out_fn, extra_arg, type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_x, x)              \
    do {                                                                                                               \
        EMTRACE_F_10(out_fn, extra_arg, type_a, a, type_b, b, type_c, c, type_d, d, type_e, e);                        \
        type_x temp = x;                                                                                               \
        EMTRACE_STATIC_ASSERT(sizeof(type_x) != EMTRACE_NULL_TERMINATED, "Size of type_x is too large");               \
        EMTRACE_STATIC_ASSERT(sizeof(type_x) != EMTRACE_LENGTH_PREFIXED, "Size of type_x is too large");               \
        out_fn(&temp, sizeof(type_x), extra_arg);                                                                      \
    } while (0)
#define EMTRACE_F_14(out_fn, extra_arg, type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_x, x)   \
    do {                                                                                                               \
        EMTRACE_F_12(out_fn, extra_arg, type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f);             \
        type_x temp = x;                                                                                               \
        EMTRACE_STATIC_ASSERT(sizeof(type_x) != EMTRACE_NULL_TERMINATED, "Size of type_x is too large");               \
        EMTRACE_STATIC_ASSERT(sizeof(type_x) != EMTRACE_LENGTH_PREFIXED, "Size of type_x is too large");               \
        out_fn(&temp, sizeof(type_x), extra_arg);                                                                      \
    } while (0)
#define EMTRACE_F_16(                                                                                                  \
    out_fn, extra_arg, type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_x, x          \
)                                                                                                                      \
    do {                                                                                                               \
        EMTRACE_F_14(out_fn, extra_arg, type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g);  \
        type_x temp = x;                                                                                               \
        EMTRACE_STATIC_ASSERT(sizeof(type_x) != EMTRACE_NULL_TERMINATED, "Size of type_x is too large");               \
        EMTRACE_STATIC_ASSERT(sizeof(type_x) != EMTRACE_LENGTH_PREFIXED, "Size of type_x is too large");               \
        out_fn(&temp, sizeof(type_x), extra_arg);                                                                      \
    } while (0)
#define EMTRACE_F_18(                                                                                                  \
    out_fn, extra_arg, type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, type_x, \
    x                                                                                                                  \
)                                                                                                                      \
    do {                                                                                                               \
        EMTRACE_F_16(out_fn, extra_arg, type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g);  \
        type_x temp = x;                                                                                               \
        EMTRACE_STATIC_ASSERT(sizeof(type_x) != EMTRACE_NULL_TERMINATED, "Size of type_x is too large");               \
        EMTRACE_STATIC_ASSERT(sizeof(type_x) != EMTRACE_LENGTH_PREFIXED, "Size of type_x is too large");               \
        out_fn(&temp, sizeof(type_x), extra_arg);                                                                      \
    } while (0)
#define EMTRACE_F_20(                                                                                                  \
    out_fn, extra_arg, type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, type_i, \
    i, type_x, x                                                                                                       \
)                                                                                                                      \
    do {                                                                                                               \
        EMTRACE_F_18(                                                                                                  \
            out_fn, extra_arg, type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h  \
        );                                                                                                             \
        type_x temp = x;                                                                                               \
        EMTRACE_STATIC_ASSERT(sizeof(type_x) != EMTRACE_NULL_TERMINATED, "Size of type_x is too large");               \
        EMTRACE_STATIC_ASSERT(sizeof(type_x) != EMTRACE_LENGTH_PREFIXED, "Size of type_x is too large");               \
        out_fn(&temp, sizeof(type_x), extra_arg);                                                                      \
    } while (0)
#define EMTRACE_F_22(                                                                                                  \
    out_fn, extra_arg, type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, type_i, \
    i, type_j, j, type_x, x                                                                                            \
)                                                                                                                      \
    do {                                                                                                               \
        EMTRACE_F_20(                                                                                                  \
            out_fn, extra_arg, type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, \
            type_i, i                                                                                                  \
        );                                                                                                             \
        type_x temp = x;                                                                                               \
        EMTRACE_STATIC_ASSERT(sizeof(type_x) != EMTRACE_NULL_TERMINATED, "Size of type_x is too large");               \
        EMTRACE_STATIC_ASSERT(sizeof(type_x) != EMTRACE_LENGTH_PREFIXED, "Size of type_x is too large");               \
        out_fn(&temp, sizeof(type_x), extra_arg);                                                                      \
    } while (0)
#define EMTRACE_F_24(                                                                                                  \
    out_fn, extra_arg, type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, type_i, \
    i, type_j, j, type_k, k, type_x, x                                                                                 \
)                                                                                                                      \
    do {                                                                                                               \
        EMTRACE_F_22(                                                                                                  \
            out_fn, extra_arg, type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, \
            type_i, i, type_j, j                                                                                       \
        );                                                                                                             \
        type_x temp = x;                                                                                               \
        EMTRACE_STATIC_ASSERT(sizeof(type_x) != EMTRACE_NULL_TERMINATED, "Size of type_x is too large");               \
        EMTRACE_STATIC_ASSERT(sizeof(type_x) != EMTRACE_LENGTH_PREFIXED, "Size of type_x is too large");               \
        out_fn(&temp, sizeof(type_x), extra_arg);                                                                      \
    } while (0)
#define EMTRACE_F_26(                                                                                                  \
    out_fn, extra_arg, type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, type_i, \
    i, type_j, j, type_k, k, type_l, l, type_x, x                                                                      \
)                                                                                                                      \
    do {                                                                                                               \
        EMTRACE_F_24(                                                                                                  \
            out_fn, extra_arg, type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, \
            type_i, i, type_j, j, type_k, k                                                                            \
        );                                                                                                             \
        type_x temp = x;                                                                                               \
        EMTRACE_STATIC_ASSERT(sizeof(type_x) != EMTRACE_NULL_TERMINATED, "Size of type_x is too large");               \
        EMTRACE_STATIC_ASSERT(sizeof(type_x) != EMTRACE_LENGTH_PREFIXED, "Size of type_x is too large");               \
        out_fn(&temp, sizeof(type_x), extra_arg);                                                                      \
    } while (0)
#define EMTRACE_F_28(                                                                                                  \
    out_fn, extra_arg, type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, type_i, \
    i, type_j, j, type_k, k, type_l, l, type_m, m, type_x, x                                                           \
)                                                                                                                      \
    do {                                                                                                               \
        EMTRACE_F_26(                                                                                                  \
            out_fn, extra_arg, type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, \
            type_i, i, type_j, j, type_k, k, type_l, l                                                                 \
        );                                                                                                             \
        type_x temp = x;                                                                                               \
        EMTRACE_STATIC_ASSERT(sizeof(type_x) != EMTRACE_NULL_TERMINATED, "Size of type_x is too large");               \
        EMTRACE_STATIC_ASSERT(sizeof(type_x) != EMTRACE_LENGTH_PREFIXED, "Size of type_x is too large");               \
        out_fn(&temp, sizeof(type_x), extra_arg);                                                                      \
    } while (0)
#define EMTRACE_F_30(                                                                                                  \
    out_fn, extra_arg, type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, type_i, \
    i, type_j, j, type_k, k, type_l, l, type_m, m, type_n, n, type_x, x                                                \
)                                                                                                                      \
    do {                                                                                                               \
        EMTRACE_F_28(                                                                                                  \
            out_fn, extra_arg, type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, \
            type_i, i, type_j, j, type_k, k, type_l, l, type_m, m                                                      \
        );                                                                                                             \
        type_x temp = x;                                                                                               \
        EMTRACE_STATIC_ASSERT(sizeof(type_x) != EMTRACE_NULL_TERMINATED, "Size of type_x is too large");               \
        EMTRACE_STATIC_ASSERT(sizeof(type_x) != EMTRACE_LENGTH_PREFIXED, "Size of type_x is too large");               \
        out_fn(&temp, sizeof(type_x), extra_arg);                                                                      \
    } while (0)
#define EMTRACE_F_32(                                                                                                  \
    out_fn, extra_arg, type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, type_i, \
    i, type_j, j, type_k, k, type_l, l, type_m, m, type_n, n, type_o, o, type_x, x                                     \
)                                                                                                                      \
    do {                                                                                                               \
        EMTRACE_F_30(                                                                                                  \
            out_fn, extra_arg, type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, \
            type_i, i, type_j, j, type_k, k, type_l, l, type_m, m, type_n, n                                           \
        );                                                                                                             \
        type_x temp = x;                                                                                               \
        EMTRACE_STATIC_ASSERT(sizeof(type_x) != EMTRACE_NULL_TERMINATED, "Size of type_x is too large");               \
        EMTRACE_STATIC_ASSERT(sizeof(type_x) != EMTRACE_LENGTH_PREFIXED, "Size of type_x is too large");               \
        out_fn(&temp, sizeof(type_x), extra_arg);                                                                      \
    } while (0)

#define EMTRACE_F_HELPER(x, ...) EMTRACE_F_HELPER2(x, __VA_ARGS__)
#define EMTRACE_F_HELPER2(x, ...) EMTRACE_F_##x(__VA_ARGS__)

#define EMTRACE_F_INFO_MEMBER_0(a)
#define EMTRACE_F_INFO_MEMBER_2(type_x, x) char type_1[sizeof(#type_x)];
#define EMTRACE_F_INFO_MEMBER_4(type_a, a, type_x, x)                                                                  \
    EMTRACE_F_INFO_MEMBER_2(type_a, a)                                                                                 \
    char type_2[sizeof(#type_x)];
#define EMTRACE_F_INFO_MEMBER_6(type_a, a, type_b, b, type_x, x)                                                       \
    EMTRACE_F_INFO_MEMBER_4(type_a, a, type_b, b)                                                                      \
    char type_3[sizeof(#type_x)];
#define EMTRACE_F_INFO_MEMBER_8(type_a, a, type_b, b, type_c, c, type_x, x)                                            \
    EMTRACE_F_INFO_MEMBER_6(type_a, a, type_b, b, type_c, c)                                                           \
    char type_4[sizeof(#type_x)];
#define EMTRACE_F_INFO_MEMBER_10(type_a, a, type_b, b, type_c, c, type_d, d, type_x, x)                                \
    EMTRACE_F_INFO_MEMBER_8(type_a, a, type_b, b, type_c, c, type_d, d)                                                \
    char type_5[sizeof(#type_x)];
#define EMTRACE_F_INFO_MEMBER_12(type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_x, x)                     \
    EMTRACE_F_INFO_MEMBER_10(type_a, a, type_b, b, type_c, c, type_d, d, type_e, e)                                    \
    char type_6[sizeof(#type_x)];
#define EMTRACE_F_INFO_MEMBER_14(type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_x, x)          \
    EMTRACE_F_INFO_MEMBER_12(type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f)                         \
    char type_7[sizeof(#type_x)];
#define EMTRACE_F_INFO_MEMBER_16(                                                                                      \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_x, x                             \
)                                                                                                                      \
    EMTRACE_F_INFO_MEMBER_14(type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g)              \
    char type_8[sizeof(#type_x)];
#define EMTRACE_F_INFO_MEMBER_18(                                                                                      \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, type_x, x                  \
)                                                                                                                      \
    EMTRACE_F_INFO_MEMBER_16(type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g)              \
    char type_9[sizeof(#type_x)];
#define EMTRACE_F_INFO_MEMBER_20(                                                                                      \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, type_i, i, type_x, x       \
)                                                                                                                      \
    EMTRACE_F_INFO_MEMBER_18(type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h)   \
    char type_10[sizeof(#type_x)];
#define EMTRACE_F_INFO_MEMBER_22(                                                                                      \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, type_i, i, type_j, j,      \
    type_x, x                                                                                                          \
)                                                                                                                      \
    EMTRACE_F_INFO_MEMBER_20(                                                                                          \
        type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, type_i, i              \
    )                                                                                                                  \
    char type_11[sizeof(#type_x)];
#define EMTRACE_F_INFO_MEMBER_24(                                                                                      \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, type_i, i, type_j, j,      \
    type_k, k, type_x, x                                                                                               \
)                                                                                                                      \
    EMTRACE_F_INFO_MEMBER_22(                                                                                          \
        type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, type_i, i, type_j, j   \
    )                                                                                                                  \
    char type_12[sizeof(#type_x)];
#define EMTRACE_F_INFO_MEMBER_26(                                                                                      \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, type_i, i, type_j, j,      \
    type_k, k, type_l, l, type_x, x                                                                                    \
)                                                                                                                      \
    EMTRACE_F_INFO_MEMBER_24(                                                                                          \
        type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, type_i, i, type_j, j,  \
        type_k, k                                                                                                      \
    )                                                                                                                  \
    char type_13[sizeof(#type_x)];
#define EMTRACE_F_INFO_MEMBER_28(                                                                                      \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, type_i, i, type_j, j,      \
    type_k, k, type_l, l, type_m, m, type_x, x                                                                         \
)                                                                                                                      \
    EMTRACE_F_INFO_MEMBER_26(                                                                                          \
        type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, type_i, i, type_j, j,  \
        type_k, k, type_l, l                                                                                           \
    )                                                                                                                  \
    char type_14[sizeof(#type_x)];
#define EMTRACE_F_INFO_MEMBER_30(                                                                                      \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, type_i, i, type_j, j,      \
    type_k, k, type_l, l, type_m, m, type_n, n, type_x, x                                                              \
)                                                                                                                      \
    EMTRACE_F_INFO_MEMBER_28(                                                                                          \
        type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, type_i, i, type_j, j,  \
        type_k, k, type_l, l, type_m, m                                                                                \
    )                                                                                                                  \
    char type_15[sizeof(#type_x)];
#define EMTRACE_F_INFO_MEMBER_32(                                                                                      \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, type_i, i, type_j, j,      \
    type_k, k, type_l, l, type_m, m, type_n, n, type_o, o, type_x, x                                                   \
)                                                                                                                      \
    EMTRACE_F_INFO_MEMBER_30(                                                                                          \
        type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, type_i, i, type_j, j,  \
        type_k, k, type_l, l, type_m, m, type_n, n                                                                     \
    )                                                                                                                  \
    char type_16[sizeof(#type_x)];

#define EMTRACE_F_INFO_MEMBER_HELPER2(n, ...) EMTRACE_F_INFO_MEMBER_##n(__VA_ARGS__)
#define EMTRACE_F_INFO_MEMBER_HELPER(n, ...) EMTRACE_F_INFO_MEMBER_HELPER2(n, __VA_ARGS__)

#define EMTRACE_F_INFO_0(a)
#define EMTRACE_F_INFO_2(type_x, x) #type_x,
#define EMTRACE_F_INFO_4(type_a, a, type_x, x) EMTRACE_F_INFO_2(type_a, a) #type_x,
#define EMTRACE_F_INFO_6(type_a, a, type_b, b, type_x, x) EMTRACE_F_INFO_4(type_a, a, type_b, b) #type_x,
#define EMTRACE_F_INFO_6(type_a, a, type_b, b, type_x, x) EMTRACE_F_INFO_4(type_a, a, type_b, b) #type_x,
#define EMTRACE_F_INFO_8(type_a, a, type_b, b, type_c, c, type_x, x)                                                   \
    EMTRACE_F_INFO_6(type_a, a, type_b, b, type_c, c) #type_x,
#define EMTRACE_F_INFO_10(type_a, a, type_b, b, type_c, c, type_d, d, type_x, x)                                       \
    EMTRACE_F_INFO_8(type_a, a, type_b, b, type_c, c, type_d, d) #type_x,
#define EMTRACE_F_INFO_12(type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_x, x)                            \
    EMTRACE_F_INFO_10(type_a, a, type_b, b, type_c, c, type_d, d, type_e, e)                                           \
    #type_x,
#define EMTRACE_F_INFO_14(type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_x, x)                 \
    EMTRACE_F_INFO_12(type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f)                                \
    #type_x,
#define EMTRACE_F_INFO_16(type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_x, x)      \
    EMTRACE_F_INFO_14(type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g)                     \
    #type_x,
#define EMTRACE_F_INFO_18(                                                                                             \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, type_x, x                  \
)                                                                                                                      \
    EMTRACE_F_INFO_16(type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h)          \
    #type_x,
#define EMTRACE_F_INFO_20(                                                                                             \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, type_i, i, type_x, x       \
)                                                                                                                      \
    EMTRACE_F_INFO_18(                                                                                                 \
        type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, type_i, i              \
    )                                                                                                                  \
    #type_x,
#define EMTRACE_F_INFO_22(                                                                                             \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, type_i, i, type_j, j,      \
    type_x, x                                                                                                          \
)                                                                                                                      \
    EMTRACE_F_INFO_20(                                                                                                 \
        type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, type_i, i, type_j, j   \
    )                                                                                                                  \
    #type_x,
#define EMTRACE_F_INFO_24(                                                                                             \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, type_i, i, type_j, j,      \
    type_k, k, type_x, x                                                                                               \
)                                                                                                                      \
    EMTRACE_F_INFO_22(                                                                                                 \
        type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, type_i, i, type_j, j,  \
        type_k, k                                                                                                      \
    )                                                                                                                  \
    #type_x,
#define EMTRACE_F_INFO_26(                                                                                             \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, type_i, i, type_j, j,      \
    type_k, k, type_l, l, type_x, x                                                                                    \
)                                                                                                                      \
    EMTRACE_F_INFO_24(                                                                                                 \
        type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, type_i, i, type_j, j,  \
        type_k, k, type_l, l                                                                                           \
    )                                                                                                                  \
    #type_x,
#define EMTRACE_F_INFO_28(                                                                                             \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, type_i, i, type_j, j,      \
    type_k, k, type_l, l, type_m, m, type_x, x                                                                         \
)                                                                                                                      \
    EMTRACE_F_INFO_26(                                                                                                 \
        type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, type_i, i, type_j, j,  \
        type_k, k, type_l, l, type_m, m                                                                                \
    )                                                                                                                  \
    #type_x,
#define EMTRACE_F_INFO_30(                                                                                             \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, type_i, i, type_j, j,      \
    type_k, k, type_l, l, type_m, m, type_n, n, type_x, x                                                              \
)                                                                                                                      \
    EMTRACE_F_INFO_28(                                                                                                 \
        type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, type_i, i, type_j, j,  \
        type_k, k, type_l, l, type_m, m, type_n, n                                                                     \
    )                                                                                                                  \
    #type_x,
#define EMTRACE_F_INFO_32(                                                                                             \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, type_i, i, type_j, j,      \
    type_k, k, type_l, l, type_m, m, type_n, n, type_o, o, type_x, x                                                   \
)                                                                                                                      \
    EMTRACE_F_INFO_30(                                                                                                 \
        type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, type_i, i, type_j, j,  \
        type_k, k, type_l, l, type_m, m, type_n, n, type_o, o                                                          \
    )                                                                                                                  \
    #type_x,

#define EMTRACE_F_INFO_HELPER2(n, ...) EMTRACE_F_INFO_##n(__VA_ARGS__)
#define EMTRACE_F_INFO_HELPER(n, ...) EMTRACE_F_INFO_HELPER2(n, __VA_ARGS__)

#define EMTRACE_F_LAYOUT_0(a)
#define EMTRACE_F_LAYOUT_2(type_x, x) , offsetof(info_t, type_1), sizeof(type_x)
#define EMTRACE_F_LAYOUT_4(type_a, a, type_x, x) EMTRACE_F_LAYOUT_2(type_a, a), offsetof(info_t, type_2), sizeof(type_x)
#define EMTRACE_F_LAYOUT_6(type_a, a, type_b, b, type_x, x)                                                            \
    EMTRACE_F_LAYOUT_4(type_a, a, type_b, b), offsetof(info_t, type_3), sizeof(type_x)
#define EMTRACE_F_LAYOUT_8(type_a, a, type_b, b, type_c, c, type_x, x)                                                 \
    EMTRACE_F_LAYOUT_6(type_a, a, type_b, b, type_c, c), offsetof(info_t, type_4), sizeof(type_x)
#define EMTRACE_F_LAYOUT_10(type_a, a, type_b, b, type_c, c, type_d, d, type_x, x)                                     \
    EMTRACE_F_LAYOUT_8(type_a, a, type_b, b, type_c, c, type_d, d), offsetof(info_t, type_5), sizeof(type_x)
#define EMTRACE_F_LAYOUT_12(type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_x, x)                          \
    EMTRACE_F_LAYOUT_10(type_a, a, type_b, b, type_c, c, type_d, d, type_e, e), offsetof(info_t, type_6), sizeof(type_x)
#define EMTRACE_F_LAYOUT_14(type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_x, x)               \
    EMTRACE_F_LAYOUT_12(type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f), offsetof(info_t, type_7),   \
        sizeof(type_x)
#define EMTRACE_F_LAYOUT_16(type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_x, x)    \
    EMTRACE_F_LAYOUT_14(type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g),                  \
        offsetof(info_t, type_8), sizeof(type_x)
#define EMTRACE_F_LAYOUT_18(                                                                                           \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, type_x, x                  \
)                                                                                                                      \
    EMTRACE_F_LAYOUT_16(type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h),       \
        offsetof(info_t, type_9), sizeof(type_x)
#define EMTRACE_F_LAYOUT_20(                                                                                           \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, type_i, i, type_x, x       \
)                                                                                                                      \
    EMTRACE_F_LAYOUT_18(                                                                                               \
        type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, type_i, i              \
    ),                                                                                                                 \
        offsetof(info_t, type_10), sizeof(type_x)
#define EMTRACE_F_LAYOUT_22(                                                                                           \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, type_i, i, type_j, j,      \
    type_x, x                                                                                                          \
)                                                                                                                      \
    EMTRACE_F_LAYOUT_20(                                                                                               \
        type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, type_i, i, type_j, j   \
    ),                                                                                                                 \
        offsetof(info_t, type_11), sizeof(type_x)
#define EMTRACE_F_LAYOUT_24(                                                                                           \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, type_i, i, type_j, j,      \
    type_k, k, type_x, x                                                                                               \
)                                                                                                                      \
    EMTRACE_F_LAYOUT_22(                                                                                               \
        type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, type_i, i, type_j, j,  \
        type_k, k                                                                                                      \
    ),                                                                                                                 \
        offsetof(info_t, type_12), sizeof(type_x)
#define EMTRACE_F_LAYOUT_26(                                                                                           \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, type_i, i, type_j, j,      \
    type_k, k, type_l, l, type_x, x                                                                                    \
)                                                                                                                      \
    EMTRACE_F_LAYOUT_24(                                                                                               \
        type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, type_i, i, type_j, j,  \
        type_k, k, type_l, l                                                                                           \
    ),                                                                                                                 \
        offsetof(info_t, type_13), sizeof(type_x)
#define EMTRACE_F_LAYOUT_28(                                                                                           \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, type_i, i, type_j, j,      \
    type_k, k, type_l, l, type_m, m, type_x, x                                                                         \
)                                                                                                                      \
    EMTRACE_F_LAYOUT_26(                                                                                               \
        type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, type_i, i, type_j, j,  \
        type_k, k, type_l, l, type_m, m                                                                                \
    ),                                                                                                                 \
        offsetof(info_t, type_14), sizeof(type_x)
#define EMTRACE_F_LAYOUT_30(                                                                                           \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, type_i, i, type_j, j,      \
    type_k, k, type_l, l, type_m, m, type_n, n, type_x, x                                                              \
)                                                                                                                      \
    EMTRACE_F_LAYOUT_28(                                                                                               \
        type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, type_i, i, type_j, j,  \
        type_k, k, type_l, l, type_m, m, type_n, n                                                                     \
    ),                                                                                                                 \
        offsetof(info_t, type_15), sizeof(type_x)
#define EMTRACE_F_LAYOUT_32(                                                                                           \
    type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, type_i, i, type_j, j,      \
    type_k, k, type_l, l, type_m, m, type_n, n, type_o, o, type_x, x                                                   \
)                                                                                                                      \
    EMTRACE_F_LAYOUT_30(                                                                                               \
        type_a, a, type_b, b, type_c, c, type_d, d, type_e, e, type_f, f, type_g, g, type_h, h, type_i, i, type_j, j,  \
        type_k, k, type_l, l, type_m, m, type_n, n, type_o, o                                                          \
    ),                                                                                                                 \
        offsetof(info_t, type_16), sizeof(type_x)

#define EMTRACE_F_LAYOUT_HELPER2(n, ...) EMTRACE_F_LAYOUT_##n(__VA_ARGS__)
#define EMTRACE_F_LAYOUT_HELPER(n, ...) EMTRACE_F_LAYOUT_HELPER2(n, __VA_ARGS__)

#define EMTRACE_F_(fmt_info_attribute, out_fn, extra_arg, postfix, ...)                                                \
    do {                                                                                                               \
        typedef struct {                                                                                               \
            size_t layout[NUM_ARGS_REST(__VA_ARGS__) + 4];                                                             \
            char fmt[sizeof(FIRST_ARG(__VA_ARGS__) postfix)];                                                          \
            EMTRACE_F_INFO_MEMBER_HELPER(NUM_ARGS_REST(__VA_ARGS__), REST_ARGS(__VA_ARGS__))                           \
            char file[sizeof(__FILE__)];                                                                               \
        } info_t;                                                                                                      \
        EMTRACE_STATIC_ASSERT(offsetof(info_t, layout) == 0, "layout member in info struct must have offset 0");       \
        fmt_info_attribute const static info_t info = {                                                                \
            {NUM_ARGS_REST(__VA_ARGS__) / 2,                                                                           \
             offsetof(info_t, fmt) EMTRACE_F_LAYOUT_HELPER(NUM_ARGS_REST(__VA_ARGS__), REST_ARGS(__VA_ARGS__)),        \
             offsetof(info_t, file), __LINE__},                                                                        \
            FIRST_ARG(__VA_ARGS__) postfix,                                                                            \
            EMTRACE_F_INFO_HELPER(NUM_ARGS_REST(__VA_ARGS__), REST_ARGS(__VA_ARGS__)) __FILE__,                        \
        };                                                                                                             \
        const void* info_ptr = &info;                                                                                  \
        out_fn(&info_ptr, sizeof(info_ptr), extra_arg);                                                                \
        EMTRACE_F_HELPER(NUM_ARGS_REST(__VA_ARGS__), out_fn, extra_arg, REST_ARGS(__VA_ARGS__));                       \
    } while (0)

#define EMTRACE_(fmt_info_attribute, out_fn, extra_arg, string)                                                        \
    do {                                                                                                               \
        typedef struct {                                                                                               \
            size_t layout[4];                                                                                          \
            char str[sizeof(string)];                                                                                  \
            char file[sizeof(__FILE__)];                                                                               \
        } info_t;                                                                                                      \
        fmt_info_attribute const static info_t info = {                                                                \
            {EMTRACE_NO_FORMAT, offsetof(info_t, str), offsetof(info_t, file), __LINE__},                              \
            string,                                                                                                    \
            __FILE__,                                                                                                  \
        };                                                                                                             \
        const void* info_ptr = &info;                                                                                  \
        out_fn(&info_ptr, sizeof(info_ptr), extra_arg);                                                                \
    } while (0)

#define EMTRACE_S_(fmt_info_attribute, out_fn, extra_arg, postfix, str)                                                \
    do {                                                                                                               \
        typedef struct {                                                                                               \
            size_t layout[6];                                                                                          \
            char fmt[sizeof("{}" postfix)];                                                                            \
            char type_1[sizeof("string")];                                                                             \
            char file[sizeof(__FILE__)];                                                                               \
        } info_t;                                                                                                      \
        fmt_info_attribute const static info_t info = {                                                                \
            {1, offsetof(info_t, fmt), offsetof(info_t, type_1), EMTRACE_NULL_TERMINATED, offsetof(info_t, file),      \
             __LINE__},                                                                                                \
            "{}" postfix,                                                                                              \
            "string",                                                                                                  \
            __FILE__,                                                                                                  \
        };                                                                                                             \
        const void* info_ptr = &info;                                                                                  \
        out_fn(&info_ptr, sizeof(info_ptr), extra_arg);                                                                \
        const char* ptr = str;                                                                                         \
        while (1) {                                                                                                    \
            out_fn(ptr, sizeof(char), extra_arg);                                                                      \
            if (*ptr == 0)                                                                                             \
                break;                                                                                                 \
            ptr++;                                                                                                     \
        }                                                                                                              \
    } while (0)

#define EMTRACE_F(...)                                                                                                 \
    EMTRACE_F_(__attribute__((used)) __attribute__((section(".emtrace"))), emtrace_out_file, stdout, "", __VA_ARGS__)
#define EMTRACE(str) EMTRACE_(__attribute__((used)) __attribute__((section(".emtrace"))), emtrace_out_file, stdout, str)
#define EMTRACE_S(str)                                                                                                 \
    EMTRACE_S_(__attribute__((used)) __attribute__((section(".emtrace"))), emtrace_out_file, stdout, "", str)

#define EMTRACELN_F(...)                                                                                               \
    EMTRACE_F_(__attribute__((used)) __attribute__((section(".emtrace"))), emtrace_out_file, stdout, "\n", __VA_ARGS__)
#define EMTRACELN(str)                                                                                                 \
    EMTRACE_(__attribute__((used)) __attribute__((section(".emtrace"))), emtrace_out_file, stdout, str "\n")
#define EMTRACELN_S(str)                                                                                               \
    EMTRACE_S_(__attribute__((used)) __attribute__((section(".emtrace"))), emtrace_out_file, stdout, "\n", str)

// #define EMTRACE_F(sec_attr, out_fn, ...) EMTRACE_F_IMPL(sec_attr, out_fn,
// EMTRACE_F_HELPER(NUM_ARGS_REST(__VA_ARGS__)),
// __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif // EMTRACE_F_EMTRACE_F_H
