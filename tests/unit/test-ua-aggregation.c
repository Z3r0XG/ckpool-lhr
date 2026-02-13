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
    normalize_ua_buf("cgminer/4.5.15", out, sizeof(out));
    assert(strcmp(out, "cgminer") == 0);

    normalize_ua_buf("cpuminer-multi (linux)", out, sizeof(out));
    assert(strcmp(out, "cpuminer-multi") == 0);

    /* With case and space preservation: " BM1387 Miner " -> "BM1387 Miner"
     * (leading/trailing spaces stripped, but internal space preserved, case preserved) */
    normalize_ua_buf(" BM1387 Miner ", out, sizeof(out));
    assert(strcmp(out, "BM1387 Miner") == 0);
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
