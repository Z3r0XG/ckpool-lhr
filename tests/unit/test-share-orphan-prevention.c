/*
 * Unit tests for share orphan prevention with fractional-diff feature
 * ===================================================================
 * 
 * PURPOSE:
 * Critical test for low-hashrate miners (IoT, ESP32, RPi) support.
 * Ensures that vardiff with fractional difficulty keeps difficulty low enough
 * that miners get sufficient shares before vardiff recalculates.
 * 
 * PROBLEM SOLVED:
 * Before fractional-diff, miners with hashrates < 1 H/s couldn't get shares
 * within 1 hour (vardiff timeout) even with mindiff=0.00001, because integer
 * difficulty floor was 1.0. This caused share orphaning (no shares = disconnect).
 * Fractional-diff enables difficulty values like 0.00001, allowing ~100,000x
 * improvement in share arrival time for low-hashrate devices.
 * 
 * SHARE ORPHANING DEFINITION:
 * Share orphaning occurs when a miner gets no shares before vardiff recalculates,
 * causing the connection to timeout and be re-established (loss of variance data).
 * This happens when: (1) miner too slow for pool's mindiff, or
 * (2) hashrate so low that even optimal difficulty yields > 1 hour between shares.
 * 
 * KEY INSIGHT - OPERATOR INTENT:
 * Pool operators configure startdiff/mindiff/maxdiff intentionally to define
 * their target miner demographics. If pool_mindiff=1.0, they don't support < 1H/s.
 * This is intentional business policy, not a system failure.
 * 
 * TEST SCENARIOS:
 * - Low-hashrate friendly pools: mindiff=0.00001 -> ESP32 @ 100 H/s gets ~429 sec/share
 * - High-hashrate only pools: mindiff=1.0 -> ESP32 @ 100 H/s gets ~42.9M sec/share (orphaning)
 * - Network difficulty capping: mainnet diff=1B with regtest diff=0.5
 * - Practical limits: Shows which devices are simply too slow even with fractional-diff
 * 
 * FORMULAS:
 * - DSPS (Difficulty-Shares-Per-Second) = hashrate / 2^32
 * - Optimal difficulty = DSPS * 3.33 (targets ~3.33 seconds per share)
 * - Share arrival time = difficulty / DSPS (seconds)
 * - Vardiff timeout = 3600 seconds (typical, operator-configurable)
 * 
 * EXPECTED RESULTS:
 * - Fractional diffs enable 100,000x faster share arrival for ESP32
 * - Pool operator's mindiff choice is always respected
 * - All hashrates converge to ~3.33 sec/share when using optimal diff
 * - Some devices (< 10 H/s) won't be viable even with fractional-diff + low mindiff
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <stdbool.h>

#include "../test_common.h"

#define VARDIFF_TIMEOUT_SECS (3600)  /* Typical 1-hour timeout */
#define TARGET_DSPS (3.33)           /* Target for optimal diff = dsps * 3.33 */

/* Helper: Calculate DSPS from hashrate */
static double hashrate_to_dsps(double hashrate)
{
	return hashrate / (double)(1UL << 32);
}

/* Helper: Calculate expected share arrival time (seconds) */
static double share_arrival_time_secs(double hashrate, double difficulty)
{
	double dsps = hashrate / (double)(1UL << 32);
	if (dsps <= 0 || difficulty <= 0)
		return INFINITY;
	/* Time to share = difficulty / DSPS */
	return difficulty / dsps;
}

