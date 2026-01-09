/*
 * Unit tests for password field difficulty suggestion
 * Tests parsing of diff=X from password field
 *
 * INTERACTION WITH STRATUM mining.suggest_difficulty:
 * -------------------------------------------------
 * Password diff parsing occurs during mining.authorize (authentication).
 * When a password diff is set, client->password_diff_set is marked true.
 * This prevents subsequent mining.suggest_difficulty messages from overwriting it.
 *
 * Scenario 1: suggest_difficulty BEFORE auth
 *   1. Client sends mining.suggest_difficulty(128) → client->suggest_diff = 128
 *   2. Client sends mining.authorize with password "diff=200" → password_diff_set=true, suggest_diff=200
 *   Result: Password diff wins (200)
 *
 * Scenario 2: suggest_difficulty AFTER auth (with password diff set)
 *   1. Client sends mining.authorize with password "diff=200" → password_diff_set=true, suggest_diff=200
 *   2. Client sends mining.suggest_difficulty(50) → IGNORED (blocked by password_diff_set check)
 *   Result: Password diff remains (200)
 *
 * Password diff is "sticky"—once set via password during auth, it blocks subsequent
 * stratum suggest_difficulty messages from overwriting it. This ensures user-configured
 * difficulty takes precedence over miner-suggested difficulty.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

#include "../test_common.h"

/*
 * Mirrors the password difficulty parsing logic in stratifier.c parse_authorise()
 * (cleaned version: clamps to mindiff only, no maxdiff clamp)
 *
 * NOTE: This function only tests the parsing. The actual stratifier.c code also checks:
 *   - if (sdiff == client->suggest_diff) goto skip; (avoid redundant updates)
 *   - if (client->diff == sdiff) goto skip; (avoid redundant sends)
 * These checks prevent unnecessary diff changes when values haven't actually changed.
 */
static double parse_password_diff(const char *password, double mindiff)
{
    double pass_diff = 0;
    char *endptr;
    char *diff_ptr;
    char *pwd;
    size_t pwd_len;

    if (!password || !strlen(password))
        return 0;

    /* Create a modifiable copy for trimming */
    pwd = strdup(password);
    if (!pwd)
        return 0;

    /* Trim leading whitespace */
    char *start = pwd;
    while (*start && (*start == ' ' || *start == '\t'))
        start++;

    /* Find the end (ignoring trailing whitespace) */
    pwd_len = strlen(start);
    while (pwd_len > 0 && (start[pwd_len - 1] == ' ' || start[pwd_len - 1] == '\t'))
        pwd_len--;

    /* Null-terminate at the end of trimmed string */
    if (pwd_len > 0)
        start[pwd_len] = '\0';

    /* Search for "diff=" in the trimmed password string */
    if (pwd_len > 0) {
        diff_ptr = strstr(start, "diff=");
        if (diff_ptr) {
            /* Enforce word boundary: "diff" must be preceded by start-of-string or comma */
            bool valid_prefix = (diff_ptr == start); /* Start of string */
            if (!valid_prefix && diff_ptr > start) {
                char prev_char = *(diff_ptr - 1);
                /* Valid delimiter before "diff": comma only */
                valid_prefix = (prev_char == ',');
            }

            if (valid_prefix) {
                const char *value_start = diff_ptr + strlen("diff=");

                /* Reject if there's a space immediately after "diff=" (e.g., "diff =1") */
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
                        /* Valid delimiters: end of string (\0) or comma only */
                        /* Whitespace after number is NOT valid (e.g., "diff=200 ,x" fails) */
                        if (*endptr != '\0' && *endptr != ',') {
                            /* Invalid delimiter after number - reject */
                            pass_diff = 0;
                        }
                    }
                }
            }
        }
    }

    free(pwd);

    /* If we got a valid positive number, apply constraints */
    if (pass_diff > 0) {
        double sdiff = pass_diff;

        /* Respect mindiff - clamp to pool minimum */
        if (sdiff < mindiff)
            sdiff = mindiff;

        return sdiff;
    }

    return 0;
}

