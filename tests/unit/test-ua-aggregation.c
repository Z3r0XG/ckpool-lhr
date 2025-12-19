#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../test_common.h"
#include "ua_utils.h"

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

int main(void)
{
    test_normalize_basic();
    test_normalize_truncate();
    printf("ua-normalization tests passed\n");
    return 0;
}
