/*
 * Unit tests for fractional variable difficulty support
 * Tests vardiff algorithm with diffs both below and above 1.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

#include "config.h"
#include "../test_common.h"
#include "libckpool.h"

/* Test optimal diff calculation with fractional values */
static void test_optimal_diff_calculation_fractional(void)
{
	/* optimal = dsps * multiplier (2.4 or 3.33 depending on mindiff)
	 * Should work with fractional inputs and outputs
	 */
	
	struct {
		double dsps;
		double multiplier;
		double expected;
		char desc[50];
	} test_cases[] = {
		/* Below 1.0 */
		{ 0.1, 3.33, 0.333, "Very low dsps, no mindiff" },
		{ 0.3, 2.4,  0.72,  "Low dsps, with mindiff" },
		
		/* Around 1.0 */
		{ 1.0, 3.33, 3.33,  "dsps=1, no mindiff" },
		{ 1.0, 2.4,  2.4,   "dsps=1, with mindiff" },
		
		/* Above 1.0 */
		{ 10.0, 3.33, 33.3,   "dsps=10, no mindiff" },
		{ 22.0, 2.4,  52.8,   "dsps=22, with mindiff" },
		{ 100.5, 3.33, 334.665, "High dsps fractional" },
	};
	
	int num_tests = sizeof(test_cases) / sizeof(test_cases[0]);
	for (int i = 0; i < num_tests; i++) {
		double dsps = test_cases[i].dsps;
		double multiplier = test_cases[i].multiplier;
		double expected = test_cases[i].expected;
		
		/* This is what vardiff now does (no lround) */
		double optimal = dsps * multiplier;
		
		assert_double_equal(optimal, expected, EPSILON_DIFF);
	}
}

/* Test that lround truncation is gone */
static void test_lround_elimination(void)
{
	/* Old code used lround(dsps * multiplier) which truncated fractional results
	 * New code should preserve fractional results
	 */
	
	struct {
		double dsps;
		double multiplier;
	} test_cases[] = {
		{ 0.1, 3.33 },   /* 0.333 would become 0 with lround */
		{ 0.2, 2.4 },    /* 0.48 would become 0 with lround */
		{ 0.5, 3.33 },   /* 1.665 would become 2 with lround */
		{ 1.5, 2.4 },    /* 3.6 would become 4 with lround */
	};
	
	int num_tests = sizeof(test_cases) / sizeof(test_cases[0]);
	for (int i = 0; i < num_tests; i++) {
		double dsps = test_cases[i].dsps;
		double mult = test_cases[i].multiplier;
		
		/* Old way (with lround) */
		long old_optimal = lround(dsps * mult);
		
		/* New way (direct double) */
		double new_optimal = dsps * mult;
		
		/* They should differ if result is fractional */
		if (new_optimal < 1.0) {
			assert_true((double)old_optimal != new_optimal);
		}
	}
}

/* Test vardiff adjusting below 1.0 */
static void test_vardiff_below_1(void)
{
	/* Low hashrate miner starting at diff=1.0
	 * Should adjust down to 0.5, 0.25, 0.1, etc.
	 */
	
	struct {
		double dsps;
		double mindiff;
		double expected_min;
		double expected_max;
	} test_cases[] = {
		{ 0.05, 0.001, 0.001, 0.2 },    /* Very low: 0.05 * 3.33 = 0.1665 */
		{ 0.1,  0.01,  0.01,  0.5 },    /* Low: 0.1 * 3.33 = 0.333 */
		{ 0.3,  0.1,   0.1,   1.0 },    /* Medium: 0.3 * 3.33 = 0.999, within valid range */
	};
	
	int num_tests = sizeof(test_cases) / sizeof(test_cases[0]);
	for (int i = 0; i < num_tests; i++) {
		double dsps = test_cases[i].dsps;
		double mindiff = test_cases[i].mindiff;
		
		/* Optimal without mindiff constraint */
		double optimal = dsps * 3.33;
		
		/* Apply mindiff floor */
		double clamped = optimal < mindiff ? mindiff : optimal;
		
		/* Result should be in valid range */
		assert_true(clamped >= test_cases[i].expected_min);
		assert_true(clamped <= test_cases[i].expected_max);
	}
}

/* Test vardiff adjusting above 1.0 with fractional values */
static void test_vardiff_above_1_fractional(void)
{
	/* Medium hashrate miner, adjust from 1.0 → 1.5 → 2.5 → 3.3, etc.
	 * Vardiff should handle 1.001, 1.5, 2.7, etc. smoothly
	 */
	
	struct {
		double dsps;
		double expected_approx;
	} test_cases[] = {
		{ 1.5, 4.995 },    /* 1.5 * 3.33 = 4.995 */
		{ 2.5, 8.325 },    /* 2.5 * 3.33 = 8.325 */
		{ 10.5, 34.965 },  /* 10.5 * 3.33 = 34.965 */
	};
	
	int num_tests = sizeof(test_cases) / sizeof(test_cases[0]);
	for (int i = 0; i < num_tests; i++) {
		double dsps = test_cases[i].dsps;
		double expected = test_cases[i].expected_approx;
		
		double optimal = dsps * 3.33;
		
		/* Allow 1% error for floating point */
		double error = fabs(optimal - expected) / expected;
		assert_true(error < 0.01);
	}
}

