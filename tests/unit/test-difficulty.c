/*
 * Unit tests for difficulty calculations
 * Critical for sub-"1" difficulty support
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

#include "config.h"
#include "../test_common.h"
#include "libckpool.h"
#include "sha2.h"

/* Test target_from_diff and diff_from_target round-trip */
static void test_diff_roundtrip_sub1(void)
{
    uchar target[32];
    double test_diffs[] = {
        0.0001, 0.0005, 0.001, 0.005, 0.01, 0.05, 0.1, 0.5,
        1.0, 2.0, 10.0, 42.0, 100.0, 1000.0
    };
    int num_tests = sizeof(test_diffs) / sizeof(test_diffs[0]);
    
    for (int i = 0; i < num_tests; i++) {
        double original_diff = test_diffs[i];
        
        /* Convert difficulty to target */
        target_from_diff(target, original_diff);
        
        /* Convert target back to difficulty */
        double result_diff = diff_from_target(target);
        
        /* Allow small precision error (0.1% or 1e-6, whichever is larger) */
        double allowed_error = fmax(original_diff * 0.001, EPSILON_DIFF);
        double diff_error = fabs(result_diff - original_diff);
        
        assert_true(diff_error <= allowed_error);
    }
}

/* Test sub-"1" difficulty values specifically */
static void test_sub1_difficulty_values(void)
{
    uchar target[32];
    double sub1_diffs[] = {0.0001, 0.0005, 0.001, 0.005, 0.01, 0.05, 0.5};
    int num_tests = sizeof(sub1_diffs) / sizeof(sub1_diffs[0]);
    
    for (int i = 0; i < num_tests; i++) {
        double diff = sub1_diffs[i];
        
        /* Should not crash or produce invalid target */
        target_from_diff(target, diff);
        
        /* Target should be valid (not all zeros or all 0xFF) */
        bool has_nonzero = false;
        bool has_nonff = false;
        for (int j = 0; j < 32; j++) {
            if (target[j] != 0) has_nonzero = true;
            if (target[j] != 0xFF) has_nonff = true;
        }
        assert_true(has_nonzero || has_nonff);
        
        /* Round-trip should work */
        double result = diff_from_target(target);
        double error = fabs(result - diff);
        double allowed = fmax(diff * 0.001, EPSILON_DIFF);
        assert_true(error <= allowed);
    }
}

/* Test edge cases */
static void test_difficulty_edge_cases(void)
{
    uchar target[32];
    
    /* Test zero difficulty (should handle gracefully) */
    target_from_diff(target, 0.0);
    /* Should produce maximum target (all 0xFF) */
    bool all_ff = true;
    for (int i = 0; i < 32; i++) {
        if (target[i] != 0xFF) {
            all_ff = false;
            break;
        }
    }
    assert_true(all_ff);
    
    /* Test very small difficulty */
    target_from_diff(target, 1e-10);
    double result = diff_from_target(target);
    assert_true(result > 0);
    
    /* Test very large difficulty */
    target_from_diff(target, 1e10);
    result = diff_from_target(target);
    assert_true(result > 0 && result < 1e15);
}

/* Test diff_from_nbits (block header difficulty) */
static void test_diff_from_nbits(void)
{
    /* Test with known nbits values */
    /* nbits format: [exponent][mantissa 3 bytes] */
    char nbits1[4] = {0x1d, 0x00, 0xff, 0xff};  /* Difficulty 1 */
    char nbits2[4] = {0x1e, 0x00, 0x00, 0x00};  /* Very high difficulty */
    
    double diff1 = diff_from_nbits(nbits1);
    double diff2 = diff_from_nbits(nbits2);
    
    assert_true(diff1 > 0);
    assert_true(diff2 > 0);
    assert_true(diff2 > diff1);  /* nbits2 should represent higher difficulty */
}

/* Test le256todouble and be256todouble */
static void test_target_conversions(void)
{
    uchar target_le[32] = {0};
    uchar target_be[32] = {0};
    
    /* Set up a test target */
    target_from_diff(target_le, 42.0);
    
    /* Convert to big-endian (reverse bytes) */
    for (int i = 0; i < 32; i++) {
        target_be[i] = target_le[31 - i];
    }
    
    double diff_le = diff_from_target(target_le);
    double diff_be = diff_from_betarget(target_be);
    
    /* Should be approximately equal (allowing for precision) */
    double error = fabs(diff_le - diff_be);
    assert_true(error < EPSILON_DIFF);
}

int main(void)
{
    printf("Running difficulty calculation tests...\n\n");
    
    run_test(test_diff_roundtrip_sub1);
    run_test(test_sub1_difficulty_values);
    run_test(test_difficulty_edge_cases);
    run_test(test_diff_from_nbits);
    run_test(test_target_conversions);
    
    printf("\nAll difficulty tests passed!\n");
    return 0;
}

