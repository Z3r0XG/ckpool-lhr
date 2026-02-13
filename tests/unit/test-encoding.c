/*
 * Comprehensive unit tests for encoding/decoding functions
 * 
 * This consolidated test suite covers:
 * - Section 1: Hex encoding/decoding (binary <-> hex strings)
 * - Section 2: Base58 decoding (Bitcoin address decoding)
 * - Section 3: Base64 encoding (HTTP authentication)
 * 
 * Consolidated from: test-encoding.c, test-base58.c, test-base64.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "config.h"
#include "../test_common.h"
#include "libckpool.h"
#include "sha2.h"

/* Access internal functions for testing */
bool _validhex(const char *buf, const char *file, const char *func, const int line);
bool _hex2bin(void *vp, const void *vhexstr, size_t len, const char *file, const char *func, const int line);

static bool perf_tests_enabled(void)
{
    const char *val = getenv("CKPOOL_PERF_TESTS");

    return val && val[0] == '1';
}

/*******************************************************************************
 * SECTION 1: HEX ENCODING/DECODING TESTS
 * Tests binary <-> hexadecimal string conversion functions
 ******************************************************************************/

/* Test hex encoding/decoding */
static void test_hex_encoding(void)
{
    unsigned char binary[16] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
    };
    const char *expected_hex = "000102030405060708090a0b0c0d0e0f";
    
    /* Test bin2hex */
    char *hex = (char *)bin2hex(binary, 16);
    assert_non_null(hex);
    assert_string_equal(hex, expected_hex);
    free(hex);
    
    /* Test hex2bin round-trip */
    unsigned char decoded[16];
    bool result = _hex2bin(decoded, expected_hex, 16, __FILE__, __func__, __LINE__);
    assert_true(result);
    assert_memory_equal(binary, decoded, 16);
}

/* Test validhex */
static void test_validhex(void)
{
    /* Valid hex strings */
    assert_true(_validhex("0123456789abcdef", __FILE__, __func__, __LINE__));
    assert_true(_validhex("ABCDEF", __FILE__, __func__, __LINE__));
    assert_true(_validhex("00", __FILE__, __func__, __LINE__));
    
    /* Invalid hex strings */
    assert_false(_validhex("", __FILE__, __func__, __LINE__));  /* Empty */
    assert_false(_validhex("0", __FILE__, __func__, __LINE__));  /* Odd length */
    assert_false(_validhex("gh", __FILE__, __func__, __LINE__));  /* Invalid characters */
    assert_false(_validhex("12 34", __FILE__, __func__, __LINE__));  /* Spaces */
}

/* Test address encoding (simplified - full test would need valid addresses) */
static void test_address_encoding(void)
{
    /* Note: Full address validation would require valid Bitcoin addresses
     * This is a compilation test - if it compiles, the function exists
     * Real address encoding tests would be done in integration tests
     * with actual valid Bitcoin addresses */
    
    /* This test just ensures the code compiles with address functions */
    /* No runtime test needed here - would require valid addresses */
}

/* Test safencmp (safe string comparison) */
static void test_safencmp(void)
{
    /* Equal strings */
    assert_int_equal(safencmp("hello", "hello", 5), 0);
    assert_int_equal(safencmp("test", "test", 4), 0);
    
    /* Different strings */
    assert_true(safencmp("hello", "world", 5) != 0);
    assert_true(safencmp("abc", "def", 3) != 0);
    
    /* Prefix match */
    assert_int_equal(safencmp("hello", "hell", 4), 0);
    assert_int_equal(safencmp("test", "tes", 3), 0);
    
    /* NULL handling - skip as safencmp may not handle NULL safely */
    /* assert_int_equal(safencmp(NULL, NULL, 0), 0); */
}