/* Test: Parse simple diff=X format */
static void test_parse_simple_diff(void)
{
    double result;

    result = parse_password_diff("diff=0.001", 0.0001);
    assert_double_equal(result, 0.001, EPSILON_DIFF);

    result = parse_password_diff("diff=200", 1.0);
    assert_double_equal(result, 200.0, EPSILON_DIFF);

    result = parse_password_diff("diff=1.5", 1.0);
    assert_double_equal(result, 1.5, EPSILON_DIFF);
}

/* Test: Parse diff=X from comma-separated parameters */
static void test_parse_comma_separated(void)
{
    double result;

    /* Comma-separated: no spaces allowed (internal spaces cause parsing failure) */
    result = parse_password_diff("x,diff=200,f=9", 1.0);
    assert_double_equal(result, 200.0, EPSILON_DIFF);

    result = parse_password_diff("diff=0.001,other=5", 0.0001);
    assert_double_equal(result, 0.001, EPSILON_DIFF);

    result = parse_password_diff("a=1,diff=42,b=2", 1.0);
    assert_double_equal(result, 42.0, EPSILON_DIFF);
}

/* Test: Reject space after diff= */
static void test_reject_space_after_equals(void)
{
    double result;

    result = parse_password_diff("diff= 200", 1.0);
    assert_double_equal(result, 0.0, EPSILON_DIFF);

    result = parse_password_diff("diff= 0.001", 0.0001);
    assert_double_equal(result, 0.0, EPSILON_DIFF);

    result = parse_password_diff("x, diff= 200, f=9", 1.0);
    assert_double_equal(result, 0.0, EPSILON_DIFF);
}

/* Test: Reject invalid characters after number */
static void test_reject_invalid_chars(void)
{
    double result;

    result = parse_password_diff("diff=200x", 1.0);
    assert_double_equal(result, 0.0, EPSILON_DIFF);

    result = parse_password_diff("diff=0.001abc", 0.0001);
    assert_double_equal(result, 0.0, EPSILON_DIFF);

    result = parse_password_diff("diff=200other=5", 1.0);
    assert_double_equal(result, 0.0, EPSILON_DIFF);
}

/* Test: Enforce word boundary and trim whitespace first */
static void test_word_boundary_enforcement(void)
{
    double result;

    /* Bug case: "xdiff=" should NOT match "diff=" */
    result = parse_password_diff("xdiff=0.1", 0.001);
    assert_double_equal(result, 0.0, EPSILON_DIFF);

    /* More variants of invalid prefixes */
    result = parse_password_diff("adiff=200", 1.0);
    assert_double_equal(result, 0.0, EPSILON_DIFF);

    result = parse_password_diff("_diff=100", 1.0);
    assert_double_equal(result, 0.0, EPSILON_DIFF);

    result = parse_password_diff("ndiff=50", 1.0);
    assert_double_equal(result, 0.0, EPSILON_DIFF);

    /* Leading/trailing spaces are TRIMMED first, then comma logic applies */
    result = parse_password_diff(" diff=100", 1.0);
    assert_double_equal(result, 100.0, EPSILON_DIFF);

    result = parse_password_diff("diff=150 ", 1.0);
    assert_double_equal(result, 150.0, EPSILON_DIFF);

    result = parse_password_diff(" diff=200 ", 1.0);
    assert_double_equal(result, 200.0, EPSILON_DIFF);

    result = parse_password_diff("\tdiff=75", 1.0);
    assert_double_equal(result, 75.0, EPSILON_DIFF);

    /* Spaces in the middle (not trimmed) still break parsing if no comma */
    result = parse_password_diff("x diff=0.1", 0.001);
    assert_double_equal(result, 0.0, EPSILON_DIFF);

    result = parse_password_diff("diff =1", 1.0);
    assert_double_equal(result, 0.0, EPSILON_DIFF);

    result = parse_password_diff("diff = 1", 1.0);
    assert_double_equal(result, 0.0, EPSILON_DIFF);

    /* Valid prefix cases: comma or start-of-string (after trimming) */
    result = parse_password_diff(",diff=200", 1.0);
    assert_double_equal(result, 200.0, EPSILON_DIFF);

    result = parse_password_diff("x,diff=0.1", 0.001);
    assert_double_equal(result, 0.1, EPSILON_DIFF);

    result = parse_password_diff(" ,diff=100 ", 1.0);
    assert_double_equal(result, 100.0, EPSILON_DIFF);

    /* Comma with space after it: internal space, will fail to parse */
    result = parse_password_diff("x, diff=0.1", 0.001);
    assert_double_equal(result, 0.0, EPSILON_DIFF);
}