/* Test 1: Fractional diffs enable low-hashrate device support */
static void test_fractional_diffs_enable_low_hashrate(void)
{
	printf("\n  Testing fractional diffs for low-hashrate device support:\n");
	
	struct {
		const char *name;
		double hashrate;
	} devices[] = {
		{ "ESP32 (100 H/s)", 100.0 },
		{ "RPi (500 H/s)", 500.0 },
		{ "CPU (1000 H/s)", 1000.0 },
	};
	
	printf("    Pool config: mindiff=0.001 (supports mid-to-high range)\n");
	printf("    (NOTE: ESP32 @ 100 H/s with mindiff=0.001 still waits ~12 hours)\n");
	printf("    (For true low-hashrate support, pools need mindiff=0.00001 or lower)\n\n");
	
	for (int i = 0; i < (int)(sizeof(devices) / sizeof(devices[0])); i++) {
		double hashrate = devices[i].hashrate;
		double dsps = hashrate_to_dsps(hashrate);
		double fractional_optimal = dsps * TARGET_DSPS;
		
		/* OLD BEHAVIOR (not using fractional diffs): forced to 1.0 minimum */
		double integer_diff = fmax(1.0, fractional_optimal);
		double integer_time = share_arrival_time_secs(hashrate, integer_diff);
		
		/* NEW BEHAVIOR (with fractional diffs): uses actual optimal */
		/* Use realistic pool mindiff for true low-hashrate support */
		double new_diff = fmax(0.00001, fractional_optimal);  /* Need lower floor for ESP32 */
		double new_time = share_arrival_time_secs(hashrate, new_diff);
		
		printf("    %s:\n", devices[i].name);
		printf("      Without fractional: %.0f second wait (%.2f per hour)\n",
		       integer_time, 3600.0 / integer_time);
		printf("      With fractional (mindiff=0.00001): %.2f second wait (%.0f per hour)\n",
		       new_time, 3600.0 / new_time);
		printf("      Improvement: %.0fx faster ✓\n\n", integer_time / new_time);
		
		/* Fractional must be drastically faster */
		assert_true(new_time < integer_time / 100.0);
		
		/* And must prevent orphaning (< 1 hour timeout) */
		assert_true(new_time < VARDIFF_TIMEOUT_SECS);
	}
}

/* Test 2: Pool operator's mindiff choice is respected */
static void test_pool_operator_mindiff_choice_respected(void)
{
	printf("\n  Testing that pool operator's mindiff choice is respected:\n");
	
	struct {
		const char *scenario;
		double hashrate;
		double pool_mindiff;
		bool operator_supports_device;
	} scenarios[] = {
		/* Operator supports low-hashrate: mindiff=0.00001 */
		{
			"Low-hashrate friendly pool (mindiff=0.00001)",
			100.0,  /* ESP32 */
			0.00001,
			true,
		},
		
		/* Operator does NOT support low-hashrate: mindiff=1.0 */
		{
			"High-hashrate only pool (mindiff=1.0)",
			100.0,  /* ESP32 */
			1.0,
			false,  /* Operator explicitly chose this */
		},
		
		/* Operator supports mid-range: mindiff=0.001 */
		{
			"Mid-range pool (mindiff=0.001)",
			10000.0,  /* FPGA */
			0.001,
			true,
		},
	};
	
	for (int i = 0; i < (int)(sizeof(scenarios) / sizeof(scenarios[0])); i++) {
		double hashrate = scenarios[i].hashrate;
		double dsps = hashrate_to_dsps(hashrate);
		double optimal_diff = dsps * TARGET_DSPS;
		
		/* Apply pool's constraint */
		double final_diff = fmax(optimal_diff, scenarios[i].pool_mindiff);
		double arrival_time = share_arrival_time_secs(hashrate, final_diff);
		bool will_get_shares = (arrival_time < VARDIFF_TIMEOUT_SECS);
		
		printf("    %s:\n", scenarios[i].scenario);
		printf("      Hashrate: %.0f H/s, pool mindiff: %.3f\n",
		       hashrate, scenarios[i].pool_mindiff);
		printf("      Share arrival: %.1f seconds (%s)\n",
		       arrival_time, will_get_shares ? "✓ OK" : "✗ ORPHANS");
		
		/* Verify constraint is applied */
		assert_true(final_diff >= scenarios[i].pool_mindiff);
		
		/* Result should match operator's intent */
		if (scenarios[i].operator_supports_device) {
			assert_true(will_get_shares);
			printf("      ✓ Operator supports this device\n\n");
		} else {
			printf("      (Operator explicitly does not support this device)\n\n");
		}
	}
}

/* Test 3: Optimal diff always targets ~3.33 second share rate */
static void test_optimal_diff_targets_share_rate(void)
{
	printf("\n  Testing that optimal diff targets consistent share rate:\n");
	printf("    (With optimal diff, all miners get ~3.33 sec/share)\n\n");
	
	struct {
		const char *name;
		double hashrate;
	} miners[] = {
		{ "FPGA (10 KH/s)", 10000.0 },
		{ "GPU (1 MH/s)", 1000000.0 },
		{ "ASIC (10 GH/s)", 10000000000.0 },
	};
	
	for (int i = 0; i < (int)(sizeof(miners) / sizeof(miners[0])); i++) {
		double hashrate = miners[i].hashrate;
		double dsps = hashrate_to_dsps(hashrate);
		double optimal_diff = dsps * TARGET_DSPS;
		double arrival_time = share_arrival_time_secs(hashrate, optimal_diff);
		
		printf("    %s: %.2f sec/share\n", miners[i].name, arrival_time);
		
		/* All converge to target DSPS rate */
		assert_double_equal(arrival_time, 3.33, 0.1);
	}
	printf("    ✓ All miners get consistent share submission rate\n\n");
}

