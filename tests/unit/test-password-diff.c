/*
 * Unit tests for password field difficulty suggestion
 * Tests parsing of diff=X from password field
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

#include "../test_common.h"

/*
 * Simulates the password difficulty parsing logic from stratifier.c parse_authorise()
 * This mirrors the actual code that parses diff=X from the password field
 */
static double parse_password_diff(const char *password, double mindiff, int64_t maxdiff)
{
	double pass_diff = 0;
	char *endptr;
	const char *pass_str = password;
	char *diff_ptr;
	
	if (!password || !strlen(password))
		return 0;
	
	/* Search for "diff=" anywhere in the password string */
	diff_ptr = strstr(pass_str, "diff=");
	if (diff_ptr) {
		const char *value_start = diff_ptr + strlen("diff=");
		
		/* Reject if there's a space immediately after "diff=" */
		if (*value_start == ' ' || *value_start == '\t') {
			pass_diff = 0;
		} else {
			/* Found "diff=" - parse the value after it */
			pass_diff = strtod(value_start, &endptr);
			
			/* Check if parsing succeeded and result is finite */
			if (endptr == value_start || !isfinite(pass_diff)) {
				/* Failed to parse or invalid value (inf/nan) - reset to 0 */
				pass_diff = 0;
			} else {
				/* Parsing succeeded - verify it's followed by valid delimiter */
				/* Valid: end of string, comma, or whitespace */
				if (*endptr != '\0' && *endptr != ',' && 
				    *endptr != ' ' && *endptr != '\t') {
					/* Invalid character after number (e.g., "diff=200x") - reject */
					pass_diff = 0;
				}
			}
		}
	}
	
	/* If we got a valid positive number, apply constraints */
	if (pass_diff > 0) {
		double sdiff = pass_diff;
		
		/* Respect mindiff - clamp to pool minimum */
		if (sdiff < mindiff)
			sdiff = mindiff;
		
		/* Respect maxdiff - clamp to pool maximum if set */
		if (maxdiff && sdiff > maxdiff)
			sdiff = maxdiff;
		
		return sdiff;
	}
	
	return 0;
}

/* Test: Parse simple diff=X format */
static void test_parse_simple_diff(void)
{
	double result;
	
	result = parse_password_diff("diff=0.001", 0.0001, 0);
	assert_double_equal(result, 0.001, EPSILON_DIFF);
	
	result = parse_password_diff("diff=200", 1.0, 0);
	assert_double_equal(result, 200.0, EPSILON_DIFF);
	
	result = parse_password_diff("diff=1.5", 1.0, 0);
	assert_double_equal(result, 1.5, EPSILON_DIFF);
}

/* Test: Parse diff=X from comma-separated parameters */
static void test_parse_comma_separated(void)
{
	double result;
	
	result = parse_password_diff("x, diff=200, f=9", 1.0, 0);
	assert_double_equal(result, 200.0, EPSILON_DIFF);
	
	result = parse_password_diff("diff=0.001, other=5", 0.0001, 0);
	assert_double_equal(result, 0.001, EPSILON_DIFF);
	
	result = parse_password_diff("a=1, diff=42, b=2", 1.0, 0);
	assert_double_equal(result, 42.0, EPSILON_DIFF);
}

/* Test: Reject space after diff= */
static void test_reject_space_after_equals(void)
{
	double result;
	
	result = parse_password_diff("diff= 200", 1.0, 0);
	assert_double_equal(result, 0.0, EPSILON_DIFF);
	
	result = parse_password_diff("diff= 0.001", 0.0001, 0);
	assert_double_equal(result, 0.0, EPSILON_DIFF);
	
	result = parse_password_diff("x, diff= 200, f=9", 1.0, 0);
	assert_double_equal(result, 0.0, EPSILON_DIFF);
}

/* Test: Reject invalid characters after number */
static void test_reject_invalid_chars(void)
{
	double result;
	
	result = parse_password_diff("diff=200x", 1.0, 0);
	assert_double_equal(result, 0.0, EPSILON_DIFF);
	
	result = parse_password_diff("diff=0.001abc", 0.0001, 0);
	assert_double_equal(result, 0.0, EPSILON_DIFF);
	
	result = parse_password_diff("diff=200other=5", 1.0, 0);
	assert_double_equal(result, 0.0, EPSILON_DIFF);
}

/* Test: Accept valid delimiters after number */
static void test_accept_valid_delimiters(void)
{
	double result;
	
	result = parse_password_diff("diff=200", 1.0, 0);
	assert_double_equal(result, 200.0, EPSILON_DIFF);
	
	result = parse_password_diff("diff=200,", 1.0, 0);
	assert_double_equal(result, 200.0, EPSILON_DIFF);
	
	result = parse_password_diff("diff=200 ", 1.0, 0);
	assert_double_equal(result, 200.0, EPSILON_DIFF);
	
	result = parse_password_diff("diff=200\t", 1.0, 0);
	assert_double_equal(result, 200.0, EPSILON_DIFF);
}

/* Test: Clamp to mindiff */
static void test_clamp_to_mindiff(void)
{
	double result;
	
	result = parse_password_diff("diff=0.0001", 0.001, 0);
	assert_double_equal(result, 0.001, EPSILON_DIFF);
	
	result = parse_password_diff("diff=0.5", 1.0, 0);
	assert_double_equal(result, 1.0, EPSILON_DIFF);
	
	result = parse_password_diff("diff=50", 100.0, 0);
	assert_double_equal(result, 100.0, EPSILON_DIFF);
}

