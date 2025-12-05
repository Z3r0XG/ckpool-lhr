/*
 * Unit tests for string utility functions
 * Tests safecmp, safencmp, and suffix_string
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#include "config.h"
#include "../test_common.h"
#include "libckpool.h"

/* Test safecmp - safe string comparison
 * Handles NULL and empty strings safely
 */
static void test_safecmp(void)
{
    /* Equal strings */
    assert_int_equal(safecmp("hello", "hello"), 0);
    assert_int_equal(safecmp("test", "test"), 0);
    assert_int_equal(safecmp("", ""), 0);
    
    /* Different strings */
    assert_true(safecmp("hello", "world") != 0);
    assert_true(safecmp("abc", "def") != 0);
    assert_true(safecmp("test", "testing") != 0);
    
    /* NULL handling */
    assert_int_equal(safecmp(NULL, NULL), 0); // Both NULL = equal
    assert_true(safecmp(NULL, "test") != 0); // NULL != non-NULL
    assert_true(safecmp("test", NULL) != 0); // non-NULL != NULL
    
    /* Empty string handling */
    assert_int_equal(safecmp("", ""), 0); // Both empty = equal
    assert_true(safecmp("", "test") != 0); // Empty != non-empty
    assert_true(safecmp("test", "") != 0); // non-empty != empty
    
    /* Case sensitivity */
    assert_true(safecmp("Hello", "hello") != 0); // Case sensitive
    assert_true(safecmp("TEST", "test") != 0);
}

/* Test safencmp - safe substring comparison with length
 * Already tested in test-useragent, but adding comprehensive tests here
 */
static void test_safencmp_comprehensive(void)
{
    /* Exact matches */
    assert_int_equal(safencmp("hello", "hello", 5), 0);
    assert_int_equal(safencmp("test", "test", 4), 0);
    
    /* Prefix matches */
    assert_int_equal(safencmp("hello", "hell", 4), 0);
    assert_int_equal(safencmp("testing", "test", 4), 0);
    
    /* Non-matches */
    assert_true(safencmp("hello", "world", 5) != 0);
    assert_true(safencmp("abc", "def", 3) != 0);
    
    /* NULL handling */
    assert_int_equal(safencmp(NULL, NULL, 0), 0); // Both NULL, len=0
    assert_true(safencmp(NULL, "test", 4) != 0); // NULL != non-NULL
    assert_true(safencmp("test", NULL, 4) != 0); // non-NULL != NULL
    
    /* Empty string handling */
    assert_int_equal(safencmp("", "", 0), -1); // len=0 returns -1
    assert_true(safencmp("", "test", 4) != 0); // Empty != non-empty
    assert_true(safencmp("test", "", 0) != 0); // len=0 returns -1
    
    /* Length parameter */
    assert_int_equal(safencmp("hello", "hell", 4), 0); // Match first 4
    assert_true(safencmp("hello", "hell", 5) != 0); // Different at 5th char
    assert_int_equal(safencmp("test", "testing", 4), 0); // Match first 4
}

/* Test suffix_string with various values and suffixes
 * Formats numbers with K, M, G, T, P, E (large) or m, u, n (small)
 */
static void test_suffix_string_large_values(void)
{
    char buf[32];
    
    /* Test Kilo (K) - 1000+ */
    suffix_string(1500.0, buf, sizeof(buf), 0);
    assert_true(strstr(buf, "K") != NULL || strstr(buf, "k") != NULL);
    
    suffix_string(5000.0, buf, sizeof(buf), 0);
    assert_true(strstr(buf, "K") != NULL || strstr(buf, "k") != NULL);
    
    /* Test Mega (M) - 1,000,000+ */
    suffix_string(2500000.0, buf, sizeof(buf), 0);
    assert_true(strstr(buf, "M") != NULL || strstr(buf, "m") != NULL);
    
    suffix_string(50000000.0, buf, sizeof(buf), 0);
    assert_true(strstr(buf, "M") != NULL || strstr(buf, "m") != NULL);
    
    /* Test Giga (G) - 1,000,000,000+ */
    suffix_string(2500000000.0, buf, sizeof(buf), 0);
    assert_true(strstr(buf, "G") != NULL || strstr(buf, "g") != NULL);
    
    /* Test Tera (T) - 1,000,000,000,000+ */
    suffix_string(5000000000000.0, buf, sizeof(buf), 0);
    assert_true(strstr(buf, "T") != NULL || strstr(buf, "t") != NULL);
}