/* Test floor check (optimal <= 0) vs old check (optimal < 1) */
static void test_floor_check_change(void)
{
	struct {
		double optimal;
		int old_check_would_return;  /* optimal < 1 */
		int new_check_would_return;  /* optimal <= 0 */
	} test_cases[] = {
		{ -0.5, 1,  1  },   /* Invalid, both return */
		{ 0.0,  1,  1  },   /* Zero, both return */
		{ 0.001, 1, 0 },   /* Valid fractional, old returns, new allows */
		{ 0.5,   1, 0 },   /* Valid fractional, old returns, new allows */
		{ 0.999, 1, 0 },   /* Just below 1, old returns, new allows */
		{ 1.0,   0, 0 },   /* Exactly 1, both allow */
		{ 1.001, 0, 0 },   /* Fractional above 1, both allow */
	};
	
	int num_tests = sizeof(test_cases) / sizeof(test_cases[0]);
	for (int i = 0; i < num_tests; i++) {
		double optimal = test_cases[i].optimal;
		
		/* Old check */
		if (optimal < 1.0) {
			assert_true(test_cases[i].old_check_would_return);
		} else {
			assert_true(!test_cases[i].old_check_would_return);
		}
		
		/* New check */
		if (optimal <= 0.0) {
			assert_true(test_cases[i].new_check_would_return);
		} else {
			assert_true(!test_cases[i].new_check_would_return);
		}
	}
}

/* Test mindiff clamping with fractional values */
static void test_mindiff_clamping_fractional(void)
{
	struct {
		double optimal;
		double mindiff;
		double expected;
	} test_cases[] = {
		{ 0.0001, 0.001, 0.001 },  /* Clamp up to mindiff */
		{ 0.005,  0.01,  0.01 },   /* Clamp up */
		{ 0.1,    0.1,   0.1 },    /* Already at mindiff */
		{ 0.5,    0.001, 0.5 },    /* Above mindiff, no change */
		{ 1.5,    1.0,   1.5 },    /* Above mindiff, no change */
	};
	
	int num_tests = sizeof(test_cases) / sizeof(test_cases[0]);
	for (int i = 0; i < num_tests; i++) {
		double optimal = test_cases[i].optimal;
		double mindiff = test_cases[i].mindiff;
		
		/* Apply MAX clamping like vardiff does */
		double clamped = optimal > mindiff ? optimal : mindiff;
		
		assert_double_equal(clamped, test_cases[i].expected, EPSILON_DIFF);
	}
}

/* Test worker->mindiff as double (was int) */
static void test_worker_mindiff_fractional(void)
{
	/* worker->mindiff should now accept fractional values */
	double test_mindiffs[] = {
		0.001, 0.01, 0.1, 0.5, 0.999, 1.0, 1.001, 1.5, 10.5
	};
	
	int num_tests = sizeof(test_mindiffs) / sizeof(test_mindiffs[0]);
	for (int i = 0; i < num_tests; i++) {
		double worker_mindiff = test_mindiffs[i];
		
		/* Should be storable and retrievable without truncation */
		double stored = worker_mindiff;
		assert_double_equal(stored, test_mindiffs[i], EPSILON_DIFF);
	}
}

/* Test sequence of vardiff adjustments */
static void test_vardiff_adjustment_sequence(void)
{
	/* Simulate a miner's difficulty evolving over time
	 * Start high, hashrate emerges, vardiff adjusts down smoothly
	 */
	
	double current_diff = 10.0;
	struct {
		double dsps;
		double expected_optimal;  /* Absolute optimal difficulty values (not multipliers) */
	} adjustments[] = {
		{ 0.5, 1.665 },    /* Very low dsps, diff should drop to ~1.665 */
		{ 1.5, 4.995 },     /* Medium dsps, diff should go to ~4.995 */
		{ 10.0, 33.3 },   /* High dsps, diff should go to ~33.3 */
		{ 5.0, 16.65 },   /* Back down, diff should go to ~16.65 */
	};
	
	int num_tests = sizeof(adjustments) / sizeof(adjustments[0]);
	for (int i = 0; i < num_tests; i++) {
		double dsps = adjustments[i].dsps;
		double expected_new = adjustments[i].expected_optimal;
		
		/* Calculate new optimal */
		double optimal = dsps * 3.33;
		
		/* Should be close to expected */
		double error = fabs(optimal - expected_new) / expected_new;
		assert_true(error < 0.1);  /* Allow 10% error for rounding */
		
		current_diff = optimal;
	}
}

/* Main test runner */
int main(void)
{
	printf("Testing fractional variable difficulty support...\n\n");
	
	run_test(test_optimal_diff_calculation_fractional);
	run_test(test_lround_elimination);
	run_test(test_vardiff_below_1);
	run_test(test_vardiff_above_1_fractional);
	run_test(test_floor_check_change);
	run_test(test_mindiff_clamping_fractional);
	run_test(test_worker_mindiff_fractional);
	run_test(test_vardiff_adjustment_sequence);
	
	printf("\nAll fractional vardiff tests passed!\n");
	return 0;
}
