/*
 * Unit tests for time utility functions
 * Tests time difference calculations and decay functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <sys/time.h>
#include <time.h>

#include "config.h"
#include "../test_common.h"
#include "libckpool.h"

static bool perf_tests_enabled(void)
{
    const char *val = getenv("CKPOOL_PERF_TESTS");

    return val && val[0] == '1';
}

/* Test us_tvdiff - microseconds time difference
 * Limits to 60 seconds max
 */
static void test_us_tvdiff(void)
{
    tv_t start, end;
    double diff;
    
    /* Test 1: Small difference (1 second) */
    start.tv_sec = 1000;
    start.tv_usec = 0;
    end.tv_sec = 1001;
    end.tv_usec = 0;
    diff = us_tvdiff(&end, &start);
    assert_double_equal(diff, 1000000.0, EPSILON); // 1 second = 1,000,000 microseconds
    
    /* Test 2: With microseconds */
    start.tv_sec = 1000;
    start.tv_usec = 500000; // 0.5 seconds
    end.tv_sec = 1001;
    end.tv_usec = 250000; // 0.25 seconds
    diff = us_tvdiff(&end, &start);
    assert_double_equal(diff, 750000.0, EPSILON); // 0.75 seconds = 750,000 microseconds
    
    /* Test 3: Very small difference */
    start.tv_sec = 1000;
    start.tv_usec = 0;
    end.tv_sec = 1000;
    end.tv_usec = 1000; // 1 millisecond
    diff = us_tvdiff(&end, &start);
    assert_double_equal(diff, 1000.0, EPSILON);
    
    /* Test 4: Maximum limit (60 seconds) */
    start.tv_sec = 1000;
    start.tv_usec = 0;
    end.tv_sec = 1070; // 70 seconds later
    end.tv_usec = 0;
    diff = us_tvdiff(&end, &start);
    assert_double_equal(diff, 60000000.0, EPSILON); // Should be clamped to 60 seconds
}

/* Test ms_tvdiff - milliseconds time difference
 * Limits to 1 hour max
 */
static void test_ms_tvdiff(void)
{
    tv_t start, end;
    int diff;
    
    /* Test 1: Small difference (1 second) */
    start.tv_sec = 1000;
    start.tv_usec = 0;
    end.tv_sec = 1001;
    end.tv_usec = 0;
    diff = ms_tvdiff(&end, &start);
    assert_int_equal(diff, 1000); // 1 second = 1,000 milliseconds
    
    /* Test 2: With microseconds */
    start.tv_sec = 1000;
    start.tv_usec = 500000; // 0.5 seconds
    end.tv_sec = 1001;
    end.tv_usec = 250000; // 0.25 seconds
    diff = ms_tvdiff(&end, &start);
    assert_int_equal(diff, 750); // 0.75 seconds = 750 milliseconds
    
    /* Test 3: Maximum limit (1 hour) */
    start.tv_sec = 1000;
    start.tv_usec = 0;
    end.tv_sec = 1000 + 7200; // 2 hours later
    end.tv_usec = 0;
    diff = ms_tvdiff(&end, &start);
    assert_int_equal(diff, 3600000); // Should be clamped to 1 hour (3,600,000 ms)
}

/* Test tvdiff - seconds time difference as double
 * No limits, returns exact difference
 */
static void test_tvdiff(void)
{
    tv_t start, end;
    double diff;
    
    /* Test 1: Small difference */
    start.tv_sec = 1000;
    start.tv_usec = 0;
    end.tv_sec = 1001;
    end.tv_usec = 0;
    diff = tvdiff(&end, &start);
    assert_double_equal(diff, 1.0, EPSILON);
    
    /* Test 2: With microseconds */
    start.tv_sec = 1000;
    start.tv_usec = 500000; // 0.5 seconds
    end.tv_sec = 1001;
    end.tv_usec = 250000; // 0.25 seconds
    diff = tvdiff(&end, &start);
    assert_double_equal(diff, 0.75, EPSILON);
    
    /* Test 3: Large difference (no clamping) */
    start.tv_sec = 1000;
    start.tv_usec = 0;
    end.tv_sec = 2000;
    end.tv_usec = 0;
    diff = tvdiff(&end, &start);
    assert_double_equal(diff, 1000.0, EPSILON); // 1000 seconds, not clamped
}

