/*
 * Unit tests for number serialization functions
 * Tests ser_number() and get_sernumber() for Bitcoin transaction scripts
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "config.h"
#include "../test_common.h"
#include "libckpool.h"

/* Test ser_number() - serialize numbers for Bitcoin scripts
 * Returns number of bytes used (1-5 bytes)
 */
static void test_ser_number_small_values(void)
{
    uchar buf[5];
    int len;
    int32_t recovered;
    
    /* Test 1: Value < 0x80 (1 byte value + 1 byte length = 2 total) */
    /* Note: buf[0] stores the number of value bytes (1), not total length */
    len = ser_number(buf, 0);
    assert_int_equal(len, 2); // Total length: 1 byte length + 1 byte value
    assert_int_equal(buf[0], 1); // Length field stores number of value bytes
    recovered = get_sernumber(buf);
    assert_int_equal(recovered, 0);
    
    len = ser_number(buf, 0x7F);
    assert_int_equal(len, 2);
    assert_int_equal(buf[0], 1);
    recovered = get_sernumber(buf);
    assert_int_equal(recovered, 0x7F);
    
    len = ser_number(buf, 1);
    assert_int_equal(len, 2);
    assert_int_equal(buf[0], 1);
    recovered = get_sernumber(buf);
    assert_int_equal(recovered, 1);
    
    len = ser_number(buf, 42);
    assert_int_equal(len, 2);
    assert_int_equal(buf[0], 1);
    recovered = get_sernumber(buf);
    assert_int_equal(recovered, 42);
}

static void test_ser_number_medium_values(void)
{
    uchar buf[5];
    int len;
    int32_t recovered;
    
    /* Test: Value < 0x8000 (2 bytes value + 1 byte length = 3 total) */
    len = ser_number(buf, 0x80);
    assert_int_equal(len, 3); // Total length: 1 byte length + 2 bytes value
    assert_int_equal(buf[0], 2); // Length field stores number of value bytes
    
    len = ser_number(buf, 0x7FFF);
    assert_int_equal(len, 3);
    assert_int_equal(buf[0], 2);
    recovered = get_sernumber(buf);
    assert_int_equal(recovered, 0x7FFF);
    
    len = ser_number(buf, 256);
    assert_int_equal(len, 3);
    assert_int_equal(buf[0], 2);
    recovered = get_sernumber(buf);
    assert_int_equal(recovered, 256);
}

static void test_ser_number_large_values(void)
{
    uchar buf[5];
    int len;
    int32_t recovered;
    
    /* Test: Value < 0x800000 (3 bytes value + 1 byte length = 4 total) */
    len = ser_number(buf, 0x8000);
    assert_int_equal(len, 4); // Total length: 1 byte length + 3 bytes value
    assert_int_equal(buf[0], 3); // Length field stores number of value bytes
    
    len = ser_number(buf, 0x7FFFFF);
    assert_int_equal(len, 4);
    assert_int_equal(buf[0], 3);
    recovered = get_sernumber(buf);
    assert_int_equal(recovered, 0x7FFFFF);
    
    len = ser_number(buf, 65536);
    assert_int_equal(len, 4);
    assert_int_equal(buf[0], 3);
    recovered = get_sernumber(buf);
    assert_int_equal(recovered, 65536);
}

static void test_ser_number_very_large_values(void)
{
    uchar buf[5];
    int len;
    int32_t recovered;
    
    /* Test: Value >= 0x800000 (4 bytes value + 1 byte length = 5 total) */
    len = ser_number(buf, 0x800000);
    assert_int_equal(len, 5); // Total length: 1 byte length + 4 bytes value
    assert_int_equal(buf[0], 4); // Length field stores number of value bytes
    
    len = ser_number(buf, 0x7FFFFFFF);
    assert_int_equal(len, 5);
    assert_int_equal(buf[0], 4);
    recovered = get_sernumber(buf);
    assert_int_equal(recovered, 0x7FFFFFFF);
    
    len = ser_number(buf, 16777216);
    assert_int_equal(len, 5);
    assert_int_equal(buf[0], 4);
    recovered = get_sernumber(buf);
    assert_int_equal(recovered, 16777216);
}

/* Test get_sernumber() - deserialize numbers from Bitcoin scripts
 * Returns the deserialized value
 */
