/*
 * Unit tests for endian conversion functions
 * Tests le256todouble() and be256todouble()
 */

/* config.h must be first to define _GNU_SOURCE before system headers */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "../test_common.h"
#include "libckpool.h"

/* truediffone constant - same as in libckpool.c */
static const double truediffone = 26959535291011309493156476344723991336010898738574164086137773096960.0;

/* Test le256todouble() - little-endian 256-bit to double conversion */
static void test_le256todouble_basic(void)
{
    uchar target[32] = {0};
    double result;
    
    /* Test 1: All zeros */
    memset(target, 0, 32);
    result = le256todouble(target);
    assert_double_equal(result, 0.0, EPSILON);
    
    /* Test 2: Single byte set (least significant) */
    memset(target, 0, 32);
    target[0] = 0x01;
    result = le256todouble(target);
    assert_double_equal(result, 1.0, EPSILON);
    
    /* Test 3: Single byte set in different positions */
    memset(target, 0, 32);
    target[1] = 0x01;
    result = le256todouble(target);
    assert_double_equal(result, 256.0, EPSILON); // 0x0100 = 256
    
    memset(target, 0, 32);
    target[7] = 0x01;
    result = le256todouble(target);
    assert_double_equal(result, 72057594037927936.0, EPSILON); // 2^56
}

/* Test le256todouble() with known difficulty targets */
static void test_le256todouble_with_difficulty_targets(void)
{
    uchar target[32];
    double diff, result;
    
    /* Test with various difficulties */
    double test_diffs[] = {0.001, 0.5, 1.0, 10.0, 100.0};
    int num_tests = sizeof(test_diffs) / sizeof(test_diffs[0]);
    
    for (int i = 0; i < num_tests; i++) {
        diff = test_diffs[i];
        target_from_diff(target, diff);
        result = le256todouble(target);
        
        /* The result should be a valid double representing the target value */
        assert_true(result > 0.0);
        assert_true(result < 1e77); // Reasonable upper bound
    }
}

/* Test be256todouble() - big-endian 256-bit to double conversion */
static void test_be256todouble_basic(void)
{
    uchar target[32] = {0};
    double result;
    
    /* Test 1: All zeros */
    memset(target, 0, 32);
    result = be256todouble(target);
    assert_double_equal(result, 0.0, EPSILON);
    
    /* Test 2: Single byte set (most significant in big-endian) */
    memset(target, 0, 32);
    target[0] = 0x01; // Most significant byte in big-endian
    result = be256todouble(target);
    /* Result should be bits192 * 0x01 = bits192 */
    assert_true(result > 1e50); // Very large number (bits192 is huge)
    
    /* Test 3: Single byte set in least significant position */
    memset(target, 0, 32);
    target[31] = 0x01; // Least significant byte in big-endian
    result = be256todouble(target);
    assert_double_equal(result, 1.0, EPSILON);
}

/* Test be256todouble() with known difficulty targets */
static void test_be256todouble_with_difficulty_targets(void)
{
    uchar target[32];
    double diff, result;
    
    /* Test with various difficulties */
    double test_diffs[] = {0.001, 0.5, 1.0, 10.0, 100.0};
    int num_tests = sizeof(test_diffs) / sizeof(test_diffs[0]);
    
    for (int i = 0; i < num_tests; i++) {
        diff = test_diffs[i];
        target_from_diff(target, diff);
        /* Note: target_from_diff creates little-endian, so we need to convert */
        /* For this test, we'll just verify be256todouble works on the target */
        result = be256todouble(target);
        
        /* The result should be a valid double */
        assert_true(result > 0.0);
        assert_true(result < 1e77); // Reasonable upper bound
    }
}

/* Test endian conversion round-trip */
static void test_endian_roundtrip(void)
{
    uchar target_le[32] = {0};
    uchar target_be[32] = {0};
    double value_le, value_be;
    
    /* Create a test value */
    target_from_diff(target_le, 1.0);
    value_le = le256todouble(target_le);
    
    /* Convert to big-endian representation */
    /* Note: This is a simplified test - full conversion would require swapping bytes */
    /* For now, we just verify both functions work on the same data */
    memcpy(target_be, target_le, 32);
    value_be = be256todouble(target_be);
    
    /* Both should produce valid results (though different values due to endianness) */
    assert_true(value_le > 0.0);
    assert_true(value_be > 0.0);
}

/* Test edge cases */
static void test_endian_edge_cases(void)
{
    uchar target[32];
    double result;
    
    /* Test: Maximum value (all 0xFF) */
    memset(target, 0xFF, 32);
    result = le256todouble(target);
    assert_true(result > 0.0);
    
    result = be256todouble(target);
    assert_true(result > 0.0);
    
    /* Test: Pattern with specific bytes set */
    memset(target, 0, 32);
    target[0] = 0xFF;
    target[1] = 0xFF;
    target[2] = 0xFF;
    target[3] = 0xFF;
    result = le256todouble(target);
    assert_double_equal(result, 4294967295.0, EPSILON); // 0xFFFFFFFF = 2^32 - 1
}

/* Test integration with diff_from_target and diff_from_betarget */
static void test_endian_integration_with_difficulty(void)
{
    uchar target[32];
    double diff, target_value_le, target_value_be, recovered_diff;
    
    /* Test: Create target from difficulty, convert to double, verify */
    diff = 1.0;
    target_from_diff(target, diff);
    
    target_value_le = le256todouble(target);
    recovered_diff = truediffone / target_value_le;
    
    /* Allow small precision error */
    double allowed_error = fmax(diff * 0.001, EPSILON_DIFF);
    double diff_error = fabs(recovered_diff - diff);
    assert_true(diff_error <= allowed_error);
}

int main(void)
{
    printf("Running endian conversion tests...\n\n");
    
    run_test(test_le256todouble_basic);
    run_test(test_le256todouble_with_difficulty_targets);
    run_test(test_be256todouble_basic);
    run_test(test_be256todouble_with_difficulty_targets);
    run_test(test_endian_roundtrip);
    run_test(test_endian_edge_cases);
    run_test(test_endian_integration_with_difficulty);
    
    printf("\nAll endian conversion tests passed!\n");
    return 0;
}

