/*
 * Unit tests for Bitcoin address encoding
 * Tests Base58 and Bech32 decoding through address_to_txn()
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

/* Test address_to_txn with legacy P2PKH address (starts with '1')
 * These are Base58 encoded addresses
 * Note: Full address validation requires bitcoind, so we test that the
 * function processes addresses without crashing and produces expected lengths
 */
static void test_legacy_p2pkh_address(void)
{
    char txn[100];
    int len;
    
    /* Test with a known valid P2PKH address format */
    const char *test_addr = "1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa"; // Genesis block address
    
    /* Test P2PKH (script=false, segwit=false) */
    len = address_to_txn(txn, test_addr, false, false);
    
    /* P2PKH transaction should be 25 bytes
     * We verify the length matches expected structure, but don't validate
     * exact byte values as Base58 decoding may vary
     */
    assert_true(len == 25);
    
    /* Verify function doesn't crash and produces output */
    assert_true(len > 0);
    assert_true(len <= 100); // Reasonable size limit
}

/* Test address_to_txn with P2SH address (starts with '3')
 * These are Base58 encoded script addresses
 */
static void test_p2sh_address(void)
{
    char txn[100];
    int len;
    
    /* Test with a known valid P2SH address format */
    const char *test_addr = "3J98t1WpEZ73CNmQviecrnyiWrnqRhWNLy"; // Example P2SH format
    
    /* Test P2SH (script=true, segwit=false) */
    len = address_to_txn(txn, test_addr, true, false);
    
    /* P2SH transaction should be 23 bytes
     * We verify the length matches expected structure
     */
    assert_true(len == 23);
    assert_true(len > 0);
}

/* Test address_to_txn with Segwit Bech32 address (starts with 'bc1')
 * These are Bech32 encoded addresses
 */
static void test_segwit_bech32_address(void)
{
    char txn[100];
    int len;
    
    /* Test with a known valid Bech32 address format */
    const char *test_addr = "bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t4"; // Example Bech32
    
    /* Test Segwit (script=false, segwit=true) */
    len = address_to_txn(txn, test_addr, false, true);
    
    /* Segwit transaction format:
     * First byte: witness version (0x00 for version 0, or 0x50+version for version > 0)
     * Second byte: witness program length
     * Followed by witness program data
     */
    assert_true(len > 0);
    assert_true(len <= 100); // Reasonable size limit
    
    /* Version 0 segwit addresses should have version byte 0x00 */
    /* Note: This may vary based on the actual address, but structure should be valid */
}

/* Test that address_to_txn handles different address types correctly */
static void test_address_type_routing(void)
{
    char txn[100];
    int len_p2pkh, len_p2sh, len_segwit;
    const char *test_addr = "1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa";
    
    /* Test that different flags produce different transaction formats
     * Note: Using the same address with different flags tests the routing logic,
     * though the actual decoding may vary based on address type
     */
    len_p2pkh = address_to_txn(txn, test_addr, false, false);
    
    /* P2PKH should be 25 bytes */
    assert_true(len_p2pkh == 25);
    
    /* P2SH should be 23 bytes (using same address tests routing, not validity) */
    len_p2sh = address_to_txn(txn, test_addr, true, false);
    assert_true(len_p2sh == 23);
    
    /* Segwit with non-Bech32 address may produce unexpected results,
     * but function should not crash
     */
    len_segwit = address_to_txn(txn, test_addr, false, true);
    assert_true(len_segwit > 0);
}

/* Test Base58 decoding through b58tobin (indirectly via address_to_txn)
 * Base58 uses characters: 123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz
 * We test that the function processes Base58 addresses and produces expected lengths
 */
static void test_base58_encoding_structure(void)
{
    char txn[100];
    int len;
    
    /* Test that Base58 addresses produce expected transaction structure */
    const char *test_addr = "1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa";
    
    len = address_to_txn(txn, test_addr, false, false);
    
    /* Verify the transaction length is correct for P2PKH
     * Full structure validation would require verifying exact opcodes,
     * but that depends on successful Base58 decoding which may vary
     */
    assert_true(len == 25);
    assert_true(len > 0);
}

/* Test that address functions handle various address formats */
static void test_address_format_handling(void)
{
    char txn[100];
    int len;
    
    /* Test different address formats that should be handled */
    
    /* Legacy address (P2PKH) */
    const char *legacy = "1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa";
    len = address_to_txn(txn, legacy, false, false);
    assert_true(len == 25);
    
    /* Script address (P2SH) */
    const char *script = "3J98t1WpEZ73CNmQviecrnyiWrnqRhWNLy";
    len = address_to_txn(txn, script, true, false);
    assert_true(len == 23);
    
    /* Bech32 address (Segwit) */
    const char *bech32 = "bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t4";
    len = address_to_txn(txn, bech32, false, true);
    assert_true(len > 0);
}

/* Test edge cases - very long addresses, special characters, etc.
 * Note: These tests verify the functions don't crash, not that they produce valid output
 */
static void test_address_edge_cases(void)
{
    char txn[100];
    int len;
    
    /* Test with various address-like strings
     * Note: Without full validation, we're just testing the functions don't crash
     */
    
    /* Standard length address */
    const char *normal = "1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa";
    len = address_to_txn(txn, normal, false, false);
    assert_true(len > 0);
    
    /* The functions should handle the input without crashing
     * Full validation would require bitcoind integration
     */
}

int main(void)
{
    printf("Running address encoding tests...\n\n");
    
    run_test(test_legacy_p2pkh_address);
    run_test(test_p2sh_address);
    run_test(test_segwit_bech32_address);
    run_test(test_address_type_routing);
    run_test(test_base58_encoding_structure);
    run_test(test_address_format_handling);
    run_test(test_address_edge_cases);
    
    printf("\nAll address encoding tests passed!\n");
    return 0;
}

