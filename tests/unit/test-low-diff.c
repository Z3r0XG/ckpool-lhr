/*
 * Unit tests for allow_low_diff feature
 * Tests network difficulty floor behavior
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

#include "../test_common.h"

/*
 * Simulates the network_diff clamping logic from stratifier.c add_base()
 * This mirrors the actual code:
 *   if (!ckp->allow_low_diff && wb->network_diff < 1)
 *       wb->network_diff = 1;
 */
static double apply_network_diff_floor(double raw_diff, bool allow_low_diff)
{
    double network_diff = raw_diff;
    
    /* This is the actual logic from stratifier.c */
    if (!allow_low_diff && network_diff < 1)
        network_diff = 1;
    
    return network_diff;
}

/* Test: With allow_low_diff=false, diff should be clamped to 1.0 */
static void test_low_diff_disabled_clamps_to_one(void)
{
    double test_diffs[] = {0.0001, 0.001, 0.01, 0.1, 0.5, 0.9, 0.99};
    int num_tests = sizeof(test_diffs) / sizeof(test_diffs[0]);
    
    for (int i = 0; i < num_tests; i++) {
        double result = apply_network_diff_floor(test_diffs[i], false);
        assert_double_equal(result, 1.0, EPSILON_DIFF);
    }
}

/* Test: With allow_low_diff=false, diff >= 1.0 should pass through unchanged */
static void test_low_diff_disabled_passes_high_diff(void)
{
    double test_diffs[] = {1.0, 1.5, 2.0, 10.0, 100.0, 1000000.0};
    int num_tests = sizeof(test_diffs) / sizeof(test_diffs[0]);
    
    for (int i = 0; i < num_tests; i++) {
        double result = apply_network_diff_floor(test_diffs[i], false);
        assert_double_equal(result, test_diffs[i], EPSILON_DIFF);
    }
}

/* Test: With allow_low_diff=true, low diff should pass through unchanged */
static void test_low_diff_enabled_passes_low_diff(void)
{
    double test_diffs[] = {0.0001, 0.001, 0.01, 0.1, 0.5, 0.9, 0.99};
    int num_tests = sizeof(test_diffs) / sizeof(test_diffs[0]);
    
    for (int i = 0; i < num_tests; i++) {
        double result = apply_network_diff_floor(test_diffs[i], true);
        assert_double_equal(result, test_diffs[i], EPSILON_DIFF);
    }
}

/* Test: With allow_low_diff=true, high diff should also pass through */
static void test_low_diff_enabled_passes_high_diff(void)
{
    double test_diffs[] = {1.0, 1.5, 2.0, 10.0, 100.0, 1000000.0};
    int num_tests = sizeof(test_diffs) / sizeof(test_diffs[0]);
    
    for (int i = 0; i < num_tests; i++) {
        double result = apply_network_diff_floor(test_diffs[i], true);
        assert_double_equal(result, test_diffs[i], EPSILON_DIFF);
    }
}

/* Test: Edge case - exactly 1.0 should pass through regardless of setting */
static void test_diff_exactly_one(void)
{
    assert_double_equal(apply_network_diff_floor(1.0, false), 1.0, EPSILON_DIFF);
    assert_double_equal(apply_network_diff_floor(1.0, true), 1.0, EPSILON_DIFF);
}

/* Test: Edge case - zero diff */
static void test_diff_zero(void)
{
    /* With flag disabled, should clamp to 1.0 */
    assert_double_equal(apply_network_diff_floor(0.0, false), 1.0, EPSILON_DIFF);
    
    /* With flag enabled, should stay 0.0 (regtest edge case) */
    assert_double_equal(apply_network_diff_floor(0.0, true), 0.0, EPSILON_DIFF);
}

/* Test: Regtest-like very low diff */
static void test_regtest_diff(void)
{
    /* Regtest often has extremely low network diff */
    double regtest_diff = 0.00000001;
    
    /* Disabled: should clamp */
    assert_double_equal(apply_network_diff_floor(regtest_diff, false), 1.0, EPSILON_DIFF);
    
    /* Enabled: should pass through */
    assert_double_equal(apply_network_diff_floor(regtest_diff, true), regtest_diff, EPSILON_DIFF);
}

int main(void)
{
    printf("Running allow_low_diff feature tests...\n\n");
    
    run_test(test_low_diff_disabled_clamps_to_one);
    run_test(test_low_diff_disabled_passes_high_diff);
    run_test(test_low_diff_enabled_passes_low_diff);
    run_test(test_low_diff_enabled_passes_high_diff);
    run_test(test_diff_exactly_one);
    run_test(test_diff_zero);
    run_test(test_regtest_diff);
    
    printf("\nAll allow_low_diff tests passed!\n");
    return 0;
}
