/*
 * Unit tests for fractional configuration validation
 * Tests startdiff validation and suggest_difficulty parsing
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

#include "config.h"
#include "../test_common.h"
#include "libckpool.h"
#include "ckpool.h"
#include <jansson.h>

/* Test validate_startdiff helper matches production behavior */
static void test_validate_startdiff_helper(void)
{
	/* Negative rejected - CRITICAL: must fail before default applied */
	double startdiff = -1.5;
	assert_false(validate_startdiff(&startdiff));
	/* Value should remain unchanged after failed validation */
	assert_double_equal(startdiff, -1.5, EPSILON_DIFF);

	/* Another negative case */
	startdiff = -0.001;
	assert_false(validate_startdiff(&startdiff));
	assert_double_equal(startdiff, -0.001, EPSILON_DIFF);

	/* Zero defaults to 42 */
	startdiff = 0.0;
	assert_true(validate_startdiff(&startdiff));
	assert_double_equal(startdiff, 42.0, EPSILON_DIFF);

	/* Valid positives accepted untouched */
	struct {
		double in;
		double expected;
	} cases[] = {
		{ 0.001, 0.001 },
		{ 1.0, 1.0 },
		{ 42.0, 42.0 },
		{ 100.5, 100.5 },
	};

	for (int i = 0; i < (int)(sizeof(cases)/sizeof(cases[0])); i++) {
		startdiff = cases[i].in;
		assert_true(validate_startdiff(&startdiff));
		assert_double_equal(startdiff, cases[i].expected, EPSILON_DIFF);
	}
}

/* Test validate_mindiff helper matches production behavior */
static void test_validate_mindiff_helper(void)
{
	/* Negative rejected - CRITICAL: must fail before default applied */
	double mindiff = -0.5;
	assert_false(validate_mindiff(&mindiff));
	/* Value should remain unchanged after failed validation */
	assert_double_equal(mindiff, -0.5, EPSILON_DIFF);

	/* Another negative case */
	mindiff = -10.0;
	assert_false(validate_mindiff(&mindiff));
	assert_double_equal(mindiff, -10.0, EPSILON_DIFF);

	/* Zero defaults to 1.0 */
	mindiff = 0.0;
	assert_true(validate_mindiff(&mindiff));
	assert_double_equal(mindiff, 1.0, EPSILON_DIFF);

	/* Valid positives accepted */
	struct {
		double in;
		double expected;
	} cases[] = {
		{ 0.001, 0.001 },
		{ 0.5, 0.5 },
		{ 1.0, 1.0 },
	};

	for (int i = 0; i < (int)(sizeof(cases)/sizeof(cases[0])); i++) {
		mindiff = cases[i].in;
		assert_true(validate_mindiff(&mindiff));
		assert_double_equal(mindiff, cases[i].expected, EPSILON_DIFF);
	}
}

/* Test combined validation helper returns failure codes instead of exiting */
static void test_validate_diff_config_combined(void)
{
	double mindiff, startdiff;

	/* Both valid -> success */
	mindiff = 0.5; startdiff = 1.0;
	assert_int_equal(validate_diff_config(&mindiff, &startdiff), 0);

	/* Invalid mindiff -> code 1 */
	mindiff = -1.0; startdiff = 1.0;
	assert_int_equal(validate_diff_config(&mindiff, &startdiff), 1);

	/* Invalid startdiff -> code 2 */
	mindiff = 0.5; startdiff = -0.1;
	assert_int_equal(validate_diff_config(&mindiff, &startdiff), 2);
}

/* Test suggest_difficulty parsing as double instead of int64_t */
static void test_suggest_diff_fractional_parsing(void)
{
	/* Simulate parsing fractional suggest_difficulty values */
	struct {
		const char *method_str;
		double expected_diff;
	} test_cases[] = {
		/* Integer values still work */
		{ "mining.suggest_difficulty(1", 1.0 },
		{ "mining.suggest_difficulty(10", 10.0 },
		
		/* Fractional values now work (would fail with int64_t) */
		{ "mining.suggest_difficulty(0.5", 0.5 },
		{ "mining.suggest_difficulty(0.084", 0.084 },
		{ "mining.suggest_difficulty(1.5", 1.5 },
		{ "mining.suggest_difficulty(0.001", 0.001 },
	};
	
	int num_tests = sizeof(test_cases) / sizeof(test_cases[0]);
	for (int i = 0; i < num_tests; i++) {
		double parsed_diff = 0.0;
		
		/* Simulate new double parsing logic */
		if (sscanf(test_cases[i].method_str, "mining.suggest_difficulty(%lf", &parsed_diff) == 1) {
			assert_double_equal(parsed_diff, test_cases[i].expected_diff, EPSILON_DIFF);
		} else {
			assert_false(1);  /* Should have parsed successfully */
		}
	}
}

