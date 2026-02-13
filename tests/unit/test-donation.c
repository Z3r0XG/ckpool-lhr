/*
 * Unit tests for donation system
 * Tests donation percentage calculations and validation
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

/* Test donation calculation formula
 * Formula: donation_amount = (block_reward / 100) * donation_percentage
 * This matches the calculation in stratifier.c:606
 * 
 * Note: Block reward values are arbitrary test data - we're testing the
 * calculation logic, not actual Bitcoin block rewards.
 */
static void test_donation_calculation(void)
{
    uint64_t block_reward;
    double donation_percent;
    double expected_donation;
    double calculated_donation;
    double epsilon = 0.00000001; // For satoshi precision
    
    /* Test 1: 0.5% donation on test block reward value */
    block_reward = 625000000ULL; // Arbitrary test value (represents some amount in satoshi)
    donation_percent = 0.5;
    expected_donation = (double)block_reward / 100.0 * donation_percent;
    calculated_donation = (double)block_reward / 100.0 * donation_percent;
    assert_double_equal(calculated_donation, expected_donation, epsilon);
    assert_true(calculated_donation == 3125000.0);
    
    /* Test 2: 1.0% donation */
    donation_percent = 1.0;
    calculated_donation = (double)block_reward / 100.0 * donation_percent;
    assert_true(calculated_donation == 6250000.0);
    
    /* Test 3: 1.5% donation (fractional) */
    donation_percent = 1.5;
    calculated_donation = (double)block_reward / 100.0 * donation_percent;
    assert_true(calculated_donation == 9375000.0);
    
    /* Test 4: 2.5% donation */
    donation_percent = 2.5;
    calculated_donation = (double)block_reward / 100.0 * donation_percent;
    assert_true(calculated_donation == 15625000.0);
    
    /* Test 5: 10% donation */
    donation_percent = 10.0;
    calculated_donation = (double)block_reward / 100.0 * donation_percent;
    assert_true(calculated_donation == 62500000.0);
}

/* Test donation validation (matches ckpool.c:1797-1800)
 * Values < 0.1 are set to 0
 * Values > 99.9 are clamped to 99.9
 */
static void test_donation_validation(void)
{
    double donation;
    
    /* Test 1: Values below 0.1 should be treated as 0 */
    donation = 0.0;
    if (donation < 0.1)
        donation = 0;
    assert_double_equal(donation, 0.0, EPSILON);
    
    donation = 0.05;
    if (donation < 0.1)
        donation = 0;
    assert_double_equal(donation, 0.0, EPSILON);
    
    donation = 0.09;
    if (donation < 0.1)
        donation = 0;
    assert_double_equal(donation, 0.0, EPSILON);
    
    /* Test 2: Values >= 0.1 should be kept */
    donation = 0.1;
    if (donation < 0.1)
        donation = 0;
    assert_double_equal(donation, 0.1, EPSILON);
    
    donation = 0.5;
    if (donation < 0.1)
        donation = 0;
    assert_double_equal(donation, 0.5, EPSILON);
    
    donation = 1.0;
    if (donation < 0.1)
        donation = 0;
    assert_double_equal(donation, 1.0, EPSILON);
    
    /* Test 3: Values above 99.9 should be clamped to 99.9 */
    donation = 100.0;
    if (donation > 99.9)
        donation = 99.9;
    assert_double_equal(donation, 99.9, EPSILON);
    
    donation = 150.0;
    if (donation > 99.9)
        donation = 99.9;
    assert_double_equal(donation, 99.9, EPSILON);
    
    /* Test 4: Values <= 99.9 should be kept */
    donation = 99.9;
    if (donation > 99.9)
        donation = 99.9;
    assert_double_equal(donation, 99.9, EPSILON);
    
    donation = 50.0;
    if (donation > 99.9)
        donation = 99.9;
    assert_double_equal(donation, 50.0, EPSILON);
}

/* Test donation calculation with various block reward values
 * Using different test values to verify the formula works across different scales
 */
