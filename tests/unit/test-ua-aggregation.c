#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "../test_common.h"
#include "ua_utils.h"

static int perf_tests_enabled(void)
{
    const char *val = getenv("CKPOOL_PERF_TESTS");

    return val && val[0] == '1';
}

static void test_normalize_basic()
{
    char out[65];

    /* ── existing baseline ── */
    normalize_ua_buf("cgminer/4.5.15", out, sizeof(out));
    assert(strcmp(out, "cgminer") == 0);

    /* Rule 3: cpuminer-multi collapses to cpuminer */
    normalize_ua_buf("cpuminer-multi (linux)", out, sizeof(out));
    assert(strcmp(out, "cpuminer") == 0);

    /* plain name with spaces is preserved (leading/trailing ws stripped) */
    normalize_ua_buf(" BM1387 Miner ", out, sizeof(out));
    assert(strcmp(out, "BM1387 Miner") == 0);

    /* ── Rule 2: BM chip-model suffix strip ── */
    /* space-separated BM suffix, no slash */
    normalize_ua_buf("LuckyMiner BM1366", out, sizeof(out));
    assert(strcmp(out, "LuckyMiner") == 0);

    /* dash-separated BM suffix */
    normalize_ua_buf("LuckyMiner-BM1366", out, sizeof(out));
    assert(strcmp(out, "LuckyMiner") == 0);

    /* bare "-BM" or " BM" with no trailing digits must NOT be stripped */
    normalize_ua_buf("FooMiner-BM", out, sizeof(out));
    assert(strcmp(out, "FooMiner-BM") == 0);

    normalize_ua_buf("FooMiner BM", out, sizeof(out));
    assert(strcmp(out, "FooMiner BM") == 0);

    /* BM with no separator (e.g. "SomeMinerBM1366") must NOT be stripped */
    normalize_ua_buf("SomeMinerBM1366", out, sizeof(out));
    assert(strcmp(out, "SomeMinerBM1366") == 0);

    /* standalone "BM1366" (no name prefix) must NOT be stripped */
    normalize_ua_buf("BM1366", out, sizeof(out));
    assert(strcmp(out, "BM1366") == 0);

    /* lowercase "bm1366" suffix must NOT be stripped (only uppercase BM matches) */
    normalize_ua_buf("FooMiner-bm1366", out, sizeof(out));
    assert(strcmp(out, "FooMiner-bm1366") == 0);

    /* slash-separated already handled by version-separator stop */
    normalize_ua_buf("LuckyMiner/BM1366/1.2.0", out, sizeof(out));
    assert(strcmp(out, "LuckyMiner") == 0);

    /* BM suffix on NerdQAxe comes via slash, name preserved intact */
    normalize_ua_buf("NerdQAxe++/BM1370/v1.0.36", out, sizeof(out));
    assert(strcmp(out, "NerdQAxe++") == 0);

    /* bitaxe variant */
    normalize_ua_buf("bitaxe/BM1370/v2.13.0", out, sizeof(out));
    assert(strcmp(out, "bitaxe") == 0);

    /* Rule 2 fires before Rule 3: cpuminer-BM1366 -> cpuminer (R2 strips BM, R3 then sees exact length) */
    normalize_ua_buf("cpuminer-BM1366", out, sizeof(out));
    assert(strcmp(out, "cpuminer") == 0);

    /* ── Rule 3: cpuminer family ── */
    normalize_ua_buf("cpuminer-multi/1.3.7", out, sizeof(out));
    assert(strcmp(out, "cpuminer") == 0);

    normalize_ua_buf("cpuminer-opt-v2.5.0", out, sizeof(out));
    assert(strcmp(out, "cpuminer") == 0);

    normalize_ua_buf("cpuminer_gc3355/3.0", out, sizeof(out));
    assert(strcmp(out, "cpuminer") == 0);

    /* slash terminates the token; result is "cpuminer" */
    normalize_ua_buf("cpuminer/2.5.1", out, sizeof(out));
    assert(strcmp(out, "cpuminer") == 0);

    /* bare "cpuminer" with no suffix remains unchanged */
    normalize_ua_buf("cpuminer", out, sizeof(out));
    assert(strcmp(out, "cpuminer") == 0);

    /* "cpuminers-variant" shares the prefix but is a distinct product; must NOT collapse */
    normalize_ua_buf("cpuminers-variant", out, sizeof(out));
    assert(strcmp(out, "cpuminers-variant") == 0);

    /* Rule 3 is case-insensitive match but preserves input casing */
    normalize_ua_buf("CPUMINER-multi", out, sizeof(out));
    assert(strcmp(out, "CPUMINER") == 0);

    /* ── Rule 4: dash-version suffix strip ── */
    normalize_ua_buf("xminer-1.2.7", out, sizeof(out));
    assert(strcmp(out, "xminer") == 0);

    normalize_ua_buf("FooMiner-2.0", out, sizeof(out));
    assert(strcmp(out, "FooMiner") == 0);

    /* Rule 4 regression: trailing dot must NOT be stripped */
    normalize_ua_buf("foo-1.", out, sizeof(out));
    assert(strcmp(out, "foo-1.") == 0);

    /* Rule 4 regression: dot-led suffix must NOT be stripped */
    normalize_ua_buf("foo-.1", out, sizeof(out));
    assert(strcmp(out, "foo-.1") == 0);

    /* Rule 4 regression: bare dash must NOT be stripped */
    normalize_ua_buf("foo-", out, sizeof(out));
    assert(strcmp(out, "foo-") == 0);

    /* Rule 4: v-prefixed version (e.g. "FooMiner-v2") must NOT be stripped
     * because 'v' is not a digit so the suffix-start check fails */
    normalize_ua_buf("FooMiner-v2", out, sizeof(out));
    assert(strcmp(out, "FooMiner-v2") == 0);

    /* ── Preserved names (no rule should fire) ── */
    normalize_ua_buf("Nerd Miner", out, sizeof(out));
    assert(strcmp(out, "Nerd Miner") == 0);

    normalize_ua_buf("ESP32 TacoMiner", out, sizeof(out));
    assert(strcmp(out, "ESP32 TacoMiner") == 0);

    normalize_ua_buf("stratum-ping/1.0.0", out, sizeof(out));
    assert(strcmp(out, "stratum-ping") == 0);

    normalize_ua_buf("esp32s3-toy/1.0", out, sizeof(out));
    assert(strcmp(out, "esp32s3-toy") == 0);

    /* Antminer with date-string version */
    normalize_ua_buf("Antminer S19k Pro/Fri Oct 10 11:13:07 CST 2025", out, sizeof(out));
    assert(strcmp(out, "Antminer S19k Pro") == 0);

    normalize_ua_buf("GreenBit", out, sizeof(out));
    assert(strcmp(out, "GreenBit") == 0);

    normalize_ua_buf("NMMiner", out, sizeof(out));
    assert(strcmp(out, "NMMiner") == 0);
}

