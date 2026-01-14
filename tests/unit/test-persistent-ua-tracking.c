#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include "uthash.h"

#define UA_OTHER "Other"

/* UA normalization (mirror of ua_utils.c) */
static void normalize_ua_buf(const char *src, char *dst, int len)
{
	int i = 0;
	if (!src || !dst || len <= 0) {
		if (dst && len > 0)
			dst[0] = '\0';
		return;
	}
	/* Skip leading whitespace */
	while (*src && isspace((unsigned char)*src)) src++;

	while (*src && i < len - 1) {
		if (*src == '/' || *src == '(')
			break;
		dst[i++] = (unsigned char)*src;
		src++;
	}
	dst[i] = '\0';
	
	/* Strip trailing whitespace */
	while (i > 0 && isspace((unsigned char)dst[i - 1])) {
		i--;
		dst[i] = '\0';
	}
}

/* UA item struct matching stratifier.c definition */
typedef struct ua_item {
	UT_hash_handle hh;
	char *ua;
	int count;
	double dsps5;
	double best_diff;
} ua_item_t;

/* Helper: Add or increment UA in the persistent map (with normalization) */
static void ua_tracking_add_client(ua_item_t **ua_map, const char *useragent)
{
	if (!useragent || !useragent[0])
		return;

	/* Normalize UA string */
	char normalized_ua[256];
	normalize_ua_buf(useragent, normalized_ua, sizeof(normalized_ua));
	/* Use "Other" for empty normalized UA (e.g., whitespace-only input) */
	const char *ua_key = normalized_ua[0] != '\0' ? normalized_ua : UA_OTHER;

	ua_item_t *ua_it_find = NULL;
	HASH_FIND_STR(*ua_map, ua_key, ua_it_find);
	if (ua_it_find) {
		ua_it_find->count++;
	} else {
		ua_item_t *ua_new = malloc(sizeof(ua_item_t));
		assert_non_null(ua_new);
		ua_new->ua = malloc(strlen(ua_key) + 1);
		assert_non_null(ua_new->ua);
		memcpy(ua_new->ua, ua_key, strlen(ua_key) + 1);
		ua_new->count = 1;
		ua_new->dsps5 = 0;
		ua_new->best_diff = 0;
		HASH_ADD_STR(*ua_map, ua, ua_new);
	}
}

/* Helper: Decrement UA count and cleanup if reaches 0 */
static void ua_tracking_remove_client(ua_item_t **ua_map, const char *useragent)
{
	if (!useragent || !useragent[0])
		return;

	/* Normalize UA to match what was added */
	char normalized_ua[256];
	normalize_ua_buf(useragent, normalized_ua, sizeof(normalized_ua));
	/* Use "Other" for empty normalized UA (e.g., whitespace-only input) */
	const char *ua_key = normalized_ua[0] != '\0' ? normalized_ua : UA_OTHER;

	ua_item_t *ua_it_find = NULL;
	HASH_FIND_STR(*ua_map, ua_key, ua_it_find);
	if (ua_it_find) {
		ua_it_find->count--;
		if (ua_it_find->count <= 0) {
			HASH_DEL(*ua_map, ua_it_find);
			free(ua_it_find->ua);
			free(ua_it_find);
		}
	}
}

/* Helper: Get count for a UA (with normalization) */
static int ua_tracking_get_count(ua_item_t *ua_map, const char *useragent)
{
	if (!useragent || !useragent[0])
		return 0;
	
	/* Normalize UA to match what was added */
	char normalized_ua[256];
	normalize_ua_buf(useragent, normalized_ua, sizeof(normalized_ua));
	/* Use "Other" for empty normalized UA (e.g., whitespace-only input) */
	const char *ua_key = normalized_ua[0] != '\0' ? normalized_ua : UA_OTHER;
	
	ua_item_t *ua_it = NULL;
	HASH_FIND_STR(ua_map, ua_key, ua_it);
	return ua_it ? ua_it->count : 0;
}