/* Test: Accept valid delimiters after number */
static void test_accept_valid_delimiters(void)
{
    double result;

    result = parse_password_diff("diff=200", 1.0);
    assert_double_equal(result, 200.0, EPSILON_DIFF);

    result = parse_password_diff("diff=200,", 1.0);
    assert_double_equal(result, 200.0, EPSILON_DIFF);

    /* Trailing spaces are trimmed away, so these work */
    result = parse_password_diff("diff=200 ", 1.0);
    assert_double_equal(result, 200.0, EPSILON_DIFF);

    result = parse_password_diff("diff=200\t", 1.0);
    assert_double_equal(result, 200.0, EPSILON_DIFF);

    /* Space after number (before comma) is NOT a valid delimiter - should fail */
    result = parse_password_diff("diff=200 ,x", 1.0);
    assert_double_equal(result, 0.0, EPSILON_DIFF); /* Space is not valid delimiter */

    result = parse_password_diff("diff=.1 ,f=9", 0.001);
    assert_double_equal(result, 0.0, EPSILON_DIFF); /* Space after .1 is not valid delimiter */

    /* Test different value formats with proper delimiters */
    result = parse_password_diff("diff=.1", 0.001);
    assert_double_equal(result, 0.1, EPSILON_DIFF); /* Leading dot works */

    result = parse_password_diff("diff=0.1", 0.001);
    assert_double_equal(result, 0.1, EPSILON_DIFF); /* Explicit zero works */

    result = parse_password_diff("diff=.1,other=x", 0.001);
    assert_double_equal(result, 0.1, EPSILON_DIFF); /* .1 with comma delimiter */

    result = parse_password_diff("diff=0.1,other=x", 0.001);
    assert_double_equal(result, 0.1, EPSILON_DIFF); /* 0.1 with comma delimiter */
}

/* Test: Clamp to mindiff */
static void test_clamp_to_mindiff(void)
{
    double result;

    result = parse_password_diff("diff=0.0001", 0.001);
    assert_double_equal(result, 0.001, EPSILON_DIFF);

    result = parse_password_diff("diff=0.5", 1.0);
    assert_double_equal(result, 1.0, EPSILON_DIFF);

    result = parse_password_diff("diff=50", 100.0);
    assert_double_equal(result, 100.0, EPSILON_DIFF);
}

/* Test: No diff= found returns 0 */
static void test_no_diff_returns_zero(void)
{
    double result;

    result = parse_password_diff("x", 1.0);
    assert_double_equal(result, 0.0, EPSILON_DIFF);

    result = parse_password_diff("password123", 1.0);
    assert_double_equal(result, 0.0, EPSILON_DIFF);

    result = parse_password_diff("", 1.0);
    assert_double_equal(result, 0.0, EPSILON_DIFF);
}

/* Test: NULL or empty password returns 0 */
static void test_null_empty_password(void)
{
    double result;

    result = parse_password_diff(NULL, 1.0);
    assert_double_equal(result, 0.0, EPSILON_DIFF);

    result = parse_password_diff("", 1.0);
    assert_double_equal(result, 0.0, EPSILON_DIFF);
}

/* Test: Reject negative values */
static void test_reject_negative_values(void)
{
    double result;

    result = parse_password_diff("diff=-100", 1.0);
    assert_double_equal(result, 0.0, EPSILON_DIFF);

    result = parse_password_diff("diff=-0.001", 1.0);
    assert_double_equal(result, 0.0, EPSILON_DIFF);

    result = parse_password_diff("diff=-1", 1.0);
    assert_double_equal(result, 0.0, EPSILON_DIFF);
}

