/*
 * Unit tests for time conversion functions
 * Tests us_to_tv(), ts_to_tv(), tv_to_ts(), timeraddspec(), ms_to_tv(), ms_to_ts()
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/time.h>
#include <time.h>

#include "config.h"
#include "../test_common.h"
#include "libckpool.h"

/* Test us_to_tv() - convert microseconds to timeval */
static void test_us_to_tv(void)
{
    tv_t tv;
    
    /* Test 1: Zero microseconds */
    us_to_tv(&tv, 0);
    assert_int_equal(tv.tv_sec, 0);
    assert_int_equal(tv.tv_usec, 0);
    
    /* Test 2: Less than 1 second */
    us_to_tv(&tv, 500000); // 0.5 seconds
    assert_int_equal(tv.tv_sec, 0);
    assert_int_equal(tv.tv_usec, 500000);
    
    /* Test 3: Exactly 1 second */
    us_to_tv(&tv, 1000000);
    assert_int_equal(tv.tv_sec, 1);
    assert_int_equal(tv.tv_usec, 0);
    
    /* Test 4: More than 1 second */
    us_to_tv(&tv, 2500000); // 2.5 seconds
    assert_int_equal(tv.tv_sec, 2);
    assert_int_equal(tv.tv_usec, 500000);
    
    /* Test 5: Large value */
    us_to_tv(&tv, 1234567890); // ~1234.567890 seconds
    assert_int_equal(tv.tv_sec, 1234);
    assert_int_equal(tv.tv_usec, 567890);
}

/* Test ms_to_tv() - convert milliseconds to timeval */
static void test_ms_to_tv(void)
{
    tv_t tv;
    
    /* Test 1: Zero milliseconds */
    ms_to_tv(&tv, 0);
    assert_int_equal(tv.tv_sec, 0);
    assert_int_equal(tv.tv_usec, 0);
    
    /* Test 2: Less than 1 second */
    ms_to_tv(&tv, 500); // 0.5 seconds
    assert_int_equal(tv.tv_sec, 0);
    assert_int_equal(tv.tv_usec, 500000);
    
    /* Test 3: Exactly 1 second */
    ms_to_tv(&tv, 1000);
    assert_int_equal(tv.tv_sec, 1);
    assert_int_equal(tv.tv_usec, 0);
    
    /* Test 4: More than 1 second */
    ms_to_tv(&tv, 2500); // 2.5 seconds
    assert_int_equal(tv.tv_sec, 2);
    assert_int_equal(tv.tv_usec, 500000);
}

/* Test ts_to_tv() - convert timespec to timeval */
static void test_ts_to_tv(void)
{
    ts_t ts;
    tv_t tv;
    
    /* Test 1: Zero time */
    ts.tv_sec = 0;
    ts.tv_nsec = 0;
    ts_to_tv(&tv, &ts);
    assert_int_equal(tv.tv_sec, 0);
    assert_int_equal(tv.tv_usec, 0);
    
    /* Test 2: Seconds only */
    ts.tv_sec = 5;
    ts.tv_nsec = 0;
    ts_to_tv(&tv, &ts);
    assert_int_equal(tv.tv_sec, 5);
    assert_int_equal(tv.tv_usec, 0);
    
    /* Test 3: With nanoseconds */
    ts.tv_sec = 1;
    ts.tv_nsec = 500000000; // 0.5 seconds in nanoseconds
    ts_to_tv(&tv, &ts);
    assert_int_equal(tv.tv_sec, 1);
    assert_int_equal(tv.tv_usec, 500000);
    
    /* Test 4: Nanoseconds less than 1 microsecond (should truncate) */
    ts.tv_sec = 1;
    ts.tv_nsec = 500; // 0.5 microseconds
    ts_to_tv(&tv, &ts);
    assert_int_equal(tv.tv_sec, 1);
    assert_int_equal(tv.tv_usec, 0); // Truncated
}

/* Test tv_to_ts() - convert timeval to timespec */
static void test_tv_to_ts(void)
{
    tv_t tv;
    ts_t ts;
    
    /* Test 1: Zero time */
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    tv_to_ts(&ts, &tv);
    assert_int_equal(ts.tv_sec, 0);
    assert_int_equal(ts.tv_nsec, 0);
    
    /* Test 2: Seconds only */
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    tv_to_ts(&ts, &tv);
    assert_int_equal(ts.tv_sec, 5);
    assert_int_equal(ts.tv_nsec, 0);
    
    /* Test 3: With microseconds */
    tv.tv_sec = 1;
    tv.tv_usec = 500000; // 0.5 seconds
    tv_to_ts(&ts, &tv);
    assert_int_equal(ts.tv_sec, 1);
    assert_int_equal(ts.tv_nsec, 500000000);
}

/* Test ts_to_tv() and tv_to_ts() round-trip */
static void test_tv_ts_roundtrip(void)
{
    tv_t tv_original, tv_result;
    ts_t ts;
    
    /* Test round-trip: tv -> ts -> tv */
    tv_original.tv_sec = 123;
    tv_original.tv_usec = 456789;
    
    tv_to_ts(&ts, &tv_original);
    ts_to_tv(&tv_result, &ts);
    
    assert_int_equal(tv_result.tv_sec, tv_original.tv_sec);
    /* Note: Some precision may be lost in conversion (nanoseconds -> microseconds) */
    assert_int_equal(tv_result.tv_usec, tv_original.tv_usec);
}

