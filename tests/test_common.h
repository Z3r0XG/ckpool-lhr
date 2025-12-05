/*
 * Common test utilities and macros
 * Provides a simple testing framework
 */

#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <setjmp.h>

/* Simple test framework */
#define assert_true(expr) \
    do { \
        if (!(expr)) { \
            fprintf(stderr, "FAIL: %s:%d: Assertion failed: %s\n", \
                    __FILE__, __LINE__, #expr); \
            exit(1); \
        } \
    } while (0)

#define assert_false(expr) assert_true(!(expr))

#define assert_int_equal(a, b) \
    do { \
        if ((a) != (b)) { \
            fprintf(stderr, "FAIL: %s:%d: Expected %d, got %d\n", \
                    __FILE__, __LINE__, (int)(b), (int)(a)); \
            exit(1); \
        } \
    } while (0)

#define assert_double_equal(a, b, epsilon) \
    do { \
        double _diff = fabs((a) - (b)); \
        if (_diff > (epsilon)) { \
            fprintf(stderr, "FAIL: %s:%d: Expected %.10f, got %.10f (diff: %.10f)\n", \
                    __FILE__, __LINE__, (double)(b), (double)(a), _diff); \
            exit(1); \
        } \
    } while (0)

#define assert_string_equal(a, b) \
    do { \
        if (strcmp((a), (b)) != 0) { \
            fprintf(stderr, "FAIL: %s:%d: Expected '%s', got '%s'\n", \
                    __FILE__, __LINE__, (b), (a)); \
            exit(1); \
        } \
    } while (0)

#define assert_ptr_equal(a, b) \
    do { \
        if ((a) != (b)) { \
            fprintf(stderr, "FAIL: %s:%d: Pointers not equal\n", \
                    __FILE__, __LINE__); \
            exit(1); \
        } \
    } while (0)

#define assert_null(ptr) assert_ptr_equal(ptr, NULL)
#define assert_non_null(ptr) assert_true((ptr) != NULL)

#define assert_memory_equal(a, b, size) \
    do { \
        if (memcmp((a), (b), (size)) != 0) { \
            fprintf(stderr, "FAIL: %s:%d: Memory not equal\n", \
                    __FILE__, __LINE__); \
            exit(1); \
        } \
    } while (0)

#define run_test(name) \
    do { \
        printf("Running: %s\n", #name); \
        name(); \
        printf("  PASS: %s\n", #name); \
    } while (0)

#define TEST_SUCCESS 0
#define TEST_FAILURE 1

/* Common test utilities */
#define EPSILON 1e-9
#define EPSILON_DIFF 1e-6  /* For difficulty calculations */

#endif /* TEST_COMMON_H */
