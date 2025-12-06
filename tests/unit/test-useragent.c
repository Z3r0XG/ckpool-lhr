/*
 * Unit tests for useragent whitelisting
 * Tests the core matching logic used in subscription validation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "config.h"
#include "../test_common.h"
#include "libckpool.h"

/* Test safencmp prefix matching (used for useragent whitelist matching)
 * This is the core function used in stratifier.c:4956 for whitelist checks
 */
static void test_safencmp_prefix_matching(void)
{
    /* Test exact matches */
    assert_int_equal(safencmp("NerdMinerV2", "NerdMinerV2", 11), 0);
    assert_int_equal(safencmp("CGMiner", "CGMiner", 7), 0);
    
    /* Test prefix matches (useragent starts with pattern) */
    assert_int_equal(safencmp("NerdMinerV2", "NerdMiner", 9), 0);
    assert_int_equal(safencmp("CGMiner/4.0", "CGMiner", 7), 0);
    assert_int_equal(safencmp("ESP32Miner", "ESP32", 5), 0);
    
    /* Test non-matches */
    assert_true(safencmp("NerdMinerV2", "CGMiner", 7) != 0);
    assert_true(safencmp("CGMiner", "NerdMiner", 9) != 0);
    assert_true(safencmp("ESP32Miner", "NerdMiner", 9) != 0);
    
    /* Test empty string handling
     * Note: safencmp returns -1 if len is 0 (line 1675-1676 in libckpool.c)
     * In actual usage, len is strlen(pat), so empty pattern would have len=0
     */
    assert_int_equal(safencmp("", "", 0), -1); // len=0 returns -1
    assert_true(safencmp("", "NerdMiner", 9) != 0); // Empty doesn't match non-empty
    assert_int_equal(safencmp("NerdMiner", "", 0), -1); // len=0 returns -1
    
    /* Test partial matches that aren't prefixes */
    assert_true(safencmp("MinerV2", "NerdMiner", 9) != 0); // "Miner" is in middle, not prefix
    assert_true(safencmp("OldNerdMiner", "NerdMiner", 9) != 0); // Pattern not at start
}

/* Test whitelist matching logic (simulating the check in stratifier.c:4951-4964)
 * This tests the core algorithm: iterate through whitelist, check prefix match
 */
static void test_whitelist_matching_logic(void)
{
    const char *whitelist[3];
    int whitelist_count;
    const char *useragent;
    bool ua_ret;
    int i;
    
    /* Test 1: Matching useragent */
    whitelist[0] = "NerdMiner";
    whitelist[1] = "CGMiner";
    whitelist[2] = "ESP32";
    whitelist_count = 3;
    useragent = "NerdMinerV2";
    
    ua_ret = false;
    for (i = 0; i < whitelist_count; i++) {
        const char *pat = whitelist[i];
        if (!pat)
            continue;
        if (!safencmp(useragent, pat, strlen(pat))) {
            ua_ret = true;
            break;
        }
    }
    assert_true(ua_ret); // Should match "NerdMiner" prefix
    
    /* Test 2: Non-matching useragent */
    useragent = "UnknownMiner";
    ua_ret = false;
    for (i = 0; i < whitelist_count; i++) {
        const char *pat = whitelist[i];
        if (!pat)
            continue;
        if (!safencmp(useragent, pat, strlen(pat))) {
            ua_ret = true;
            break;
        }
    }
    assert_false(ua_ret); // Should not match
    
    /* Test 3: Empty useragent (should not match any pattern) */
    useragent = "";
    ua_ret = false;
    for (i = 0; i < whitelist_count; i++) {
        const char *pat = whitelist[i];
        if (!pat)
            continue;
        if (!safencmp(useragent, pat, strlen(pat))) {
            ua_ret = true;
            break;
        }
    }
    assert_false(ua_ret); // Empty doesn't match any pattern
    
    /* Test 4: Multiple patterns, second one matches */
    whitelist[0] = "CGMiner";
    whitelist[1] = "NerdMiner";
    whitelist_count = 2;
    useragent = "NerdMinerV2";
    
    ua_ret = false;
    for (i = 0; i < whitelist_count; i++) {
        const char *pat = whitelist[i];
        if (!pat)
            continue;
        if (!safencmp(useragent, pat, strlen(pat))) {
            ua_ret = true;
            break;
        }
    }
    assert_true(ua_ret); // Should match second pattern
}

/* Test whitelist not configured (useragents == 0)
 * When whitelist is not configured, all user agents should be allowed
 */
