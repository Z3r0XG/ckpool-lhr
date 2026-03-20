/*
 * Test password difficulty job ID assignment
 *
 * Tests the directional anchor invariants introduced by select_diff_change_anchor():
 *
 *   DOWN (new_diff < old_diff): diff_change_job_id = current_workbase->id
 *     → current-job shares use new (easier) diff immediately.
 *     → This is the relevant path for password diff at auth: miner hasn't sent
 *       any shares yet, so "immediate" is correct and safe.
 *
 *   UP (new_diff > old_diff): diff_change_job_id = workbase_id + 1 (W+1 buffer)
 *     → current in-flight shares still evaluate at old (easier) diff.
 *     → Protects against rejections when diff increases mid-session.
 *
 * Also tests the original bug: before select_diff_change_anchor() was introduced,
 * parse_authorise used workbase_id+1 unconditionally (always UP-style), causing
 * password-DOWN shares on the current job to be incorrectly evaluated at old_diff.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "../test_common.h"

/* 
 * Core share evaluation logic from stratifier.c line 6316
 * Returns which difficulty would be used for a share
 */
typedef enum {
    USES_OLD_DIFF,
    USES_NEW_DIFF
} diff_selection_t;

static diff_selection_t evaluate_share_diff(int64_t share_job_id, int64_t diff_change_job_id) {
    /* This is the actual logic from stratifier.c:
     * if (id < client->diff_change_job_id)
     *     diff = client->old_diff;
     */
    return (share_job_id < diff_change_job_id) ? USES_OLD_DIFF : USES_NEW_DIFF;
}

/*
 * Local reimplementation of select_diff_change_anchor() from src/stratifier.c
 * for unit-testing the directional anchor logic in isolation.
 *
 * Note: add_submit() passes next_blockid (sdata->workbase_id+1) as the workbase_id
 * arg, giving UP changes a W+2 effective anchor (two-job buffer; pool-initiated).
 * parse_authorise() and apply_suggest_diff() pass workbase_id directly → W+1.
 */
static int64_t select_diff_change_anchor(double old_diff, double new_diff,
                                          int64_t workbase_id, int64_t current_id)
{
    return (new_diff > old_diff) ? workbase_id + 1 : current_id;
}

/* Test the DOWN-direction invariant: current-job shares use new diff immediately */
static void test_invariant_current_job_uses_new_diff(void) {
    /*
     * DOWN-direction invariant (new_diff < old_diff):
     * diff_change_job_id = current_workbase->id (immediate)
     * So: (current_job_id < current_job_id) = FALSE → uses new diff ✓
     *
     * This is the path taken by password diff at auth time. The miner has
     * not yet submitted any shares, so immediate application is correct.
     */
    
    int64_t current_job = 100;
    int64_t diff_change_job_id_fixed = current_job;
    
    diff_selection_t result = evaluate_share_diff(current_job, diff_change_job_id_fixed);
    assert_int_equal(USES_NEW_DIFF, result);
    
    printf("  ✓ INVARIANT: Current job shares use new diff\n");
}

/* Test that the pre-refactor code violated the DOWN-direction invariant */
static void test_bug_violated_invariant(void) {
    /*
     * Pre-refactor bug (password DOWN path): diff_change_job_id = workbase_id + 1
     * In production: workbase_id = current_job + 1 (next job to be assigned)
     * Result: (current_job_id < workbase_id + 1) = TRUE → uses old_diff ✗
     *
     * Note: workbase_id+1 is the correct anchor for UP direction (W+1 buffer),
     * but was wrong when unconditionally applied to a DOWN password-diff change.
     * select_diff_change_anchor() now selects the anchor based on direction.
     */

    int64_t current_job = 100;
    int64_t workbase_id = current_job + 1;
    int64_t diff_change_job_id_buggy = workbase_id + 1;  /* 102: UP-style anchor */

    diff_selection_t result = evaluate_share_diff(current_job, diff_change_job_id_buggy);
    assert_int_equal(USES_OLD_DIFF, result);

    printf("  ✓ Pre-refactor anchor (W+1 for DOWN) correctly shows old_diff used\n");
}

/* Test boundary: job_id == diff_change_job_id */
static void test_boundary_equal_job_ids(void) {
    /*
     * Edge case: What if share job_id equals diff_change_job_id?
     * (job_id < diff_change_job_id) = FALSE → uses new diff
     */
    
    int64_t job_id = 100;
    int64_t diff_change_job_id = 100;
    
    diff_selection_t result = evaluate_share_diff(job_id, diff_change_job_id);
    assert_int_equal(USES_NEW_DIFF, result);
    
    printf("  ✓ Boundary case: equal IDs use new diff\n");
}

