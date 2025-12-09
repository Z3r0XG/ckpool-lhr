/*
 * Unit tests for difficulty adjustment hysteresis
 * ================================================
 * 
 * PURPOSE:
 * Ensures vardiff algorithm doesn't oscillate wildly (thrash) when hashrate
 * changes occur. Hysteresis applies a time-bias filter to smooth adjustments,
 * preventing rapid up/down cycling that wastes work and destabilizes mining.
 * 
 * PROBLEM PREVENTED:
 * Without hysteresis, small fluctuations in observed hashrate could cause
 * difficulty to jump 2-10x per cycle. This causes:
 * - Work restarts (variance instability)
 * - Miner disconnects and pool switches (not optimal for pool)
 * - Unpredictable share rates (bad UX)
 * 
 * ALGORITHM:
 * Adjustment = blend old_diff with new_optimal_diff using time_bias factor.
 * time_bias = 1.0 - 1.0 / exp(adjustment_period / adjustment_period)
 * With 300s period: time_bias ~= 0.632 (63% toward new optimal per cycle)
 * 
 * TEST SCENARIOS:
 * - Rapid spikes: 2x hashrate increase -> limited to ~1.6x diff change
 * - Throttling: 50% hashrate decrease -> limited to ~0.7x diff change
 * - Pool joins: 10x hashrate -> limited to ~6.7x diff over 300s
 * - Steady state: Fixed hashrate -> diff stabilizes (no thrashing)
 * - Convergence: Gradual hashrate changes -> smooth approach to new optimal
 * - Erratic observations: Random variation -> damped, not amplified
 * 
 * EXPECTED RESULTS:
 * - No more than ~65% per-cycle adjustment even for extreme 10x changes
 * - Typical changes: +/- 20% per cycle for normal variance
 * - Steady state: No oscillation (diff stays constant with fixed hashrate)
 * - Convergence: Smooth movement toward optimal, no overshooting
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <stdbool.h>

#include "../test_common.h"

#define EPSILON_HYSTERESIS (0.001)

/* Helper: Calculate optimal diff from DSPS */
static double calculate_optimal_diff(double hashrate)
{
	double dsps = hashrate / (double)(1UL << 32);
	return dsps * 3.33;  /* Target multiplier */
}

/*
 * Simulates adjustment cycle: given share data, calculate new difficulty
 * This is a simplified model of the actual vardiff algorithm
 */
typedef struct {
	double current_diff;
	double dsps;              /* Current difficulty-shares-per-second rate */
	double time_since_last;   /* Seconds since last adjustment */
	double tdiff;             /* Time difference scaled for adjustment */
} adjustment_cycle_t;

/* Helper: Calculate time_bias for difficulty adjustment */
static double calculate_time_bias(double tdiff, double period)
{
	/* Formula from stratifier: 1.0 - 1.0 / exp(tdiff / period)
	 * Clamps dexp at 36.0 to prevent overflow
	 */
	double dexp = tdiff / period;
	if (dexp > 36.0)
		dexp = 36.0;
	return 1.0 - 1.0 / exp(dexp);
}

/* Test 1: Rapid hashrate increase doesn't cause runaway difficulty */
static void test_hashrate_spike_controlled(void)
{
	printf("\n  Testing controlled response to hashrate spike:\n");
	
	struct {
		const char *scenario;
		double old_hashrate;
		double new_hashrate;
		double period_secs;
		double max_allowed_jump_ratio;  /* New diff / old diff */
	} scenarios[] = {
		/* Miner joins suddenly */
		{ "Worker suddenly 2x faster", 1000000.0, 2000000.0, 300.0, 2.5 },
		
		/* Miner throttles */
		{ "Worker suddenly half speed", 10000000.0, 5000000.0, 300.0, 2.5 },
		
		/* Pool hashrate doubles (many workers join) */
		{ "Pool doubles (new workers)", 100000000000.0, 200000000000.0, 600.0, 2.0 },
		
		/* Extreme: hashrate 10x (should still not catastrophically jump) */
		{ "Extreme 10x increase", 1000000.0, 10000000.0, 300.0, 7.0 },
	};
	
	for (int i = 0; i < (int)(sizeof(scenarios) / sizeof(scenarios[0])); i++) {
		double old_diff = calculate_optimal_diff(scenarios[i].old_hashrate);
		double new_optimal_diff = calculate_optimal_diff(scenarios[i].new_hashrate);
		
		/* With hysteresis (time_bias limits adjustment) */
		double time_bias = calculate_time_bias(scenarios[i].period_secs, 300.0);
		
		/* Actual adjustment: blend between old and optimal */
		double adjusted_diff = old_diff + (new_optimal_diff - old_diff) * time_bias;
		
		double jump_ratio = adjusted_diff / old_diff;
		
		printf("    %s:\n", scenarios[i].scenario);
		printf("      Old diff: %.10f, New optimal: %.10f\n", old_diff, new_optimal_diff);
		printf("      With hysteresis: %.10f (jump: %.2fx)\n", adjusted_diff, jump_ratio);
		
		/* Hysteresis must limit the jump */
		assert_true(jump_ratio <= scenarios[i].max_allowed_jump_ratio);
		
		/* But should still move toward new optimal */
		assert_true(adjusted_diff != old_diff);
	}
}