static void test_get_sernumber_roundtrip(void)
{
    uchar buf[5];
    int32_t original, recovered;
    int len;
    
    /* Test round-trip: ser_number() → get_sernumber() */
    
    /* Test 1: Small value */
    original = 42;
    len = ser_number(buf, original);
    recovered = get_sernumber(buf);
    assert_int_equal(recovered, original);
    
    /* Test 2: Medium value */
    original = 0x1234;
    len = ser_number(buf, original);
    recovered = get_sernumber(buf);
    assert_int_equal(recovered, original);
    
    /* Test 3: Large value */
    original = 0x123456;
    len = ser_number(buf, original);
    recovered = get_sernumber(buf);
    assert_int_equal(recovered, original);
    
    /* Test 4: Very large value */
    original = 0x12345678;
    len = ser_number(buf, original);
    recovered = get_sernumber(buf);
    assert_int_equal(recovered, original);
    
    /* Test 5: Maximum positive value */
    original = 0x7FFFFFFF;
    len = ser_number(buf, original);
    recovered = get_sernumber(buf);
    assert_int_equal(recovered, original);
}

static void test_get_sernumber_edge_cases(void)
{
    uchar buf[5];
    int32_t value;
    
    /* Test 1: Zero */
    ser_number(buf, 0);
    value = get_sernumber(buf);
    assert_int_equal(value, 0);
    
    /* Test 2: One */
    ser_number(buf, 1);
    value = get_sernumber(buf);
    assert_int_equal(value, 1);
    
    /* Test 3: Boundary values */
    ser_number(buf, 0x7F);
    value = get_sernumber(buf);
    assert_int_equal(value, 0x7F);
    
    ser_number(buf, 0x80);
    value = get_sernumber(buf);
    assert_int_equal(value, 0x80);
    
    ser_number(buf, 0x7FFF);
    value = get_sernumber(buf);
    assert_int_equal(value, 0x7FFF);
    
    ser_number(buf, 0x8000);
    value = get_sernumber(buf);
    assert_int_equal(value, 0x8000);
    
    ser_number(buf, 0x7FFFFF);
    value = get_sernumber(buf);
    assert_int_equal(value, 0x7FFFFF);
    
    ser_number(buf, 0x800000);
    value = get_sernumber(buf);
    assert_int_equal(value, 0x800000);
}

static void test_get_sernumber_invalid_length(void)
{
    uchar buf[5];
    int32_t value;
    
    /* Test: Invalid length (0) */
    buf[0] = 0;
    value = get_sernumber(buf);
    assert_int_equal(value, 0); // Should return 0 for invalid length
    
    /* Test: Invalid length (> 4) */
    buf[0] = 5;
    value = get_sernumber(buf);
    assert_int_equal(value, 0); // Should return 0 for invalid length
    
    buf[0] = 10;
    value = get_sernumber(buf);
    assert_int_equal(value, 0); // Should return 0 for invalid length
}

static void test_ser_number_various_values(void)
{
    uchar buf[5];
    int32_t test_values[] = {
        0, 1, 42, 100, 255,
        256, 1000, 0x7FFF,
        0x8000, 65536, 0x7FFFFF,
        0x800000, 16777216, 0x7FFFFFFF
    };
    int num_tests = sizeof(test_values) / sizeof(test_values[0]);
    int i;
    
    for (i = 0; i < num_tests; i++) {
        int len = ser_number(buf, test_values[i]);
        int32_t recovered = get_sernumber(buf);
        
        assert_int_equal(recovered, test_values[i]);
        assert_true(len >= 2 && len <= 5); // Valid length range (1 byte length + 1-4 bytes value)
        assert_true(buf[0] >= 1 && buf[0] <= 4); // Length field stores number of value bytes (1-4)
    }
}

int main(void)
{
    printf("Running number serialization tests...\n\n");
    
    run_test(test_ser_number_small_values);
    run_test(test_ser_number_medium_values);
    run_test(test_ser_number_large_values);
    run_test(test_ser_number_very_large_values);
    run_test(test_get_sernumber_roundtrip);
    run_test(test_get_sernumber_edge_cases);
    run_test(test_get_sernumber_invalid_length);
    run_test(test_ser_number_various_values);
    
    printf("\nAll number serialization tests passed!\n");
    return 0;
}