/* Helper: Cleanup entire map */
static void ua_tracking_cleanup(ua_item_t **ua_map)
{
	ua_item_t *ua_it, *ua_tmp;
	HASH_ITER(hh, *ua_map, ua_it, ua_tmp) {
		HASH_DEL(*ua_map, ua_it);
		free(ua_it->ua);
		free(ua_it);
	}
}

/* Test: Single client subscribe and unsubscribe */
static void test_single_client_lifecycle(void **state)
{
	ua_item_t *ua_map = NULL;

	/* Subscribe */
	ua_tracking_add_client(&ua_map, "Antminer S21");
	assert_int_equal(ua_tracking_get_count(ua_map, "Antminer S21"), 1);

	/* Unsubscribe */
	ua_tracking_remove_client(&ua_map, "Antminer S21");
	assert_int_equal(ua_tracking_get_count(ua_map, "Antminer S21"), 0);
	assert_null(ua_map);

	ua_tracking_cleanup(&ua_map);
}

/* Test: Multiple clients with same UA */
static void test_multiple_same_ua(void **state)
{
	ua_item_t *ua_map = NULL;

	/* 3 clients with same UA */
	ua_tracking_add_client(&ua_map, "cpuminer-multi/1.3.7");
	assert_int_equal(ua_tracking_get_count(ua_map, "cpuminer-multi/1.3.7"), 1);

	ua_tracking_add_client(&ua_map, "cpuminer-multi/1.3.7");
	assert_int_equal(ua_tracking_get_count(ua_map, "cpuminer-multi/1.3.7"), 2);

	ua_tracking_add_client(&ua_map, "cpuminer-multi/1.3.7");
	assert_int_equal(ua_tracking_get_count(ua_map, "cpuminer-multi/1.3.7"), 3);

	/* Disconnect one */
	ua_tracking_remove_client(&ua_map, "cpuminer-multi/1.3.7");
	assert_int_equal(ua_tracking_get_count(ua_map, "cpuminer-multi/1.3.7"), 2);

	/* Disconnect remaining */
	ua_tracking_remove_client(&ua_map, "cpuminer-multi/1.3.7");
	assert_int_equal(ua_tracking_get_count(ua_map, "cpuminer-multi/1.3.7"), 1);

	ua_tracking_remove_client(&ua_map, "cpuminer-multi/1.3.7");
	assert_int_equal(ua_tracking_get_count(ua_map, "cpuminer-multi/1.3.7"), 0);
	assert_null(ua_map);

	ua_tracking_cleanup(&ua_map);
}

/* Test: Multiple different UAs tracked independently */
static void test_multiple_different_uas(void **state)
{
	ua_item_t *ua_map = NULL;

	/* Add different UAs */
	ua_tracking_add_client(&ua_map, "Antminer S19 XP");
	ua_tracking_add_client(&ua_map, "Whatsminer M30S++");
	ua_tracking_add_client(&ua_map, "Antminer S21");

	/* Verify independent counts */
	assert_int_equal(ua_tracking_get_count(ua_map, "Antminer S19 XP"), 1);
	assert_int_equal(ua_tracking_get_count(ua_map, "Whatsminer M30S++"), 1);
	assert_int_equal(ua_tracking_get_count(ua_map, "Antminer S21"), 1);

	/* Add more of one type */
	ua_tracking_add_client(&ua_map, "Antminer S19 XP");
	assert_int_equal(ua_tracking_get_count(ua_map, "Antminer S19 XP"), 2);
	assert_int_equal(ua_tracking_get_count(ua_map, "Whatsminer M30S++"), 1);

	/* Remove one */
	ua_tracking_remove_client(&ua_map, "Whatsminer M30S++");
	assert_int_equal(ua_tracking_get_count(ua_map, "Whatsminer M30S++"), 0);
	assert_int_equal(ua_tracking_get_count(ua_map, "Antminer S19 XP"), 2);
	assert_int_equal(ua_tracking_get_count(ua_map, "Antminer S21"), 1);

	ua_tracking_cleanup(&ua_map);
}