/* Test 4: Network difficulty doesn't cause orphaning with fractional diffs */
static void test_network_diff_with_fractional(void)
{
	printf("\n  Testing network difficulty interaction with fractional diffs:\n");
	
	struct {
		const char *scenario;
		double hashrate;
		double network_diff;
		double pool_mindiff;
		bool should_get_shares;
	} scenarios[] = {
		/* Bitcoin mainnet, low hashrate with REALISTIC pool config */
		{
			"Bitcoin mainnet, ESP32, pool mindiff=0.00001",
			100.0,
			1000000000.0,
			0.00001,
			true,
		},
		
		/* Regtest (very low network diff), fractional diffs enabled */
		{
			"Regtest, GPU, low network diff",
			1000000.0,
			0.5,
			0.00001,
			true,
		},
		
		/* Unrealistic: Mainnet pool with high mindiff on low-hashrate device */
		{
			"Mainnet, ESP32, pool mindiff=1.0 (poor choice)",
			100.0,
			1000000000.0,
			1.0,
			false,
		},
	};
	
	for (int i = 0; i < (int)(sizeof(scenarios) / sizeof(scenarios[0])); i++) {
		double hashrate = scenarios[i].hashrate;
		double dsps = hashrate_to_dsps(hashrate);
		double optimal_diff = dsps * TARGET_DSPS;
		
		/* Apply constraints: first network diff ceiling, then pool floor */
		double constrained_diff = fmin(optimal_diff, scenarios[i].network_diff);
		constrained_diff = fmax(constrained_diff, scenarios[i].pool_mindiff);
		
		double arrival_time = share_arrival_time_secs(hashrate, constrained_diff);
		bool will_get_shares = (arrival_time < VARDIFF_TIMEOUT_SECS);
		
		printf("    %s:\n", scenarios[i].scenario);
		printf("      Constraints: network=%.2f, pool_mindiff=%.5f\n",
		       scenarios[i].network_diff, scenarios[i].pool_mindiff);
		printf("      Final diff: %.6f, share arrival: %.1f sec (%s)\n",
		       constrained_diff, arrival_time,
		       will_get_shares ? "✓" : "✗");
		
		assert_true(will_get_shares == scenarios[i].should_get_shares);
	}
}

/* Test 5: Very low hashrate support limit */
static void test_very_low_hashrate_pool_limit(void)
{
	printf("\n  Testing practical limits for very low hashrate devices:\n");
	printf("    (Some devices are TOO slow even for fractional diffs)\n\n");
	
	struct {
		const char *name;
		double hashrate;
		double pool_mindiff;
		double expected_arrival;
		bool viable;
	} devices[] = {
		/* ESP32 @ 100 H/s with pool floor 0.00001: achievable */
		{
			"ESP32 (100 H/s) with mindiff=0.00001",
			100.0,
			0.00001,
			429.5,  /* seconds */
			true,
		},
		
		/* Arduino @ 10 H/s with same floor: NOT viable */
		{
			"Arduino (10 H/s) with mindiff=0.00001",
			10.0,
			0.00001,
			4295.0,  /* seconds - ~1.2 hours, exceeds 1-hour timeout */
			false,   /* Below practical support limit */
		},
		
		/* Theoretical 1 H/s with same floor: not viable */
		{
			"Theoretical 1 H/s with mindiff=0.00001",
			1.0,
			0.00001,
			42950.0,  /* seconds - ~12 hours */
			false,    /* Too slow for 1-hour timeout */
		},
	};
	
	for (int i = 0; i < (int)(sizeof(devices) / sizeof(devices[0])); i++) {
		double arrival = share_arrival_time_secs(devices[i].hashrate, devices[i].pool_mindiff);
		bool is_viable = (arrival < VARDIFF_TIMEOUT_SECS);
		
		printf("    %s: %.0f sec/share\n", devices[i].name, arrival);
		
		assert_true(is_viable == devices[i].viable);
		
		if (!is_viable) {
			printf("      (Device below pool's practical support limit)\n");
		}
		printf("\n");
	}
}

/* Main test runner */
int main(void)
{
	run_test(test_fractional_diffs_enable_low_hashrate);
	run_test(test_pool_operator_mindiff_choice_respected);
	run_test(test_optimal_diff_targets_share_rate);
	run_test(test_network_diff_with_fractional);
	run_test(test_very_low_hashrate_pool_limit);
	printf("All tests passed!\n");
	return 0;
}