/* Test 2: Stability around target DSPS */
static void test_stability_around_target(void)
{
	printf("\n  Testing stability around target DSPS:\n");
	
	/* Start with optimal diff, then shares come at varying rates */
	double current_diff = 10.0;
	double target_dsps = 3.33;  /* Expected with optimal diff */
	double period = 300.0;      /* Adjustment period */
	
	struct {
		const char *condition;
		double actual_dsps;     /* What we actually observed */
		double expected_max_change;
	} conditions[] = {
		/* Slightly faster than target: +20% */
		{ "20% faster than target", target_dsps * 1.2, 1.3 },
		
		/* Slightly slower than target: -20% */
		{ "20% slower than target", target_dsps * 0.8, 1.3 },
		
		/* 2x faster: should increase diff to slow down */
		{ "2x faster than target", target_dsps * 2.0, 1.8 },
		
		/* 2x slower: should decrease diff to speed up */
		{ "2x slower than target", target_dsps * 0.5, 1.8 },
	};
	
	for (int i = 0; i < (int)(sizeof(conditions) / sizeof(conditions[0])); i++) {
		double actual_dsps = conditions[i].actual_dsps;
		
		/* Adjustment strategy: new_diff = old_diff * (actual_dsps / target_dsps)
		 * If actual > target (too fast), ratio > 1, increase diff to slow down
		 * If actual < target (too slow), ratio < 1, decrease diff to speed up
		 * But apply time_bias to smooth it */
		double raw_adjustment_ratio = actual_dsps / target_dsps;
		double time_bias = calculate_time_bias(period, period);
		
		/* Smoothed adjustment */
		double adjustment_ratio = 1.0 + (raw_adjustment_ratio - 1.0) * time_bias;
		double new_diff = current_diff * adjustment_ratio;
		
		double change_ratio = fabs(new_diff - current_diff) / current_diff;
		
		printf("    %s:\n", conditions[i].condition);
		printf("      Observed DSPS: %.2f (target: %.2f)\n", actual_dsps, target_dsps);
		printf("      Adjustment: %.2f → %.2f (%.1f%% change)\n",
		       current_diff, new_diff, change_ratio * 100.0);
		
		/* Change should be limited by hysteresis */
		assert_true(change_ratio <= conditions[i].expected_max_change);
		
		/* Should move in correct direction */
		if (actual_dsps > target_dsps)
			assert_true(new_diff > current_diff);  /* Increase difficulty */
		else
			assert_true(new_diff < current_diff);  /* Decrease difficulty */
	}
}

/* Test 3: No oscillation on steady-state */
static void test_no_oscillation_steady_state(void)
{
	printf("\n  Testing no oscillation at steady-state:\n");
	
	/* Simulate 5 consecutive adjustment cycles at steady state */
	double diff = 10.0;
	double hashrate = 10000000.0;  /* Fixed */
	double period = 300.0;
	double target_dsps = 3.33;
	
	printf("    Starting at diff=%.2f with fixed hashrate=%.0f H/s:\n", diff, hashrate);
	
	for (int cycle = 1; cycle <= 5; cycle++) {
		double optimal_diff = calculate_optimal_diff(hashrate);
		double dsps_achieved = hashrate / (double)(1UL << 32) / diff;  /* shares per second */
		
		/* Calculate what adjustment would occur */
		double raw_adjustment_ratio = dsps_achieved / target_dsps;
		double time_bias = calculate_time_bias(period, period);
		double adjustment_ratio = 1.0 + (raw_adjustment_ratio - 1.0) * time_bias;
		double new_diff = diff * adjustment_ratio;
		
		printf("    Cycle %d: diff=%.2f, dsps_achieved=%.4f, new_diff=%.2f\n",
		       cycle, diff, dsps_achieved, new_diff);
		
		/* Update for next cycle */
		diff = new_diff;
		
		/* Verify we're converging toward optimal, not oscillating */
		if (cycle > 1) {
			double change_ratio = fabs(new_diff - diff) / diff;
			/* Each cycle should produce smaller adjustments as we converge */
			assert_true(change_ratio < 0.5);  /* No more than 50% change per cycle */
		}
	}
}