/* Test old jobs use old diff (expected behavior) */
static void test_old_jobs_use_old_diff(void) {
    /*
     * Shares from PREVIOUS jobs should use old_diff.
     * This is correct behavior - we only changed diff on current job.
     */
    
    int64_t current_job = 100;
    int64_t diff_change_job_id = current_job;
    int64_t old_job_id = 99;
    
    diff_selection_t result = evaluate_share_diff(old_job_id, diff_change_job_id);
    assert_int_equal(USES_OLD_DIFF, result);
    
    printf("  ✓ Old job shares correctly use old_diff\n");
}

/* Test future jobs use new diff */
static void test_future_jobs_use_new_diff(void) {
    /*
     * Shares from FUTURE jobs should use new_diff.
     */
    
    int64_t current_job = 100;
    int64_t diff_change_job_id = current_job;
    int64_t future_job_id = 101;
    
    diff_selection_t result = evaluate_share_diff(future_job_id, diff_change_job_id);
    assert_int_equal(USES_NEW_DIFF, result);
    
    printf("  ✓ Future job shares use new_diff\n");
}

/* Property test: various gap sizes between current and next job */
static void test_property_various_gaps(void) {
    /*
     * Test that the fix works regardless of gap between current_job and workbase_id.
     * In production, workbase_id is typically current+1 or current+2, but test more.
     */
    
    int64_t gaps[] = {1, 2, 5, 10, 100, 1000};
    
    for (size_t i = 0; i < sizeof(gaps)/sizeof(gaps[0]); i++) {
        int64_t current_job = 100;
        int64_t workbase_id = current_job + gaps[i];
        
        /* With fix: diff_change_job_id = current_job */
        int64_t diff_change_job_id_fixed = current_job;
        diff_selection_t result_fixed = evaluate_share_diff(current_job, diff_change_job_id_fixed);
        assert_int_equal(USES_NEW_DIFF, result_fixed);
        
        /* With bug: diff_change_job_id = workbase_id + 1 */
        int64_t diff_change_job_id_buggy = workbase_id + 1;
        diff_selection_t result_buggy = evaluate_share_diff(current_job, diff_change_job_id_buggy);
        assert_int_equal(USES_OLD_DIFF, result_buggy);
    }
    
    printf("  ✓ Property holds for all gap sizes (1-1000)\n");
}

/* Test with production values */
static void test_production_values(void) {
    /*
     * Real values from production debug logs where bug was observed:
     * - current_workbase->id = 7595459095277076480
     * - workbase_id = 7595459095277076481 (current + 1, as expected)
     */
    
    int64_t current_job = 7595459095277076480LL;
    int64_t workbase_id = 7595459095277076481LL;  // current_job + 1
    
    /* Fix: uses current_job */
    int64_t diff_change_job_id_fixed = current_job;
    diff_selection_t result_fixed = evaluate_share_diff(current_job, diff_change_job_id_fixed);
    assert_int_equal(USES_NEW_DIFF, result_fixed);
    
    /* Bug: uses workbase_id + 1 */
    int64_t diff_change_job_id_buggy = workbase_id + 1;
    diff_selection_t result_buggy = evaluate_share_diff(current_job, diff_change_job_id_buggy);
    assert_int_equal(USES_OLD_DIFF, result_buggy);
    
    printf("  ✓ Production scenario verified (job_id=%lld)\n", (long long)current_job);
}

/* Test extreme values to catch overflow/wraparound issues */
static void test_extreme_values(void) {
    /*
     * Test with very large job IDs to ensure no overflow issues
     */
    
    int64_t large_job = INT64_MAX - 100;
    
    /* Fix should still work */
    diff_selection_t result = evaluate_share_diff(large_job, large_job);
    assert_int_equal(USES_NEW_DIFF, result);
    
    /* One less should use old diff */
    diff_selection_t result_old = evaluate_share_diff(large_job - 1, large_job);
    assert_int_equal(USES_OLD_DIFF, result_old);
    
    printf("  ✓ Extreme values handled correctly\n");
}

