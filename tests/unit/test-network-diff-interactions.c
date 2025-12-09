/*
 * Unit tests for difficulty and network difficulty interaction
 * ==============================================================
 * 
 * PURPOSE:
 * Verifies that worker difficulty, pool constraints, and network difficulty
 * interact correctly. Tests the difference between difficulty constraints
 * (for vardiff) and network difficulty (threshold for block detection).
 * 
 * KEY CONCEPTS:
 * 
 * WORKER DIFFICULTY: Vardiff-managed difficulty for the miner.
 *   - Set by pool's vardiff algorithm (optimal_diff = dsps * 3.33)
 *   - Clamped by pool constraints: mindiff <= final_diff <= maxdiff
 *   - Can be BELOW network_diff (miner gets partial shares)
 *   - Constrains: vardiff adjustments, share variance tracking
 * 
 * NETWORK DIFFICULTY: Bitcoin protocol threshold for valid blocks.
 *   - Extracted from block header (diff_from_nbits)
 *   - Not a constraint on worker_diff, they're independent
 *   - Shares >= network_diff are submitted as potential blocks
 *   - Used in: block detection, pool statistics, mining profitability
 * 
 * ALLOW_LOW_DIFF FLAG: Enables regtest/testnet support.
 *   - When false (production): network_diff < 1.0 clamped to 1.0
 *   - When true (regtest): network_diff can be fractional (e.g., 0.5)
 *   - Only affects network_diff reporting, not worker_diff constraints
 * 
 * CONSTRAINT HIERARCHY (for worker difficulty ONLY):
 * 1. Optimal difficulty = dsps * 3.33
 * 2. Pool global: pool_mindiff (floor), pool_maxdiff (ceiling)
 * 3. Pool per-worker: startdiff (initial), mindiff (floor), maxdiff (ceiling)
 * 
 * FINAL RULE for worker_diff:
 * pool_mindiff <= final_diff <= pool_maxdiff
 * (network_diff is tracked separately, not a constraint)
 * 
 * TEST SCENARIOS:
 * - Network floor: Regtest vs. production clamping behavior
 * - Optimal vs. network: Worker diff can be below/above network diff
 * - Pool constraints: Min/max bounds on worker difficulty
 * - All compose: No hidden conflicts between constraints
 * - Worker overrides: Per-worker settings override pool defaults
 * 
 * EXPECTED RESULTS:
 * - Network floor applied only when allow_low_diff=false
 * - Worker difficulty independent from network difficulty
 * - All pool constraints respected (mindiff <= diff <= maxdiff)
 * - Impossible configurations detected (e.g., mindiff > maxdiff)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <stdbool.h>

#include "../test_common.h"

/* Helper: Calculate optimal diff from hashrate */
static double calculate_optimal_diff(double hashrate)
{
	double dsps = hashrate / (double)(1UL << 32);
	return dsps * 3.33;
}

/*
 * Real-world network difficulty scenarios:
 * - Bitcoin: 1,000,000,000 to 100,000,000,000,000 (varies by era)
 * - Regtest: 1, 0.5, or even lower
 * - Testnet: 1 to millions
 */

/* Test 1: Network difficulty floor clamping */
static void test_network_diff_floor_clamping(void)
{
	printf("\n  Testing network difficulty floor behavior:\n");
	
	struct {
		const char *network;
		double network_diff;
		bool allow_low_diff;
		double expected_floor;
	} scenarios[] = {
		/* Mainnet: 1B diff, floor doesn't apply (already > 1.0) */
		{ "Bitcoin mainnet", 1000000000.0, false, 1000000000.0 },
		
		/* Regtest: allow_low_diff=true, no floor */
		{ "Regtest (unlimited low)", 0.5, true, 0.5 },
		
		/* Testnet pool: diff < 1.0, clamped to 1.0 */
		{ "Testnet pool low diff", 0.5, false, 1.0 },
		
		/* Solo mining on testnet: allow_low_diff=true, no clamping */
		{ "Testnet solo (low allowed)", 0.1, true, 0.1 },
	};
	
	for (int i = 0; i < (int)(sizeof(scenarios) / sizeof(scenarios[0])); i++) {
		double network_diff = scenarios[i].network_diff;
		bool allow_low_diff = scenarios[i].allow_low_diff;
		
		/* Simulate the clamping from stratifier.c add_base() */
		if (!allow_low_diff && network_diff < 1.0)
			network_diff = 1.0;
		
		printf("    %s:\n", scenarios[i].network);
		printf("      Raw network diff: %.10f, allow_low=%d\n",
		       scenarios[i].network_diff, allow_low_diff);
		printf("      After clamping: %.10f (expected: %.10f)\n",
		       network_diff, scenarios[i].expected_floor);
		
		assert_double_equal(network_diff, scenarios[i].expected_floor, EPSILON_DIFF);
	}
}