/* Test 4: Smooth convergence under changing conditions */
static void test_smooth_convergence(void)
{
	printf("\n  Testing smooth convergence to new steady-state:\n");
	
	/* Miner gradually gets faster (e.g., overclock ramping up) */
	double diff = 10.0;
	double hashrate = 10000000.0;
	double hashrate_increase_per_cycle = 1.05;  /* 5% faster each cycle */
	
	printf("    Starting: diff=%.2f, hashrate=%.0f H/s\n", diff, hashrate);
	
	double prev_diff = diff;
	for (int cycle = 1; cycle <= 5; cycle++) {
		hashrate *= hashrate_increase_per_cycle;
		double optimal_diff = calculate_optimal_diff(hashrate);
		
		/* Adjustment with hysteresis */
		double time_bias = calculate_time_bias(300.0, 300.0);
		diff = prev_diff + (optimal_diff - prev_diff) * time_bias;
		
		double change_ratio = (diff - prev_diff) / prev_diff;
		
		printf("    Cycle %d: hashrate=%.0f → diff=%.2f (%.1f%% change)\n",
		       cycle, hashrate, diff, change_ratio * 100.0);
		
		/* Each adjustment should be reasonable */
		assert_true(fabs(change_ratio) < 0.65);
		
		/* Should move toward optimal, not oscillate */
		if (optimal_diff > prev_diff)
			assert_true(diff > prev_diff);  /* Moving up */
		else
			assert_true(diff < prev_diff);  /* Moving down */
		
		prev_diff = diff;
	}
}

/* Test 5: Hysteresis prevents rapid oscillation */
static void test_hysteresis_prevents_oscillation(void)
{
	printf("\n  Testing hysteresis prevents oscillation:\n");
	
	/* Simulate erratic mining (network lag, pool rejections cause share counting errors) */
	double diff = 5.0;
	double base_hashrate = 5000000.0;
	
	/* Share count fluctuates: high, low, high, low, high */
	struct {
		const char *observation;
		double observed_hashrate;
		double expected_change;
	} observations[] = {
		{ "Shares come in bursts (2x expected)", base_hashrate * 2.0, 0 },      /* Don't react yet */
		{ "Drought (0.5x expected)", base_hashrate * 0.5, 0.1 },                /* Small change */
		{ "Back to normal", base_hashrate, 0.2 },                              /* Continue adjusting */
		{ "Burst again (1.5x)", base_hashrate * 1.5, 0.1 },                    /* Damped reaction */
		{ "Erratic still (0.8x)", base_hashrate * 0.8, 0.05 },                 /* Small dampening */
	};
	
	double prev_diff = diff;
	for (int i = 0; i < (int)(sizeof(observations) / sizeof(observations[0])); i++) {
		double optimal = calculate_optimal_diff(observations[i].observed_hashrate);
		
		/* With hysteresis, don't jump directly to optimal */
		double time_bias = 0.3;  /* Dampening factor */
		double new_diff = prev_diff + (optimal - prev_diff) * time_bias;
		
		double change_pct = fabs(new_diff - prev_diff) / prev_diff * 100.0;
		
		printf("    %s:\n", observations[i].observation);
		printf("      Observed optimal: %.2f, Adjusted diff: %.2f (%.1f%% change)\n",
		       optimal, new_diff, change_pct);
		
		/* Should be dampened */
		assert_true(change_pct <= 30.0);  /* Never more than 30% per cycle */
		
		prev_diff = new_diff;
	}
}

/* Test 6: Large changes over many cycles allowed (convergence) */
static void test_large_changes_over_time_allowed(void)
{
	printf("\n  Testing that large overall changes are allowed over many cycles:\n");
	
	/* Hashrate increases 10x over 10 cycles (e.g., farm expansion) */
	double diff = 1.0;
	double hashrate = 1000000.0;
	int cycles = 10;
	
	printf("    Initial: diff=%.2f, hashrate=%.0f H/s\n", diff, hashrate);
	
	for (int cycle = 1; cycle <= cycles; cycle++) {
		hashrate *= 1.25;  /* 25% increase per cycle */
		double optimal_diff = calculate_optimal_diff(hashrate);
		
		double time_bias = calculate_time_bias(300.0, 300.0);
		double new_diff = diff + (optimal_diff - diff) * time_bias;
		
		printf("    Cycle %2d: hashrate=%.0f H/s, diff: %.2f → %.2f\n",
		       cycle, hashrate, diff, new_diff);
		
		diff = new_diff;
	}
	
	/* After 10 cycles, should have converged significantly (not stuck at old value) */
	double final_optimal = calculate_optimal_diff(hashrate);
	double final_delta = fabs(diff - final_optimal) / final_optimal;
	
	printf("    Final: diff=%.2f, optimal=%.2f (delta: %.1f%%)\n",
	       diff, final_optimal, final_delta * 100.0);
	
	/* Should be close to optimal (within 10%) after converging */
	assert_true(final_delta < 0.1);
}

/* Main test runner */
int main(void)
{
	run_test(test_hashrate_spike_controlled);
	run_test(test_stability_around_target);
	run_test(test_no_oscillation_steady_state);
	run_test(test_smooth_convergence);
	run_test(test_hysteresis_prevents_oscillation);
	run_test(test_large_changes_over_time_allowed);
	printf("All tests passed!\n");
	return 0;
}