/* Test sane_tdiff - sanitized time difference
 * Ensures minimum of 0.001 seconds
 */
static void test_sane_tdiff(void)
{
    tv_t start, end;
    double diff;
    
    /* Test 1: Normal difference */
    start.tv_sec = 1000;
    start.tv_usec = 0;
    end.tv_sec = 1001;
    end.tv_usec = 0;
    diff = sane_tdiff(&end, &start);
    assert_double_equal(diff, 1.0, EPSILON);
    
    /* Test 2: Very small difference (should be clamped to 0.001) */
    start.tv_sec = 1000;
    start.tv_usec = 0;
    end.tv_sec = 1000;
    end.tv_usec = 500; // 0.0005 seconds
    diff = sane_tdiff(&end, &start);
    assert_double_equal(diff, 0.001, EPSILON); // Should be clamped to minimum
    
    /* Test 3: Zero difference (should be clamped) */
    start.tv_sec = 1000;
    start.tv_usec = 0;
    end.tv_sec = 1000;
    end.tv_usec = 0;
    diff = sane_tdiff(&end, &start);
    assert_double_equal(diff, 0.001, EPSILON); // Should be clamped to minimum
    
    /* Test 4: Negative difference (should be clamped) */
    start.tv_sec = 1000;
    start.tv_usec = 0;
    end.tv_sec = 999;
    end.tv_usec = 0;
    diff = sane_tdiff(&end, &start);
    assert_double_equal(diff, 0.001, EPSILON); // Should be clamped to minimum
}

/* Test decay_time - exponential decay calculation
 * Formula: fprop = 1.0 - 1/exp(fsecs/interval)
 *          *f += (fadd / fsecs * fprop)
 *          *f /= (1.0 + fprop)
 */
static void test_decay_time(void)
{
    double f;
    double fadd, fsecs, interval;
    
    /* Test 1: Basic decay with positive values */
    f = 100.0;
    fadd = 50.0;
    fsecs = 60.0; // 1 minute
    interval = 60.0; // 1 minute interval
    decay_time(&f, fadd, fsecs, interval);
    /* f should be updated based on exponential decay formula */
    assert_true(f >= 0.0); // Should be non-negative
    assert_true(f < 200.0); // Should be less than sum
    
    /* Test 2: Decay with zero fadd (just decay, no addition) */
    f = 100.0;
    fadd = 0.0;
    fsecs = 60.0;
    interval = 60.0;
    double f_before = f;
    decay_time(&f, fadd, fsecs, interval);
    assert_true(f < f_before); // Should decay (decrease)
    assert_true(f >= 0.0);
    
    /* Test 3: Very small time (should handle gracefully) */
    f = 100.0;
    fadd = 10.0;
    fsecs = 0.1; // 0.1 seconds
    interval = 60.0;
    decay_time(&f, fadd, fsecs, interval);
    assert_true(f >= 0.0);
    
    /* Test 4: fsecs <= 0 (should return early) */
    f = 100.0;
    fadd = 10.0;
    fsecs = 0.0;
    interval = 60.0;
    double f_unchanged = f;
    decay_time(&f, fadd, fsecs, interval);
    assert_double_equal(f, f_unchanged, EPSILON); // Should be unchanged
    
    /* Test 5: Very large dexp (should be clamped to 36) */
    f = 100.0;
    fadd = 10.0;
    fsecs = 3600.0; // 1 hour
    interval = 1.0; // 1 second (very small interval)
    decay_time(&f, fadd, fsecs, interval);
    assert_true(f >= 0.0);
    
    /* Test 6: Very small result (should be clamped to 0 if < 2E-16) */
    f = 1E-20; // Very small value
    fadd = 0.0;
    fsecs = 60.0;
    interval = 60.0;
    decay_time(&f, fadd, fsecs, interval);
    assert_true(f >= 0.0);
    /* May be clamped to 0 if below threshold */
}