/* Test: Reject zero value */
static void test_reject_zero_value(void)
{
    double result;

    result = parse_password_diff("diff=0", 1.0);
    assert_double_equal(result, 0.0, EPSILON_DIFF);

    result = parse_password_diff("diff=0.0", 1.0);
    assert_double_equal(result, 0.0, EPSILON_DIFF);

    result = parse_password_diff("diff=0.000", 1.0);
    assert_double_equal(result, 0.0, EPSILON_DIFF);
}

/* Test: Reject special floating point values (inf, nan) */
static void test_reject_special_fp_values(void)
{
    double result;

    result = parse_password_diff("diff=inf", 1.0);
    assert_double_equal(result, 0.0, EPSILON_DIFF);

    result = parse_password_diff("diff=infinity", 1.0);
    assert_double_equal(result, 0.0, EPSILON_DIFF);

    result = parse_password_diff("diff=-inf", 1.0);
    assert_double_equal(result, 0.0, EPSILON_DIFF);

    result = parse_password_diff("diff=nan", 1.0);
    assert_double_equal(result, 0.0, EPSILON_DIFF);
}

/* Test: Scientific notation support */
static void test_scientific_notation(void)
{
    double result;

    /* Scientific notation should be parsed correctly */
    result = parse_password_diff("diff=1e-3", 0.0001);
    assert_double_equal(result, 0.001, EPSILON_DIFF);

    result = parse_password_diff("diff=2e2", 1.0);
    assert_double_equal(result, 200.0, EPSILON_DIFF);

    result = parse_password_diff("diff=1.5e-1", 0.01);
    assert_double_equal(result, 0.15, EPSILON_DIFF);

    /* Scientific notation with clamping */
    result = parse_password_diff("diff=1e-5", 0.001);
    assert_double_equal(result, 0.001, EPSILON_DIFF); /* Clamped to mindiff */
}

/* Test: Duplicate diff= parameters (first one wins) */
static void test_duplicate_diff_parameters(void)
{
    double result;

    /* First diff= is used; subsequent ones are ignored (strstr finds first occurrence) */
    result = parse_password_diff("diff=.1,diff=1", 0.001);
    assert_double_equal(result, 0.1, EPSILON_DIFF); /* First value (.1) used */

    result = parse_password_diff("diff=1,diff=.1", 0.001);
    assert_double_equal(result, 1.0, EPSILON_DIFF); /* First value (1) used */

    result = parse_password_diff("diff=200,other=5,diff=100", 1.0);
    assert_double_equal(result, 200.0, EPSILON_DIFF); /* First diff= (200) used */

    /* Duplicates with different delimiters */
    result = parse_password_diff("a,diff=50,b,diff=150,c", 1.0);
    assert_double_equal(result, 50.0, EPSILON_DIFF); /* First diff= (50) used */
}

/* Test: Leading dot and common formatting variations */
static void test_leading_dot_and_formatting(void)
{
    double result;

    /* Leading dot (valid strtod format) */
    result = parse_password_diff("diff=.001", 0.0001);
    assert_double_equal(result, 0.001, EPSILON_DIFF);

    result = parse_password_diff("diff=.1", 0.001);
    assert_double_equal(result, 0.1, EPSILON_DIFF);

    result = parse_password_diff("diff=.5", 0.1);
    assert_double_equal(result, 0.5, EPSILON_DIFF);

    result = parse_password_diff("diff=.999", 0.001);
    assert_double_equal(result, 0.999, EPSILON_DIFF);

    /* Trailing dot */
    result = parse_password_diff("diff=1.", 0.001);
    assert_double_equal(result, 1.0, EPSILON_DIFF);

    result = parse_password_diff("diff=100.", 1.0);
    assert_double_equal(result, 100.0, EPSILON_DIFF);

    /* Double dot (invalid - second dot is not valid delimiter) */
    result = parse_password_diff("diff=.1.", 0.001);
    assert_double_equal(result, 0.0, EPSILON_DIFF); /* Second dot is invalid delimiter */

    /* Comma-separated with leading dots */
    result = parse_password_diff("x=1,diff=.5,y=2", 0.001);
    assert_double_equal(result, 0.5, EPSILON_DIFF);
}