/* Test: Clamp to maxdiff */
static void test_clamp_to_maxdiff(void)
{
	double result;
	
	result = parse_password_diff("diff=500", 1.0, 200);
	assert_double_equal(result, 200.0, EPSILON_DIFF);
	
	result = parse_password_diff("diff=1000", 1.0, 500);
	assert_double_equal(result, 500.0, EPSILON_DIFF);
	
	result = parse_password_diff("diff=100", 1.0, 50);
	assert_double_equal(result, 50.0, EPSILON_DIFF);
}

/* Test: Clamp to both mindiff and maxdiff */
static void test_clamp_both_limits(void)
{
	double result;
	
	result = parse_password_diff("diff=0.5", 1.0, 200);
	assert_double_equal(result, 1.0, EPSILON_DIFF);
	
	result = parse_password_diff("diff=500", 1.0, 200);
	assert_double_equal(result, 200.0, EPSILON_DIFF);
	
	result = parse_password_diff("diff=150", 1.0, 200);
	assert_double_equal(result, 150.0, EPSILON_DIFF);
}

/* Test: No diff= found returns 0 */
static void test_no_diff_returns_zero(void)
{
	double result;
	
	result = parse_password_diff("x", 1.0, 0);
	assert_double_equal(result, 0.0, EPSILON_DIFF);
	
	result = parse_password_diff("password123", 1.0, 0);
	assert_double_equal(result, 0.0, EPSILON_DIFF);
	
	result = parse_password_diff("", 1.0, 0);
	assert_double_equal(result, 0.0, EPSILON_DIFF);
}

/* Test: NULL or empty password returns 0 */
static void test_null_empty_password(void)
{
	double result;
	
	result = parse_password_diff(NULL, 1.0, 0);
	assert_double_equal(result, 0.0, EPSILON_DIFF);
	
	result = parse_password_diff("", 1.0, 0);
	assert_double_equal(result, 0.0, EPSILON_DIFF);
}

/* Test: Reject negative values */
static void test_reject_negative_values(void)
{
	double result;
	
	result = parse_password_diff("diff=-100", 1.0, 0);
	assert_double_equal(result, 0.0, EPSILON_DIFF);
	
	result = parse_password_diff("diff=-0.001", 1.0, 0);
	assert_double_equal(result, 0.0, EPSILON_DIFF);
	
	result = parse_password_diff("diff=-1", 1.0, 0);
	assert_double_equal(result, 0.0, EPSILON_DIFF);
}

/* Test: Reject zero value */
static void test_reject_zero_value(void)
{
	double result;
	
	result = parse_password_diff("diff=0", 1.0, 0);
	assert_double_equal(result, 0.0, EPSILON_DIFF);
	
	result = parse_password_diff("diff=0.0", 1.0, 0);
	assert_double_equal(result, 0.0, EPSILON_DIFF);
	
	result = parse_password_diff("diff=0.000", 1.0, 0);
	assert_double_equal(result, 0.0, EPSILON_DIFF);
}

/* Test: Reject special floating point values (inf, nan) */
static void test_reject_special_fp_values(void)
{
	double result;
	
	result = parse_password_diff("diff=inf", 1.0, 0);
	assert_double_equal(result, 0.0, EPSILON_DIFF);
	
	result = parse_password_diff("diff=infinity", 1.0, 0);
	assert_double_equal(result, 0.0, EPSILON_DIFF);
	
	result = parse_password_diff("diff=-inf", 1.0, 0);
	assert_double_equal(result, 0.0, EPSILON_DIFF);
	
	result = parse_password_diff("diff=nan", 1.0, 0);
	assert_double_equal(result, 0.0, EPSILON_DIFF);
}

/* Test: Scientific notation support */
static void test_scientific_notation(void)
{
	double result;
	
	/* Scientific notation should be parsed correctly */
	result = parse_password_diff("diff=1e-3", 0.0001, 0);
	assert_double_equal(result, 0.001, EPSILON_DIFF);
	
	result = parse_password_diff("diff=2e2", 1.0, 0);
	assert_double_equal(result, 200.0, EPSILON_DIFF);
	
	result = parse_password_diff("diff=1.5e-1", 0.01, 0);
	assert_double_equal(result, 0.15, EPSILON_DIFF);
	
	/* Scientific notation with clamping */
	result = parse_password_diff("diff=1e-5", 0.001, 0);
	assert_double_equal(result, 0.001, EPSILON_DIFF); /* Clamped to mindiff */
}

int main(void)
{
	printf("Running password difficulty parsing tests...\n\n");
	
	run_test(test_parse_simple_diff);
	run_test(test_parse_comma_separated);
	run_test(test_reject_space_after_equals);
	run_test(test_reject_invalid_chars);
	run_test(test_accept_valid_delimiters);
	run_test(test_clamp_to_mindiff);
	run_test(test_clamp_to_maxdiff);
	run_test(test_clamp_both_limits);
	run_test(test_no_diff_returns_zero);
	run_test(test_null_empty_password);
	run_test(test_reject_negative_values);
	run_test(test_reject_zero_value);
	run_test(test_reject_special_fp_values);
	run_test(test_scientific_notation);
	
	printf("\nAll password difficulty parsing tests passed!\n");
	return 0;
}
