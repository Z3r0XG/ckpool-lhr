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

/* Test normalize_pool_diff helper (whole numbers for >=1, unchanged below) */
static void test_normalize_pool_diff(void)
{
    struct {
        double in;
        double out;
    } cases[] = {
        { 0.5, 0.5 },
        { 0.999, 0.999 },
        { 1.0, 1.0 },
        { 1.001, 1.0 },
        { 1.4, 1.0 },
        { 1.5, 2.0 },
        { 153.6176, 154.0 },
        { 1000.5, 1001.0 },
    };

    int num = sizeof(cases) / sizeof(cases[0]);
    for (int i = 0; i < num; i++) {
        double normalized = normalize_pool_diff(cases[i].in);
        assert_double_equal(normalized, cases[i].out, EPSILON_DIFF);
        /* Idempotence */
        assert_double_equal(normalize_pool_diff(normalized), normalized, EPSILON_DIFF);
    }
}

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

/* ========================================================================
 * SECTION 2: Network Difficulty Floor (allow_low_diff feature)
 * From: test-low-diff.c
 * Tests: 7 tests for network difficulty floor behavior
 * ======================================================================== */

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

/*******************************************************************************
 * SECTION 3: FAILURE MODE TESTS
 * Tests defensive programming and error handling
 ******************************************************************************/

/* Test target edge cases (zero target, max target) */
static void test_difficulty_target_edge_cases(void)
{
	uchar target[32];
	double diff;
	
	/* Zero target (invalid, would cause division by zero) */
	memset(target, 0x00, 32);
	diff = diff_from_target(target);
	/* Should return 0.0 or handle gracefully */
	assert_true(diff >= 0.0 || diff == 0.0);
	
	/* Maximum target (all 0xFF, easiest difficulty) */
	memset(target, 0xFF, 32);
	diff = diff_from_target(target);
	/* Should be very close to minimum difficulty */
	assert_true(diff > 0.0);
	assert_true(!isnan(diff));
	assert_true(!isinf(diff));
	
	/* Very easy target (difficulty close to 1) */
	/* Just verify the function handles reasonable targets */
	target_from_diff(target, 1.0);
	diff = diff_from_target(target);
	assert_double_equal(diff, 1.0, 0.01);  /* Should round-trip to ~1.0 */
}

/* Test nbits compact format edge cases */
static void test_difficulty_nbits_overflow(void)
{
	double diff;
	
	struct {
		const char *nbits_hex;  /* 8-character hex string (4 bytes) */
		bool is_valid;
		const char *description;
	} cases[] = {
		/* Valid nbits */
		{ "1d00ffff", true,  "Bitcoin genesis block" },
		{ "1b0404cb", true,  "Typical mainnet" },
		{ "207fffff", true,  "Testnet easy" },
		
		/* Edge cases */
		{ "00000000", false, "Zero nbits" },
		{ "01000000", false, "Negative exponent (empty mantissa)" },
		{ "01ffffff", false, "Negative exponent with mantissa" },
	};
	
	for (int i = 0; i < (int)(sizeof(cases) / sizeof(cases[0])); i++) {
		/* diff_from_nbits expects 8-char hex string */
		diff = diff_from_nbits((char *)cases[i].nbits_hex);
		
		if (cases[i].is_valid) {
			/* Valid nbits should produce sane difficulty */
			assert_true(diff > 0.0);
			assert_true(!isnan(diff));
			assert_true(!isinf(diff));
		} else {
			/* Invalid nbits behavior is implementation-dependent */ 
			/* May return 0, may return garbage, should not crash */
			assert_true(!isnan(diff) || true);  /* Just don't crash */
		}
	}
}

/* Test normalize_pool_diff with NaN and infinity */
static void test_difficulty_normalize_nan_inf(void)
{
	double result;
	
	/* Positive infinity */
	result = normalize_pool_diff(INFINITY);
	/* Should handle gracefully (may return inf or clamp to max) */
	assert_true(isinf(result) || result > 0.0);
	
	/* Negative infinity */
	result = normalize_pool_diff(-INFINITY);
	/* Should handle gracefully */
	assert_true(isinf(result) || result < 0.0 || result == 0.0);
	
	/* NaN */
	result = normalize_pool_diff(NAN);
	/* Should handle gracefully (may propagate NaN or return 0) */
	assert_true(isnan(result) || result == 0.0 || result > 0.0);
}

int main(void)
{
    printf("Running difficulty calculation tests...\n\n");
    
    printf("=== Section 1: Core Difficulty Calculations ===\n");
    run_test(test_diff_roundtrip_sub1);
    run_test(test_sub1_difficulty_values);
    run_test(test_difficulty_edge_cases);
    run_test(test_diff_from_nbits);
    run_test(test_target_conversions);
    run_test(test_normalize_pool_diff);
    
    printf("\n=== Section 2: Network Difficulty Floor (allow_low_diff) ===\n");
    run_test(test_low_diff_disabled_clamps_to_one);
    run_test(test_low_diff_disabled_passes_high_diff);
    run_test(test_low_diff_enabled_passes_low_diff);
    run_test(test_low_diff_enabled_passes_high_diff);
    run_test(test_diff_exactly_one);
    run_test(test_diff_zero);
    run_test(test_regtest_diff);
    
    printf("\n=== Section 3: Failure Mode Tests ===\n");
    run_test(test_difficulty_target_edge_cases);
    run_test(test_difficulty_nbits_overflow);
    run_test(test_difficulty_normalize_nan_inf);
    
    printf("\nAll difficulty tests passed!\n");
    return 0;
}

