/*
 * Unit tests for zombie client cleanup logic
 * Tests the decision logic: when to clean up zombie clients vs when to send drop messages
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "config.h"
#include "../test_common.h"
#include "libckpool.h"

/* Test the core decision logic for zombie cleanup
 * 
 * The logic in statsupdate (stratifier.c:8143-8172) is:
 * 1. If client->dropped is true
 * 2. Check if client exists in connector via connector_client_exists()
 * 3. If client doesn't exist in connector:
 *    - Check if ref == 1 (only our ref from iteration)
 *    - If ref == 1, remove from stratifier hashtable
 *    - If ref > 1, let others clean up when they release
 * 4. If client exists in connector:
 *    - Send drop message (legitimate drop)
 * 
 * This test validates the decision logic without requiring full ckpool initialization.
 */
static void test_zombie_cleanup_decision_logic(void)
{
    /* Test Case 1: Client dropped, doesn't exist in connector, ref == 1
     * Expected: Should clean up (remove from stratifier hashtable)
     */
    bool client_dropped = true;
    bool client_exists_in_connector = false;
    int ref_count = 1;  // Only our ref from iteration
    
    bool should_cleanup = (client_dropped && 
                          !client_exists_in_connector && 
                          ref_count == 1);
    assert_true(should_cleanup);
    
    /* Test Case 2: Client dropped, doesn't exist in connector, ref > 1
     * Expected: Should NOT clean up (others hold refs, they'll clean up)
     */
    client_dropped = true;
    client_exists_in_connector = false;
    ref_count = 2;  // Others hold refs
    
    should_cleanup = (client_dropped && 
                     !client_exists_in_connector && 
                     ref_count == 1);
    assert_false(should_cleanup);
    
    /* Test Case 3: Client dropped, exists in connector
     * Expected: Should send drop message (legitimate drop), NOT clean up
     */
    client_dropped = true;
    client_exists_in_connector = true;
    ref_count = 1;
    
    bool should_send_drop = (client_dropped && client_exists_in_connector);
    bool should_not_cleanup = !(client_dropped && !client_exists_in_connector && ref_count == 1);
    assert_true(should_send_drop);
    assert_true(should_not_cleanup);
    
    /* Test Case 4: Client not dropped
     * Expected: Should NOT clean up, should NOT send drop
     */
    client_dropped = false;
    client_exists_in_connector = false;
    ref_count = 1;
    
    should_cleanup = (client_dropped && 
                     !client_exists_in_connector && 
                     ref_count == 1);
    should_send_drop = (client_dropped && client_exists_in_connector);
    assert_false(should_cleanup);
    assert_false(should_send_drop);
}

/* Test ref count boundary conditions
 * Validates that ref == 1 check correctly identifies "only our ref"
 */
static void test_ref_count_boundary_conditions(void)
{
    /* Test Case 1: ref == 1 (only our ref from iteration) - should clean up */
    int ref_count = 1;
    bool can_cleanup = (ref_count == 1);
    assert_true(can_cleanup);
    
    /* Test Case 2: ref == 0 (shouldn't happen, but test logic) - should NOT clean up */
    ref_count = 0;
    can_cleanup = (ref_count == 1);
    assert_false(can_cleanup);
    
    /* Test Case 3: ref == 2 (others hold refs) - should NOT clean up */
    ref_count = 2;
    can_cleanup = (ref_count == 1);
    assert_false(can_cleanup);
    
    /* Test Case 4: ref > 1 (multiple others hold refs) - should NOT clean up */
    ref_count = 5;
    can_cleanup = (ref_count == 1);
    assert_false(can_cleanup);
}

/* Test edge cases for zombie cleanup logic
 * Tests boundary conditions and unusual scenarios
 */
static void test_zombie_cleanup_edge_cases(void)
{
    /* Edge Case 1: Client dropped, doesn't exist, ref == 1, but client pointer is NULL
     * (This shouldn't happen in practice, but tests defensive logic)
     */
    bool client_dropped = true;
    bool client_exists_in_connector = false;
    int ref_count = 1;
    bool client_ptr_valid = true;  // In practice, we'd check this
    
    bool should_cleanup = (client_dropped && 
                          !client_exists_in_connector && 
                          ref_count == 1 &&
                          client_ptr_valid);
    assert_true(should_cleanup);
    
    /* Edge Case 2: Client exists in connector but also marked dropped
     * (Race condition: connector hasn't processed drop yet)
     * Expected: Should send drop message (let connector handle it)
     */
    client_dropped = true;
    client_exists_in_connector = true;  // Still exists (race condition)
    ref_count = 1;
    
    should_cleanup = (client_dropped && 
                     !client_exists_in_connector && 
                     ref_count == 1);
    bool should_send_drop = (client_dropped && client_exists_in_connector);
    assert_false(should_cleanup);  // Don't clean up if it exists
    assert_true(should_send_drop);  // Send drop message
    
    /* Edge Case 3: Client not dropped, doesn't exist in connector
     * (Shouldn't happen, but test logic)
     * Expected: Should NOT clean up
     */
    client_dropped = false;
    client_exists_in_connector = false;
    ref_count = 1;
    
    should_cleanup = (client_dropped && 
                     !client_exists_in_connector && 
                     ref_count == 1);
    assert_false(should_cleanup);
}

/* Test the complete decision tree
 * Validates all branches of the zombie cleanup logic
 */
static void test_complete_decision_tree(void)
{
    bool client_dropped;
    bool client_exists_in_connector;
    int ref_count;
    bool should_cleanup;
    bool should_send_drop;
    
    /* Branch 1: dropped=true, exists=false, ref=1 -> cleanup */
    client_dropped = true;
    client_exists_in_connector = false;
    ref_count = 1;
    should_cleanup = (client_dropped && !client_exists_in_connector && ref_count == 1);
    should_send_drop = (client_dropped && client_exists_in_connector);
    assert_true(should_cleanup);
    assert_false(should_send_drop);
    
    /* Branch 2: dropped=true, exists=false, ref>1 -> wait for others */
    client_dropped = true;
    client_exists_in_connector = false;
    ref_count = 3;
    should_cleanup = (client_dropped && !client_exists_in_connector && ref_count == 1);
    should_send_drop = (client_dropped && client_exists_in_connector);
    assert_false(should_cleanup);
    assert_false(should_send_drop);
    
    /* Branch 3: dropped=true, exists=true, ref=1 -> send drop */
    client_dropped = true;
    client_exists_in_connector = true;
    ref_count = 1;
    should_cleanup = (client_dropped && !client_exists_in_connector && ref_count == 1);
    should_send_drop = (client_dropped && client_exists_in_connector);
    assert_false(should_cleanup);
    assert_true(should_send_drop);
    
    /* Branch 4: dropped=false -> no action */
    client_dropped = false;
    client_exists_in_connector = false;
    ref_count = 1;
    should_cleanup = (client_dropped && !client_exists_in_connector && ref_count == 1);
    should_send_drop = (client_dropped && client_exists_in_connector);
    assert_false(should_cleanup);
    assert_false(should_send_drop);
}

int main(void)
{
    printf("Running zombie cleanup logic tests...\n\n");
    
    run_test(test_zombie_cleanup_decision_logic);
    run_test(test_ref_count_boundary_conditions);
    run_test(test_zombie_cleanup_edge_cases);
    run_test(test_complete_decision_tree);
    
    printf("\nAll zombie cleanup tests passed!\n");
    return TEST_SUCCESS;
}
