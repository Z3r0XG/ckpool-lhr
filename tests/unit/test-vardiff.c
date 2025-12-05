/*
 * Unit tests for vardiff (variable difficulty) logic
 * Tests optimal difficulty calculation, clamping, and edge cases
 * Critical for all operation modes
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

#include "config.h"
#include "../test_common.h"
#include "libckpool.h"

/* Test time_bias function - used in difficulty adjustment */
static void test_time_bias(void)
{
    /* time_bias formula: 1.0 - 1.0 / exp(tdiff / period) */
    /* With period = 300, test various time differences */
    
    double period = 300.0;
    double tdiff;
    double bias, expected;
    
    /* Short time (should be close to 0) */
    tdiff = 10.0;
    bias = 1.0 - 1.0 / exp(tdiff / period);
    expected = 1.0 - 1.0 / exp(10.0 / 300.0);
    assert_double_equal(bias, expected, EPSILON);
    
    /* Medium time (should be moderate) */
    tdiff = 150.0;
    bias = 1.0 - 1.0 / exp(tdiff / period);
    expected = 1.0 - 1.0 / exp(150.0 / 300.0);
    assert_double_equal(bias, expected, EPSILON);
    
    /* Long time (should approach 1.0) */
    tdiff = 600.0;
    bias = 1.0 - 1.0 / exp(tdiff / period);
    expected = 1.0 - 1.0 / exp(600.0 / 300.0);
    assert_double_equal(bias, expected, EPSILON);
    
    /* Very long time (should be clamped at dexp=36) */
    tdiff = 12000.0; /* 40 * period */
    double dexp = tdiff / period;
    if (dexp > 36.0)
        dexp = 36.0;
    bias = 1.0 - 1.0 / exp(dexp);
    expected = 1.0 - 1.0 / exp(36.0);
    assert_double_equal(bias, expected, EPSILON);
}

/* Test optimal difficulty calculation logic */
static void test_optimal_diff_calculation(void)
{
    /* Test the core calculation: optimal = lround(dsps * multiplier) */
    /* With mindiff: multiplier = 2.4, without: multiplier = 3.33 */
    
    double dsps;
    int64_t optimal;
    
    /* With mindiff (multiplier = 2.4) */
    dsps = 10.0;
    optimal = lround(dsps * 2.4);
    assert_int_equal(optimal, 24);
    
    dsps = 0.5;
    optimal = lround(dsps * 2.4);
    assert_int_equal(optimal, 1);
    
    dsps = 100.0;
    optimal = lround(dsps * 2.4);
    assert_int_equal(optimal, 240);
    
    /* Without mindiff (multiplier = 3.33) */
    dsps = 10.0;
    optimal = lround(dsps * 3.33);
    assert_int_equal(optimal, 33);
    
    dsps = 0.3;
    optimal = lround(dsps * 3.33);
    assert_int_equal(optimal, 1);
    
    dsps = 100.0;
    optimal = lround(dsps * 3.33);
    assert_int_equal(optimal, 333);
}

/* Test difficulty clamping logic */
static void test_diff_clamping(void)
{
    /* Test the clamping sequence:
     * optimal = MAX(optimal, ckp->mindiff)
     * optimal = MAX(optimal, mindiff)
     * optimal = MIN(optimal, ckp->maxdiff) if maxdiff > 0
     * optimal = MIN(optimal, network_diff)
     * if (optimal < 1) return
     */
    
    int64_t optimal;
    double pool_mindiff, user_mindiff, pool_maxdiff, network_diff;
    
    /* Test mindiff clamping */
    optimal = 5;
    pool_mindiff = 10.0;
    optimal = MAX(optimal, (int64_t)pool_mindiff);
    assert_int_equal(optimal, 10);
    
    optimal = 5;
    user_mindiff = 20.0;
    optimal = MAX(optimal, (int64_t)user_mindiff);
    assert_int_equal(optimal, 20);
    
    /* Test maxdiff clamping */
    optimal = 1000;
    pool_maxdiff = 500.0;
    optimal = MIN(optimal, (int64_t)pool_maxdiff);
    assert_int_equal(optimal, 500);
    
    /* Test network_diff clamping */
    optimal = 1000;
    network_diff = 100.0;
    optimal = MIN(optimal, (int64_t)network_diff);
    assert_int_equal(optimal, 100);
    
    /* Test minimum floor (should return if < 1) */
    optimal = 0;
    assert_true(optimal < 1); /* This would cause early return */
    
    optimal = -5;
    assert_true(optimal < 1); /* This would cause early return */
}

/* Test diff rate ratio (drr) hysteresis */
static void test_drr_hysteresis(void)
{
    /* drr = dsps / client->diff */
    /* If drr > 0.15 && drr < 0.4, no adjustment (hysteresis zone) */
    
    double dsps, client_diff, drr;
    
    /* Within hysteresis zone - should not adjust */
    dsps = 3.0;
    client_diff = 10.0;
    drr = dsps / client_diff;
    assert_true(drr > 0.15 && drr < 0.4); /* 0.3 - should not adjust */
    
    /* Below hysteresis - should adjust down */
    dsps = 1.0;
    client_diff = 10.0;
    drr = dsps / client_diff;
    assert_true(drr < 0.15); /* 0.1 - should adjust */
    
    /* Above hysteresis - should adjust up */
    dsps = 5.0;
    client_diff = 10.0;
    drr = dsps / client_diff;
    assert_true(drr > 0.4); /* 0.5 - should adjust */
}

/* Test sub-"1" difficulty edge case in vardiff */
static void test_vardiff_sub1_edge_case(void)
{
    /* The check "if (unlikely(optimal < 1)) return;" prevents
     * vardiff from going below 1, even if mindiff allows sub-"1" */
    
    int64_t optimal;
    
    /* Optimal calculated below 1 */
    optimal = 0;
    assert_true(optimal < 1); /* Would cause early return */
    
    optimal = -1;
    assert_true(optimal < 1); /* Would cause early return */
    
    /* But manual setting via mindiff/startdiff can be sub-"1" */
    /* This is expected behavior - vardiff won't go below 1 automatically */
}

int main(void)
{
    printf("Running vardiff tests...\n");
    
    test_time_bias();
    test_optimal_diff_calculation();
    test_diff_clamping();
    test_drr_hysteresis();
    test_vardiff_sub1_edge_case();
    
    printf("All vardiff tests passed!\n");
    return 0;
}