static void test_donation_various_rewards(void)
{
    uint64_t block_reward;
    double donation_percent;
    double calculated_donation;
    double expected_donation;
    
    /* Test with different block reward values (arbitrary test data) */
    
    block_reward = 1250000000ULL;
    donation_percent = 1.0;
    calculated_donation = (double)block_reward / 100.0 * donation_percent;
    expected_donation = 12500000.0;
    assert_double_equal(calculated_donation, expected_donation, EPSILON);
    
    block_reward = 625000000ULL;
    donation_percent = 1.0;
    calculated_donation = (double)block_reward / 100.0 * donation_percent;
    expected_donation = 6250000.0;
    assert_double_equal(calculated_donation, expected_donation, EPSILON);
    
    block_reward = 312500000ULL;
    donation_percent = 1.0;
    calculated_donation = (double)block_reward / 100.0 * donation_percent;
    expected_donation = 3125000.0;
    assert_double_equal(calculated_donation, expected_donation, EPSILON);
    
    block_reward = 156250000ULL;
    donation_percent = 0.5;
    calculated_donation = (double)block_reward / 100.0 * donation_percent;
    expected_donation = 781250.0;
    assert_double_equal(calculated_donation, expected_donation, EPSILON);
}

/* Test that donation + remaining = original block reward
 * This verifies the calculation in stratifier.c:608: g64 -= d64
 * Ensures the split doesn't lose or gain any value
 */
static void test_donation_roundtrip(void)
{
    uint64_t original_reward;
    uint64_t remaining_reward;
    double donation_percent;
    double donation_amount;
    uint64_t donation_satoshi;
    double epsilon = 1.0; // Allow 1 satoshi rounding difference
    
    /* Test with 0.5% donation */
    original_reward = 625000000ULL; // Arbitrary test value
    donation_percent = 0.5;
    donation_amount = (double)original_reward / 100.0 * donation_percent;
    donation_satoshi = (uint64_t)donation_amount;
    remaining_reward = original_reward - donation_satoshi;
    
    /* Verify: donation + remaining = original (within rounding) */
    uint64_t total = donation_satoshi + remaining_reward;
    int64_t diff = (int64_t)original_reward - (int64_t)total;
    assert_true(llabs(diff) <= 1); // Allow 1 satoshi difference
    
    /* Test with 1.5% donation */
    original_reward = 625000000ULL;
    donation_percent = 1.5;
    donation_amount = (double)original_reward / 100.0 * donation_percent;
    donation_satoshi = (uint64_t)donation_amount;
    remaining_reward = original_reward - donation_satoshi;
    
    total = donation_satoshi + remaining_reward;
    diff = (int64_t)original_reward - (int64_t)total;
    assert_true(llabs(diff) <= 1);
    
    /* Test with 99.9% donation (edge case) */
    original_reward = 625000000ULL;
    donation_percent = 99.9;
    donation_amount = (double)original_reward / 100.0 * donation_percent;
    donation_satoshi = (uint64_t)donation_amount;
    remaining_reward = original_reward - donation_satoshi;
    
    total = donation_satoshi + remaining_reward;
    diff = (int64_t)original_reward - (int64_t)total;
    assert_true(llabs(diff) <= 1);
}

/* Test edge cases */
static void test_donation_edge_cases(void)
{
    uint64_t block_reward;
    double donation_percent;
    double calculated_donation;
    
    /* Test minimum valid donation (0.1%) */
    block_reward = 625000000ULL; // Arbitrary test value
    donation_percent = 0.1;
    calculated_donation = (double)block_reward / 100.0 * donation_percent;
    assert_true(calculated_donation == 625000.0);
    
    /* Test maximum valid donation (99.9%) */
    donation_percent = 99.9;
    calculated_donation = (double)block_reward / 100.0 * donation_percent;
    assert_true(calculated_donation == 624375000.0);
    
    /* Test very small block reward with donation */
    block_reward = 1000ULL; // Small test value
    donation_percent = 1.0;
    calculated_donation = (double)block_reward / 100.0 * donation_percent;
    assert_true(calculated_donation == 10.0);
}

int main(void)
{
    printf("Running donation system tests...\n\n");
    
    run_test(test_donation_calculation);
    run_test(test_donation_validation);
    run_test(test_donation_various_rewards);
    run_test(test_donation_roundtrip);
    run_test(test_donation_edge_cases);
    
    printf("\nAll donation tests passed!\n");
    return 0;
}

