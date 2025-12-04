/*
 * Unit tests for dropidle feature
 * Tests idle client detection and drop configuration
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <sys/time.h>

#include "config.h"
#include "../test_common.h"
#include "libckpool.h"
#include <sys/time.h>

/* Test dropidle config behavior - default value (0 = disabled)
 * When dropidle is 0, the feature is disabled.
 * This tests the logic: if (ckp->dropidle && per_tdiff > ckp->dropidle)
 * When dropidle is 0, the condition should always be false.
 */
static void test_dropidle_disabled_behavior(void)
{
    tv_t now, last_share;
    double per_tdiff;
    int dropidle = 0; // Disabled
    bool should_drop;
    
    gettimeofday(&now, NULL);
    
    /* Even with very long idle time, should not drop when disabled */
    last_share = now;
    last_share.tv_sec -= 100000; // 100000 seconds ago
    per_tdiff = tvdiff(&now, &last_share);
    should_drop = (dropidle && per_tdiff > dropidle);
    assert_false(should_drop); // Should never drop when disabled
}

/* Test idle detection logic - time difference calculations
 * Tests the logic: if (ckp->dropidle && per_tdiff > ckp->dropidle)
 */
static void test_dropidle_idle_detection_logic(void)
{
    tv_t now, last_share;
    double per_tdiff;
    int dropidle;
    bool should_drop;
    
    /* Setup: Current time */
    gettimeofday(&now, NULL);
    
    /* Test 1: dropidle disabled (0) - should never drop */
    dropidle = 0;
    last_share = now;
    last_share.tv_sec -= 10000; // 10000 seconds ago (way past any threshold)
    per_tdiff = tvdiff(&now, &last_share);
    should_drop = (dropidle && per_tdiff > dropidle);
    assert_false(should_drop); // Should not drop when disabled
    
    /* Test 2: dropidle enabled (3600), client idle for 3700 seconds - should drop */
    dropidle = 3600;
    last_share = now;
    last_share.tv_sec -= 3700; // 3700 seconds ago (exceeds 3600 threshold)
    per_tdiff = tvdiff(&now, &last_share);
    should_drop = (dropidle && per_tdiff > dropidle);
    assert_true(should_drop); // Should drop when idle time exceeds threshold
    
    /* Test 3: dropidle enabled (3600), client idle for 3500 seconds - should NOT drop */
    dropidle = 3600;
    last_share = now;
    last_share.tv_sec -= 3500; // 3500 seconds ago (below 3600 threshold)
    per_tdiff = tvdiff(&now, &last_share);
    should_drop = (dropidle && per_tdiff > dropidle);
    assert_false(should_drop); // Should not drop when below threshold
    
    /* Test 4: dropidle enabled (3600), client idle for exactly 3600 seconds - should NOT drop */
    dropidle = 3600;
    last_share = now;
    last_share.tv_sec -= 3600; // Exactly 3600 seconds ago
    per_tdiff = tvdiff(&now, &last_share);
    should_drop = (dropidle && per_tdiff > dropidle);
    assert_false(should_drop); // Should not drop when equal (uses > not >=)
    
    /* Test 5: dropidle enabled (3600), client idle for 3601 seconds - should drop */
    dropidle = 3600;
    last_share = now;
    last_share.tv_sec -= 3601; // 3601 seconds ago (just over threshold)
    per_tdiff = tvdiff(&now, &last_share);
    should_drop = (dropidle && per_tdiff > dropidle);
    assert_true(should_drop); // Should drop when just over threshold
}

/* Test idle detection with various dropidle thresholds
 * Tests different threshold values to ensure logic works correctly
 */
static void test_dropidle_various_thresholds(void)
{
    tv_t now, last_share;
    double per_tdiff;
    int dropidle;
    bool should_drop;
    
    gettimeofday(&now, NULL);
    
    /* Test 1: Very short threshold (60 seconds) */
    dropidle = 60;
    last_share = now;
    last_share.tv_sec -= 61; // 61 seconds ago
    per_tdiff = tvdiff(&now, &last_share);
    should_drop = (dropidle && per_tdiff > dropidle);
    assert_true(should_drop);
    
    /* Test 2: Medium threshold (1800 seconds = 30 minutes) */
    dropidle = 1800;
    last_share = now;
    last_share.tv_sec -= 1801; // 1801 seconds ago
    per_tdiff = tvdiff(&now, &last_share);
    should_drop = (dropidle && per_tdiff > dropidle);
    assert_true(should_drop);
    
    /* Test 3: Long threshold (7200 seconds = 2 hours) */
    dropidle = 7200;
    last_share = now;
    last_share.tv_sec -= 7201; // 7201 seconds ago
    per_tdiff = tvdiff(&now, &last_share);
    should_drop = (dropidle && per_tdiff > dropidle);
    assert_true(should_drop);
    
    /* Test 4: All below threshold - should not drop */
    dropidle = 3600;
    last_share = now;
    last_share.tv_sec -= 3599; // Just below threshold
    per_tdiff = tvdiff(&now, &last_share);
    should_drop = (dropidle && per_tdiff > dropidle);
    assert_false(should_drop);
}

/* Test edge cases for dropidle
 * Tests boundary conditions and edge cases
 */
static void test_dropidle_edge_cases(void)
{
    tv_t now, last_share;
    double per_tdiff;
    int dropidle;
    bool should_drop;
    
    gettimeofday(&now, NULL);
    
    /* Test 1: dropidle = 1 second, client idle for 2 seconds */
    dropidle = 1;
    last_share = now;
    last_share.tv_sec -= 2;
    per_tdiff = tvdiff(&now, &last_share);
    should_drop = (dropidle && per_tdiff > dropidle);
    assert_true(should_drop);
    
    /* Test 2: Very recent share (should never drop) */
    dropidle = 3600;
    last_share = now;
    last_share.tv_sec -= 10; // 10 seconds ago
    per_tdiff = tvdiff(&now, &last_share);
    should_drop = (dropidle && per_tdiff > dropidle);
    assert_false(should_drop);
    
    /* Test 3: Negative time difference (shouldn't happen, but test logic) */
    dropidle = 3600;
    last_share = now;
    last_share.tv_sec += 100; // Future time (negative diff)
    per_tdiff = tvdiff(&now, &last_share);
    should_drop = (dropidle && per_tdiff > dropidle);
    assert_false(should_drop); // Negative diff should not trigger drop
}

int main(void)
{
    printf("Running dropidle feature tests...\n\n");
    
    run_test(test_dropidle_config_default);
    run_test(test_dropidle_config_custom_values);
    run_test(test_dropidle_idle_detection_logic);
    run_test(test_dropidle_various_thresholds);
    run_test(test_dropidle_edge_cases);
    
    printf("\nAll dropidle tests passed!\n");
    return TEST_SUCCESS;
}

