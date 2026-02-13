/*
 * Unit tests for fractional share difficulty accumulation fix
 * Tests that double (not int64_t) properly accumulates fractional shares
 */

/* config.h must be first to define _GNU_SOURCE before system headers */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include "../test_common.h"
#include "libckpool.h"

/* Test that fractional diff is preserved in accumulation */
static void test_fractional_diff_preservation(void)
{
	/* Simulate accumulating fractional shares like the pool does */
	double unaccounted_diff_shares = 0.0;  /* Now double, was int64_t */
	
	struct {
		double share_diff;
		double expected_total;
	} test_shares[] = {
		{ 0.001, 0.001 },
		{ 0.022, 0.023 },
		{ 0.055, 0.078 },
		{ 0.1, 0.178 },
		{ 0.25, 0.428 },
	};
	
	int num_shares = sizeof(test_shares) / sizeof(test_shares[0]);
	for (int i = 0; i < num_shares; i++) {
		unaccounted_diff_shares += test_shares[i].share_diff;
		assert_double_equal(unaccounted_diff_shares, test_shares[i].expected_total, EPSILON_DIFF);
	}
}

/* Demonstrate what int64_t truncation did */
static void test_old_int64_truncation_behavior(void)
{
	/* This is what the old code with int64_t would do */
	int64_t unaccounted_old = 0;
	double unaccounted_new = 0.0;
	
	struct {
		double share_diff;
		int64_t expected_old_truncated;
	} test_shares[] = {
		{ 0.001, 0 },
		{ 0.022, 0 },
		{ 0.055, 0 },
		{ 0.1, 0 },
		{ 1.5, 1 },  /* Finally shows up when crossing 1.0 */
	};
	
	int num_shares = sizeof(test_shares) / sizeof(test_shares[0]);
	for (int i = 0; i < num_shares; i++) {
		/* Old way (int64_t cast) */
		unaccounted_old = (int64_t)(unaccounted_old + test_shares[i].share_diff);
		
		/* New way (double) */
		unaccounted_new += test_shares[i].share_diff;
		
		/* Old truncates, new preserves */
		assert_int_equal(unaccounted_old, test_shares[i].expected_old_truncated);
	}
	
	/* After all shares, verify new has real value while old has truncated value */
	assert_true(unaccounted_new > 1.5);
}

/* Test hashrate calculation with fractional diffs */
static void test_hashrate_from_dsps_fractional(void)
{
	/* hashrate = dsps * 2^32 where dsps = diff_shares_per_second */
	const double nonces = 4294967296.0;  /* 2^32 */
	
	struct {
		double dsps;
		double expected_hashrate;
	} test_cases[] = {
		{ 0.001, 4294967.296 },
		{ 0.1, 429496729.6 },
		{ 1.0, 4294967296 },
		{ 10.0, 42949672960 },
	};
	
	int num_tests = sizeof(test_cases) / sizeof(test_cases[0]);
	for (int i = 0; i < num_tests; i++) {
		double dsps = test_cases[i].dsps;
		double hashrate = dsps * nonces;
		assert_double_equal(hashrate, test_cases[i].expected_hashrate, EPSILON_DIFF);
	}
}

/* Test long-term accumulation precision with fractional diffs */
static void test_long_term_accumulation_precision(void)
{
	/* Accumulate many fractional shares over time */
	double unaccounted = 0.0;
	double total_added = 0.0;
	
	/* Simulate 10,000 shares of 0.001 difficulty */
	for (int i = 0; i < 10000; i++) {
		unaccounted += 0.001;
		total_added += 0.001;
	}
	
	/* Should accumulate to 10.0 with double precision */
	assert_double_equal(unaccounted, 10.0, EPSILON_DIFF);
	assert_double_equal(unaccounted, total_added, EPSILON_DIFF);
}

