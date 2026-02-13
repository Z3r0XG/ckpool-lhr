/*
 * Unit tests for fulltest() - Hash vs Target validation
 * This is CRITICAL for share validation
 */

/* config.h must be first to define _GNU_SOURCE before system headers */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "../test_common.h"
#include "libckpool.h"

/* Test fulltest() - validates if hash meets target
 * Returns true if hash <= target (valid share)
 * Returns false if hash > target (invalid share)
 */
static void test_fulltest_hash_less_than_target(void)
{
    uchar hash[32] = {0};
    uchar target[32] = {0};
    
    /* Test 1: Hash all zeros, target all zeros (equal, should pass) */
    memset(hash, 0, 32);
    memset(target, 0, 32);
    assert_true(fulltest(hash, target)); // Equal should pass
    
    /* Test 2: Hash less than target (should pass) */
    memset(hash, 0, 32);
    memset(target, 0, 32);
    target[31] = 0x01; // Target is slightly larger
    assert_true(fulltest(hash, target)); // Hash < target, should pass
    
    /* Test 3: Hash much less than target */
    memset(hash, 0, 32);
    memset(target, 0xFF, 32); // Target is maximum
    assert_true(fulltest(hash, target)); // Hash << target, should pass
}

static void test_fulltest_hash_greater_than_target(void)
{
    uchar hash[32] = {0};
    uchar target[32] = {0};
    
    /* Test 1: Hash greater than target (should fail) */
    memset(hash, 0, 32);
    memset(target, 0, 32);
    hash[31] = 0x01; // Hash is slightly larger
    assert_false(fulltest(hash, target)); // Hash > target, should fail
    
    /* Test 2: Hash much greater than target */
    memset(hash, 0xFF, 32); // Hash is maximum
    memset(target, 0, 32);
    assert_false(fulltest(hash, target)); // Hash >> target, should fail
    
    /* Test 3: Hash greater in middle bytes */
    memset(hash, 0, 32);
    memset(target, 0, 32);
    hash[16] = 0x01; // Hash has value in middle
    target[16] = 0x00; // Target is zero at same position
    assert_false(fulltest(hash, target)); // Hash > target, should fail
}

static void test_fulltest_hash_equal_target(void)
{
    uchar hash[32] = {0};
    uchar target[32] = {0};
    
    /* Test: Hash exactly equals target (should pass) */
    memset(hash, 0x42, 32);
    memset(target, 0x42, 32);
    assert_true(fulltest(hash, target)); // Equal should pass
    
    /* Test: Hash equals target with specific pattern */
    memset(hash, 0, 32);
    memset(target, 0, 32);
    hash[0] = 0x12;
    hash[31] = 0x34;
    target[0] = 0x12;
    target[31] = 0x34;
    assert_true(fulltest(hash, target)); // Equal should pass
}

static void test_fulltest_little_endian_handling(void)
{
    uchar hash[32] = {0};
    uchar target[32] = {0};
    
    /* fulltest() uses le32toh() to handle little-endian conversion
     * Test that it correctly compares 32-bit words in little-endian order
     */
    
    /* Test: Hash and target differ in first word (most significant) */
    memset(hash, 0, 32);
    memset(target, 0, 32);
    /* Set first 32-bit word (little-endian, bytes 0-3) */
    hash[3] = 0x01; // Hash first word = 0x01000000
    target[3] = 0x02; // Target first word = 0x02000000
    assert_true(fulltest(hash, target)); // Hash < target, should pass
    
    /* Test: Hash and target differ in last word (least significant) */
    memset(hash, 0, 32);
    memset(target, 0, 32);
    hash[28] = 0x01; // Hash last word (bytes 28-31)
    target[28] = 0x02; // Target last word
    assert_true(fulltest(hash, target)); // Hash < target, should pass
    
    /* Test: Hash greater in first word */
    memset(hash, 0, 32);
    memset(target, 0, 32);
    hash[3] = 0x02; // Hash first word = 0x02000000
    target[3] = 0x01; // Target first word = 0x01000000
    assert_false(fulltest(hash, target)); // Hash > target, should fail
}

static void test_fulltest_edge_cases(void)
{
    uchar hash[32] = {0};
    uchar target[32] = {0};
    
    /* Test 1: All zeros (minimum values) */
    memset(hash, 0, 32);
    memset(target, 0, 32);
    assert_true(fulltest(hash, target)); // Both zero, should pass
    
    /* Test 2: All 0xFF (maximum values) */
    memset(hash, 0xFF, 32);
    memset(target, 0xFF, 32);
    assert_true(fulltest(hash, target)); // Both max, should pass
    
    /* Test 3: Hash all zeros, target all 0xFF */
    memset(hash, 0, 32);
    memset(target, 0xFF, 32);
    assert_true(fulltest(hash, target)); // Hash << target, should pass
    
    /* Test 4: Hash all 0xFF, target all zeros */
    memset(hash, 0xFF, 32);
    memset(target, 0, 32);
    assert_false(fulltest(hash, target)); // Hash >> target, should fail
    
    /* Test 5: Single byte difference at various positions */
    for (int i = 0; i < 32; i++) {
        memset(hash, 0, 32);
        memset(target, 0, 32);
        hash[i] = 0x01;
        assert_false(fulltest(hash, target)); // Hash > target, should fail
        
        memset(hash, 0, 32);
        memset(target, 0, 32);
        target[i] = 0x01;
        assert_true(fulltest(hash, target)); // Hash < target, should pass
    }
}

static void test_fulltest_with_difficulty_targets(void)
{
    uchar hash[32] = {0};
    uchar target[32] = {0};
    double diff;
    
    /* Test with actual difficulty targets
     * Generate targets for various difficulties and test hashes
     */
    
    /* Test 1: Difficulty 1.0 - target should be truediffone */
    target_from_diff(target, 1.0);
    memset(hash, 0, 32);
    assert_true(fulltest(hash, target)); // Hash (all zeros) < target, should pass
    
    /* Test 2: Difficulty 0.5 (sub-1 difficulty) */
    target_from_diff(target, 0.5);
    memset(hash, 0, 32);
    assert_true(fulltest(hash, target)); // Hash < target, should pass
    
    /* Test 3: Difficulty 0.001 (very low difficulty) */
    target_from_diff(target, 0.001);
    memset(hash, 0, 32);
    assert_true(fulltest(hash, target)); // Hash < target, should pass
    
    /* Test 4: Create a hash that's just below target */
    target_from_diff(target, 1.0);
    memset(hash, 0, 32);
    /* Copy target to hash, then make hash slightly smaller */
    memcpy(hash, target, 32);
    if (hash[31] > 0) {
        hash[31]--; // Make hash slightly smaller
        assert_true(fulltest(hash, target)); // Hash < target, should pass
    }
    
    /* Test 5: Create a hash that's just above target */
    target_from_diff(target, 1.0);
    memset(hash, 0, 32);
    memcpy(hash, target, 32);
    hash[31]++; // Make hash slightly larger
    assert_false(fulltest(hash, target)); // Hash > target, should fail
}

int main(void)
{
    printf("Running fulltest() tests...\n\n");
    
    run_test(test_fulltest_hash_less_than_target);
    run_test(test_fulltest_hash_greater_than_target);
    run_test(test_fulltest_hash_equal_target);
    run_test(test_fulltest_little_endian_handling);
    run_test(test_fulltest_edge_cases);
    run_test(test_fulltest_with_difficulty_targets);
    
    printf("\nAll fulltest() tests passed!\n");
    return 0;
}