/* Test us_to_ts() - convert microseconds to timespec */
static void test_us_to_ts(void)
{
    ts_t ts;
    
    /* Test 1: Zero microseconds */
    us_to_ts(&ts, 0);
    assert_int_equal(ts.tv_sec, 0);
    assert_int_equal(ts.tv_nsec, 0);
    
    /* Test 2: Less than 1 second */
    us_to_ts(&ts, 500000); // 0.5 seconds
    assert_int_equal(ts.tv_sec, 0);
    assert_int_equal(ts.tv_nsec, 500000000);
    
    /* Test 3: Exactly 1 second */
    us_to_ts(&ts, 1000000);
    assert_int_equal(ts.tv_sec, 1);
    assert_int_equal(ts.tv_nsec, 0);
    
    /* Test 4: More than 1 second */
    us_to_ts(&ts, 2500000); // 2.5 seconds
    assert_int_equal(ts.tv_sec, 2);
    assert_int_equal(ts.tv_nsec, 500000000);
}

/* Test ms_to_ts() - convert milliseconds to timespec */
static void test_ms_to_ts(void)
{
    ts_t ts;
    
    /* Test 1: Zero milliseconds */
    ms_to_ts(&ts, 0);
    assert_int_equal(ts.tv_sec, 0);
    assert_int_equal(ts.tv_nsec, 0);
    
    /* Test 2: Less than 1 second */
    ms_to_ts(&ts, 500); // 0.5 seconds
    assert_int_equal(ts.tv_sec, 0);
    assert_int_equal(ts.tv_nsec, 500000000);
    
    /* Test 3: Exactly 1 second */
    ms_to_ts(&ts, 1000);
    assert_int_equal(ts.tv_sec, 1);
    assert_int_equal(ts.tv_nsec, 0);
    
    /* Test 4: More than 1 second */
    ms_to_ts(&ts, 2500); // 2.5 seconds
    assert_int_equal(ts.tv_sec, 2);
    assert_int_equal(ts.tv_nsec, 500000000);
}

/* Test timeraddspec() - add timespec values */
static void test_timeraddspec(void)
{
    ts_t a, b;
    
    /* Test 1: Add zero */
    a.tv_sec = 5;
    a.tv_nsec = 100000000;
    b.tv_sec = 0;
    b.tv_nsec = 0;
    timeraddspec(&a, &b);
    assert_int_equal(a.tv_sec, 5);
    assert_int_equal(a.tv_nsec, 100000000);
    
    /* Test 2: Add without overflow */
    a.tv_sec = 1;
    a.tv_nsec = 500000000;
    b.tv_sec = 2;
    b.tv_nsec = 300000000;
    timeraddspec(&a, &b);
    assert_int_equal(a.tv_sec, 3);
    assert_int_equal(a.tv_nsec, 800000000);
    
    /* Test 3: Add with nsec overflow */
    a.tv_sec = 1;
    a.tv_nsec = 600000000;
    b.tv_sec = 2;
    b.tv_nsec = 500000000;
    timeraddspec(&a, &b);
    assert_int_equal(a.tv_sec, 4); // 1 + 2 + 1 (from overflow)
    assert_int_equal(a.tv_nsec, 100000000); // (600 + 500) % 1000 = 100
    
    /* Test 4: Multiple overflows */
    a.tv_sec = 0;
    a.tv_nsec = 999999999;
    b.tv_sec = 0;
    b.tv_nsec = 1;
    timeraddspec(&a, &b);
    assert_int_equal(a.tv_sec, 1);
    assert_int_equal(a.tv_nsec, 0);
}

/* Test edge cases */
static void test_time_conversion_edge_cases(void)
{
    tv_t tv;
    ts_t ts;
    
    /* Test: Large values */
    us_to_tv(&tv, 1000000000LL); // 1 million microseconds = 1000 seconds
    assert_int_equal(tv.tv_sec, 1000);
    assert_int_equal(tv.tv_usec, 0);
    
    ms_to_tv(&tv, 1000000); // 1000 seconds
    assert_int_equal(tv.tv_sec, 1000);
    assert_int_equal(tv.tv_usec, 0);
    
    us_to_ts(&ts, 1000000000LL); // 1 million microseconds = 1000 seconds
    assert_int_equal(ts.tv_sec, 1000);
    assert_int_equal(ts.tv_nsec, 0);
    
    ms_to_ts(&ts, 1000000);
    assert_int_equal(ts.tv_sec, 1000);
    assert_int_equal(ts.tv_nsec, 0);
    
    /* Test: Large milliseconds value */
    ms_to_ts(&ts, 1234567); // 1234.567 seconds
    assert_int_equal(ts.tv_sec, 1234);
    assert_int_equal(ts.tv_nsec, 567000000);
}

int main(void)
{
    printf("Running time conversion tests...\n\n");
    
    run_test(test_us_to_tv);
    run_test(test_ms_to_tv);
    run_test(test_ts_to_tv);
    run_test(test_tv_to_ts);
    run_test(test_tv_ts_roundtrip);
    run_test(test_us_to_ts);
    run_test(test_ms_to_ts);
    run_test(test_timeraddspec);
    run_test(test_time_conversion_edge_cases);
    
    printf("\nAll time conversion tests passed!\n");
    return 0;
}

