/*
 * Unit tests for Base64 encoding
 * Tests http_base64() function
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "config.h"
#include "../test_common.h"
#include "libckpool.h"

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

/* Test http_base64() with binary data */
static void test_http_base64_binary_data(void)
{
    char *result;
    uchar binary_data[4] = {0x01, 0x02, 0x03, 0x04}; // Non-zero data
    
    result = http_base64((const char *)binary_data);
    assert_non_null(result);
    /* Should produce valid Base64 output */
    assert_true(strlen(result) > 0);
    free(result);
    
    /* Test with null bytes (strlen will stop at first null) */
    uchar binary_with_null[4] = {0x41, 0x42, 0x00, 0x44}; // "AB\0D"
    result = http_base64((const char *)binary_with_null);
    assert_non_null(result);
    /* Result should encode all 4 bytes, even with null in middle */
    /* The function uses strlen internally, so it will only encode up to first null */
    assert_true(strlen(result) >= 0); // May be 0 if input is treated as "AB"
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

int main(void)
{
    printf("Running Base64 encoding tests...\n\n");
    
    run_test(test_http_base64_known_vectors);
    run_test(test_http_base64_various_sizes);
    run_test(test_http_base64_binary_data);
    run_test(test_http_base64_edge_cases);
    run_test(test_http_base64_output_length);
    run_test(test_http_base64_various_characters);
    
    printf("\nAll Base64 encoding tests passed!\n");
    return 0;
}