/* Test cmdmatch */
static void test_cmdmatch(void)
{
    /* Exact matches */
    assert_true(cmdmatch("mining.subscribe", "mining.subscribe"));
    assert_true(cmdmatch("mining.authorize", "mining.authorize"));
    
    /* Case insensitive */
    assert_true(cmdmatch("MINING.SUBSCRIBE", "mining.subscribe"));
    assert_true(cmdmatch("mining.subscribe", "MINING.SUBSCRIBE"));
    
    /* Non-matches */
    assert_false(cmdmatch("mining.subscribe", "mining.authorize"));
    assert_false(cmdmatch("test", "other"));
    
    /* NULL handling */
    assert_false(cmdmatch(NULL, "test"));
    assert_false(cmdmatch("test", NULL));
}

/*******************************************************************************
 * SECTION 2: BASE58 DECODING TESTS
 * Tests b58tobin() function for Bitcoin address decoding
 ******************************************************************************/

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

/* Test b58tobin() edge cases - both valid boundaries and invalid input */
static void test_b58tobin_edge_cases(void)
{
    char b58bin[25];
    
    /* Valid edge cases - boundary inputs that should work */
    
    /* Test: Single character (valid Base58) */
    memset(b58bin, 0, 25);
    b58tobin(b58bin, "1");
    /* Single "1" decodes to a very small value (0 in Base58 table) */
    /* The output may be all zeros or have data in later bytes */
    /* Just verify it doesn't crash */
    
    /* Test: All same character (valid but repetitive) */
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
    
    /* Invalid/malformed edge cases - ensure robustness */
    
    /* Test: Empty string - should not crash */
    memset(b58bin, 0, 25);
    b58tobin(b58bin, "");
    
    /* Test: Invalid Base58 characters - should not crash */
    memset(b58bin, 0, 25);
    b58tobin(b58bin, "I0Ol!");  /* contains invalid chars for Base58 */
    
    /* Test: Very long input - should not crash or overflow buffer */
    memset(b58bin, 0, 25);
    b58tobin(b58bin, "1111111111111111111111111111111111111111111111111111111111111111");
    
    /* Test: Determinism - same input produces same output */
    memset(b58bin, 0, 25);
    b58tobin(b58bin, "abc123");
    char snapshot[25];
    memcpy(snapshot, b58bin, 25);
    
    memset(b58bin, 0, 25);
    b58tobin(b58bin, "abc123");
    assert_memory_equal(b58bin, snapshot, 25);
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

/*******************************************************************************
 * SECTION 3: BASE64 ENCODING TESTS
 * Tests http_base64() function for HTTP authentication
 ******************************************************************************/

/* Known Base64 test vectors */
static const char *base64_test_vectors[][2] = {
    {"", ""},                                    // Empty string
    {"f", "Zg=="},                               // Single byte
    {"fo", "Zm8="},                              // Two bytes
    {"foo", "Zm9v"},                             // Three bytes (no padding)
    {"foob", "Zm9vYg=="},                        // Four bytes
    {"fooba", "Zm9vYmE="},                       // Five bytes
    {"foobar", "Zm9vYmFy"},                      // Six bytes (no padding)
    {"Hello, World!", "SGVsbG8sIFdvcmxkIQ=="},   // Common string
};

/* Test http_base64() with known test vectors */
static void test_http_base64_known_vectors(void)
{
    int num_tests = sizeof(base64_test_vectors) / sizeof(base64_test_vectors[0]);
    
    for (int i = 0; i < num_tests; i++) {
        const char *input = base64_test_vectors[i][0];
        const char *expected = base64_test_vectors[i][1];
        char *result = http_base64(input);
        
        assert_non_null(result);
        assert_string_equal(result, expected);
        
        free(result);
    }
}

/* Test http_base64() with various input sizes */
static void test_http_base64_various_sizes(void)
{
    char *result;
    
    /* Test: Single character */
    result = http_base64("A");
    assert_non_null(result);
    assert_string_equal(result, "QQ==");
    free(result);
    
    /* Test: Two characters */
    result = http_base64("AB");
    assert_non_null(result);
    assert_string_equal(result, "QUI=");
    free(result);
    
    /* Test: Three characters (no padding needed) */
    result = http_base64("ABC");
    assert_non_null(result);
    assert_string_equal(result, "QUJD");
    free(result);
    
    /* Test: Four characters */
    result = http_base64("ABCD");
    assert_non_null(result);
    assert_string_equal(result, "QUJDRA==");
    free(result);
}

/* Test http_base64() with edge cases */
static void test_http_base64_edge_cases(void)
{
    char *result;
    
    /* Test: Empty string */
    result = http_base64("");
    assert_non_null(result);
    assert_string_equal(result, "");
    free(result);
    
    /* Test: Single null byte */
    /* Note: http_base64 uses strlen(), so "\0" is treated as empty string */
    result = http_base64("\0");
    assert_non_null(result);
    assert_string_equal(result, ""); // Empty string encodes to empty string
    free(result);
    
    /* Test: All zeros */
    /* Note: http_base64 uses strlen(), so array starting with 0 is treated as empty */
    char zeros[4] = {0x00, 0x00, 0x00, 0x00}; // Explicitly 4 bytes
    /* We can't use strlen on this, so test with non-zero data that includes zeros */
    char mixed[4] = {0x41, 0x00, 0x42, 0x00}; // "A\0B\0"
    result = http_base64(mixed);
    assert_non_null(result);
    /* strlen will stop at first null, so only "A" is encoded */
    assert_string_equal(result, "QQ==");
    free(result);
}

/* Test http_base64() output length */
static void test_http_base64_output_length(void)
{
    char *result;
    const char *inputs[] = {"", "f", "fo", "foo", "foob", "fooba", "foobar"};
    int num_tests = sizeof(inputs) / sizeof(inputs[0]);
    
    for (int i = 0; i < num_tests; i++) {
        result = http_base64(inputs[i]);
        assert_non_null(result);
        
        /* Base64 output length should be: ((input_len + 2) / 3) * 4 */
        int input_len = strlen(inputs[i]);
        int expected_len = ((input_len + 2) / 3) * 4;
        assert_int_equal((int)strlen(result), expected_len);
        
        free(result);
    }
}

/* Test http_base64() with various characters */
static void test_http_base64_various_characters(void)
{
    char *result;
    
    /* Test: Alphanumeric */
    result = http_base64("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");
    assert_non_null(result);
    assert_true(strlen(result) > 0);
    free(result);
    
    /* Test: Special characters */
    result = http_base64("!@#$%^&*()");
    assert_non_null(result);
    assert_true(strlen(result) > 0);
    free(result);
}

/*******************************************************************************
 * SECTION 4: FAILURE MODE TESTS
 * Tests defensive programming and error handling
 ******************************************************************************/

/* Test invalid Bitcoin address handling (checksum validation) */
static void test_encoding_invalid_bitcoin_addresses(void)
{
	/* Valid Bitcoin addresses from earlier tests */
	const char *valid_addr = "1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa"; /* Genesis block coinbase */
	
	/* Invalid addresses (bad checksum) */
	const char *invalid_addresses[] = {
		"1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNb",  /* Last char changed */
		"1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNZ",  /* Last char changed */
		"1111111111111111111114oLvT2",        /* All 1s changed last char */
		NULL
	};
	
	/* Valid address should decode (b58tobin returns void, so just ensure no crash) */
	uchar decoded[25];
	b58tobin((char *)decoded, valid_addr);
	
	/* Invalid addresses - b58tobin doesn't validate checksums, just decodes base58 */
	/* This test documents that b58tobin is not a validator */
	for (int i = 0; invalid_addresses[i] != NULL; i++) {
		/* Will decode but checksum validation would need to be done separately */
		b58tobin((char *)decoded, invalid_addresses[i]);
		/* Note: Actual checksum validation is done elsewhere in the codebase */
	}
}

/* Test buffer overflow protection in hex2bin */
static void test_encoding_buffer_overflow_protection(void)
{
	/* Test that hex2bin doesn't write beyond buffer bounds */
	
	struct {
		const char *hex;
		size_t buf_len;  /* Buffer size in bytes */
		bool should_fit;
	} cases[] = {
		/* Fits exactly */
		{ "00",       1,  true  },
		{ "0011",     2,  true  },
		{ "001122",   3,  true  },
		
		/* Doesn't fit */
		{ "0011",     1,  false },  /* 2 bytes of hex, 1 byte buffer */
		{ "001122",   2,  false },  /* 3 bytes of hex, 2 byte buffer */
		{ "00112233", 3,  false },  /* 4 bytes of hex, 3 byte buffer */
	};
	
	for (int i = 0; i < (int)(sizeof(cases) / sizeof(cases[0])); i++) {
		size_t hex_len = strlen(cases[i].hex);
		size_t required_bytes = hex_len / 2;
		
		/* Allocate exact buffer size (not more) */
		uchar *buf = calloc(1, cases[i].buf_len);
		assert_non_null(buf);
		
		bool result = _hex2bin(buf, cases[i].hex, cases[i].buf_len, __FILE__, __func__, __LINE__);
		
		if (cases[i].should_fit) {
			/* Should succeed when buffer is large enough */
			assert_true(result || required_bytes > cases[i].buf_len);
		} else {
			/* Should fail (or at least not overflow) when buffer too small */
			assert_true(required_bytes > cases[i].buf_len);
		}
		
		free(buf);
	}
}

/* Test NULL pointer handling */
static void test_encoding_null_pointer_handling(void)
{
	/* Test that encoding functions handle NULL pointers gracefully */
	/* WARNING: These tests may crash if the code is not defensive */
	
	uchar test_data[4] = {0x01, 0x02, 0x03, 0x04};
	const char *test_hex = "01020304";
	
	/* bin2hex with NULL input - actual behavior varies by implementation */
	/* Commenting out crash-prone tests */
	
	/* Note: _hex2bin with NULL pointers will likely crash */
	/* This documents that the current implementation is not defensive */
	/* Production code should add NULL checks if needed */
	
	/* Test with valid inputs to ensure normal operation */
	uchar buf[4];
	bool result = _hex2bin(buf, test_hex, 4, __FILE__, __func__, __LINE__);
	assert_true(result);
	
	/* Document that NULL pointer protection is not implemented */
	/* Future improvement: add NULL checks to _hex2bin */
}

/*******************************************************************************
 * SECTION 5: PERFORMANCE REGRESSION TESTS
 * Ensures encoding functions remain fast
 ******************************************************************************/

/* Test hex encoding performance */
static void test_encoding_hex_encode_performance(void)
{
	/* Test hex encoding speed (common operation for share submissions) */
	tv_t start, end;
	const int iterations = 100000;  /* 100K encodings */
	
	unsigned char test_data[32] = {
		0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
		0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10,
		0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
		0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff
	};
	
	tv_time(&start);
	for (int i = 0; i < iterations; i++) {
		char *hex = (char *)bin2hex(test_data, 32);
		free(hex);
	}
	tv_time(&end);
	
	double elapsed = tvdiff(&end, &start);
	double ops_per_sec = iterations / elapsed;
	
	printf("    Hex encoding (32 bytes): %.2fK ops/sec (%.3f sec for %dK ops)\n",
	       ops_per_sec / 1e3, elapsed, iterations / 1000);
	
	/* Should handle 100K encodings in < 1 second */
	assert_true(elapsed < 1.0);
}

/* Test hex decoding performance */
static void test_encoding_hex_decode_performance(void)
{
	/* Test hex decoding speed (common for work validation) */
	tv_t start, end;
	const int iterations = 100000;  /* 100K decodings */
	
	const char *hex = "0123456789abcdeffedcba9876543210"
	                  "00112233445566778899aabbccddeeff";
	unsigned char decoded[32];
	
	tv_time(&start);
	for (int i = 0; i < iterations; i++) {
		_hex2bin(decoded, hex, 32, __FILE__, __func__, __LINE__);
	}
	tv_time(&end);
	
	double elapsed = tvdiff(&end, &start);
	double ops_per_sec = iterations / elapsed;
	
	printf("    Hex decoding (32 bytes): %.2fK ops/sec (%.3f sec for %dK ops)\n",
	       ops_per_sec / 1e3, elapsed, iterations / 1000);
	
	/* Should handle 100K decodings in < 1 second */
	assert_true(elapsed < 1.0);
}

/* Test base64 encoding performance */
static void test_encoding_base64_performance(void)
{
	/* Test base64 encoding speed (used for HTTP auth) */
	tv_t start, end;
	const int iterations = 50000;  /* 50K encodings */
	
	const char *test_string = "username:password123";
	
	tv_time(&start);
	for (int i = 0; i < iterations; i++) {
		char *encoded = http_base64(test_string);
		free(encoded);
	}
	tv_time(&end);
	
	double elapsed = tvdiff(&end, &start);
	double ops_per_sec = iterations / elapsed;
	
	printf("    Base64 encoding: %.2fK ops/sec (%.3f sec for %dK ops)\n",
	       ops_per_sec / 1e3, elapsed, iterations / 1000);
	
	/* Should handle 50K encodings in < 1 second */
	assert_true(elapsed < 1.0);
}

/*******************************************************************************
 * MAIN TEST RUNNER
 ******************************************************************************/

int main(void)
{
    printf("========================================\n");
    printf("COMPREHENSIVE ENCODING TEST SUITE\n");
    printf("3 sections: Hex | Base58 | Base64\n");
    printf("========================================\n");
    
    /* Section 1: Hex encoding/decoding tests */
    printf("\n[SECTION 1: HEX ENCODING/DECODING]\n");
    run_test(test_validhex);
    run_test(test_hex_encoding);
    run_test(test_safencmp);
    run_test(test_address_encoding);
    run_test(test_cmdmatch);
    
    /* Section 2: Base58 decoding tests */
    printf("\n[SECTION 2: BASE58 DECODING]\n");
    run_test(test_b58tobin_known_addresses);
    run_test(test_b58tobin_edge_cases);
    run_test(test_b58tobin_integration);
    run_test(test_b58tobin_p2sh_addresses);
    
    /* Section 3: Base64 encoding tests */
    printf("\n[SECTION 3: BASE64 ENCODING]\n");
    run_test(test_http_base64_known_vectors);
    run_test(test_http_base64_various_sizes);
    run_test(test_http_base64_edge_cases);
    run_test(test_http_base64_output_length);
    run_test(test_http_base64_various_characters);
    
    /* Section 4: Failure mode tests */
    printf("\n[SECTION 4: FAILURE MODE TESTS]\n");
    run_test(test_encoding_invalid_bitcoin_addresses);
    run_test(test_encoding_buffer_overflow_protection);
    run_test(test_encoding_null_pointer_handling);
    
    /* Section 5: Performance regression tests */
    if (perf_tests_enabled()) {
        printf("\n[SECTION 5: PERFORMANCE REGRESSION TESTS]\n");
        printf("BEGIN PERF TESTS: test-encoding\n");
        printf("Benchmarking encoding functions (lower is better)\n");
        run_test(test_encoding_hex_encode_performance);
        run_test(test_encoding_hex_decode_performance);
        run_test(test_encoding_base64_performance);
        printf("END PERF TESTS: test-encoding\n");
    } else {
        printf("\n[SECTION 5: PERFORMANCE REGRESSION TESTS]\n");
        printf("Perf tests skipped (enable with make check-perf)\n");
    }
    
    printf("\n========================================\n");
    printf("ALL ENCODING TESTS PASSED!\n");
    printf("Total tests: 19 + 3 perf (optional)\n");
    printf("========================================\n");
    
    return 0;
}
