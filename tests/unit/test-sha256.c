/*
 * Unit tests for SHA-256 hashing
 * Critical for share validation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "config.h"
#include "../test_common.h"
#include "sha2.h"

/* gen_hash: Double SHA-256 (Bitcoin standard) */
static void gen_hash(const unsigned char *data, unsigned char *hash, unsigned int len)
{
	unsigned char first_hash[32];
	sha256(data, len, first_hash);
	sha256(first_hash, 32, hash);
}

/* Known test vectors from NIST and Bitcoin */
static const struct {
    const char *input;
    const char *expected_hex;
} sha256_test_vectors[] = {
    /* Empty string */
    {"", "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"},
    /* "abc" */
    {"abc", "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"},
    /* "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq" */
    {"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
     "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1"},
};

/* Test single-shot SHA-256 */
static void test_sha256_vectors(void)
{
    for (size_t i = 0; i < sizeof(sha256_test_vectors) / sizeof(sha256_test_vectors[0]); i++) {
        unsigned char digest[32];
        unsigned char expected[32];
        const char *input = sha256_test_vectors[i].input;
        const char *expected_hex = sha256_test_vectors[i].expected_hex;
        
        /* Compute hash */
        sha256((const unsigned char *)input, strlen(input), digest);
        
        /* Convert expected hex to binary */
        for (int j = 0; j < 32; j++) {
            char hex_byte[3] = {expected_hex[j * 2], expected_hex[j * 2 + 1], 0};
            expected[j] = (unsigned char)strtoul(hex_byte, NULL, 16);
        }
        
        /* Compare */
        assert_memory_equal(digest, expected, 32);
    }
}

/* Test streaming SHA-256 */
static void test_sha256_streaming(void)
{
    const char *input = "The quick brown fox jumps over the lazy dog";
    unsigned char digest_single[32];
    unsigned char digest_stream[32];
    
    /* Single-shot */
    sha256((const unsigned char *)input, strlen(input), digest_single);
    
    /* Streaming */
    sha256_ctx ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, (const unsigned char *)input, (unsigned int)strlen(input));
    sha256_final(&ctx, digest_stream);
    
    /* Should produce same result */
    assert_memory_equal(digest_single, digest_stream, 32);
}

/* Test double SHA-256 (gen_hash) - Bitcoin standard */
static void test_gen_hash(void)
{
    /* Test with known input */
    unsigned char input[80] = {0};  /* Block header size */
    unsigned char hash[32];
    
    /* Set some test data */
    memset(input, 0x42, 80);
    
    gen_hash(input, hash, 80);
    
    /* Should produce valid hash (not all zeros) */
    bool has_nonzero = false;
    for (int i = 0; i < 32; i++) {
        if (hash[i] != 0) {
            has_nonzero = true;
            break;
        }
    }
    assert_true(has_nonzero);
    
    /* Double SHA-256 should be different from single SHA-256 */
    unsigned char single_hash[32];
    sha256(input, 80, single_hash);
    assert_true(memcmp(hash, single_hash, 32) != 0);
}

/* Test empty input */
static void test_sha256_empty(void)
{
    unsigned char digest[32];
    unsigned char expected[32];
    const char *expected_hex = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
    
    sha256(NULL, 0, digest);
    
    /* Convert expected hex to binary */
    for (int j = 0; j < 32; j++) {
        char hex_byte[3] = {expected_hex[j * 2], expected_hex[j * 2 + 1], 0};
        expected[j] = (unsigned char)strtoul(hex_byte, NULL, 16);
    }
    
    assert_memory_equal(digest, expected, 32);
}

/* Test various input sizes */
static void test_sha256_various_sizes(void)
{
    unsigned char input[1000];
    unsigned char digest[32];
    
    /* Initialize with pattern */
    for (int i = 0; i < 1000; i++) {
        input[i] = (unsigned char)(i & 0xFF);
    }
    
    /* Test different sizes */
    for (int size = 1; size <= 1000; size *= 2) {
        sha256(input, size, digest);
        
        /* Should produce valid hash */
        bool has_nonzero = false;
        for (int i = 0; i < 32; i++) {
            if (digest[i] != 0) {
                has_nonzero = true;
                break;
            }
        }
        assert_true(has_nonzero);
    }
}

int main(void)
{
    printf("Running SHA-256 hashing tests...\n\n");
    
    run_test(test_sha256_vectors);
    run_test(test_sha256_streaming);
    run_test(test_gen_hash);
    run_test(test_sha256_empty);
    run_test(test_sha256_various_sizes);
    
    printf("\nAll SHA-256 tests passed!\n");
    return 0;
}