/* Test: Edge cases with spacing and special characters */
static void test_edge_case_spacing_and_chars(void)
{
    double result;

    /* Only leading/trailing whitespace trimmed, internal spaces fail */
    result = parse_password_diff("   diff=100   ", 1.0);
    assert_double_equal(result, 100.0, EPSILON_DIFF); /* Edges trimmed */

    result = parse_password_diff("\t\tdiff=200\t\t", 1.0);
    assert_double_equal(result, 200.0, EPSILON_DIFF); /* Tab edges trimmed */

    result = parse_password_diff("  diff=50,x=1  ", 1.0);
    assert_double_equal(result, 50.0, EPSILON_DIFF); /* Edges trimmed */

    /* Internal spaces in parameter structure (no commas to separate) */
    result = parse_password_diff("some diff=100", 0.001);
    assert_double_equal(result, 0.0, EPSILON_DIFF); /* Space breaks word boundary */

    result = parse_password_diff("user-diff=100", 0.001);
    assert_double_equal(result, 0.0, EPSILON_DIFF); /* Dash is not valid prefix */

    result = parse_password_diff("user_diff=100", 0.001);
    assert_double_equal(result, 0.0, EPSILON_DIFF); /* Underscore is not valid prefix */

    /* Valid prefixes only: start-of-string or comma */
    result = parse_password_diff("diff=100", 1.0);
    assert_double_equal(result, 100.0, EPSILON_DIFF); /* Start of string */

    result = parse_password_diff(",diff=100", 1.0);
    assert_double_equal(result, 100.0, EPSILON_DIFF); /* Comma prefix */

    result = parse_password_diff("x,diff=100", 1.0);
    assert_double_equal(result, 100.0, EPSILON_DIFF); /* Comma between params */
}

/* Test: Case sensitivity - diff= must be lowercase */
static void test_case_sensitivity(void)
{
    double result;

    /* Lowercase "diff=" works */
    result = parse_password_diff("diff=100", 1.0);
    assert_double_equal(result, 100.0, EPSILON_DIFF);

    /* Uppercase variants do NOT match (case-sensitive) */
    result = parse_password_diff("Diff=100", 1.0);
    assert_double_equal(result, 0.0, EPSILON_DIFF); /* Capital D doesn't match */

    result = parse_password_diff("DIFF=100", 1.0);
    assert_double_equal(result, 0.0, EPSILON_DIFF); /* All uppercase doesn't match */

    result = parse_password_diff("DiFF=100", 1.0);
    assert_double_equal(result, 0.0, EPSILON_DIFF); /* Mixed case doesn't match */

    result = parse_password_diff("dIFF=100", 1.0);
    assert_double_equal(result, 0.0, EPSILON_DIFF); /* Different mixed case doesn't match */

    /* Uppercase in comma-separated also doesn't match */
    result = parse_password_diff("x,Diff=50,y=2", 1.0);
    assert_double_equal(result, 0.0, EPSILON_DIFF); /* Capital D after comma */

    result = parse_password_diff("x,DIFF=50,y=2", 1.0);
    assert_double_equal(result, 0.0, EPSILON_DIFF); /* All caps after comma */

    /* But lowercase with leading dot still works */
    result = parse_password_diff("diff=.5", 0.001);
    assert_double_equal(result, 0.5, EPSILON_DIFF);

    result = parse_password_diff(" Diff=.5 ", 0.001);
    assert_double_equal(result, 0.0, EPSILON_DIFF); /* Capital D with spaces - still not matched */
}

