#include "config.h"

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include "stratifier_internal.h"
#include "worker_ua.h"

/* Minimal helper to allocate a client with a UA and attach to a user */
static stratum_instance_t *alloc_client(user_instance_t *user, worker_instance_t *worker, const char *ua)
{
    stratum_instance_t *c = calloc(1, sizeof(*c));
    assert_non_null(c);
    if (ua && strlen(ua))
        c->useragent = strdup(ua);
    c->worker_instance = worker;
    c->user_instance = user;
    /* Use DL_APPEND2 with explicit field names (user_prev, user_next) */
    DL_APPEND2(user->clients, c, user_prev, user_next);
    return c;
}

static void free_clients(user_instance_t *user)
{
    stratum_instance_t *c, *tmp;
    /* Iterate safely using explicit field name (user_next) */
    DL_FOREACH_SAFE2(user->clients, c, tmp, user_next) {
        DL_DELETE2(user->clients, c, user_prev, user_next);
        if (c->useragent) free(c->useragent);
        free(c);
    }
}

static void test_no_clients_preserve_persisted(void **state)
{
    user_instance_t user = {0};
    worker_instance_t worker = {0};

    /* persisted UA should be preserved when no clients are attached */
    worker.useragent = strdup("PersistedUA");
    worker.instance_count = 0;

    recalc_worker_useragent(NULL, &user, &worker);
    assert_string_equal(worker.useragent, "PersistedUA");

    free(worker.useragent);
}

static void test_single_client_sets_client_ua(void **state)
{
    user_instance_t user = {0};
    worker_instance_t worker = {0};

    worker.instance_count = 1;
    stratum_instance_t *c = alloc_client(&user, &worker, "ClientUA123");

    recalc_worker_useragent(NULL, &user, &worker);
    assert_string_equal(worker.useragent, "ClientUA123");
    assert_string_not_equal(worker.norm_useragent, "");

    free_clients(&user);
    free(worker.useragent);
}

static void test_multiple_clients_sets_other(void **state)
{
    user_instance_t user = {0};
    worker_instance_t worker = {0};

    worker.instance_count = 2;
    stratum_instance_t *c1 = alloc_client(&user, &worker, "UA1");
    stratum_instance_t *c2 = alloc_client(&user, &worker, "UA2");

    recalc_worker_useragent(NULL, &user, &worker);
    assert_string_equal(worker.useragent, "Other");
    assert_string_equal(worker.norm_useragent, "Other");

    free_clients(&user);
    free(worker.useragent);
}

static void test_transition_1_to_2_to_1(void **state)
{
    user_instance_t user = {0};
    worker_instance_t worker = {0};

    /* Start with one client */
    worker.instance_count = 1;
    stratum_instance_t *c1 = alloc_client(&user, &worker, "UA1");
    recalc_worker_useragent(NULL, &user, &worker);
    assert_string_equal(worker.useragent, "UA1");

    /* Add a second client and update instance_count */
    stratum_instance_t *c2 = alloc_client(&user, &worker, "UA2");
    worker.instance_count = 2;
    recalc_worker_useragent(NULL, &user, &worker);
    assert_string_equal(worker.useragent, "Other");

    /* Remove second client (simulate disconnect) */
    DL_DELETE2(user.clients, c2, user_prev, user_next);
    free(c2->useragent);
    free(c2);
    worker.instance_count = 1;
    recalc_worker_useragent(NULL, &user, &worker);
    assert_string_equal(worker.useragent, "UA1");

    free_clients(&user);
    free(worker.useragent);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_no_clients_preserve_persisted),
        cmocka_unit_test(test_single_client_sets_client_ua),
        cmocka_unit_test(test_multiple_clients_sets_other),
        cmocka_unit_test(test_transition_1_to_2_to_1),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