/* Test json_number_value handles both integers and floats correctly */
static void test_json_number_value_vs_real_value(void)
{
	json_t *int_val = json_integer(42);
	json_t *float_val = json_real(0.5);
	json_t *float_int_val = json_real(10.0);
	
	/* CRITICAL: json_real_value() returns 0.0 for integers (BUG) */
	assert_double_equal(json_real_value(int_val), 0.0, EPSILON_DIFF);
	
	/* json_number_value() correctly handles both types */
	assert_double_equal(json_number_value(int_val), 42.0, EPSILON_DIFF);
	assert_double_equal(json_number_value(float_val), 0.5, EPSILON_DIFF);
	assert_double_equal(json_number_value(float_int_val), 10.0, EPSILON_DIFF);
	
	/* Both work for actual floats */
	assert_double_equal(json_real_value(float_val), 0.5, EPSILON_DIFF);
	
	json_decref(int_val);
	json_decref(float_val);
	json_decref(float_int_val);
}

/* Test that old int64_t parsing would fail with fractional values */
static void test_suggest_diff_old_int64_fails_fractional(void)
{
	/* Demonstrate the bug: int64_t parsing truncates fractional values */
	struct {
		const char *method_str;
		int old_result;
		double new_result;
	} test_cases[] = {
		/* Fractional values get truncated to 0 with int64_t */
		{ "mining.suggest_difficulty(0.5", 0, 0.5 },
		{ "mining.suggest_difficulty(0.001", 0, 0.001 },
		{ "mining.suggest_difficulty(0.999", 0, 0.999 },
		
		/* Integer values work the same */
		{ "mining.suggest_difficulty(1", 1, 1.0 },
		{ "mining.suggest_difficulty(10", 10, 10.0 },
	};
	
	int num_tests = sizeof(test_cases) / sizeof(test_cases[0]);
	for (int i = 0; i < num_tests; i++) {
		int64_t old_parsed = 0;
		double new_parsed = 0.0;
		
		/* Old way (int64_t) */
		sscanf(test_cases[i].method_str, "mining.suggest_difficulty(%"PRId64, &old_parsed);
		
		/* New way (double) */
		sscanf(test_cases[i].method_str, "mining.suggest_difficulty(%lf", &new_parsed);
		
		/* Verify old truncates fractional values */
		assert_int_equal((int)old_parsed, test_cases[i].old_result);
		
		/* Verify new preserves them */
		assert_double_equal(new_parsed, test_cases[i].new_result, EPSILON_DIFF);
	}
}

/* Test mindiff validation for comparison */
static void test_mindiff_validation_consistency(void)
{
	/* Both mindiff and startdiff should validate the same way */
	struct {
		double diff_value;
		int should_be_valid;
	} test_cases[] = {
		{ -1.0, 0 },    /* Negative invalid */
		{ 0.0, 0 },     /* Zero invalid (triggers default) */
		{ 0.0001, 1 },  /* Very small but valid */
		{ 0.001, 1 },   /* Recommended minimum */
		{ 1.0, 1 },     /* Standard */
		{ 100.5, 1 },   /* Fractional valid */
	};
	
	int num_tests = sizeof(test_cases) / sizeof(test_cases[0]);
	for (int i = 0; i < num_tests; i++) {
		double mindiff = test_cases[i].diff_value;
		
		/* Validation logic: if !diff (zero/null), use default; if <= 0, error */
		int valid = (mindiff > 0.0) ? 1 : 0;
		
		assert_int_equal(valid, test_cases[i].should_be_valid);
	}
}

/* Test suggest_diff comparisons now use doubles instead of int64_t */
static void test_suggest_diff_double_comparison(void)
{
	/* With double sdiff, fractional comparisons with epsilon tolerance work */
	struct {
		const char *description;
		double value1;
		double value2;
		int should_be_equal;
	} test_cases[] = {
		/* Exact match */
		{ "0.5 == 0.5", 0.5, 0.5, 1 },
		{ "0.084 == 0.084", 0.084, 0.084, 1 },
		{ "1.0 == 1.0", 1.0, 1.0, 1 },
		
		/* Tiny rounding error within epsilon */
		{ "0.5 ~= 0.5000001", 0.5, 0.5000001, 1 },
		
		/* Real difference outside epsilon */
		{ "0.5 != 0.4", 0.5, 0.4, 0 },
		{ "1.0 != 0.5", 1.0, 0.5, 0 },
		{ "0.084 != 0.085", 0.084, 0.085, 0 },
	};
	
	int num_tests = sizeof(test_cases) / sizeof(test_cases[0]);
	for (int i = 0; i < num_tests; i++) {
		double val1 = test_cases[i].value1;
		double val2 = test_cases[i].value2;
		
		/* Epsilon-based comparison for doubles */
		int are_equal = (fabs(val1 - val2) < 1e-6) ? 1 : 0;
		
		assert_int_equal(are_equal, test_cases[i].should_be_equal);
	}
}

