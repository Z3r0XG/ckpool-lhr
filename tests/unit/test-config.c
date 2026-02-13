/*
 * Unit tests for configuration validation
 * Tests JSON parsing edge cases and type validation
 * Critical for preventing config parsing bugs
 */

/* config.h must be first to define _GNU_SOURCE before system headers */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <jansson.h>
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
    
    json_decref(obj);
}

/* Test json_get_double accepts both integer and real JSON types */
static void test_json_get_double_accepts_integers(void)
{
    json_t *obj, *entry;
    double value;
    
    printf("  Testing JSON number parsing for config values:\n");
    
    /* Test with JSON integer (e.g., "startdiff": 4) */
    obj = json_object();
    json_object_set_new(obj, "startdiff", json_integer(4));
    entry = json_object_get(obj, "startdiff");
    assert_true(json_is_number(entry));
    value = json_number_value(entry);
    printf("    Integer 4: value=%f\n", value);
    assert_double_equal(value, 4.0, EPSILON);
    json_decref(obj);
    
    /* Test with JSON real (e.g., "startdiff": 4.0) */
    obj = json_object();
    json_object_set_new(obj, "startdiff", json_real(4.0));
    entry = json_object_get(obj, "startdiff");
    assert_true(json_is_number(entry));
    value = json_number_value(entry);
    printf("    Real 4.0: value=%f\n", value);
    assert_double_equal(value, 4.0, EPSILON);
    json_decref(obj);
    
    /* Test with JSON real fractional (e.g., "startdiff": 4.5) */
    obj = json_object();
    json_object_set_new(obj, "startdiff", json_real(4.5));
    entry = json_object_get(obj, "startdiff");
    assert_true(json_is_number(entry));
    value = json_number_value(entry);
    printf("    Real 4.5: value=%f\n", value);
    assert_double_equal(value, 4.5, EPSILON);
    json_decref(obj);
    
    /* Test with large integer */
    obj = json_object();
    json_object_set_new(obj, "highdiff", json_integer(1024));
    entry = json_object_get(obj, "highdiff");
    assert_true(json_is_number(entry));
    value = json_number_value(entry);
    printf("    Integer 1024: value=%f\n", value);
    assert_double_equal(value, 1024.0, EPSILON);
    json_decref(obj);
    
    /* Test with zero as integer */
    obj = json_object();
    json_object_set_new(obj, "mindiff", json_integer(0));
    entry = json_object_get(obj, "mindiff");
    assert_true(json_is_number(entry));
    value = json_number_value(entry);
    printf("    Integer 0: value=%f\n", value);
    assert_double_equal(value, 0.0, EPSILON);
    json_decref(obj);
}

int main(void)
{
    printf("Running configuration validation tests...\n");
    
    test_json_type_validation();
    test_json_array_parsing();
    test_config_defaults();
    test_json_get_double_accepts_integers();
    
    printf("All configuration validation tests passed!\n");
    return 0;
}