/* Test: Leading plus sign and other numeric edge cases */
static void test_numeric_edge_cases(void)
{
    double result;

    /* Plus sign (valid in strtod) */
    result = parse_password_diff("diff=+100", 1.0);
    assert_double_equal(result, 100.0, EPSILON_DIFF);

    result = parse_password_diff("diff=+.5", 0.001);
    assert_double_equal(result, 0.5, EPSILON_DIFF);

    /* Leading zeros */
    result = parse_password_diff("diff=0001", 1.0);
    assert_double_equal(result, 1.0, EPSILON_DIFF);

    result = parse_password_diff("diff=0100", 1.0);
    assert_double_equal(result, 100.0, EPSILON_DIFF);

    result = parse_password_diff("diff=00.5", 0.001);
    assert_double_equal(result, 0.5, EPSILON_DIFF);

    /* Just a dot (invalid number) */
    result = parse_password_diff("diff=.", 1.0);
    assert_double_equal(result, 0.0, EPSILON_DIFF); /* . alone doesn't parse as valid number */

    /* Exponent with various formats */
    result = parse_password_diff("diff=1e+10", 1.0);
    assert_double_equal(result, 1e10, EPSILON_DIFF); /* e+ is valid */

    result = parse_password_diff("diff=1E10", 1.0);
    assert_double_equal(result, 1e10, EPSILON_DIFF); /* Capital E works */

    result = parse_password_diff("diff=1E+10", 1.0);
    assert_double_equal(result, 1e10, EPSILON_DIFF); /* Capital E with plus */

    /* Exponent edge cases */
    result = parse_password_diff("diff=1e0", 1.0);
    assert_double_equal(result, 1.0, EPSILON_DIFF); /* e0 is valid (1e0 = 1) */

    result = parse_password_diff("diff=100e-2", 1.0);
    assert_double_equal(result, 1.0, EPSILON_DIFF); /* 100e-2 = 1.0 */

    result = parse_password_diff("diff=.5e2", 1.0);
    assert_double_equal(result, 50.0, EPSILON_DIFF); /* .5e2 = 50 */
}

/* Test: Empty values and missing/malformed parameters */
static void test_malformed_parameters(void)
{
    double result;

    /* diff= with nothing after (empty value) */
    result = parse_password_diff("diff=", 1.0);
    assert_double_equal(result, 0.0, EPSILON_DIFF); /* Empty value fails parse */

    /* diff= followed by only whitespace (edge-trimmed away) */
    result = parse_password_diff("diff=   ", 1.0);
    assert_double_equal(result, 0.0, EPSILON_DIFF); /* Whitespace trimmed, empty value */

    result = parse_password_diff("diff=\t\t", 1.0);
    assert_double_equal(result, 0.0, EPSILON_DIFF); /* Tab-only trimmed away */

    /* diff= as last parameter with comma before */
    result = parse_password_diff("x=1,diff=", 1.0);
    assert_double_equal(result, 0.0, EPSILON_DIFF); /* Empty value */

    /* Multiple commas */
    result = parse_password_diff("diff=100,,x=5", 1.0);
    assert_double_equal(result, 100.0, EPSILON_DIFF); /* Double comma is fine, parsed before second comma */

    result = parse_password_diff("x=1,,diff=50", 1.0);
    assert_double_equal(result, 50.0, EPSILON_DIFF); /* Double comma before diff */

    /* Comma at exact end after value */
    result = parse_password_diff("diff=100,", 1.0);
    assert_double_equal(result, 100.0, EPSILON_DIFF); /* Trailing comma is valid delimiter */

    result = parse_password_diff("diff=100,  ", 1.0);
    assert_double_equal(result, 100.0, EPSILON_DIFF); /* Trailing comma then spaces (edges trimmed) */

    /* diff= with no equals becomes literal text search */
    result = parse_password_diff("diff100", 1.0);
    assert_double_equal(result, 0.0, EPSILON_DIFF); /* No equals sign, not matched */
}

/* Test: Password positioning and boundary cases */
static void test_password_positioning(void)
{
    double result;

    /* diff= at start */
    result = parse_password_diff("diff=100,x=5,y=10", 1.0);
    assert_double_equal(result, 100.0, EPSILON_DIFF);

    /* diff= in middle */
    result = parse_password_diff("a=1,diff=50,b=2", 1.0);
    assert_double_equal(result, 50.0, EPSILON_DIFF);

    /* diff= at end (no trailing comma) */
    result = parse_password_diff("x=1,y=2,diff=200", 1.0);
    assert_double_equal(result, 200.0, EPSILON_DIFF);

    /* diff= surrounded by similar-named params */
    result = parse_password_diff("difficulty=100,diff=25,differ=50", 1.0);
    assert_double_equal(result, 25.0, EPSILON_DIFF); /* Finds exact "diff=" */

    /* Very long password with diff in middle */
    result = parse_password_diff("aaaaaaaaaa,bbbbbbbbbb,cccccccccc,diff=75,dddddddddd,eeeeeeeeee", 1.0);
    assert_double_equal(result, 75.0, EPSILON_DIFF);
}