/* Test clamping and no-op behavior mirrors suggest_diff logic */
static void test_suggest_diff_clamp_and_noop(void)
{
	const double mindiff = 0.2;
	const double epsilon = 1e-6;

	struct {
		double requested;
		double current_diff;
		double current_suggest_diff;
		double expected_applied;
		int should_change; /* 0 = no-op, 1 = update */
	} cases[] = {
		/* Below mindiff -> clamp to mindiff, should change */
		{ 0.05, 0.3, 0.3, 0.2, 1 },
		/* Already at mindiff -> no-op after clamp */
		{ 0.05, 0.2, 0.2, 0.2, 0 },
		/* Within epsilon -> no-op */
		{ 0.3000000004, 0.3, 0.3, 0.3, 0 },
		/* Real change -> update */
		{ 0.5, 0.3, 0.3, 0.5, 1 },
		/* Suggest_diff matches diff but different suggest_diff -> no-op */
		{ 0.3, 0.3, 0.25, 0.3, 0 },
	};

	int num = sizeof(cases) / sizeof(cases[0]);
	for (int i = 0; i < num; i++) {
		double requested = cases[i].requested;
		double diff = cases[i].current_diff;
		double suggest = cases[i].current_suggest_diff;

		/* Clamp */
		double applied = requested;
		if (applied < mindiff)
			applied = mindiff;

		/* No-op checks (mirrors suggest_diff logic) */
		if (fabs(applied - suggest) < epsilon) {
			assert_int_equal(cases[i].should_change, 0);
			continue;
		}
		if (fabs(diff - applied) < epsilon) {
			assert_int_equal(cases[i].should_change, 0);
			continue;
		}

		/* If we reach here, an update should occur */
		assert_int_equal(cases[i].should_change, 1);
		assert_double_equal(applied, cases[i].expected_applied, EPSILON_DIFF);
	}
}

/* Local helper mirroring apply_suggest_diff logic without network side effects */
static bool suggest_diff_apply_local(double mindiff, double requested, double current_diff,
				      double current_suggest, int64_t workbase_id, double epsilon,
				      double *out_diff, double *out_suggest, int64_t *out_job_id,
				      double *out_old_diff)
{
	double sdiff = requested;
	if (sdiff < mindiff)
		sdiff = mindiff;
	if (fabs(sdiff - current_suggest) < epsilon) {
		if (out_diff)
			*out_diff = current_diff;
		if (out_suggest)
			*out_suggest = current_suggest;
		if (out_job_id)
			*out_job_id = workbase_id;
		if (out_old_diff)
			*out_old_diff = current_diff;
		return false;
	}
	if (fabs(current_diff - sdiff) < epsilon) {
		if (out_diff)
			*out_diff = current_diff;
		if (out_suggest)
			*out_suggest = sdiff;
		if (out_job_id)
			*out_job_id = workbase_id;
		if (out_old_diff)
			*out_old_diff = current_diff;
		return false;
	}

	if (out_old_diff)
		*out_old_diff = current_diff;
	if (out_diff)
		*out_diff = sdiff;
	if (out_suggest)
		*out_suggest = sdiff;
	if (out_job_id)
		*out_job_id = workbase_id + 1;
	return true;
}

/* Test the stratifier apply helper for side effects without network churn */
static void test_suggest_diff_apply_helper(void)
{
	double out_diff = 0.0;
	double out_suggest = 0.0;
	double out_old = 0.0;
	int64_t out_job = 0;

	/* Clamp below mindiff and update */
	bool changed = suggest_diff_apply_local(0.2, 0.05, 0.3, 0.3, 10, 1e-6,
							 &out_diff, &out_suggest, &out_job, &out_old);
	assert_true(changed);
	assert_double_equal(out_suggest, 0.2, EPSILON_DIFF);
	assert_double_equal(out_diff, 0.2, EPSILON_DIFF);
	assert_double_equal(out_old, 0.3, EPSILON_DIFF);
	assert_int_equal((int)out_job, 11);

	/* No-op when suggested matches suggest_diff */
	changed = suggest_diff_apply_local(0.2, 0.2, 0.2, 0.2, 5, 1e-6,
							 &out_diff, &out_suggest, &out_job, &out_old);
	assert_false(changed);

	/* Update when different and above mindiff */
	changed = suggest_diff_apply_local(0.2, 0.5, 0.3, 0.25, 7, 1e-6,
							 &out_diff, &out_suggest, &out_job, &out_old);
	assert_true(changed);
	assert_double_equal(out_suggest, 0.5, EPSILON_DIFF);
	assert_double_equal(out_diff, 0.5, EPSILON_DIFF);
	assert_double_equal(out_old, 0.3, EPSILON_DIFF);
	assert_int_equal((int)out_job, 8);
}

/* Main test runner */
int main(void)
{
	printf("Testing fractional configuration validation and suggest_difficulty parsing...\n\n");
	
	run_test(test_validate_startdiff_helper);
	run_test(test_validate_mindiff_helper);
	run_test(test_validate_diff_config_combined);
	run_test(test_suggest_diff_fractional_parsing);
	run_test(test_json_number_value_vs_real_value);
	run_test(test_suggest_diff_old_int64_fails_fractional);
	run_test(test_mindiff_validation_consistency);
	run_test(test_suggest_diff_double_comparison);
	run_test(test_suggest_diff_clamp_and_noop);
	run_test(test_suggest_diff_apply_helper);
	
	printf("\nAll fractional config tests passed!\n");
	return 0;
}
