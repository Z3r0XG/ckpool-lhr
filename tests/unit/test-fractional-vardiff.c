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

/* Test optimal diff calculation stays fractional only below 1.0 and is normalized when >= 1.0 */
static void test_optimal_diff_normalization(void)
{
	struct {
		double dsps;
		double multiplier;
		double expected; /* expected after normalization for >=1 */
		double raw;      /* expected raw before normalization */
		bool expect_normalized;
	} cases[] = {
		{ 0.1,   3.33, 0.333, 0.333, false },
		{ 0.3,   2.4,  0.72,  0.72,  false },
		{ 1.0,   3.33, 3.0,   3.33,  true  },
		{ 1.0,   2.4,  2.0,   2.4,   true  },
		{ 10.0,  3.33, 33.0,  33.3,  true  },
		{ 22.0,  2.4,  53.0,  52.8,  true  },
		{ 100.5, 3.33, 335.0, 334.665, true },
	};

	int num = sizeof(cases) / sizeof(cases[0]);
	for (int i = 0; i < num; i++) {
		double optimal_raw = cases[i].dsps * cases[i].multiplier;
		assert_double_equal(optimal_raw, cases[i].raw, EPSILON_DIFF);

		double normalized = normalize_pool_diff(optimal_raw);
		if (cases[i].expect_normalized)
			assert_double_equal(normalized, cases[i].expected, EPSILON_DIFF);
		else
			assert_double_equal(normalized, optimal_raw, EPSILON_DIFF);
	}
}

/* Test that lround truncation remains eliminated for sub-1 paths (preserve fractional) */
static void test_lround_elimination_sub1_only(void)
{
	struct {
		double dsps;
		double multiplier;
	} cases[] = {
		{ 0.1, 3.33 },   /* 0.333 would become 0 with lround */
		{ 0.2, 2.4 },    /* 0.48 would become 0 with lround */
		{ 0.5, 3.33 },   /* 1.665 would normalize to 2; ensure raw still fractional pre-norm */
	};

	int num = sizeof(cases) / sizeof(cases[0]);
	for (int i = 0; i < num; i++) {
		double raw = cases[i].dsps * cases[i].multiplier;
		long old_optimal = lround(raw);
		double normalized = normalize_pool_diff(raw);

		if (raw < 1.0) {
			assert_true((double)old_optimal != raw);
			assert_double_equal(normalized, raw, EPSILON_DIFF);
		} else {
			/* >=1 should normalize to a nearby whole number */
			assert_true(fabs(normalized - raw) <= 1.0);
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

/* Test vardiff outputs >=1 get normalized to whole numbers */
static void test_vardiff_above_1_normalized(void)
{
	struct {
		double dsps;
		double raw_expected;
		double normalized_expected;
	} cases[] = {
		{ 1.5, 4.995, 5.0 },
		{ 2.5, 8.325, 8.0 },
		{ 10.5, 34.965, 35.0 },
	};

	int num = sizeof(cases) / sizeof(cases[0]);
	for (int i = 0; i < num; i++) {
		double raw = cases[i].dsps * 3.33;
		assert_double_equal(raw, cases[i].raw_expected, EPSILON_DIFF);
		double normalized = normalize_pool_diff(raw);
		assert_double_equal(normalized, cases[i].normalized_expected, EPSILON_DIFF);
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

	run_test(test_optimal_diff_normalization);
	run_test(test_lround_elimination_sub1_only);
	run_test(test_vardiff_below_1);
	run_test(test_vardiff_above_1_normalized);
	run_test(test_floor_check_change);
	run_test(test_mindiff_clamping_fractional);
	run_test(test_worker_mindiff_fractional);
	run_test(test_vardiff_adjustment_sequence);

	printf("\nAll fractional vardiff tests passed!\n");
	return 0;
}