/* Test: NULL and empty UA handling */
static void test_null_empty_ua(void **state)
{
	ua_item_t *ua_map = NULL;

	/* NULL should not be added */
	ua_tracking_add_client(&ua_map, NULL);
	assert_null(ua_map);

	/* Empty string should not be added */
	ua_tracking_add_client(&ua_map, "");
	assert_null(ua_map);

	/* Remove from empty map should not crash */
	ua_tracking_remove_client(&ua_map, "nonexistent");
	assert_null(ua_map);

	/* Remove NULL should not crash */
	ua_tracking_remove_client(&ua_map, NULL);
	assert_null(ua_map);

	ua_tracking_cleanup(&ua_map);
}

/* Test: Cascade multiple adds and removes */
static void test_cascade_operations(void **state)
{
	ua_item_t *ua_map = NULL;

	/* Add pattern */
	ua_tracking_add_client(&ua_map, "Type A");
	ua_tracking_add_client(&ua_map, "Type A");
	ua_tracking_add_client(&ua_map, "Type B");
	ua_tracking_add_client(&ua_map, "Type C");
	ua_tracking_add_client(&ua_map, "Type C");
	ua_tracking_add_client(&ua_map, "Type C");

	assert_int_equal(ua_tracking_get_count(ua_map, "Type A"), 2);
	assert_int_equal(ua_tracking_get_count(ua_map, "Type B"), 1);
	assert_int_equal(ua_tracking_get_count(ua_map, "Type C"), 3);

	/* Remove all of Type B (should auto-cleanup) */
	ua_tracking_remove_client(&ua_map, "Type B");
	assert_int_equal(ua_tracking_get_count(ua_map, "Type B"), 0);

	/* Verify others unchanged */
	assert_int_equal(ua_tracking_get_count(ua_map, "Type A"), 2);
	assert_int_equal(ua_tracking_get_count(ua_map, "Type C"), 3);

	/* Remove all of Type A */
	ua_tracking_remove_client(&ua_map, "Type A");
	ua_tracking_remove_client(&ua_map, "Type A");
	assert_int_equal(ua_tracking_get_count(ua_map, "Type A"), 0);

	/* Type C still there */
	assert_int_equal(ua_tracking_get_count(ua_map, "Type C"), 3);

	/* Remove all of Type C */
	ua_tracking_remove_client(&ua_map, "Type C");
	ua_tracking_remove_client(&ua_map, "Type C");
	ua_tracking_remove_client(&ua_map, "Type C");
	assert_null(ua_map);

	ua_tracking_cleanup(&ua_map);
}

/* Test: Re-add after complete removal */
static void test_readd_after_removal(void **state)
{
	ua_item_t *ua_map = NULL;

	/* Add and remove */
	ua_tracking_add_client(&ua_map, "Type A");
	assert_int_equal(ua_tracking_get_count(ua_map, "Type A"), 1);

	ua_tracking_remove_client(&ua_map, "Type A");
	assert_null(ua_map);

	/* Re-add should work */
	ua_tracking_add_client(&ua_map, "Type A");
	assert_int_equal(ua_tracking_get_count(ua_map, "Type A"), 1);

	ua_tracking_cleanup(&ua_map);
}

/* Test: UA normalization - different versions counted as same UA */
static void test_ua_normalization(void **state)
{
	ua_item_t *ua_map = NULL;

	/* Add clients with same UA but different version suffixes */
	ua_tracking_add_client(&ua_map, "cpuminer-multi/1.3.7");
	assert_int_equal(ua_tracking_get_count(ua_map, "cpuminer-multi/1.3.7"), 1);

	/* Same UA with different version - should be counted as same */
	ua_tracking_add_client(&ua_map, "cpuminer-multi/1.3.8");
	assert_int_equal(ua_tracking_get_count(ua_map, "cpuminer-multi/1.3.7"), 2);
	assert_int_equal(ua_tracking_get_count(ua_map, "cpuminer-multi/1.3.8"), 2);

	/* Another version */
	ua_tracking_add_client(&ua_map, "cpuminer-multi/1.4.0");
	assert_int_equal(ua_tracking_get_count(ua_map, "cpuminer-multi/1.3.7"), 3);

	/* With parenthesis also should normalize to same */
	ua_tracking_add_client(&ua_map, "cpuminer-multi (custom build)");
	assert_int_equal(ua_tracking_get_count(ua_map, "cpuminer-multi"), 4);

	/* Remove one - should decrement shared counter */
	ua_tracking_remove_client(&ua_map, "cpuminer-multi/1.3.7");
	assert_int_equal(ua_tracking_get_count(ua_map, "cpuminer-multi/1.3.8"), 3);

	ua_tracking_cleanup(&ua_map);
}