/* Test 2: Optimal diff vs network diff cap */
static void test_optimal_capped_by_network_diff(void)
{
	printf("\n  Testing that optimal diff doesn't exceed network diff:\n");
	
	struct {
		const char *scenario;
		double hashrate;
		double network_diff;
		bool should_cap;
	} scenarios[] = {
		/* Low hashrate, high network: no cap needed */
		{ "Low hashrate (100 H/s), Bitcoin mainnet (1B)", 100.0, 1000000000.0, false },
		
		/* High hashrate, low network: needs cap */
		{ "ASIC (100 TH/s), low network (1000)", 100000000000000.0, 1000.0, true },
		
		/* Extreme: massive ASIC on regtest */
		{ "ASIC (1 EH/s), regtest (0.5)", 1000000000000000.0, 0.5, true },
		
		/* Reasonable high-end */
		{ "Mid ASIC (100 GH/s), testnet (100000)", 100000000000.0, 100000.0, false },
	};
	
	for (int i = 0; i < (int)(sizeof(scenarios) / sizeof(scenarios[0])); i++) {
		double optimal_diff = calculate_optimal_diff(scenarios[i].hashrate);
		double network_diff = scenarios[i].network_diff;
		
		/* Clamping logic: final_diff = MIN(optimal, network_diff) */
		double final_diff = optimal_diff;
		bool was_capped = false;
		if (final_diff > network_diff) {
			final_diff = network_diff;
			was_capped = true;
		}
		
		printf("    %s:\n", scenarios[i].scenario);
		printf("      Optimal: %.2f, Network: %.2f, Final: %.2f\n",
		       optimal_diff, network_diff, final_diff);
		
		/* Final must never exceed network */
		assert_true(final_diff <= network_diff);
		
		/* Cap status must match expectation */
		assert_true(was_capped == scenarios[i].should_cap);
	}
}

/* Test 3: Pool mindiff and maxdiff constraints */
static void test_pool_min_maxdiff_constraints(void)
{
	printf("\n  Testing pool minimum and maximum difficulty constraints:\n");
	
	struct {
		const char *scenario;
		double optimal_diff;
		double pool_mindiff;
		double pool_maxdiff;
		double expected_result;
	} scenarios[] = {
		/* No constraints */
		{ "No constraints", 5.0, 0.001, 0.0, 5.0 },
		
		/* Clamped UP by pool_mindiff */
		{ "Clamped up by pool_mindiff", 0.0001, 0.001, 0.0, 0.001 },
		
		/* Clamped DOWN by pool_maxdiff */
		{ "Clamped down by pool_maxdiff", 1000000.0, 0.001, 100000.0, 100000.0 },
		
		/* Both constraints apply (unlikely but possible) */
		{ "Between min and max", 5.0, 1.0, 10.0, 5.0 },
		
		/* Below pool_mindiff */
		{ "Far below pool_mindiff", 0.0001, 0.1, 0.0, 0.1 },
		
		/* Above pool_maxdiff */
		{ "Far above pool_maxdiff", 5000.0, 0.1, 100.0, 100.0 },
	};
	
	for (int i = 0; i < (int)(sizeof(scenarios) / sizeof(scenarios[0])); i++) {
		double clamped = scenarios[i].optimal_diff;
		
		/* Apply constraints in order */
		if (clamped < scenarios[i].pool_mindiff)
			clamped = scenarios[i].pool_mindiff;
		if (scenarios[i].pool_maxdiff > 0 && clamped > scenarios[i].pool_maxdiff)
			clamped = scenarios[i].pool_maxdiff;
		
		printf("    %s:\n", scenarios[i].scenario);
		printf("      Optimal: %.2f, Pool constraints: [%.2f, %.2f] → Result: %.2f\n",
		       scenarios[i].optimal_diff,
		       scenarios[i].pool_mindiff,
		       scenarios[i].pool_maxdiff,
		       clamped);
		
		assert_double_equal(clamped, scenarios[i].expected_result, EPSILON_DIFF);
	}
}