/* Test: Boundary values and limits */
static void test_boundary_values(void)
{
    double result;

    /* Exactly at mindiff */
    result = parse_password_diff("diff=0.001", 0.001);
    assert_double_equal(result, 0.001, EPSILON_DIFF); /* Exact match, no clamp */

    /* Just below mindiff */
    result = parse_password_diff("diff=0.0009", 0.001);
    assert_double_equal(result, 0.001, EPSILON_DIFF); /* Clamped to mindiff */

    /* Just above mindiff */
    result = parse_password_diff("diff=0.0011", 0.001);
    assert_double_equal(result, 0.0011, EPSILON_DIFF); /* No clamp needed */

    /* Very large number */
    result = parse_password_diff("diff=1000000000", 1.0);
    assert_double_equal(result, 1000000000.0, EPSILON_DIFF);

    /* Very large exponent */
    result = parse_password_diff("diff=1e100", 1.0);
    assert_double_equal(result, 1e100, EPSILON_DIFF); /* Huge but valid */

    /* Very small positive number (above mindiff) */
    result = parse_password_diff("diff=0.002", 0.001);
    assert_double_equal(result, 0.002, EPSILON_DIFF);

    /* Very small exponent (will be clamped) */
    result = parse_password_diff("diff=1e-10", 0.001);
    assert_double_equal(result, 0.001, EPSILON_DIFF); /* Clamped to mindiff */
}

/* Test: Whitespace variations and combinations */
static void test_whitespace_variations(void)
{
    double result;

    /* Mix of spaces and tabs in leading whitespace */
    result = parse_password_diff("  \t  diff=100  \t  ", 1.0);
    assert_double_equal(result, 100.0, EPSILON_DIFF); /* Mixed whitespace trimmed */

    /* Trailing comma with whitespace */
    result = parse_password_diff("diff=100,   ", 1.0);
    assert_double_equal(result, 100.0, EPSILON_DIFF); /* Trailing spaces after comma trimmed */

    /* Comma and space before diff */
    result = parse_password_diff("x=1, diff=50", 1.0);
    assert_double_equal(result, 0.0, EPSILON_DIFF); /* Space after comma is internal, not trimmed */

    /* Tab after comma before diff */
    result = parse_password_diff("x=1,\tdiff=50", 1.0);
    assert_double_equal(result, 0.0, EPSILON_DIFF); /* Tab after comma breaks it */

    /* Multiple spaces between comma and next param (no diff=) */
    result = parse_password_diff("x,  y,  z", 1.0);
    assert_double_equal(result, 0.0, EPSILON_DIFF); /* No diff= anywhere */
}

/* Test: diff= as part of larger words (should fail) */
static void test_substring_matching_failures(void)
{
    double result;

    /* diff= preceded by alphanumeric */
    result = parse_password_diff("worker_diff=100", 1.0);
    assert_double_equal(result, 0.0, EPSILON_DIFF); /* Underscore prefix */

    result = parse_password_diff("maindiff=100", 1.0);
    assert_double_equal(result, 0.0, EPSILON_DIFF); /* Letter prefix */

    result = parse_password_diff("1diff=100", 1.0);
    assert_double_equal(result, 0.0, EPSILON_DIFF); /* Number prefix */

    /* diff= as part of value in another param */
    result = parse_password_diff("name=user_diff=fake,x=1", 1.0);
    assert_double_equal(result, 0.0, EPSILON_DIFF);

    /* Actually, let's test what does match as word boundary */
    result = parse_password_diff(",worker,diff=200", 1.0);
    assert_double_equal(result, 200.0, EPSILON_DIFF); /* Comma before diff= is word boundary */
}