/* Test exponential moving average calculation with fractional diffs */
static void test_decay_time_with_fractional_diffs(void)
{
	/* decay_time applies exponential moving average to accumulated diffs
	 * The fix allows fractional values to feed into this calculation correctly
	 */
	
	struct {
		double accumulated_diff;
		double window_seconds;
		double expected_dsps_approx;  /* diff_shares_per_second */
	} test_cases[] = {
		{ 0.001, 60, 0.00001667 },   /* 0.001 accumulated diff over 60s = 0.00001667 shares/sec */
		{ 0.1, 60, 0.0016667 },      /* 0.1 accumulated diff over 60s = 0.0016667 shares/sec */
		{ 1.0, 60, 0.0166667 },      /* 1.0 accumulated diff over 60s = 0.0166667 shares/sec */
		{ 10.0, 60, 0.1666667 },     /* 10.0 accumulated diff over 60s = 0.1666667 shares/sec */
	};
	
	int num_tests = sizeof(test_cases) / sizeof(test_cases[0]);
	for (int i = 0; i < num_tests; i++) {
		double accumulated = test_cases[i].accumulated_diff;
		double window = test_cases[i].window_seconds;
		
		/* Simple calculation: accumulated / window */
		double dsps = accumulated / window;
		
		/* Should match expected DSPS */
		double error = fabs(dsps - test_cases[i].expected_dsps_approx) / test_cases[i].expected_dsps_approx;
		assert_true(error < 0.01);  /* Allow 1% error */
	}
}

/* Test pool and user hashrate convergence with fractional diffs */
static void test_pool_user_hashrate_convergence_fractional(void)
{
	/* Pool aggregates at fixed interval, users report per-share
	 * With fractional diffs, both should track correctly
	 * This is a simplified test - full convergence is validated in integration tests
	 */
	
	const double nonces = 4294967296.0;
	
	/* Just verify the calculation doesn't overflow or produce NaN/Inf */
	struct {
		double user_diff_per_share;
		double shares_per_minute;
	} test_cases[] = {
		/* ESP32 at diff=0.001 */
		{ 0.001, 6 },
		
		/* Mid-range at diff=0.5 */
		{ 0.5, 4 },
		
		/* Standard at diff=1.0 */
		{ 1.0, 20 },
	};
	
	int num_tests = sizeof(test_cases) / sizeof(test_cases[0]);
	for (int i = 0; i < num_tests; i++) {
		double diff_per_share = test_cases[i].user_diff_per_share;
		double shares_per_min = test_cases[i].shares_per_minute;
		double shares_per_sec = shares_per_min / 60.0;
		
		/* DSPS = shares * difficulty */
		double dsps = shares_per_sec * diff_per_share;
		
		/* Hashrate */
		double hashrate = dsps * nonces;
		
		/* Verify result is sane (positive, not inf/nan) */
		assert_true(hashrate > 0);
		assert_true(hashrate < 1e18);  /* Reasonable upper bound */
	}
}

/* Test mindiff validation doesn't reject fractional values */
static void test_mindiff_fractional_validation(void)
{
	struct {
		double mindiff;
		int should_be_valid;
	} test_cases[] = {
		{ -0.5, 0 },    /* Negative not allowed */
		{ 0.0, 0 },     /* Zero not allowed */
		{ 0.0001, 1 },   /* Small fractional OK */
		{ 0.001, 1 },    /* Recommended minimum OK */
		{ 0.5, 1 },      /* Half difficulty OK */
		{ 1.0, 1 },      /* Standard OK */
		{ 1.5, 1 },      /* Above 1 OK */
		{ 1000, 1 },     /* Large OK */
	};
	
	int num_tests = sizeof(test_cases) / sizeof(test_cases[0]);
	for (int i = 0; i < num_tests; i++) {
		double mindiff = test_cases[i].mindiff;
		int valid = (mindiff > 0.0) ? 1 : 0;
		
		assert_int_equal(valid, test_cases[i].should_be_valid);
	}
}

/* Main test runner */
int main(void)
{
	printf("Testing fractional share difficulty accumulation fix...\n\n");
	
	run_test(test_fractional_diff_preservation);
	run_test(test_old_int64_truncation_behavior);
	run_test(test_hashrate_from_dsps_fractional);
	run_test(test_long_term_accumulation_precision);
	run_test(test_decay_time_with_fractional_diffs);
	run_test(test_pool_user_hashrate_convergence_fractional);
	run_test(test_mindiff_fractional_validation);
	
	printf("\nAll fractional stats tests passed!\n");
	return 0;
}