/* Test decay_time with various intervals (common use cases) */
static void test_decay_time_intervals(void)
{
    double f;
    double fadd = 10.0;
    double fsecs = 60.0; // 1 minute
    
    /* Test with 1 minute interval */
    f = 100.0;
    decay_time(&f, fadd, fsecs, 60.0); // MIN1
    assert_true(f >= 0.0);
    
    /* Test with 5 minute interval */
    f = 100.0;
    decay_time(&f, fadd, fsecs, 300.0); // MIN5
    assert_true(f >= 0.0);
    
    /* Test with 1 hour interval */
    f = 100.0;
    decay_time(&f, fadd, fsecs, 3600.0); // HOUR
    assert_true(f >= 0.0);
    
    /* Test with 1 day interval */
    f = 100.0;
    decay_time(&f, fadd, fsecs, 86400.0); // DAY
    assert_true(f >= 0.0);
}

/* Test edge cases for time functions */
static void test_time_edge_cases(void)
{
    tv_t start, end;
    double diff;
    
    /* Test us_tvdiff with exactly 60 seconds */
    start.tv_sec = 1000;
    start.tv_usec = 0;
    end.tv_sec = 1060; // Exactly 60 seconds
    end.tv_usec = 0;
    diff = us_tvdiff(&end, &start);
    assert_double_equal(diff, 60000000.0, EPSILON);
    
    /* Test ms_tvdiff with exactly 1 hour */
    start.tv_sec = 1000;
    start.tv_usec = 0;
    end.tv_sec = 1000 + 3600; // Exactly 1 hour
    end.tv_usec = 0;
    diff = ms_tvdiff(&end, &start);
    assert_int_equal(diff, 3600000);
    
    /* Test tvdiff with microseconds wrapping */
    start.tv_sec = 1000;
    start.tv_usec = 800000;
    end.tv_sec = 1001;
    end.tv_usec = 200000;
    diff = tvdiff(&end, &start);
    assert_double_equal(diff, 0.4, EPSILON); // 0.4 seconds
}

/* ========================================================================
 * SECTION 2: Time Conversion Functions
 * From: test-time-conversion.c
 * Tests: 9 tests for time format conversion functions
 * ======================================================================== */

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

/* Test edge cases for time conversions */
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

/* Performance test: time utils hot loops */
static void test_time_utils_performance(void)
{
    tv_t start, end;
    start.tv_sec = 1000;
    start.tv_usec = 100;
    end.tv_sec = 1001;
    end.tv_usec = 900;

    const int iterations = 2000000;
    clock_t cstart = clock();
    for (int i = 0; i < iterations; i++) {
        (void)us_tvdiff(&end, &start);
        (void)ms_tvdiff(&end, &start);
        (void)tvdiff(&end, &start);
    }
    clock_t cend = clock();

    double elapsed = (double)(cend - cstart) / CLOCKS_PER_SEC;
    if (elapsed > 0.0) {
        double ops_per_sec = (double)(iterations * 3) / elapsed;
        printf("    time utils: %.2fM ops/sec (%.3f sec for %d ops)\n",
               ops_per_sec / 1e6, elapsed, iterations * 3);
    }

    assert_true(elapsed < 5.0);
}

int main(void)
{
    printf("Running time utility tests...\n\n");
    
    printf("=== Section 1: Time Difference and Decay Functions ===\n");
    run_test(test_us_tvdiff);
    run_test(test_ms_tvdiff);
    run_test(test_tvdiff);
    run_test(test_sane_tdiff);
    run_test(test_decay_time);
    run_test(test_decay_time_intervals);
    run_test(test_time_edge_cases);
    
    printf("\n=== Section 2: Time Conversion Functions ===\n");
    run_test(test_us_to_tv);
    run_test(test_ms_to_tv);
    run_test(test_ts_to_tv);
    run_test(test_tv_to_ts);
    run_test(test_tv_ts_roundtrip);
    run_test(test_us_to_ts);
    run_test(test_ms_to_ts);
    run_test(test_timeraddspec);
    run_test(test_time_conversion_edge_cases);

    if (perf_tests_enabled()) {
        printf("\n[PERFORMANCE REGRESSION TESTS]\n");
        printf("BEGIN PERF TESTS: test-time-utils\n");
        run_test(test_time_utils_performance);
        printf("END PERF TESTS: test-time-utils\n");
    }
    
    printf("\nAll time utility tests passed!\n");
    return 0;
}