/* Test: Overlapping patterns and tricky positions */
static void test_overlapping_patterns(void)
{
    double result;

    /* Multiple diff appearances but only one is valid */
    result = parse_password_diff("difficulty=100,diff=50", 1.0);
    assert_double_equal(result, 50.0, EPSILON_DIFF);

    /* diff= inside quoted string (quote is not valid prefix, so rejected at prefix check) */
    result = parse_password_diff("comment=\"diff=999\",diff=25", 1.0);
    assert_double_equal(result, 0.0, EPSILON_DIFF);

    /* consecutive diff= parameters (no delimiter between them) */
    result = parse_password_diff("diff=100diff=200", 1.0);
    assert_double_equal(result, 0.0, EPSILON_DIFF); /* 'd' after 100 is invalid delimiter, not comma */

    /* Hyphenated words containing diff */
    result = parse_password_diff("worker-difficulty=100", 1.0);
    assert_double_equal(result, 0.0, EPSILON_DIFF); /* Dash prefix, not comma */

    result = parse_password_diff("my-diff=100", 1.0);
    assert_double_equal(result, 0.0, EPSILON_DIFF); /* Dash before diff */
}

/* Test: Interaction with existing suggest_diff (simulated) */
static void test_suggest_diff_interaction(void)
{
    double result;
    double mindiff = 0.001;

    /*
     * Simulate scenario where client previously had suggest_diff set.
     * Password parsing returns the new value; the actual stratifier.c code
     * would then overwrite client->suggest_diff if different.
     */

    /* Case 1: Password diff is different from existing suggest_diff */
    result = parse_password_diff("diff=10", mindiff);
    assert_double_equal(result, 10.0, EPSILON_DIFF); /* New value would overwrite */

    /* Case 2: Password diff same as existing suggest_diff (would skip update) */
    result = parse_password_diff("diff=5.0", mindiff);
    assert_double_equal(result, 5.0, EPSILON_DIFF);

    /* Case 3: No password diff (returns 0, wouldn't overwrite) */
    result = parse_password_diff("x", mindiff);
    assert_double_equal(result, 0.0, EPSILON_DIFF); /* No diff= found */

    /* Case 4: Invalid password diff (returns 0, wouldn't overwrite) */
    result = parse_password_diff("xdiff=10", mindiff);
    assert_double_equal(result, 0.0, EPSILON_DIFF); /* Invalid word boundary */

    /*
     * Real-world behavior in stratifier.c parse_authorise():
     * 
     * if (pass_diff > 0) {
     *     if (sdiff == client->suggest_diff) goto skip;  // Skip if same
     *     client->suggest_diff = sdiff;                   // Overwrite if different
     *     ...
     * }
     * 
     * This means:
     * - Valid password diff (>0) and different → overwrites suggest_diff
     * - Valid password diff (>0) and same → no-op (skips update)
     * - No password diff (0) → suggest_diff unchanged
     */
}

int main(void)
{
    printf("Running password difficulty parsing tests...\n\n");

    run_test(test_parse_simple_diff);
    run_test(test_parse_comma_separated);
    run_test(test_reject_space_after_equals);
    run_test(test_reject_invalid_chars);
    run_test(test_word_boundary_enforcement);
    run_test(test_accept_valid_delimiters);
    run_test(test_clamp_to_mindiff);
    run_test(test_no_diff_returns_zero);
    run_test(test_null_empty_password);
    run_test(test_reject_negative_values);
    run_test(test_reject_zero_value);
    run_test(test_reject_special_fp_values);
    run_test(test_scientific_notation);
    run_test(test_duplicate_diff_parameters);
    run_test(test_leading_dot_and_formatting);
    run_test(test_edge_case_spacing_and_chars);
    run_test(test_case_sensitivity);
    run_test(test_numeric_edge_cases);
    run_test(test_malformed_parameters);
    run_test(test_password_positioning);
    run_test(test_boundary_values);
    run_test(test_whitespace_variations);
    run_test(test_substring_matching_failures);
    run_test(test_overlapping_patterns);
    run_test(test_suggest_diff_interaction);

    printf("\nAll password difficulty parsing tests passed!\n");
    return 0;
}