/* Test: UA special characters like ++ are preserved */
static void test_ua_special_chars(void **state)
{
	ua_item_t *ua_map = NULL;

	/* Whatsminer with ++ should be preserved */
	ua_tracking_add_client(&ua_map, "Whatsminer M30S++");
	assert_int_equal(ua_tracking_get_count(ua_map, "Whatsminer M30S++"), 1);

	/* Different variant with version should normalize to same */
	ua_tracking_add_client(&ua_map, "Whatsminer M30S++ (v1.0)");
	assert_int_equal(ua_tracking_get_count(ua_map, "Whatsminer M30S++"), 2);

	/* NerdQAxe++ should also be preserved */
	ua_tracking_add_client(&ua_map, "NerdQAxe++");
	assert_int_equal(ua_tracking_get_count(ua_map, "NerdQAxe++"), 1);

	/* Different model should NOT match */
	ua_tracking_add_client(&ua_map, "NerdQAxe");
	assert_int_equal(ua_tracking_get_count(ua_map, "NerdQAxe"), 1);
	assert_int_equal(ua_tracking_get_count(ua_map, "NerdQAxe++"), 1);

	ua_tracking_cleanup(&ua_map);
}

/* Test: Whitespace-only UA normalizes to "Other" */
static void test_whitespace_ua_falls_back_to_other(void **state)
{
	ua_item_t *ua_map = NULL;

	/* Whitespace-only should become "Other" */
	ua_tracking_add_client(&ua_map, "   ");
	
	/* Debug: Check count */
	int count_val = ua_tracking_get_count(ua_map, "Other");
	/* Print to see what's happening (use assert_int_equal to display value) */
	assert_int_equal(count_val, 1);

	/* Add more whitespace UAs */
	ua_tracking_add_client(&ua_map, "\t\t");
	assert_int_equal(ua_tracking_get_count(ua_map, "Other"), 2);

	/* Add normal UA for comparison */
	ua_tracking_add_client(&ua_map, "cpuminer-multi");
	assert_int_equal(ua_tracking_get_count(ua_map, "cpuminer-multi"), 1);
	assert_int_equal(ua_tracking_get_count(ua_map, "Other"), 2);

	/* Remove one whitespace UA */
	ua_tracking_remove_client(&ua_map, "   ");
	assert_int_equal(ua_tracking_get_count(ua_map, "Other"), 1);

	/* Remove the last whitespace UA (should cleanup "Other") */
	ua_tracking_remove_client(&ua_map, "\t\t");
	assert_int_equal(ua_tracking_get_count(ua_map, "Other"), 0);
	assert_int_equal(ua_tracking_get_count(ua_map, "cpuminer-multi"), 1);

	ua_tracking_cleanup(&ua_map);
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_single_client_lifecycle),
		cmocka_unit_test(test_multiple_same_ua),
		cmocka_unit_test(test_multiple_different_uas),
		cmocka_unit_test(test_null_empty_ua),
		cmocka_unit_test(test_cascade_operations),
		cmocka_unit_test(test_readd_after_removal),
		cmocka_unit_test(test_ua_normalization),
		cmocka_unit_test(test_ua_special_chars),
		cmocka_unit_test(test_whitespace_ua_falls_back_to_other),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