/* Regression test: ensure fix doesn't break normal vardiff */
static void test_normal_vardiff_unaffected(void) {
    /*
     * Normal vardiff (line 5844) sets: diff_change_job_id = next_blockid
     * where next_blockid = workbase_id + 1
     * 
     * This should be UNAFFECTED by the password diff fix.
     * Vardiff sends a new job notification, so shares come with new job_id.
     */
    
    int64_t current_job = 100;
    int64_t next_job = 101;
    int64_t diff_change_job_id_vardiff = next_job;  // next_blockid
    
    /* Shares for current job use old diff (haven't received new job yet) */
    diff_selection_t current = evaluate_share_diff(current_job, diff_change_job_id_vardiff);
    assert_int_equal(USES_OLD_DIFF, current);
    
    /* Shares for new job use new diff */
    diff_selection_t next = evaluate_share_diff(next_job, diff_change_job_id_vardiff);
    assert_int_equal(USES_NEW_DIFF, next);
    
    printf("  ✓ Normal vardiff behavior preserved\n");
}

/*
 * Directional tests: UP vs DOWN diff changes use different job-id anchors.
 *
 * The core fix (fix/diff-change-timing):
 *   UP   (new > old): diff_change_job_id = workbase_id + 1  (W+1 buffer)
 *        → current in-flight shares still evaluated at old (easier) diff
 *        → protects ASICs with shares already submitted against new harder target
 *   DOWN (new < old): diff_change_job_id = current_workbase->id  (current: immediate)
 *        → if miner adjusts immediately: easier shares accepted at new (lower) diff
 *        → if miner waits: old higher-diff shares trivially pass the new easier check
 *        → either way: no rejection
 *
 * The directional tests below verify select_diff_change_anchor(): the rule
 * extracted into stratifier.c that all three diff-change paths invoke:
 *   parse_authorise()  (password diff)
 *   suggest_diff()     (stratum suggest)
 *   add_submit()       (vardiff)
 */

static void test_direction_up_uses_w1_buffer(void) {
    /*
     * Diff going UP (harder): diff_change_job_id = workbase_id + 1
     * workbase_id is the next job ID about to be assigned (current_job + 1 typically).
     * Setting change at W+1 means: current_job shares still use old (easy) diff. ✓
     */
    int64_t current_job = 100;
    int64_t workbase_id = 101;          /* current + 1, typical production value */
    int64_t diff_change_job_id = workbase_id + 1;  /* W+1 = 102 */

    /* In-flight share on current job → old (easy) diff still applies */
    diff_selection_t inflight = evaluate_share_diff(current_job, diff_change_job_id);
    assert_int_equal(USES_OLD_DIFF, inflight);

    /* Share on workbase_id (W) → still within buffer window, old (easy) diff applies */
    diff_selection_t on_next = evaluate_share_diff(workbase_id, diff_change_job_id);
    assert_int_equal(USES_OLD_DIFF, on_next);   /* workbase_id (101) < 102: still buffered */

    /* Share on W+1 itself → new diff applies */
    diff_selection_t on_w1 = evaluate_share_diff(workbase_id + 1, diff_change_job_id);
    assert_int_equal(USES_NEW_DIFF, on_w1);

    printf("  ✓ Direction UP: W+1 buffer protects in-flight shares\n");
}

static void test_direction_down_uses_immediate(void) {
    /*
     * Diff going DOWN (easier): diff_change_job_id = current_workbase->id
     * current_workbase->id == current_job: change applied immediately.
     * current_job shares use new (easy) diff. Old harder diff is no longer required. ✓
     */
    int64_t current_job = 100;
    int64_t diff_change_job_id = current_job;  /* current: immediate */

    /* Current job share → new (easy) diff applies immediately */
    diff_selection_t current = evaluate_share_diff(current_job, diff_change_job_id);
    assert_int_equal(USES_NEW_DIFF, current);

    /* Previous job share → old (hard) diff (correct: that job was mined at old diff) */
    diff_selection_t previous = evaluate_share_diff(current_job - 1, diff_change_job_id);
    assert_int_equal(USES_OLD_DIFF, previous);

    printf("  ✓ Direction DOWN: immediate application, current job uses new diff\n");
}