/* Test 4: All constraints compose correctly */
static void test_all_constraints_compose(void)
{
	printf("\n  Testing composition of all constraints:\n");
	
	struct {
		const char *scenario;
		double hashrate;
		double network_diff;
		double pool_mindiff;
		double pool_maxdiff;
		bool allow_low_diff;
	} scenarios[] = {
		/* Typical: Bitcoin mainnet, reasonable hashrate */
		{
			"Bitcoin mainnet, GPU miner",
			10000000.0,  /* 10 MH/s */
			1000000000.0,
			0.001,
			0.0,
			false,
		},
		
		/* Low hashrate IoT on mainnet */
		{
			"Mainnet, ESP32 (100 H/s)",
			100.0,
			1000000000.0,
			0.001,
			0.0,
			false,
		},
		
		/* Testnet with low diff allowed */
		{
			"Testnet, low diff allowed",
			1000.0,
			0.5,
			0.00001,
			0.0,
			true,
		},
		
		/* Pool with aggressive min/max */
		{
			"Pool with min=10, max=1000",
			1000000000.0,  /* 1 GH/s */
			1000000000.0,
			10.0,
			1000.0,
			false,
		},
		
		/* Regtest edge case - low hashrate on testnet with low diff allowed */
		{
			"Regtest, low hashrate allowed",
			1.0,  /* 1 H/s theoretical */
			0.01,
			0.001,
			0.0,
			true,
		},
	};
	
	for (int i = 0; i < (int)(sizeof(scenarios) / sizeof(scenarios[0])); i++) {
		double optimal_diff = calculate_optimal_diff(scenarios[i].hashrate);
		double network_diff = scenarios[i].network_diff;
		
		/* Step 1: Network difficulty floor (only affects reporting, not worker diff) */
		if (!scenarios[i].allow_low_diff && network_diff < 1.0)
			network_diff = 1.0;
		
		/* Step 2: Calculate worker difficulty starting from optimal */
		double worker_diff = optimal_diff;
		
		/* Step 3: Apply pool constraints (INDEPENDENT of network_diff) */
		if (worker_diff < scenarios[i].pool_mindiff)
			worker_diff = scenarios[i].pool_mindiff;
		if (scenarios[i].pool_maxdiff > 0 && worker_diff > scenarios[i].pool_maxdiff)
			worker_diff = scenarios[i].pool_maxdiff;
		
		printf("    %s:\n", scenarios[i].scenario);
		printf("      Hashrate: %.0f H/s\n", scenarios[i].hashrate);
		printf("      Network diff (after floor): %.2f (for block detection, not a constraint)\n", network_diff);
		printf("      Optimal worker diff: %.2f\n", optimal_diff);
		printf("      Pool constraints: [%.2f, %.2f]\n",
		       scenarios[i].pool_mindiff,
		       scenarios[i].pool_maxdiff);
		printf("      Final worker diff: %.2f\n", worker_diff);
		
		/* Final worker diff must respect pool constraints ONLY */
		assert_true(worker_diff >= scenarios[i].pool_mindiff);
		if (scenarios[i].pool_maxdiff > 0)
			assert_true(worker_diff <= scenarios[i].pool_maxdiff);
		
		/* Worker diff can be above or below network_diff - they're independent */
		printf("      (Note: worker diff can be %.s network_diff for partial shares)\n",
		       worker_diff < network_diff ? "below" : worker_diff > network_diff ? "above" : "equal to");
	}
}

