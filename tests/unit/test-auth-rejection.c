/*
 * Unit tests for share rejection logic during the authorization window.
 * This verifies the "reject and move on" strategy for high-speed miners
 * that send shares before the authorizer thread has finished.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "config.h"
#include "../test_common.h"

/* 
 * Rule Definitions (Mirrored from stratifier.c:parse_method)
 * 
 * Logic to be tested:
 * if (!subscribed) -> drop_client
 * if (!authorised) {
 *    if (method == "mining.submit") -> respond "Stale"
 *    else if (method == "mining.suggest") -> apply_suggest_diff
 * }
 */

typedef struct {
    bool subscribed;
    bool authorised;
    bool authorising;
} mock_client_t;

typedef enum {
    ACTION_ACCEPT,
    ACTION_REJECT_STALE,
    ACTION_DROP,
    ACTION_QUEUE_DIFF
} test_action_t;

static test_action_t determine_action(const char *method, mock_client_t *client)
{
    if (!client->subscribed)
        return ACTION_DROP;

    if (!client->authorised) {
        if (strcmp(method, "mining.suggest") == 0)
            return ACTION_QUEUE_DIFF;
        
        if (strcmp(method, "mining.submit") == 0)
            return ACTION_REJECT_STALE;
        
        /* Other methods like mining.configure are accepted/handled */
        return ACTION_ACCEPT;
    }

    return ACTION_ACCEPT;
}

static void test_auth_race_rejection(void)
{
    mock_client_t client = { .subscribed = true, .authorised = false, .authorising = true };

    /* 
     * The Race Condition: mining.submit arrives while authorising is true 
     * but authorised is still false. 
     */
    assert_int_equal(determine_action("mining.submit", &client), ACTION_REJECT_STALE);
}

static void test_post_auth_acceptance(void)
{
    mock_client_t client = { .subscribed = true, .authorised = true, .authorising = false };

    /* Once authorised, shares must be accepted */
    assert_int_equal(determine_action("mining.submit", &client), ACTION_ACCEPT);
}

static void test_unsubscribed_drop(void)
{
    mock_client_t client = { .subscribed = false, .authorised = false, .authorising = false };

    /* Any request from unsubscribed clients results in a drop */
    assert_int_equal(determine_action("mining.submit", &client), ACTION_DROP);
    assert_int_equal(determine_action("mining.auth", &client), ACTION_DROP);
}

static void test_early_suggest_diff(void)
{
    mock_client_t client = { .subscribed = true, .authorised = false, .authorising = true };

    /* 
     * mining.suggest is an exception; it should be queued/applied even 
     * if not yet authorised (as seen in parse_method logic).
     */
    assert_int_equal(determine_action("mining.suggest", &client), ACTION_QUEUE_DIFF);
}

static void test_other_methods_allowed(void)
{
    mock_client_t client = { .subscribed = true, .authorised = false, .authorising = true };

    /* 
     * mining.configure and others should not trigger the "Stale" rejection.
     */
    assert_int_equal(determine_action("mining.configure", &client), ACTION_ACCEPT);
}

int main(void)
{
    printf("Running auth rejection unit tests...\n");
    
    run_test(test_auth_race_rejection);
    run_test(test_post_auth_acceptance);
    run_test(test_unsubscribed_drop);
    run_test(test_early_suggest_diff);
    run_test(test_other_methods_allowed);
    
    printf("All auth rejection unit tests passed!\n");
    return 0;
}