static void test_normalize_truncate()
{
    char out[33];
    normalize_ua_buf("averyveryveryverylonguseragentstring/1.0", out, sizeof(out));
    assert(out[sizeof(out)-1] == '\0');
}

static void test_normalize_performance(void)
{
    const char *inputs[] = {
        "cgminer/4.5.15",
        "cpuminer-multi (linux)",
        " BM1387 Miner ",
        "NerdMinerV2/1.2.3",
        "asic_boost/3.1.4"
    };
    const int inputs_count = sizeof(inputs) / sizeof(inputs[0]);
    const int iterations = 1000000;
    char out[65];
    clock_t start = clock();

    for (int i = 0; i < iterations; i++) {
        normalize_ua_buf(inputs[i % inputs_count], out, sizeof(out));
    }

    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
    if (elapsed > 0.0) {
        double ops_per_sec = (double)iterations / elapsed;
        printf("    normalize_ua_buf: %.2fM ops/sec (%.3f sec for %d ops)\n",
               ops_per_sec / 1e6, elapsed, iterations);
    }

    assert(elapsed < 5.0);
}

int main(void)
{
    test_normalize_basic();
    test_normalize_truncate();

    if (perf_tests_enabled()) {
        printf("[PERFORMANCE REGRESSION TESTS]\n");
        printf("BEGIN PERF TESTS: test-ua-aggregation\n");
        test_normalize_performance();
        printf("END PERF TESTS: test-ua-aggregation\n");
    }
    printf("ua-normalization tests passed\n");
    return 0;
}
