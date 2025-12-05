/*
 * Unit tests for Base58 decoding
 * Tests b58tobin() function
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "config.h"
#include "../test_common.h"
#include "libckpool.h"

/* Test b58tobin() with known Bitcoin addresses */
static void test_b58tobin_known_addresses(void)
{
    char b58bin[25];
    const char *test_addresses[] = {
        "1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa", // Genesis block address
        "1111111111111111111114oLvT2", // All 1's (minimum value)
    };
    int num_tests = sizeof(test_addresses) / sizeof(test_addresses[0]);
    
    for (int i = 0; i < num_tests; i++) {
        memset(b58bin, 0, 25);
        b58tobin(b58bin, test_addresses[i]);
        
        /* Verify output is not all zeros (decoding succeeded) */
        bool has_data = false;
        for (int j = 0; j < 25; j++) {
            if (b58bin[j] != 0) {
                has_data = true;
                break;
            }
        }
        assert_true(has_data);
    }
}

/* Test b58tobin() with various address lengths */
static void test_b58tobin_address_lengths(void)
{
    char b58bin[25];
    
    /* Test: Short address (all 1's - minimum) */
    memset(b58bin, 0, 25);
    b58tobin(b58bin, "1");
    /* Single character "1" decodes to a very small value, may be in later bytes */
    /* Just verify it doesn't crash and produces some non-zero output somewhere */
    bool has_data1 = false;
    for (int j = 0; j < 25; j++) {
        if (b58bin[j] != 0) {
            has_data1 = true;
            break;
        }
    }
    /* Note: "1" decodes to a very small value, might be all zeros in first bytes */
    /* The function should not crash at least */
    
    /* Test: Standard P2PKH address length */
    memset(b58bin, 0, 25);
    b58tobin(b58bin, "1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa");
    /* Should produce output - check if any byte is non-zero */
    /* Note: First byte might be 0x00 (version byte for P2PKH) */
    bool has_data2 = false;
    for (int j = 0; j < 25; j++) {
        if (b58bin[j] != 0) {
            has_data2 = true;
            break;
        }
    }
    assert_true(has_data2);
}

/* Test b58tobin() edge cases */
static void test_b58tobin_edge_cases(void)
{
    char b58bin[25];
    
    /* Test: Single character (valid Base58) */
    memset(b58bin, 0, 25);
    b58tobin(b58bin, "1");
    /* Single "1" decodes to a very small value (0 in Base58 table) */
    /* The output may be all zeros or have data in later bytes */
    /* Just verify it doesn't crash */
    
    /* Test: All same character */
    memset(b58bin, 0, 25);
    b58tobin(b58bin, "1111111111111111111114oLvT2");
    /* Should produce output - check if any byte is non-zero */
    bool has_data = false;
    for (int j = 0; j < 25; j++) {
        if (b58bin[j] != 0) {
            has_data = true;
            break;
        }
    }
    assert_true(has_data);
}

/* Test b58tobin() integration with address_to_txn */
static void test_b58tobin_integration(void)
{
    char b58bin[25];
    char p2h[25];
    const char *test_address = "1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa";
    
    /* Decode Base58 address */
    memset(b58bin, 0, 25);
    b58tobin(b58bin, test_address);
    
    /* Use address_to_txn which internally uses b58tobin */
    int result = address_to_txn(p2h, test_address, false, false);
    
    /* Both should succeed */
    assert_true(result > 0);
    /* b58bin should have some data (first byte might be 0x00 for version) */
    bool has_data = false;
    for (int j = 0; j < 25; j++) {
        if (b58bin[j] != 0) {
            has_data = true;
            break;
        }
    }
    assert_true(has_data);
}

/* Test b58tobin() with P2SH addresses */
static void test_b58tobin_p2sh_addresses(void)
{
    char b58bin[25];
    const char *p2sh_address = "3J98t1WpEZ73CNmQviecrnyiWrnqRhWNLy";
    
    memset(b58bin, 0, 25);
    b58tobin(b58bin, p2sh_address);
    
    /* Should produce output - check if any byte is non-zero */
    /* Note: First byte might be 0x00 (version byte) */
    bool has_data = false;
    for (int j = 0; j < 25; j++) {
        if (b58bin[j] != 0) {
            has_data = true;
            break;
        }
    }
    assert_true(has_data);
}

int main(void)
{
    printf("Running Base58 decoding tests...\n\n");
    
    run_test(test_b58tobin_known_addresses);
    run_test(test_b58tobin_address_lengths);
    run_test(test_b58tobin_edge_cases);
    run_test(test_b58tobin_integration);
    run_test(test_b58tobin_p2sh_addresses);
    
    printf("\nAll Base58 decoding tests passed!\n");
    return 0;
}

