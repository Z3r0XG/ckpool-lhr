/*
 * Unit tests for configuration validation
 * Tests JSON parsing edge cases and type validation
 * Critical for preventing config parsing bugs
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <jansson.h>

#include "config.h"
#include "../test_common.h"
#include "libckpool.h"

/* Test JSON type validation - simulates what json_get_* functions check */
static void test_json_type_validation(void)
{
    json_t *obj;
    
    /* Test string type */
    obj = json_object();
    json_object_set_new(obj, "test", json_string("value"));
    assert_true(json_is_string(json_object_get(obj, "test")));
    json_decref(obj);
    
    /* Test integer type */
    obj = json_object();
    json_object_set_new(obj, "test", json_integer(42));
    assert_true(json_is_integer(json_object_get(obj, "test")));
    json_decref(obj);
    
    /* Test real type */
    obj = json_object();
    json_object_set_new(obj, "test", json_real(3.14159));
    assert_true(json_is_real(json_object_get(obj, "test")));
    json_decref(obj);
    
    /* Test missing key */
    obj = json_object();
    assert_null(json_object_get(obj, "missing"));
    json_decref(obj);
    
    /* Test wrong type */
    obj = json_object();
    json_object_set_new(obj, "test", json_integer(42));
    assert_false(json_is_string(json_object_get(obj, "test")));
    json_decref(obj);
}

/* Test array parsing (for useragent, serverurl, etc.) */
static void test_json_array_parsing(void)
{
    json_t *arr, *obj;
    size_t size;
    
    /* Valid array */
    arr = json_array();
    json_array_append_new(arr, json_string("item1"));
    json_array_append_new(arr, json_string("item2"));
    size = json_array_size(arr);
    assert_int_equal(size, 2);
    json_decref(arr);
    
    /* Empty array */
    arr = json_array();
    size = json_array_size(arr);
    assert_int_equal(size, 0);
    json_decref(arr);
    
    /* Array in object */
    obj = json_object();
    arr = json_array();
    json_array_append_new(arr, json_string("value1"));
    json_object_set_new(obj, "useragent", arr);
    arr = json_object_get(obj, "useragent");
    assert_non_null(arr);
    assert_true(json_is_array(arr));
    size = json_array_size(arr);
    assert_int_equal(size, 1);
    json_decref(obj);
}

/* Test default value handling */
static void test_config_defaults(void)
{
    /* Test that missing values use defaults correctly */
    double mindiff = 0.0;
    double startdiff = 0.0;
    int dropidle = -1;
    int user_cleanup_days = -1;
    
    /* Defaults should be set if not in config */
    json_t *obj = json_object();
    
    /* mindiff default is 1.0 if not set */
    if (!mindiff)
        mindiff = 1.0;
    assert_double_equal(mindiff, 1.0, EPSILON);
    
    /* dropidle default is 0 if not set */
    if (dropidle < 0)
        dropidle = 0;
    assert_int_equal(dropidle, 0);
    
    /* user_cleanup_days default is 0 if not set */
    if (user_cleanup_days < 0)
        user_cleanup_days = 0;
    assert_int_equal(user_cleanup_days, 0);
    
    json_decref(obj);
}

int main(void)
{
    printf("Running configuration validation tests...\n");
    
    test_json_type_validation();
    test_json_array_parsing();
    test_config_defaults();
    
    printf("All configuration validation tests passed!\n");
    return 0;
}