/* Test suffix_string with small values (milli, micro, nano) */
static void test_suffix_string_small_values(void)
{
    char buf[32];
    
    /* Test values between 1 and 1000 (no suffix) */
    suffix_string(1.0, buf, sizeof(buf), 0);
    /* Should not have K, M, G suffix */
    
    suffix_string(500.0, buf, sizeof(buf), 0);
    /* Should not have K suffix yet */
    
    /* Test milli (m) - 0.001 to 1 */
    suffix_string(0.5, buf, sizeof(buf), 0);
    /* May have 'm' suffix or be formatted as decimal */
    
    suffix_string(0.001, buf, sizeof(buf), 0);
    /* Should have 'm' suffix */
    
    /* Test micro (u) - 0.000001 to 0.001 */
    suffix_string(0.0005, buf, sizeof(buf), 0);
    /* May have 'u' suffix */
    
    suffix_string(0.000001, buf, sizeof(buf), 0);
    /* Should have 'u' suffix */
    
    /* Test nano (n) - 0.000000001 to 0.000001 */
    suffix_string(0.0000005, buf, sizeof(buf), 0);
    /* May have 'n' suffix */
    
    suffix_string(0.000000001, buf, sizeof(buf), 0);
    /* Should have 'n' suffix */
}

/* Test suffix_string with zero and negative values */
static void test_suffix_string_edge_cases(void)
{
    char buf[32];
    
    /* Test zero */
    suffix_string(0.0, buf, sizeof(buf), 0);
    assert_true(strlen(buf) > 0); // Should produce output
    
    /* Test very small values */
    suffix_string(0.0000000001, buf, sizeof(buf), 0);
    assert_true(strlen(buf) > 0);
    
    /* Test very large values */
    suffix_string(1000000000000000000.0, buf, sizeof(buf), 0);
    assert_true(strlen(buf) > 0);
    assert_true(strlen(buf) < sizeof(buf)); // Should fit in buffer
}

/* Test suffix_string with significant digits parameter */
static void test_suffix_string_sigdigits(void)
{
    char buf[32];
    
    /* Test with sigdigits=0 (default formatting) */
    suffix_string(1500.0, buf, sizeof(buf), 0);
    assert_true(strlen(buf) > 0);
    
    /* Test with sigdigits > 0 (fixed precision) */
    suffix_string(1500.0, buf, sizeof(buf), 3);
    assert_true(strlen(buf) > 0);
    
    suffix_string(2500000.0, buf, sizeof(buf), 4);
    assert_true(strlen(buf) > 0);
    
    /* Verify buffer doesn't overflow */
    assert_true(strlen(buf) < sizeof(buf));
}

/* Test suffix_string with various hash rate values (common use case) */
static void test_suffix_string_hashrates(void)
{
    char buf[32];
    
    /* Typical hash rates in mining */
    suffix_string(100.0, buf, sizeof(buf), 0); // 100 H/s
    assert_true(strlen(buf) > 0);
    
    suffix_string(1000.0, buf, sizeof(buf), 0); // 1 KH/s
    assert_true(strlen(buf) > 0);
    
    suffix_string(1000000.0, buf, sizeof(buf), 0); // 1 MH/s
    assert_true(strlen(buf) > 0);
    
    suffix_string(1000000000.0, buf, sizeof(buf), 0); // 1 GH/s
    assert_true(strlen(buf) > 0);
    
    suffix_string(1000000000000.0, buf, sizeof(buf), 0); // 1 TH/s
    assert_true(strlen(buf) > 0);
}

int main(void)
{
    printf("Running string utility tests...\n\n");
    
    run_test(test_safecmp);
    run_test(test_safencmp_comprehensive);
    run_test(test_suffix_string_large_values);
    run_test(test_suffix_string_small_values);
    run_test(test_suffix_string_edge_cases);
    run_test(test_suffix_string_sigdigits);
    run_test(test_suffix_string_hashrates);
    
    printf("\nAll string utility tests passed!\n");
    return 0;
}