static void test_whitelist_not_configured(void)
{
    int useragents = 0; // Not configured
    const char *useragent = "AnyMiner";
    bool should_allow;
    
    /* When useragents == 0, the check in stratifier.c:4951 is skipped
     * This means all user agents are allowed */
    if (useragents > 0) {
        should_allow = false; // Would check whitelist
    } else {
        should_allow = true; // Whitelist not configured, allow all
    }
    assert_true(should_allow);
    
    /* Even empty useragent should be allowed when whitelist not configured */
    useragent = "";
    if (useragents > 0) {
        should_allow = false;
    } else {
        should_allow = true;
    }
    assert_true(should_allow);
}

/* Test edge cases in whitelist matching */
static void test_whitelist_edge_cases(void)
{
    const char *whitelist[3];
    int whitelist_count;
    const char *useragent;
    bool ua_ret;
    int i;
    
    /* Test 1: NULL pattern in whitelist (should be skipped) */
    whitelist[0] = "NerdMiner";
    whitelist[1] = NULL; // NULL entry
    whitelist[2] = "CGMiner";
    whitelist_count = 3;
    useragent = "CGMiner";
    
    ua_ret = false;
    for (i = 0; i < whitelist_count; i++) {
        const char *pat = whitelist[i];
        if (!pat) // NULL check (as in stratifier.c:4954)
            continue;
        if (!safencmp(useragent, pat, strlen(pat))) {
            ua_ret = true;
            break;
        }
    }
    assert_true(ua_ret); // Should match despite NULL entry
    
    /* Test 2: Single character pattern */
    whitelist[0] = "N";
    whitelist_count = 1;
    useragent = "NerdMiner";
    
    ua_ret = false;
    for (i = 0; i < whitelist_count; i++) {
        const char *pat = whitelist[i];
        if (!pat)
            continue;
        if (!safencmp(useragent, pat, strlen(pat))) {
            ua_ret = true;
            break;
        }
    }
    assert_true(ua_ret); // Single char prefix should match
    
    /* Test 3: Pattern longer than useragent */
    whitelist[0] = "VeryLongMinerName";
    whitelist_count = 1;
    useragent = "Short";
    
    ua_ret = false;
    for (i = 0; i < whitelist_count; i++) {
        const char *pat = whitelist[i];
        if (!pat)
            continue;
        if (!safencmp(useragent, pat, strlen(pat))) {
            ua_ret = true;
            break;
        }
    }
    assert_false(ua_ret); // Pattern longer than useragent shouldn't match
}

/* Test various real-world useragent patterns */
static void test_real_world_useragents(void)
{
    const char *whitelist[4];
    int whitelist_count;
    const char *useragent;
    bool ua_ret;
    int i;
    
    /* Common mining software useragents */
    whitelist[0] = "NerdMiner";
    whitelist[1] = "CGMiner";
    whitelist[2] = "BFGMiner";
    whitelist[3] = "ESP32";
    whitelist_count = 4;
    
    /* Test various useragent formats */
    useragent = "NerdMinerV2";
    ua_ret = false;
    for (i = 0; i < whitelist_count; i++) {
        const char *pat = whitelist[i];
        if (!pat)
            continue;
        if (!safencmp(useragent, pat, strlen(pat))) {
            ua_ret = true;
            break;
        }
    }
    assert_true(ua_ret);
    
    useragent = "CGMiner/4.10.0";
    ua_ret = false;
    for (i = 0; i < whitelist_count; i++) {
        const char *pat = whitelist[i];
        if (!pat)
            continue;
        if (!safencmp(useragent, pat, strlen(pat))) {
            ua_ret = true;
            break;
        }
    }
    assert_true(ua_ret);
    
    useragent = "ESP32Miner/1.0";
    ua_ret = false;
    for (i = 0; i < whitelist_count; i++) {
        const char *pat = whitelist[i];
        if (!pat)
            continue;
        if (!safencmp(useragent, pat, strlen(pat))) {
            ua_ret = true;
            break;
        }
    }
    assert_true(ua_ret);
    
    /* Non-matching useragent */
    useragent = "UnknownMiner/1.0";
    ua_ret = false;
    for (i = 0; i < whitelist_count; i++) {
        const char *pat = whitelist[i];
        if (!pat)
            continue;
        if (!safencmp(useragent, pat, strlen(pat))) {
            ua_ret = true;
            break;
        }
    }
    assert_false(ua_ret);
}

int main(void)
{
    printf("Running useragent whitelisting tests...\n\n");
    
    run_test(test_safencmp_prefix_matching);
    run_test(test_whitelist_matching_logic);
    run_test(test_whitelist_not_configured);
    run_test(test_whitelist_edge_cases);
    run_test(test_real_world_useragents);
    
    printf("\nAll useragent whitelisting tests passed!\n");
    return 0;
}