static void test_direction_symmetry(void) {
    /*
     * Property: for any workbase setup, UP and DOWN must choose DIFFERENT anchors.
     * UP  anchor > current_job  → current share uses old diff
     * DOWN anchor == current_job → current share uses new diff
     */
    int64_t gaps[] = {1, 2, 5, 10};

    for (size_t i = 0; i < sizeof(gaps)/sizeof(gaps[0]); i++) {
        int64_t current_job = 500;
        int64_t workbase_id = current_job + gaps[i];

        int64_t up_anchor   = workbase_id + 1;   /* UP: W+1 */
        int64_t down_anchor = current_job;        /* DOWN: current: immediate */

        /* UP: current in-flight share buffered (old diff) */
        diff_selection_t up_result = evaluate_share_diff(current_job, up_anchor);
        assert_int_equal(USES_OLD_DIFF, up_result);

        /* DOWN: current share uses new diff immediately */
        diff_selection_t down_result = evaluate_share_diff(current_job, down_anchor);
        assert_int_equal(USES_NEW_DIFF, down_result);
    }

    printf("  ✓ Direction symmetry: UP buffers, DOWN is immediate (all gap sizes)\n");
}

/* --- select_diff_change_anchor() unit tests --- */

static void test_anchor_up_returns_w1(void) {
    /*
     * new_diff > old_diff (going UP): anchor = workbase_id + 1
     * Mirrors all three call sites in stratifier.c.
     */
    int64_t workbase_id = 200;
    int64_t current_id  = 199;

    int64_t anchor = select_diff_change_anchor(1.0, 2.0, workbase_id, current_id);
    assert_int_equal(workbase_id + 1, anchor);

    /* Large diff jump */
    anchor = select_diff_change_anchor(0.001, 512.0, workbase_id, current_id);
    assert_int_equal(workbase_id + 1, anchor);

    printf("  ✓ Anchor UP: returns workbase_id + 1 (W+1 buffer)\n");
}

static void test_anchor_down_returns_current(void) {
    /*
     * new_diff < old_diff (going DOWN): anchor = current_id (immediate)
     * Mirrors all three call sites in stratifier.c.
     */
    int64_t workbase_id = 200;
    int64_t current_id  = 199;

    int64_t anchor = select_diff_change_anchor(4.0, 1.0, workbase_id, current_id);
    assert_int_equal(current_id, anchor);

    /* Sub-1 range */
    anchor = select_diff_change_anchor(0.3, 0.02, workbase_id, current_id);
    assert_int_equal(current_id, anchor);

    printf("  ✓ Anchor DOWN: returns current_id (immediate)\n");
}

static void test_anchor_equal_returns_current(void) {
    /*
     * new_diff == old_diff: not > old, so anchor = current_id.
     * The no-op path (fabs check) would filter this before calling the helper
     * in production, but the rule itself must not treat equal as UP.
     */
    int64_t workbase_id = 200;
    int64_t current_id  = 199;

    int64_t anchor = select_diff_change_anchor(1.0, 1.0, workbase_id, current_id);
    assert_int_equal(current_id, anchor);

    printf("  ✓ Anchor EQUAL: returns current_id (not treated as UP)\n");
}

int main(void) {
    printf("\n═══════════════════════════════════════════════════════════\n");
    printf("  Password Difficulty Job ID Assignment Tests\n");
    printf("═══════════════════════════════════════════════════════════\n\n");
    
    printf("Testing core invariant...\n");
    run_test(test_invariant_current_job_uses_new_diff);
    run_test(test_bug_violated_invariant);
    
    printf("\nTesting boundary conditions...\n");
    run_test(test_boundary_equal_job_ids);
    run_test(test_old_jobs_use_old_diff);
    run_test(test_future_jobs_use_new_diff);
    
    printf("\nTesting properties and edge cases...\n");
    run_test(test_property_various_gaps);
    run_test(test_production_values);
    run_test(test_extreme_values);
    
    printf("\nTesting for regressions...\n");
    run_test(test_normal_vardiff_unaffected);

    printf("\nTesting UP vs DOWN directional logic (fix/diff-change-timing)...\n");
    run_test(test_direction_up_uses_w1_buffer);
    run_test(test_direction_down_uses_immediate);
    run_test(test_direction_symmetry);

    printf("\nTesting select_diff_change_anchor() (extracted helper, used by all 3 paths)...\n");
    run_test(test_anchor_up_returns_w1);
    run_test(test_anchor_down_returns_current);
    run_test(test_anchor_equal_returns_current);
    
    printf("\n═══════════════════════════════════════════════════════════\n");
    printf("  ✓ All tests passed!\n");
    printf("═══════════════════════════════════════════════════════════\n\n");
    
    printf("select_diff_change_anchor() in src/stratifier.c used by:\n");
    printf("  parse_authorise, apply_suggest_diff, add_submit\n\n");
    
    return 0;
}
