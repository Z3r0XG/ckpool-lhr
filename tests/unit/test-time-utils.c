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

#include "config.h"
#include "../test_common.h"
#include "libckpool.h"

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

int main(void)
{
    printf("Running time utility tests...\n\n");
    
    run_test(test_us_tvdiff);
    run_test(test_ms_tvdiff);
    run_test(test_tvdiff);
    run_test(test_sane_tdiff);
    run_test(test_decay_time);
    run_test(test_decay_time_intervals);
    run_test(test_time_edge_cases);
    
    printf("\nAll time utility tests passed!\n");
    return 0;
}