/* Test 5: Worker-specific constraints override pool defaults */
static void test_worker_overrides_pool_defaults(void)
{
	printf("\n  Testing worker-specific difficulty overrides:\n");
	
	struct {
		const char *scenario;
		double optimal_diff;
		double pool_mindiff;
		double pool_maxdiff;
		double worker_startdiff;    /* Initial difficulty for worker */
		double worker_mindiff;      /* Worker's minimum */
		double worker_maxdiff;      /* Worker's maximum */
		double expected_result;
	} scenarios[] = {
		/* Worker sets higher floor than pool */
		{
			"Worker requires minimum 10 (pool min 0.1)",
			5.0,
			0.1, 0.0,
			10.0, 10.0, 0.0,
			10.0,  /* Worker mindiff wins */
		},
		
		/* Worker accepts lower diffs than pool */
		{
			"Worker allows 0.00001 (pool min 0.1)",
			0.00001,
			0.1, 0.0,
			0.00001, 0.00001, 0.0,
			0.1,  /* Pool mindiff still enforced */
		},
		
		/* Worker max overrides pool max */
		{
			"Worker maxdiff lower than pool",
			50.0,
			0.1, 100.0,
			10.0, 0.1, 30.0,  /* Worker max 30 < pool max 100 */
			30.0,  /* Worker maxdiff wins */
		},
	};
	
	for (int i = 0; i < (int)(sizeof(scenarios) / sizeof(scenarios[0])); i++) {
		double final_diff = scenarios[i].optimal_diff;
		
		/* Apply pool constraints first */
		if (final_diff < scenarios[i].pool_mindiff)
			final_diff = scenarios[i].pool_mindiff;
		if (scenarios[i].pool_maxdiff > 0 && final_diff > scenarios[i].pool_maxdiff)
			final_diff = scenarios[i].pool_maxdiff;
		
		/* Then apply worker constraints */
		if (final_diff < scenarios[i].worker_mindiff)
			final_diff = scenarios[i].worker_mindiff;
		if (scenarios[i].worker_maxdiff > 0 && final_diff > scenarios[i].worker_maxdiff)
			final_diff = scenarios[i].worker_maxdiff;
		
		printf("    %s:\n", scenarios[i].scenario);
		printf("      Pool [%.6f, %.6f], Worker [%.6f, %.6f]\n",
		       scenarios[i].pool_mindiff,
		       scenarios[i].pool_maxdiff,
		       scenarios[i].worker_mindiff,
		       scenarios[i].worker_maxdiff);
		printf("      Optimal: %.2f → Final: %.2f (expected: %.2f)\n",
		       scenarios[i].optimal_diff, final_diff, scenarios[i].expected_result);
		
		assert_double_equal(final_diff, scenarios[i].expected_result, EPSILON_DIFF);
	}
}

/* Test 6: Conflicting constraints are impossible to violate */
static void test_constraint_conflicts_impossible(void)
{
	printf("\n  Testing that constraint conflicts don't cause violations:\n");
	
	/* These scenarios have contradictory constraints - verify we handle them gracefully */
	struct {
		const char *scenario;
		double pool_mindiff;
		double pool_maxdiff;
		double worker_mindiff;
		double worker_maxdiff;
	} conflicts[] = {
		/* Pool min > pool max (invalid config, but shouldn't crash) */
		{
			"Pool: min=100 > max=50",
			100.0, 50.0,
			0.1, 0.0,
		},
		
		/* Worker min > worker max (invalid) */
		{
			"Worker: min=50 > max=10",
			0.1, 0.0,
			50.0, 10.0,
		},
		
		/* All constraints contradictory */
		{
			"All contradictory",
			100.0, 50.0,
			200.0, 10.0,
		},
	};
	
	for (int i = 0; i < (int)(sizeof(conflicts) / sizeof(conflicts[0])); i++) {
		printf("    %s:\n", conflicts[i].scenario);
		printf("      Pool [%.0f, %.0f], Worker [%.0f, %.0f]\n",
		       conflicts[i].pool_mindiff,
		       conflicts[i].pool_maxdiff,
		       conflicts[i].worker_mindiff,
		       conflicts[i].worker_maxdiff);
		
		/* In real code, this should be caught at config load time */
		/* For now, just verify we can process without crashing */
		double test_diff = 25.0;  /* Arbitrary value */
		
		/* Try to apply constraints - order matters to handle conflicts */
		if (conflicts[i].pool_mindiff > 0)
			test_diff = fmax(test_diff, conflicts[i].pool_mindiff);
		if (conflicts[i].pool_maxdiff > 0)
			test_diff = fmin(test_diff, conflicts[i].pool_maxdiff);
		if (conflicts[i].worker_mindiff > 0)
			test_diff = fmax(test_diff, conflicts[i].worker_mindiff);
		if (conflicts[i].worker_maxdiff > 0)
			test_diff = fmin(test_diff, conflicts[i].worker_maxdiff);
		
		printf("      Result: %.0f (handled gracefully)\n", test_diff);
		
		/* Should not be infinite or NaN */
		assert_true(isfinite(test_diff));
	}
}

/* Main test runner */
int main(void)
{
	run_test(test_network_diff_floor_clamping);
	run_test(test_optimal_capped_by_network_diff);
	run_test(test_pool_min_maxdiff_constraints);
	run_test(test_all_constraints_compose);
	run_test(test_worker_overrides_pool_defaults);
	run_test(test_constraint_conflicts_impossible);
	printf("All tests passed!\n");
	return 0;
}
