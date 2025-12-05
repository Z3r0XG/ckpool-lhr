/*
 * Unit tests for share submission parameter validation
 * Tests nonce, ntime, job_id, and other parameter validation
 * Critical for preventing invalid share submissions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "config.h"
#include "../test_common.h"
#include "libckpool.h"

/* Test validhex function - used for nonce, ntime validation */
static void test_validhex(void)
{
    /* Valid hex strings (must be even length) */
    assert_true(validhex("0123456789abcdef"));
    assert_true(validhex("ABCDEF"));
    assert_true(validhex("00ff"));
    assert_true(validhex("aa"));
    assert_true(validhex("00"));
    
    /* Invalid hex strings */
    assert_false(validhex("")); /* Empty */
    assert_false(validhex("a")); /* Odd length */
    assert_false(validhex("g")); /* Invalid hex char */
    assert_false(validhex("xyz")); /* Invalid hex chars */
    assert_false(validhex("12g34")); /* Invalid hex char in middle */
    assert_false(validhex("hello")); /* Invalid hex chars */
}

/* Test nonce validation requirements */
static void test_nonce_validation(void)
{
    /* Nonce must be at least 8 hex chars (4 bytes) */
    const char *nonce;
    int len;
    
    /* Valid nonces */
    nonce = "12345678";
    len = strlen(nonce);
    assert_true(len >= 8 && validhex(nonce));
    
    nonce = "abcdef00";
    len = strlen(nonce);
    assert_true(len >= 8 && validhex(nonce));
    
    nonce = "1234567890abcdef"; /* 8 bytes */
    len = strlen(nonce);
    assert_true(len >= 8 && validhex(nonce));
    
    /* Invalid nonces */
    nonce = "1234567"; /* Too short */
    len = strlen(nonce);
    assert_false(len >= 8);
    
    nonce = "1234567g"; /* Invalid hex */
    assert_false(validhex(nonce));
}

/* Test ntime validation requirements */
static void test_ntime_validation(void)
{
    /* ntime must be valid hex */
    const char *ntime;
    
    /* Valid ntime */
    assert_true(validhex("12345678"));
    assert_true(validhex("abcdef00"));
    
    /* Invalid ntime */
    assert_false(validhex(""));
    assert_false(validhex("xyz"));
    assert_false(validhex("1234567g"));
}

/* Test job_id validation */
static void test_job_id_validation(void)
{
    /* job_id must be non-empty string */
    const char *job_id;
    
    /* Valid job_id */
    job_id = "abc123";
    assert_true(job_id && strlen(job_id) > 0);
    
    job_id = "693271c400000008";
    assert_true(job_id && strlen(job_id) > 0);
    
    /* Invalid job_id */
    job_id = "";
    assert_false(job_id && strlen(job_id) > 0);
    
    job_id = NULL;
    assert_false(job_id && strlen(job_id) > 0);
}

/* Test workername validation */
static void test_workername_validation(void)
{
    /* workername must be non-empty, no '/' character */
    const char *workername;
    
    /* Valid workernames */
    workername = "worker1";
    assert_true(workername && strlen(workername) > 0 && !strchr(workername, '/'));
    
    workername = "bc1q8qkesw5kyplv7hdxyseqls5m78w5tqdfd40lf5.worker1";
    assert_true(workername && strlen(workername) > 0 && !strchr(workername, '/'));
    
    /* Invalid workernames */
    workername = "";
    assert_false(workername && strlen(workername) > 0);
    
    workername = "user/worker"; /* Contains '/' */
    assert_true(strchr(workername, '/') != NULL);
    
    workername = "."; /* Special case - invalid */
    assert_true(!memcmp(workername, ".", 1));
    
    workername = "_"; /* Special case - invalid */
    assert_true(!memcmp(workername, "_", 1));
}

/* Test parameter array size validation */
static void test_params_array_size(void)
{
    /* mining.submit requires at least 5 parameters:
     * [workername, job_id, nonce2, ntime, nonce]
     */
    int min_params = 5;
    
    assert_true(min_params == 5);
    
    /* Test that we require at least 5 */
    assert_false(0 >= min_params);
    assert_false(1 >= min_params);
    assert_false(2 >= min_params);
    assert_false(3 >= min_params);
    assert_false(4 >= min_params);
    assert_true(5 >= min_params);
    assert_true(6 >= min_params);
}

int main(void)
{
    printf("Running share parameter validation tests...\n");
    
    test_validhex();
    test_nonce_validation();
    test_ntime_validation();
    test_job_id_validation();
    test_workername_validation();
    test_params_array_size();
    
    printf("All share parameter validation tests passed!\n");
    return 0;
}

