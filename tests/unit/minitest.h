/* SPDX-License-Identifier: MIT */
/*
 * Tiny unit-test helper: no dependencies, works with gcc/clang and sanitizers.
 *
 *   TEST(name) { ASSERT_EQ(1, 1); }
 *   int main(void) { RUN(name); return minitest_report(); }
 */
#ifndef MINITEST_H
#define MINITEST_H

#include <stdio.h>
#include <string.h>

static int mt_failed_asserts;
static int mt_tests_run;
static int mt_tests_failed;
static int mt_cur_failed;

#define TEST(name) static void name(void)

#define MT_FAIL(fmt, ...)                                                             \
    do {                                                                              \
        fprintf(stderr, "  %s:%d: " fmt "\n", __FILE__, __LINE__, __VA_ARGS__);       \
        mt_failed_asserts++;                                                          \
        mt_cur_failed = 1;                                                            \
    } while (0)

#define ASSERT_TRUE(c)                                                                \
    do {                                                                              \
        if (!(c))                                                                     \
            MT_FAIL("expected true: %s", #c);                                         \
    } while (0)

#define ASSERT_EQ(exp, act)                                                           \
    do {                                                                              \
        long long mt_e = (long long)(exp), mt_a = (long long)(act);                   \
        if (mt_e != mt_a)                                                             \
            MT_FAIL("%s == %s: expected %lld, got %lld", #exp, #act, mt_e, mt_a);     \
    } while (0)

#define ASSERT_MEM_EQ(exp, act, n)                                                    \
    do {                                                                              \
        if (memcmp((exp), (act), (n)) != 0)                                           \
            MT_FAIL("memory differs: %s vs %s (%zu bytes)", #exp, #act, (size_t)(n)); \
    } while (0)

#define RUN(name)                                                                     \
    do {                                                                              \
        mt_cur_failed = 0;                                                            \
        mt_tests_run++;                                                               \
        name();                                                                       \
        if (mt_cur_failed)                                                            \
            mt_tests_failed++;                                                        \
        printf("[%s] %s\n", mt_cur_failed ? "FAIL" : " OK ", #name);                  \
    } while (0)

static inline int minitest_report(void)
{
    printf("\n%d tests, %d failed, %d failed assertions\n", mt_tests_run, mt_tests_failed,
           mt_failed_asserts);
    return mt_tests_failed ? 1 : 0;
}

#endif /* MINITEST_H */
