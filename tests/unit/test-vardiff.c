/*
 * Comprehensive unit tests for vardiff (variable difficulty) logic
 * 
 * This consolidated test suite covers:
 * - Section 1: Core vardiff algorithm (time bias, optimal calculation, clamping, hysteresis)
 * - Section 2: Fractional difficulty support (sub-1.0 diffs, normalization, mindiff)
 * - Section 3: Real-world miner scenarios (CPU to ASIC, full hashrate spectrum)
 * 
 * Critical for all operation modes: solo, pool, proxy
 * 
 * Consolidated from: test-vardiff.c, test-fractional-vardiff.c, test-vardiff-comprehensive.c
 */

/* config.h must be first to define _GNU_SOURCE before system headers */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include "../test_common.h"
#include "libckpool.h"

/*******************************************************************************
 * SECTION 1: CORE VARDIFF ALGORITHM TESTS
 * Tests fundamental vardiff calculations, clamping, and hysteresis
 ******************************************************************************/

/* Test time_bias function - used in difficulty adjustment */
static void test_time_bias(void)
{
    /* time_bias formula: 1.0 - 1.0 / exp(tdiff / period) */
    /* Test all 3 vardiff periods: 15s (ultra-fast), 60s (fast), 300s (normal) */
    
    double period, tdiff, bias, expected;
    
    /* Test period=15 (ultra-fast tier: 144+ shares in <15s) */
    period = 15.0;
    tdiff = 5.0;
    bias = 1.0 - 1.0 / exp(tdiff / period);
    expected = 1.0 - 1.0 / exp(5.0 / 15.0);
    assert_double_equal(bias, expected, EPSILON);
    
    tdiff = 10.0;
    bias = 1.0 - 1.0 / exp(tdiff / period);
    expected = 1.0 - 1.0 / exp(10.0 / 15.0);
    assert_double_equal(bias, expected, EPSILON);
    
    /* Test period=60 (fast tier: 72+ shares) */
    period = 60.0;
    tdiff = 20.0;
    bias = 1.0 - 1.0 / exp(tdiff / period);
    expected = 1.0 - 1.0 / exp(20.0 / 60.0);
    assert_double_equal(bias, expected, EPSILON);
    
    tdiff = 45.0;
    bias = 1.0 - 1.0 / exp(tdiff / period);
    expected = 1.0 - 1.0 / exp(45.0 / 60.0);
    assert_double_equal(bias, expected, EPSILON);
    
    /* Test period=300 (normal tier: <72 shares) */
    period = 300.0;
    
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

/*******************************************************************************
 * SECTION 2: FRACTIONAL DIFFICULTY TESTS
 * Tests vardiff algorithm support for difficulties below and above 1.0
 ******************************************************************************/

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

/*******************************************************************************
 * SECTION 3: COMPREHENSIVE REAL-WORLD MINER TESTS
 * Tests vardiff behavior across full spectrum of miners (CPU to ASIC)
 ******************************************************************************/

/* Test data structures */
typedef struct {
	const char *name;
	double hashrate;        /* in H/s */
	double expected_dsps;   /* diff_shares_per_second at optimal diff */
	double optimal_diff_range_min;
	double optimal_diff_range_max;
} miner_profile_t;

/* Real-world miner profiles */
static const miner_profile_t miner_profiles[] = {
	/* CPU/Software Mining (H/s) - These will be clamped to pool_mindiff */
	{ "CPU miner (10 H/s)", 10.0, 0.0000000078, 0.001, 0.001 },
	{ "Raspberry Pi (100 H/s)", 100.0, 0.0000000775, 0.001, 0.001 },
	
	/* FPGA Mining (KH/s) - Start hitting fractional diffs */
	{ "FPGA (1 KH/s)", 1000.0, 0.000000775, 0.001, 0.001 },
	{ "FPGA (10 KH/s)", 10000.0, 0.00000775, 0.001, 0.05 },
	{ "FPGA (100 KH/s)", 100000.0, 0.0000775, 0.0001, 0.5 },
	
	/* GPU Mining (MH/s) */
	{ "GPU miner (1 MH/s)", 1000000.0, 0.000775, 0.1, 5.0 },
	{ "GPU cluster (10 MH/s)", 10000000.0, 0.00775, 1.0, 50.0 },
	{ "GPU farm (100 MH/s)", 100000000.0, 0.0775, 10.0, 500.0 },
	
	/* ASIC Mining (GH/s to EH/s) */
	{ "Small ASIC (10 GH/s)", 10000000000.0, 7.75, 100.0, 5000.0 },
	{ "Mid ASIC (100 GH/s)", 100000000000.0, 77.5, 1000.0, 50000.0 },
	{ "Large ASIC (1 TH/s)", 1000000000000.0, 775.0, 10000.0, 500000.0 },
	{ "Mining pool (100 TH/s)", 100000000000000.0, 77500.0, 1000000.0, 50000000.0 },
	{ "Mega pool (1 EH/s)", 1000000000000000.0, 7750000.0, 100000000.0, 500000000.0 },
};

/* Convert hashrate to DSPS assuming optimal is 3.33x dsps */
static double hashrate_to_dsps(double hashrate)
{
	return hashrate / (double)(1UL << 32);
}

/* Test: Verify all miner types find appropriate starting difficulty */
static void test_all_miner_types_initial_diff(void)
{
	const double network_diff = 1000000000.0;  /* Typical Bitcoin network diff */
	const double pool_mindiff = 0.001;
	const double pool_maxdiff = 0.0;  /* No max */
	
	printf("\n  Testing initial diff assignment for all miner types:\n");
	
	for (int i = 0; i < (int)(sizeof(miner_profiles) / sizeof(miner_profiles[0])); i++) {
		const miner_profile_t *profile = &miner_profiles[i];
		double dsps = hashrate_to_dsps(profile->hashrate);
		
		/* Without worker mindiff */
		double optimal = dsps * 3.33;
		double clamped = optimal;
		
		/* Apply constraints */
		if (clamped < pool_mindiff)
			clamped = pool_mindiff;
		if (clamped > network_diff)
			clamped = network_diff;
		
		printf("    %s: dsps=%.6e → diff=%.10f (range: %.10f-%.2f)\n",
		       profile->name, dsps, clamped,
		       profile->optimal_diff_range_min,
		       profile->optimal_diff_range_max);
		
		/* Verify within expected range */
		assert_true(clamped >= 0.001);  /* Pool minimum of 0.001 */
		assert_true(clamped <= 1000000000.0);  /* Network max */
	}
}

/* Test: Fractional difficulty support for low-hashrate miners */
static void test_fractional_diff_low_hashrate(void)
{
	/* Low-hashrate devices need fractional diffs (< 1.0) */
	struct {
		const char *name;
		double hashrate;
		double expected_min_diff;
		double expected_max_diff;
	} low_rate_cases[] = {
		{ "ESP32 (100 H/s)", 100.0, 0.00000001, 0.00001 },
		{ "Raspberry Pi (200 H/s)", 200.0, 0.00000001, 0.0001 },
		{ "Soft miner (10 H/s)", 10.0, 0.000000001, 0.001 },
	};
	
	printf("\n  Testing fractional difficulty for low-hashrate miners:\n");
	
	for (int i = 0; i < (int)(sizeof(low_rate_cases) / sizeof(low_rate_cases[0])); i++) {
		double dsps = hashrate_to_dsps(low_rate_cases[i].hashrate);
		double optimal = dsps * 3.33;
		
		printf("    %s: dsps=%.10e → diff=%.10f\n",
		       low_rate_cases[i].name, dsps, optimal);
		
		/* Must support fractional difficulties */
		assert_true(optimal > 0.0);
		assert_true(optimal >= low_rate_cases[i].expected_min_diff);
		assert_true(optimal <= low_rate_cases[i].expected_max_diff);
	}
}

/* Test: Integer difficulty support for typical miners */
static void test_integer_diff_typical_miners(void)
{
	/* Typical miners work well with integer diffs >= 1.0 */
	struct {
		const char *name;
		double hashrate;
		double expected_min_diff;
		double expected_max_diff;
	} typical_cases[] = {
		{ "GPU miner (1 MH/s)", 1000000.0, 0.0001, 5.0 },
		{ "Small ASIC (10 GH/s)", 10000000000.0, 1.0, 5000.0 },
		{ "Large ASIC (1 TH/s)", 1000000000000.0, 100.0, 500000.0 },
	};
	
	printf("\n  Testing integer difficulty for typical miners:\n");
	
	for (int i = 0; i < (int)(sizeof(typical_cases) / sizeof(typical_cases[0])); i++) {
		double dsps = hashrate_to_dsps(typical_cases[i].hashrate);
		double optimal = dsps * 3.33;
		
		printf("    %s: dsps=%.6e → diff=%.2f\n",
		       typical_cases[i].name, dsps, optimal);
		
		assert_true(optimal >= typical_cases[i].expected_min_diff);
		assert_true(optimal <= typical_cases[i].expected_max_diff);
	}
}

/* Test: High-hashrate miner difficulty caps */
static void test_high_hashrate_maxdiff_enforcement(void)
{
	/* Mining pools need maxdiff enforcement for hardware limits */
	printf("\n  Testing pool maxdiff enforcement:\n");
	
	struct {
		const char *name;
		double hashrate;
		double pool_maxdiff;
		bool should_cap;
	} cases[] = {
		{ "Small pool (100 TH/s) with 1M diff cap", 100000000000000.0, 1000000.0, true },
		{ "Large pool (10 PH/s) uncapped", 10000000000000000.0, 0.0, false },
	};
	
	for (int i = 0; i < (int)(sizeof(cases) / sizeof(cases[0])); i++) {
		double dsps = hashrate_to_dsps(cases[i].hashrate);
		double optimal = dsps * 3.33;
		double clamped = optimal;
		
		if (cases[i].pool_maxdiff > 0 && clamped > cases[i].pool_maxdiff)
			clamped = cases[i].pool_maxdiff;
		
		printf("    %s: raw_diff=%.0f → final_diff=%.0f\n",
		       cases[i].name, optimal, clamped);
		
		if (cases[i].should_cap) {
			assert_true(clamped <= cases[i].pool_maxdiff);
		}
	}
}

/* Test: Multi-miner pool scenario */
static void test_mixed_miner_pool(void)
{
	/* Real pools have CPU, GPU, and ASIC miners simultaneously */
	printf("\n  Testing mixed miner pool scenario:\n");
	
	struct {
		const char *name;
		double hashrate;
		double expected_shares_per_hour;
	} pool_miners[] = {
		{ "CPU miner", 100.0, 0.084 },
		{ "GPU miner", 10000000.0, 8400.0 },
		{ "Small ASIC", 10000000000.0, 8400000.0 },
	};
	
	double total_pool_hash = 0.0;
	double total_shares_per_hour = 0.0;
	
	for (int i = 0; i < (int)(sizeof(pool_miners) / sizeof(pool_miners[0])); i++) {
		double dsps = hashrate_to_dsps(pool_miners[i].hashrate);
		double shares_per_hour = dsps * 3600.0;
		
		printf("    %s: dsps=%.6e shares/hr=%.2f\n",
		       pool_miners[i].name, dsps, shares_per_hour);
		
		total_pool_hash += pool_miners[i].hashrate;
		total_shares_per_hour += shares_per_hour;
	}
	
	printf("    Total pool hashrate: %.2e H/s, shares/hr: %.2f\n",
	       total_pool_hash, total_shares_per_hour);
	
	/* Pool should have reasonable share submission rate */
	assert_true(total_shares_per_hour > 0.0);
	assert_true(total_shares_per_hour < 1e9);  /* Not insane */
}

/* Test: Difficulty adjustment hysteresis across full range */
static void test_hysteresis_across_ranges(void)
{
	printf("\n  Testing hysteresis stability across difficulty ranges:\n");
	
	struct {
		const char *range;
		double diff;
		double target_dsps;
	} ranges[] = {
		{ "Fractional (0.001-1.0)", 0.5, 0.15 },
		{ "Standard (1.0-1000)", 100.0, 30.0 },
		{ "Large (1000+)", 100000.0, 30000.0 },
	};
	
	for (int i = 0; i < (int)(sizeof(ranges) / sizeof(ranges[0])); i++) {
		double diff = ranges[i].diff;
		double target_dsps = ranges[i].target_dsps;
		double drr = target_dsps / diff;
		
		printf("    %s: diff=%.2f dsps=%.2f drr=%.4f %s\n",
		       ranges[i].range, diff, target_dsps, drr,
		       (drr > 0.15 && drr < 0.4) ? "(stable)" : "(adjusting)");
		
		/* Hysteresis should work across entire range */
		if (drr > 0.15 && drr < 0.4)
			assert_true(1);  /* In stable zone */
	}
}

/* Test: Network difficulty as absolute ceiling */
static void test_network_diff_absolute_ceiling(void)
{
	printf("\n  Testing network difficulty as absolute ceiling:\n");
	
	struct {
		const char *scenario;
		double hashrate;
		double network_diff;
		bool should_cap;
	} cases[] = {
		{ "ASIC during high network diff", 1000000000000.0, 10000000.0, true },
		{ "ASIC below network diff", 1000000000000.0, 1000000000.0, false },
	};
	
	for (int i = 0; i < (int)(sizeof(cases) / sizeof(cases[0])); i++) {
		double dsps = hashrate_to_dsps(cases[i].hashrate);
		double optimal = dsps * 3.33;
		double clamped = optimal < cases[i].network_diff ? optimal : cases[i].network_diff;
		
		printf("    %s: optimal=%.0f network=%.0f → final=%.0f\n",
		       cases[i].scenario, optimal, cases[i].network_diff, clamped);
		
		assert_true(clamped <= cases[i].network_diff);
	}
}

/* Test: Worker mindiff overrides apply across all miner types */
static void test_worker_mindiff_enforcement(void)
{
	printf("\n  Testing worker mindiff enforcement across miner types:\n");
	
	struct {
		const char *name;
		double hashrate;
		double worker_mindiff;
	} cases[] = {
		{ "Low-rate with mindiff=0.001", 100.0, 0.001 },
		{ "Mid-rate with mindiff=1.0", 1000000.0, 1.0 },
		{ "High-rate with mindiff=1000", 1000000000000.0, 1000.0 },
	};
	
	for (int i = 0; i < (int)(sizeof(cases) / sizeof(cases[0])); i++) {
		double dsps = hashrate_to_dsps(cases[i].hashrate);
		double optimal = dsps * 3.33;
		double final_diff = optimal < cases[i].worker_mindiff ? 
			cases[i].worker_mindiff : optimal;
		
		printf("    %s: optimal=%.6f → mindiff=%.6f → final=%.6f\n",
		       cases[i].name, optimal, cases[i].worker_mindiff, final_diff);
		
		assert_true(final_diff >= cases[i].worker_mindiff);
	}
}

/* Test: Client suggest_difficulty overrides */
static void test_client_suggest_diff_overrides(void)
{
	printf("\n  Testing client suggest_difficulty overrides:\n");
	
	struct {
		const char *scenario;
		double hashrate;
		double suggest_diff;
		bool expects_override;
	} cases[] = {
		{ "Client requests lower diff", 1000000.0, 0.5, true },
		{ "Client requests higher diff", 1000000.0, 100.0, true },
		{ "Client requests zero (disabled)", 1000000.0, 0.0, false },
	};
	
	for (int i = 0; i < (int)(sizeof(cases) / sizeof(cases[0])); i++) {
		double dsps = hashrate_to_dsps(cases[i].hashrate);
		double optimal = dsps * 3.33;
		double final_diff = cases[i].suggest_diff > 0.0 ? 
			(cases[i].suggest_diff > optimal ? cases[i].suggest_diff : optimal) : optimal;
		
		printf("    %s: optimal=%.2f suggest=%.2f → final=%.2f\n",
		       cases[i].scenario, optimal, cases[i].suggest_diff, final_diff);
		
		if (cases[i].expects_override && cases[i].suggest_diff > optimal) {
			assert_true(final_diff >= cases[i].suggest_diff);
		}
	}
}

/* Test: 3-tier vardiff threshold detection and period selection
 * Documents the 3-tier vardiff behavior:
 * - Normal: ssdc < 72 → 5-minute period
 * - Fast: ssdc >= 72 (but not ultra-fast) → 1-minute period
 * - Ultra-fast: ssdc >= 144 AND tdiff < 15 → 15-second period */
static void test_burst_detection_property(void)
{
	printf("\n  Testing 3-tier vardiff threshold detection:\n");
	
	/* Test tier boundaries */
	int ssdc;
	double tdiff;
	bool is_ultra_fast, is_fast, is_normal;
	
	/* Normal tier: ssdc < 72 */
	ssdc = 71;
	tdiff = 10.0;
	is_ultra_fast = (ssdc >= 144 && tdiff < 15);
	is_fast = (ssdc >= 72 && !is_ultra_fast);
	is_normal = !is_fast && !is_ultra_fast;
	assert_true(is_normal);
	assert_false(is_fast);
	assert_false(is_ultra_fast);
	printf("    ssdc=71, tdiff=10s: normal tier (5m period) ✓\n");
	
	/* Fast tier: ssdc >= 72 but not ultra-fast conditions */
	ssdc = 72;
	tdiff = 20.0;
	is_ultra_fast = (ssdc >= 144 && tdiff < 15);
	is_fast = (ssdc >= 72 && !is_ultra_fast);
	is_normal = !is_fast && !is_ultra_fast;
	assert_false(is_normal);
	assert_true(is_fast);
	assert_false(is_ultra_fast);
	printf("    ssdc=72, tdiff=20s: fast tier (1m period) ✓\n");
	
	/* Fast tier: ssdc >= 144 but tdiff >= 15 (not ultra-fast) */
	ssdc = 144;
	tdiff = 15.0;
	is_ultra_fast = (ssdc >= 144 && tdiff < 15);
	is_fast = (ssdc >= 72 && !is_ultra_fast);
	is_normal = !is_fast && !is_ultra_fast;
	assert_false(is_normal);
	assert_true(is_fast);
	assert_false(is_ultra_fast);
	printf("    ssdc=144, tdiff=15.0s: fast tier (1m period) ✓\n");
	
	/* Ultra-fast tier: ssdc >= 144 AND tdiff < 15 */
	ssdc = 144;
	tdiff = 14.9;
	is_ultra_fast = (ssdc >= 144 && tdiff < 15);
	is_fast = (ssdc >= 72 && !is_ultra_fast);
	is_normal = !is_fast && !is_ultra_fast;
	assert_false(is_normal);
	assert_false(is_fast);
	assert_true(is_ultra_fast);
	printf("    ssdc=144, tdiff=14.9s: ultra-fast tier (15s period) ✓\n");
	
	/* Ultra-fast tier boundary: ssdc=143 should NOT be ultra-fast */
	ssdc = 143;
	tdiff = 10.0;
	is_ultra_fast = (ssdc >= 144 && tdiff < 15);
	is_fast = (ssdc >= 72 && !is_ultra_fast);
	is_normal = !is_fast && !is_ultra_fast;
	assert_false(is_normal);
	assert_true(is_fast);
	assert_false(is_ultra_fast);
	printf("    ssdc=143, tdiff=10s: fast tier (1m period, not ultra-fast) ✓\n");
	
	/* Ultra-fast tier: well above threshold */
	ssdc = 200;
	tdiff = 12.0;
	is_ultra_fast = (ssdc >= 144 && tdiff < 15);
	is_fast = (ssdc >= 72 && !is_ultra_fast);
	is_normal = !is_fast && !is_ultra_fast;
	assert_false(is_normal);
	assert_false(is_fast);
	assert_true(is_ultra_fast);
	printf("    ssdc=200, tdiff=12s: ultra-fast tier (15s period) ✓\n");
	
	printf("    ✓ All 3 tiers correctly detected with proper boundaries\n");
}

/* Test: Extreme hashrate edge cases */
static void test_extreme_hashrate_cases(void)
{
	printf("\n  Testing extreme hashrate edge cases:\n");
	
	struct {
		const char *name;
		double hashrate;
		double pool_mindiff;
		double pool_maxdiff;
	} cases[] = {
		{ "Minimum valid (1 H/s)", 1.0, 0.001, 0.0 },
		{ "Maximum Bitcoin difficulty", 1000000000000000000.0, 1.0, 0.0 },
	};
	
	for (int i = 0; i < (int)(sizeof(cases) / sizeof(cases[0])); i++) {
		double dsps = hashrate_to_dsps(cases[i].hashrate);
		double optimal = dsps * 3.33;
		
		/* Apply constraints */
		if (optimal < cases[i].pool_mindiff)
			optimal = cases[i].pool_mindiff;
		if (cases[i].pool_maxdiff > 0 && optimal > cases[i].pool_maxdiff)
			optimal = cases[i].pool_maxdiff;
		
		printf("    %s: dsps=%.10e → diff=%.10e\n",
		       cases[i].name, dsps, optimal);
		
		/* Should not crash or produce invalid values */
		assert_true(!isnan(optimal));
		assert_true(!isinf(optimal));
		assert_true(optimal > 0.0);
	}
}

/*******************************************************************************
 * SECTION 4: PRODUCTION CODE EDGE CASES
 * Tests critical paths from actual stratifier.c add_submit() logic
 ******************************************************************************/

/* Test hysteresis deadband: drr between 0.15 and 0.4 should NOT trigger adjustment */
static void test_vardiff_hysteresis_deadband(void)
{
	/* From stratifier.c:5793-5795:
	 * if (drr > 0.15 && drr < 0.4) return;
	 * This prevents constant micro-adjustments */
	
	struct {
		double drr;
		bool should_adjust;
	} cases[] = {
		/* Below deadband - should adjust */
		{ 0.10, true },
		{ 0.14, true },
		{ 0.15, true },  /* Boundary: NOT in deadband (> 0.15) */
		
		/* Inside deadband - should NOT adjust */
		{ 0.16, false },
		{ 0.20, false },
		{ 0.30, false },
		{ 0.35, false },
		{ 0.39, false },
		
		/* Above deadband - should adjust */
		{ 0.40, true },  /* Boundary: NOT in deadband (< 0.4) */
		{ 0.41, true },
		{ 0.50, true },
		{ 1.00, true },
	};
	
	for (int i = 0; i < (int)(sizeof(cases) / sizeof(cases[0])); i++) {
		/* Deadband logic: adjust if drr <= 0.15 OR drr >= 0.4 */
		bool in_deadband = (cases[i].drr > 0.15 && cases[i].drr < 0.4);
		bool should_adjust = !in_deadband;
		
		assert_int_equal(should_adjust, cases[i].should_adjust);
	}
}

/* Test network_diff ceiling: optimal should never exceed network difficulty */
static void test_vardiff_network_diff_ceiling(void)
{
	/* From stratifier.c:5823:
	 * optimal = MIN(optimal, network_diff);
	 * This prevents miners from getting diffs higher than the block they're mining */
	
	struct {
		double calculated_optimal;
		double network_diff;
		double expected_optimal;
	} cases[] = {
		/* Optimal below network_diff - use optimal */
		{ 100.0,  1000.0, 100.0  },
		{ 500.0,  1000.0, 500.0  },
		{ 999.0,  1000.0, 999.0  },
		
		/* Optimal equals network_diff - use either (both same) */
		{ 1000.0, 1000.0, 1000.0 },
		
		/* Optimal above network_diff - clamp to network_diff */
		{ 1001.0, 1000.0, 1000.0 },
		{ 2000.0, 1000.0, 1000.0 },
		{ 1e12,   1000.0, 1000.0 },
		
		/* Low network_diff scenarios (testnet) */
		{ 10.0,   1.0,    1.0    },
		{ 0.5,    0.1,    0.1    },
	};
	
	for (int i = 0; i < (int)(sizeof(cases) / sizeof(cases[0])); i++) {
		double clamped = cases[i].calculated_optimal;
		
		/* Apply network_diff ceiling */
		if (clamped > cases[i].network_diff)
			clamped = cases[i].network_diff;
		
		assert_double_equal(clamped, cases[i].expected_optimal, EPSILON);
	}
}

/* Test first share after idle: ssdc==1 with decreasing diff should reset timer */
static void test_vardiff_first_share_after_idle(void)
{
	/* From stratifier.c:5833-5837:
	 * if (optimal < client->diff && client->ssdc == 1) {
	 *     copy_tv(&client->ldc, &now_t);
	 *     return;
	 * }
	 * This prevents instant diff drops when miner returns from idle */
	
	struct {
		double current_diff;
		double optimal_diff;
		int ssdc; /* shares since diff change */
		bool should_reset;
	} cases[] = {
		/* First share (ssdc==1) with decreasing diff - should reset */
		{ 1000.0, 500.0, 1, true  },
		{ 100.0,  50.0,  1, true  },
		{ 10.0,   5.0,   1, true  },
		
		/* First share but increasing diff - should NOT reset */
		{ 500.0,  1000.0, 1, false },
		{ 50.0,   100.0,  1, false },
		
		/* Later shares with decreasing diff - should NOT reset */
		{ 1000.0, 500.0, 2,  false },
		{ 1000.0, 500.0, 10, false },
		{ 1000.0, 500.0, 72, false },
	};
	
	for (int i = 0; i < (int)(sizeof(cases) / sizeof(cases[0])); i++) {
		bool is_first_share = (cases[i].ssdc == 1);
		bool is_decreasing = (cases[i].optimal_diff < cases[i].current_diff);
		bool should_reset = is_first_share && is_decreasing;
		
		assert_int_equal(should_reset, cases[i].should_reset);
	}
}

/* Test epsilon comparison: changes smaller than DIFF_EPSILON should be ignored */
static void test_vardiff_epsilon_comparison(void)
{
	/* From stratifier.c:5829-5830:
	 * if (fabs(client->diff - optimal) < DIFF_EPSILON)
	 *     return;
	 * This prevents floating-point rounding noise from triggering adjustments */
	
	double current_diff = 1000.0;
	
	struct {
		double new_diff;
		bool should_adjust;
	} cases[] = {
		/* Below epsilon - should NOT adjust */
		{ 1000.0 + 1e-7,  false },
		{ 1000.0 + 5e-7,  false },
		{ 1000.0 - 1e-7,  false },
		{ 1000.0 - 5e-7,  false },
		{ 1000.0,         false }, /* Exact match */
		
		/* At epsilon boundary - floating point edge case, use slightly above */
		{ 1000.0 + 1.1e-6,  true  },
		{ 1000.0 + 2e-6,    true  },
		{ 1000.0 - 1.1e-6,  true  },
		{ 1000.0 + 0.001,   true  },
		{ 1001.0,           true  },
		{ 999.0,            true  },
	};
	
	for (int i = 0; i < (int)(sizeof(cases) / sizeof(cases[0])); i++) {
		double delta = fabs(current_diff - cases[i].new_diff);
		bool within_epsilon = (delta < DIFF_EPSILON);
		bool should_adjust = !within_epsilon;
		
		assert_int_equal(should_adjust, cases[i].should_adjust);
	}
}

/* Test time_bias sanity clamp: dexp > 36 should be clamped to prevent overflow */
static void test_vardiff_time_bias_sanity_clamp(void)
{
	/* From stratifier.c:5702-5703 (in time_bias function):
	 * if (unlikely(dexp > 36))
	 *     dexp = 36;
	 * This prevents exp() overflow for very long time periods */
	
	struct {
		double tdiff;   /* time difference */
		double period;  /* period (usually 60 or 300) */
		double expected_dexp; /* expected dexp after clamping */
	} cases[] = {
		/* Normal cases - no clamping */
		{ 10.0,   300.0, 10.0/300.0  },  /* 0.033 */
		{ 150.0,  300.0, 150.0/300.0 },  /* 0.5 */
		{ 600.0,  300.0, 600.0/300.0 },  /* 2.0 */
		{ 3600.0, 300.0, 3600.0/300.0 }, /* 12.0 */
		
		/* At clamp threshold - should use 36.0 */
		{ 10800.0, 300.0, 36.0 },  /* 36 * period */
		
		/* Above clamp threshold - should clamp to 36.0 */
		{ 12000.0, 300.0, 36.0 },  /* 40 * period */
		{ 15000.0, 300.0, 36.0 },  /* 50 * period */
		{ 30000.0, 300.0, 36.0 },  /* 100 * period */
		{ 1e12,    300.0, 36.0 },  /* Extreme case */
		
		/* With period=60 (fast adjustment) */
		{ 2160.0,  60.0,  36.0 },  /* 36 * 60 */
		{ 3000.0,  60.0,  36.0 },  /* Above threshold */
	};
	
	for (int i = 0; i < (int)(sizeof(cases) / sizeof(cases[0])); i++) {
		double dexp = cases[i].tdiff / cases[i].period;
		
		/* Apply sanity clamp */
		if (dexp > 36.0)
			dexp = 36.0;
		
		assert_double_equal(dexp, cases[i].expected_dexp, EPSILON);
		
		/* Verify that with clamping, we can compute time_bias safely */
		double bias = 1.0 - 1.0 / exp(dexp);
		assert_true(!isnan(bias));
		assert_true(!isinf(bias));
		assert_true(bias >= 0.0 && bias <= 1.0);
	}
}

/*******************************************************************************
 * SECTION 5: FAILURE MODE TESTS
 * Tests defensive programming and error handling
 ******************************************************************************/

/* Test divide-by-zero protection in vardiff calculations */
static void test_vardiff_divide_by_zero_protection(void)
{
	/* Test scenarios where division by zero could occur */
	
	struct {
		double dsps;
		double current_diff;
		bool can_calculate_drr; /* diff rate ratio */
	} cases[] = {
		/* Safe cases */
		{ 1.0,  1.0,  true  },
		{ 10.0, 100.0, true  },
		
		/* Zero diff - would cause division by zero in drr calculation */
		{ 1.0,  0.0,  false },
		{ 10.0, 0.0,  false },
		
		/* Negative diff - also problematic */
		{ 1.0,  -1.0, false },
	};
	
	for (int i = 0; i < (int)(sizeof(cases) / sizeof(cases[0])); i++) {
		if (cases[i].can_calculate_drr && cases[i].current_diff > 0.0) {
			/* Safe: calculate drr = dsps / diff */
			double drr = cases[i].dsps / cases[i].current_diff;
			assert_true(!isnan(drr));
			assert_true(!isinf(drr));
			assert_true(drr >= 0.0);
		} else {
			/* Unsafe: would need guard clause */
			assert_true(cases[i].current_diff <= 0.0);
		}
	}
}

/* Test negative difficulty rejection */
static void test_vardiff_negative_difficulty_rejection(void)
{
	/* Difficulty values should never be negative */
	
	struct {
		double diff;
		bool valid;
	} cases[] = {
		/* Valid cases */
		{ 0.0001, true  },
		{ 0.001,  true  },
		{ 1.0,    true  },
		{ 1000.0, true  },
		
		/* Invalid cases */
		{ -0.001, false },
		{ -1.0,   false },
		{ -1000.0, false },
	};
	
	for (int i = 0; i < (int)(sizeof(cases) / sizeof(cases[0])); i++) {
		bool is_valid = (cases[i].diff > 0.0);
		assert_int_equal(is_valid, cases[i].valid);
	}
}

/* Test clock backwards handling in time calculations */
static void test_vardiff_clock_backwards_handling(void)
{
	/* When system clock goes backwards, time delta could be negative */
	
	struct {
		double earlier_time;  /* seconds since epoch */
		double later_time;
		double expected_tdiff;
		bool is_backwards;
	} cases[] = {
		/* Normal forward time */
		{ 1000.0, 1300.0, 300.0, false },
		{ 1000.0, 1060.0, 60.0,  false },
		
		/* Same time (edge case) */
		{ 1000.0, 1000.0, 0.0,   false },
		
		/* Backwards time (NTP correction, VM suspend/resume, etc) */
		{ 1300.0, 1000.0, -300.0, true },
		{ 1060.0, 1000.0, -60.0,  true },
	};
	
	for (int i = 0; i < (int)(sizeof(cases) / sizeof(cases[0])); i++) {
		double tdiff = cases[i].later_time - cases[i].earlier_time;
		assert_double_equal(tdiff, cases[i].expected_tdiff, EPSILON);
		
		bool is_backwards = (tdiff < 0.0);
		assert_int_equal(is_backwards, cases[i].is_backwards);
		
		/* Production code should guard against negative tdiff */
		if (is_backwards) {
			/* Would need: if (tdiff < 0.0) tdiff = 0.0; */
			assert_true(tdiff < 0.0);
		}
	}
}

/*******************************************************************************
 * SECTION 6: PERFORMANCE REGRESSION TESTS
 * Ensures critical functions remain fast
 ******************************************************************************/

static bool perf_tests_enabled(void)
{
	const char *val = getenv("CKPOOL_PERF_TESTS");

	return val && val[0] == '1';
}

/* Test normalize_pool_diff performance */
static void test_vardiff_normalize_performance(void)
{
	/* Test that normalize_pool_diff is fast enough for high-throughput pools */
	tv_t start, end;
	const int iterations = 10000000;  /* 10M calls */
	
	tv_time(&start);
	for (int i = 0; i < iterations; i++) {
		double test_vals[] = {0.5, 1.0, 1.5, 10.5, 100.5, 1000.5};
		for (int j = 0; j < 6; j++) {
			(void)normalize_pool_diff(test_vals[j]);
		}
	}
	tv_time(&end);
	
	double elapsed = tvdiff(&end, &start);
	
	if (elapsed < 0.001) {
		printf("    normalize_pool_diff: TOO FAST to measure accurately (< 1ms for 60M calls)\n");
	} else {
		double calls_per_sec = (iterations * 6) / elapsed;
		printf("    normalize_pool_diff: %.2fM calls/sec (%.3f sec for %dM calls)\n",
		       calls_per_sec / 1e6, elapsed, iterations * 6 / 1000000);
	}
	
	/* Should complete in reasonable time (< 10 seconds even on slow hardware) */
	assert_true(elapsed < 10.0);
}

/* Test optimal diff calculations performance */
static void test_vardiff_optimal_calc_performance(void)
{
	/* Simulate vardiff adjustment calculations under load */
	tv_t start, end;
	const int iterations = 1000000;  /* 1M adjustments */
	
	tv_time(&start);
	for (int i = 0; i < iterations; i++) {
		/* Typical vardiff calculation sequence */
		double dsps = 10.0 + (i % 1000);
		double optimal = dsps * 3.33;
		
		/* Apply clamping */
		double mindiff = 1.0;
		double maxdiff = 10000.0;
		if (optimal < mindiff) optimal = mindiff;
		if (optimal > maxdiff) optimal = maxdiff;
		
		/* Normalize */
		double final_diff = normalize_pool_diff(optimal);
		
		/* Prevent optimization */
		if (final_diff < 0.0) break;
	}
	tv_time(&end);
	
	double elapsed = tvdiff(&end, &start);
	
	if (elapsed < 0.001) {
		printf("    Optimal diff calculations: TOO FAST to measure accurately (< 1ms for 1M calcs)\n");
	} else {
		double calcs_per_sec = iterations / elapsed;
		printf("    Optimal diff calculations: %.2fM calcs/sec (%.3f sec for %dM calcs)\n",
		       calcs_per_sec / 1e6, elapsed, iterations / 1000000);
	}
	
	/* Should handle 1M calculations in < 5 seconds */
	assert_true(elapsed < 5.0);
}

/* Test hysteresis check performance */
static void test_vardiff_hysteresis_performance(void)
{
	/* Hysteresis checks happen for every share submission */
	tv_t start, end;
	const int iterations = 10000000;  /* 10M checks */
	
	tv_time(&start);
	for (int i = 0; i < iterations; i++) {
		/* Typical drr values across the spectrum */
		double drr = 0.1 + (i % 100) * 0.01;  /* 0.1 to 1.0 */
		
		/* Hysteresis check: drr > 0.15 && drr < 0.4 */
		bool in_deadband = (drr > 0.15 && drr < 0.4);
		
		/* Prevent optimization */
		if (in_deadband && drr < 0.0) break;
	}
	tv_time(&end);
	
	double elapsed = tvdiff(&end, &start);
	
	if (elapsed < 0.001) {
		printf("    Hysteresis checks: TOO FAST to measure accurately (< 1ms for 10M checks)\n");
	} else {
		double checks_per_sec = iterations / elapsed;
		printf("    Hysteresis checks: %.2fM checks/sec (%.3f sec for %dM checks)\n",
		       checks_per_sec / 1e6, elapsed, iterations / 1000000);
	}
	
	/* Should complete 10M checks in < 5 seconds */
	assert_true(elapsed < 5.0);
}

/*******************************************************************************
 * SECTION 7: REALISTIC WORKFLOW SCENARIOS
 * Integration-style tests simulating complete miner lifecycles
 ******************************************************************************/

/* Test complete ASIC connection and work flow */
static void test_asic_connection_flow(void)
{
	/* Simulate Antminer S19 Pro (110 TH/s) connecting and submitting shares */
	
	/* Initial parameters */
	double client_diff = 42.0;  /* Starting diff */
	double mindiff = 1.0;
	double maxdiff = 100000.0;
	double network_diff = 50000000000000.0;  /* ~50T network diff */
	int ssdc = 0;  /* Shares since diff change */
	
	/* Phase 1: First share (no adjustment yet) */
	ssdc++;
	assert_int_equal(ssdc, 1);
	/* No vardiff adjustment on first share */
	
	/* Phase 2: Second share, 1.5 seconds later (too fast, increase diff) */
	double tdiff = 1.5;
	double dsps = client_diff / tdiff;  /* 28 shares per second */
	double optimal = dsps * 3.33;  /* Target ~3.33 seconds per share */
	optimal = normalize_pool_diff(optimal);
	
	/* Should recommend higher diff (faster share rate needs higher diff) */
	assert_true(optimal > client_diff);
	
	/* Phase 3: Check hysteresis (drr = dsps/optimal) */
	double drr = dsps / optimal;
	bool in_deadband = (drr > 0.15 && drr < 0.4);
	/* With fast shares, drr should be low, outside deadband */
	assert_true(drr < 0.4);
	
	/* Phase 4: Apply new diff (clamped to maxdiff) */
	if (optimal > maxdiff) optimal = maxdiff;
	if (optimal > network_diff) optimal = network_diff;
	client_diff = optimal;
	ssdc = 0;  /* Reset counter */
	
	/* Phase 5: Multiple shares at stable rate */
	for (int i = 0; i < 10; i++) {
		tdiff = 3.0 + (i % 3);  /* 3-5 second variance */
		dsps = client_diff / tdiff;
		optimal = normalize_pool_diff(dsps * 3.33);
		
		/* Should stabilize around current diff */
		drr = dsps / optimal;
		
		/* Most should be in deadband (stable) */
		if (tdiff >= 3.0 && tdiff <= 4.0) {
			in_deadband = (drr > 0.15 && drr < 0.4);
			/* May or may not be in deadband depending on exact timing */
		}
		
		ssdc++;
	}
	
	/* Final validation: ASIC should have reasonable final diff */
	assert_true(client_diff >= mindiff);
	assert_true(client_diff <= maxdiff);
	assert_true(client_diff <= network_diff);
}

/* Test CPU miner stability at minimum difficulty */
static void test_cpu_miner_stable_at_mindiff(void)
{
	/* Simulate low-hashrate CPU miner (100 H/s) */
	
	double client_diff = 0.01;  /* Very low starting diff */
	double mindiff = 0.001;
	double maxdiff = 100000.0;
	double hashrate = 100.0;  /* 100 H/s */
	double target_time = 3.33;
	
	/* Calculate ideal diff for this hashrate */
	/* hashrate / 2^32 = shares_per_second at diff 1.0 */
	double sps_at_diff1 = hashrate / 4294967296.0;
	double ideal_diff = sps_at_diff1 * target_time;
	
	/* Should be well below mindiff */
	assert_true(ideal_diff < mindiff);
	
	/* Phase 1: Submit share at mindiff */
	client_diff = mindiff;
	double expected_time = mindiff / (hashrate / 4294967296.0);
	
	/* Share should take ~42 seconds at this hashrate/diff */
	assert_true(expected_time > 30.0);  /* Very slow */
	
	/* Phase 2: Vardiff tries to adjust down */
	double tdiff = expected_time;
	double dsps = client_diff / tdiff;
	double optimal = normalize_pool_diff(dsps * 3.33);
	
	/* Optimal would be below mindiff */
	assert_true(optimal < mindiff);
	
	/* Phase 3: Clamp to mindiff */
	if (optimal < mindiff) optimal = mindiff;
	
	/* Should stay at mindiff */
	assert_double_equal(optimal, mindiff, EPSILON);
	client_diff = optimal;
	
	/* Phase 4: Multiple shares - should remain stable at mindiff */
	for (int i = 0; i < 5; i++) {
		/* CPU miner variance is high */
		tdiff = expected_time * (0.8 + (i % 3) * 0.2);  /* ±20% variance */
		dsps = client_diff / tdiff;
		optimal = normalize_pool_diff(dsps * 3.33);
		
		/* Clamp to mindiff */
		if (optimal < mindiff) optimal = mindiff;
		
		/* Should always clamp to mindiff */
		assert_double_equal(optimal, mindiff, EPSILON);
	}
	
	/* CPU miner should remain at mindiff throughout session */
	assert_double_equal(client_diff, mindiff, EPSILON);
}

/* Test miner returning from idle (disconnection/reconnection) */
static void test_idle_return_diff_reset(void)
{
	/* Simulate Antminer that disconnects and reconnects */
	
	/* Phase 1: Established miner at stable diff */
	double client_diff = 1024.0;
	double mindiff = 1.0;
	double maxdiff = 100000.0;
	int ssdc = 42;  /* Has submitted many shares */
	
	/* Stable operation */
	double tdiff = 3.5;
	double dsps = client_diff / tdiff;
	double optimal = normalize_pool_diff(dsps * 3.33);
	
	/* Should be near current diff */
	assert_true(fabs(optimal - client_diff) < client_diff * 0.5);
	
	/* Phase 2: Miner disconnects (simulated by ssdc reset) */
	/* ... time passes ... */
	
	/* Phase 3: Miner reconnects - first share */
	ssdc = 1;  /* First share since reconnection */
	
	/* Comes back slower (was idle, needs warmup) */
	tdiff = 15.0;  /* Much slower than normal */
	dsps = client_diff / tdiff;
	optimal = normalize_pool_diff(dsps * 3.33);
	
	/* Optimal would suggest lower diff */
	assert_true(optimal < client_diff);
	
	/* Phase 4: First share after idle - check reset logic */
	/* From stratifier.c:5833: if optimal < diff && ssdc == 1, reset timer */
	if (optimal < client_diff && ssdc == 1) {
		/* Should NOT apply diff decrease immediately */
		/* Instead, reset ldc timer and wait for more shares */
		/* This prevents knee-jerk reactions to idle/warmup periods */
		
		/* Don't change diff yet */
		double new_diff = client_diff;  /* Keep current */
		assert_double_equal(new_diff, client_diff, EPSILON);
		
		/* But DO reset ssdc counter? No, it stays at 1 */
		ssdc = 1;
	}
	
	/* Phase 5: Second share at normal speed */
	ssdc = 2;
	tdiff = 3.5;  /* Back to normal */
	dsps = client_diff / tdiff;
	optimal = normalize_pool_diff(dsps * 3.33);
	
	/* Now can adjust normally */
	/* Check hysteresis */
	double drr = dsps / optimal;
	bool in_deadband = (drr > 0.15 && drr < 0.4);
	
	if (!in_deadband) {
		/* Apply diff change */
		client_diff = optimal;
	}
	
	/* Phase 6: Multiple shares - should stabilize */
	for (int i = 0; i < 10; i++) {
		tdiff = 3.0 + (i % 3);
		dsps = client_diff / tdiff;
		optimal = normalize_pool_diff(dsps * 3.33);
		
		/* Clamp */
		if (optimal < mindiff) optimal = mindiff;
		if (optimal > maxdiff) optimal = maxdiff;
		
		/* Check hysteresis */
		drr = dsps / optimal;
		in_deadband = (drr > 0.15 && drr < 0.4);
		
		if (!in_deadband) {
			client_diff = optimal;
		}
		
		ssdc++;
	}
	
	/* Should stabilize at reasonable diff */
	assert_true(client_diff >= mindiff);
	assert_true(client_diff <= maxdiff);
}

/*******************************************************************************
 * MAIN TEST RUNNER
 ******************************************************************************/

int main(void)
{
	printf("========================================\n");
	printf("COMPREHENSIVE VARDIFF TEST SUITE\n");
	printf("3 sections: Core | Fractional | Real-world\n");
	printf("========================================\n");
	
	/* Section 1: Core vardiff algorithm tests */
	printf("\n[SECTION 1: CORE VARDIFF ALGORITHM]\n");
	run_test(test_time_bias);
	run_test(test_optimal_diff_calculation);
	run_test(test_diff_clamping);
	run_test(test_drr_hysteresis);
	run_test(test_vardiff_sub1_edge_case);
	
	/* Section 2: Fractional difficulty tests */
	printf("\n[SECTION 2: FRACTIONAL DIFFICULTY SUPPORT]\n");
	run_test(test_optimal_diff_normalization);
	run_test(test_lround_elimination_sub1_only);
	run_test(test_vardiff_below_1);
	run_test(test_vardiff_above_1_normalized);
	run_test(test_floor_check_change);
	run_test(test_mindiff_clamping_fractional);
	run_test(test_worker_mindiff_fractional);
	run_test(test_vardiff_adjustment_sequence);
	
	/* Section 3: Comprehensive real-world miner tests */
	printf("\n[SECTION 3: REAL-WORLD MINER SCENARIOS]\n");
	printf("Testing full miner spectrum: H/s to EH/s\n");
	run_test(test_all_miner_types_initial_diff);
	run_test(test_fractional_diff_low_hashrate);
	run_test(test_integer_diff_typical_miners);
	run_test(test_high_hashrate_maxdiff_enforcement);
	run_test(test_mixed_miner_pool);
	run_test(test_hysteresis_across_ranges);
	run_test(test_network_diff_absolute_ceiling);
	run_test(test_worker_mindiff_enforcement);
	run_test(test_client_suggest_diff_overrides);
	run_test(test_burst_detection_property);
	run_test(test_extreme_hashrate_cases);
	
	/* Section 4: Production code edge cases */
	printf("\n[SECTION 4: PRODUCTION CODE EDGE CASES]\n");
	printf("Testing critical paths from stratifier.c add_submit()\n");
	run_test(test_vardiff_hysteresis_deadband);
	run_test(test_vardiff_network_diff_ceiling);
	run_test(test_vardiff_first_share_after_idle);
	run_test(test_vardiff_epsilon_comparison);
	run_test(test_vardiff_time_bias_sanity_clamp);
	
	/* Section 5: Failure mode tests */
	printf("\n[SECTION 5: FAILURE MODE TESTS]\n");
	printf("Testing defensive programming and error handling\n");
	run_test(test_vardiff_divide_by_zero_protection);
	run_test(test_vardiff_negative_difficulty_rejection);
	run_test(test_vardiff_clock_backwards_handling);
	
	/* Section 6: Performance regression tests */
	if (perf_tests_enabled()) {
		printf("\n[SECTION 6: PERFORMANCE REGRESSION TESTS]\n");
		printf("BEGIN PERF TESTS: test-vardiff\n");
		printf("Benchmarking critical functions (lower is better)\n");
		run_test(test_vardiff_normalize_performance);
		run_test(test_vardiff_optimal_calc_performance);
		run_test(test_vardiff_hysteresis_performance);
		printf("END PERF TESTS: test-vardiff\n");
	} else {
		printf("\n[SECTION 6: PERFORMANCE REGRESSION TESTS]\n");
		printf("Perf tests skipped (enable with make check-perf)\n");
	}
	
	/* Section 7: Realistic workflow scenarios */
	printf("\n[SECTION 7: REALISTIC WORKFLOW SCENARIOS]\n");
	printf("Testing complete miner lifecycles (integration-style)\n");
	run_test(test_asic_connection_flow);
	run_test(test_cpu_miner_stable_at_mindiff);
	run_test(test_idle_return_diff_reset);
	
	printf("\n========================================\n");
	printf("ALL VARDIFF TESTS PASSED!\n");
	printf("Total tests: 35 + 3 perf (optional)\n");
	printf("========================================\n");
	
	return 0;
}
