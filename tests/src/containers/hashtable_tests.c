#include "hashtable_tests.h"

#include <containers/hashtable.h>
#include "../test_manager.h"
#include "../expect.h"
#include <core/logger.h>
#include <core/cmemory.h>
#include <core/cstring.h>

// Test the creation of a hashtable for primitive data types
u8 test_hashtable_create_primitive() {
    hashtable table;
    u32 element_count = 128;
    u64 element_size = sizeof(u32);
    void* memory = callocate(element_size * element_count, MEMORY_TAG_DARRAY);
    
    hashtable_create(element_size, element_count, memory, false, &table);
    
    expect_should_be(element_size, table.element_size);
    expect_should_be(element_count, table.element_count);
    expect_should_be(memory, table.memory);
    expect_to_be_false(table.is_pointer_type);
    
    hashtable_destroy(&table);
    cfree(memory, element_size * element_count, MEMORY_TAG_DARRAY);
    
    return true;
}

// Test the creation of a hashtable for pointer data types
u8 test_hashtable_create_pointers() {
    hashtable table;
    u32 element_count = 64;
    u64 element_size = sizeof(void*);
    void* memory = callocate(element_size * element_count, MEMORY_TAG_DARRAY);
    
    hashtable_create(element_size, element_count, memory, true, &table);
    
    expect_should_be(element_size, table.element_size);
    expect_should_be(element_count, table.element_count);
    expect_should_be(memory, table.memory);
    expect_to_be_true(table.is_pointer_type);
    
    hashtable_destroy(&table);
    cfree(memory, element_size * element_count, MEMORY_TAG_DARRAY);
    
    return true;
}

// Test setting and getting primitive values
u8 test_hashtable_set_get_primitive() {
    hashtable table;
    u32 element_count = 128;
    u64 element_size = sizeof(u32);
    void* memory = callocate(element_size * element_count, MEMORY_TAG_DARRAY);
    
    hashtable_create(element_size, element_count, memory, false, &table);
    
    // Test setting and getting a single value
    u32 test_value = 42;
    b8 set_result = hashtable_set(&table, "answer", &test_value);
    expect_to_be_true(set_result);
    
    u32 retrieved_value = 0;
    b8 get_result = hashtable_get(&table, "answer", &retrieved_value);
    expect_to_be_true(get_result);
    expect_should_be(test_value, retrieved_value);
    
    // Test setting and getting multiple values
    u32 value1 = 123;
    u32 value2 = 456;
    u32 value3 = 789;
    
    hashtable_set(&table, "key1", &value1);
    hashtable_set(&table, "key2", &value2);
    hashtable_set(&table, "key3", &value3);
    
    u32 result1 = 0, result2 = 0, result3 = 0;
    
    hashtable_get(&table, "key1", &result1);
    hashtable_get(&table, "key2", &result2);
    hashtable_get(&table, "key3", &result3);
    
    expect_should_be(value1, result1);
    expect_should_be(value2, result2);
    expect_should_be(value3, result3);
    
    // Test updating a value
    u32 updated_value = 999;
    hashtable_set(&table, "key2", &updated_value);
    
    u32 updated_result = 0;
    hashtable_get(&table, "key2", &updated_result);
    expect_should_be(updated_value, updated_result);
    
    hashtable_destroy(&table);
    cfree(memory, element_size * element_count, MEMORY_TAG_DARRAY);
    
    return true;
}

// Test setting and getting pointer values
u8 test_hashtable_set_get_pointers() {
    hashtable table;
    u32 element_count = 64;
    u64 element_size = sizeof(void*);
    void* memory = callocate(element_size * element_count, MEMORY_TAG_DARRAY);
    
    hashtable_create(element_size, element_count, memory, true, &table);
    
    // Create some test structures to store as pointers
    typedef struct test_struct {
        u32 id;
        f32 value;
    } test_struct;
    
    test_struct* obj1 = callocate(sizeof(test_struct), MEMORY_TAG_UNKNOWN);
    test_struct* obj2 = callocate(sizeof(test_struct), MEMORY_TAG_UNKNOWN);
    test_struct* obj3 = callocate(sizeof(test_struct), MEMORY_TAG_UNKNOWN);
    
    obj1->id = 1;
    obj1->value = 1.1f;
    
    obj2->id = 2;
    obj2->value = 2.2f;
    
    obj3->id = 3;
    obj3->value = 3.3f;
    
    // Test setting pointers
    b8 set_result1 = hashtable_set_ptr(&table, "ptr1", (void**)&obj1);
    b8 set_result2 = hashtable_set_ptr(&table, "ptr2", (void**)&obj2);
    b8 set_result3 = hashtable_set_ptr(&table, "ptr3", (void**)&obj3);
    
    expect_to_be_true(set_result1);
    expect_to_be_true(set_result2);
    expect_to_be_true(set_result3);
    
    // Test getting pointers
    void* result1 = NULL;
    void* result2 = NULL;
    void* result3 = NULL;
    
    b8 get_result1 = hashtable_get_ptr(&table, "ptr1", &result1);
    b8 get_result2 = hashtable_get_ptr(&table, "ptr2", &result2);
    b8 get_result3 = hashtable_get_ptr(&table, "ptr3", &result3);
    
    expect_to_be_true(get_result1);
    expect_to_be_true(get_result2);
    expect_to_be_true(get_result3);
    
    expect_should_be(obj1, result1);
    expect_should_be(obj2, result2);
    expect_should_be(obj3, result3);
    
    // Verify data integrity through the pointers
    test_struct* retrieved1 = (test_struct*)result1;
    test_struct* retrieved2 = (test_struct*)result2;
    test_struct* retrieved3 = (test_struct*)result3;
    
    expect_should_be(1, retrieved1->id);
    expect_float_to_be(1.1f, retrieved1->value);
    
    expect_should_be(2, retrieved2->id);
    expect_float_to_be(2.2f, retrieved2->value);
    
    expect_should_be(3, retrieved3->id);
    expect_float_to_be(3.3f, retrieved3->value);
    
    // Test updating a pointer
    test_struct* obj4 = callocate(sizeof(test_struct), MEMORY_TAG_UNKNOWN);
    obj4->id = 4;
    obj4->value = 4.4f;
    
    hashtable_set_ptr(&table, "ptr2", (void**)&obj4);
    
    void* updated_result = NULL;
    hashtable_get_ptr(&table, "ptr2", &updated_result);
    test_struct* updated = (test_struct*)updated_result;
    
    expect_should_be(obj4, updated_result);
    expect_should_be(4, updated->id);
    expect_float_to_be(4.4f, updated->value);
    
    // Test NULL pointer
    hashtable_set_ptr(&table, "null_ptr", NULL);
    void* null_result = (void*)0xDEADBEEF; // Initialize to non-NULL
    b8 null_get_result = hashtable_get_ptr(&table, "null_ptr", &null_result);
    
    expect_to_be_false(null_get_result); // Should return false for NULL value
    expect_should_be(0, null_result);    // Should set the output to NULL
    
    // Cleanup
    hashtable_destroy(&table);
    cfree(memory, element_size * element_count, MEMORY_TAG_DARRAY);
    cfree(obj1, sizeof(test_struct), MEMORY_TAG_UNKNOWN);
    cfree(obj2, sizeof(test_struct), MEMORY_TAG_UNKNOWN);
    cfree(obj3, sizeof(test_struct), MEMORY_TAG_UNKNOWN);
    cfree(obj4, sizeof(test_struct), MEMORY_TAG_UNKNOWN);
    
    return true;
}

// Test hash collisions by using a very small hashtable
u8 test_hashtable_collisions() {
    hashtable table;
    u32 element_count = 1; // Only one slot, guaranteed collisions
    u64 element_size = sizeof(u32);
    void* memory = callocate(element_size * element_count, MEMORY_TAG_DARRAY);
    
    hashtable_create(element_size, element_count, memory, false, &table);
    
    // Fill the single slot with multiple values
    u32 value1 = 111;
    u32 value2 = 222;
    u32 value3 = 333;
    
    hashtable_set(&table, "key1", &value1);
    hashtable_set(&table, "key2", &value2);
    hashtable_set(&table, "key3", &value3);
    
    // All values should map to the same slot due to modulo operation in hash_name
    // Only the last value set should be retrievable for any key
    u32 result1 = 0;
    u32 result2 = 0;
    u32 result3 = 0;
    
    hashtable_get(&table, "key1", &result1);
    hashtable_get(&table, "key2", &result2);
    hashtable_get(&table, "key3", &result3);
    
    // All results should be value3 (the last one set)
    expect_should_be(value3, result1);
    expect_should_be(value3, result2);
    expect_should_be(value3, result3);
    
    hashtable_destroy(&table);
    cfree(memory, element_size * element_count, MEMORY_TAG_DARRAY);
    
    return true;
}

// Test the hashtable_fill function
u8 test_hashtable_fill() {
    hashtable table;
    u32 element_count = 10;
    u64 element_size = sizeof(u32);
    void* memory = callocate(element_size * element_count, MEMORY_TAG_DARRAY);
    
    hashtable_create(element_size, element_count, memory, false, &table);
    
    // Fill the hashtable with a default value
    u32 default_value = 42;
    b8 fill_result = hashtable_fill(&table, &default_value);
    expect_to_be_true(fill_result);
    
    // Verify random positions all have the default value
    u32 result1 = 0, result2 = 0, result3 = 0;
    
    hashtable_get(&table, "key1", &result1);
    hashtable_get(&table, "key2", &result2);
    hashtable_get(&table, "some_random_key", &result3);
    
    expect_should_be(default_value, result1);
    expect_should_be(default_value, result2);
    expect_should_be(default_value, result3);
    
    // Override a value and verify it changed
    u32 new_value = 99;
    hashtable_set(&table, "key1", &new_value);
    
    u32 updated_result = 0;
    hashtable_get(&table, "key1", &updated_result);
    expect_should_be(new_value, updated_result);
    
    // Other keys should still have the default
    u32 other_result = 0;
    hashtable_get(&table, "key2", &other_result);
    expect_should_be(default_value, other_result);
    
    hashtable_destroy(&table);
    cfree(memory, element_size * element_count, MEMORY_TAG_DARRAY);
    
    return true;
}

// Test error handling for invalid inputs
u8 test_hashtable_error_handling() {
    hashtable table;
    u32 element_count = 32;
    u64 element_size = sizeof(u32);
    void* memory = callocate(element_size * element_count, MEMORY_TAG_DARRAY);
    
    hashtable_create(element_size, element_count, memory, false, &table);
    
    // Test with NULL table
    u32 test_value = 123;
    b8 set_result = hashtable_set(NULL, "key", &test_value);
    expect_to_be_false(set_result);
    
    // Test with NULL name
    set_result = hashtable_set(&table, NULL, &test_value);
    expect_to_be_false(set_result);
    
    // Test with NULL value
    set_result = hashtable_set(&table, "key", NULL);
    expect_to_be_false(set_result);
    
    // Test get with NULL table
    u32 result = 0;
    b8 get_result = hashtable_get(NULL, "key", &result);
    expect_to_be_false(get_result);
    
    // Test get with NULL name
    get_result = hashtable_get(&table, NULL, &result);
    expect_to_be_false(get_result);
    
    // Test get with NULL out_value
    get_result = hashtable_get(&table, "key", NULL);
    expect_to_be_false(get_result);
    
    // Test incorrect type operations
    
    // Try to use set on a pointer-type table
    hashtable ptr_table;
    void* ptr_memory = callocate(sizeof(void*) * element_count, MEMORY_TAG_DARRAY);
    hashtable_create(sizeof(void*), element_count, ptr_memory, true, &ptr_table);
    
    set_result = hashtable_set(&ptr_table, "key", &test_value); // Wrong function for pointer table
    expect_to_be_false(set_result);
    
    void* ptr_value = &test_value;
    b8 set_ptr_result = hashtable_set_ptr(&table, "key", &ptr_value); // Wrong function for non-pointer table
    expect_to_be_false(set_ptr_result);
    
    // Test get with wrong type
    void* ptr_result = NULL;
    b8 get_ptr_result = hashtable_get_ptr(&table, "key", &ptr_result); // Wrong function for non-pointer table
    expect_to_be_false(get_ptr_result);
    
    get_result = hashtable_get(&ptr_table, "key", &result); // Wrong function for pointer table
    expect_to_be_false(get_result);
    
    // Cleanup
    hashtable_destroy(&table);
    hashtable_destroy(&ptr_table);
    cfree(memory, element_size * element_count, MEMORY_TAG_DARRAY);
    cfree(ptr_memory, sizeof(void*) * element_count, MEMORY_TAG_DARRAY);
    
    return true;
}

// Test with string values
u8 test_hashtable_string_values() {
    hashtable table;
    u32 element_count = 32;
    // Use 32 bytes for string storage
    u64 element_size = 32;
    void* memory = callocate(element_size * element_count, MEMORY_TAG_DARRAY);
    
    hashtable_create(element_size, element_count, memory, false, &table);
    
    // Test with string values
    char str1[] = "Hello, World!";
    char str2[] = "Testing string storage";
    char str3[] = "CEngine Hashtable";
    
    hashtable_set(&table, "greeting", str1);
    hashtable_set(&table, "message", str2);
    hashtable_set(&table, "title", str3);
    
    char result1[32] = {0};
    char result2[32] = {0};
    char result3[32] = {0};
    
    hashtable_get(&table, "greeting", result1);
    hashtable_get(&table, "message", result2);
    hashtable_get(&table, "title", result3);
    
    expect_to_be_true(string_equals(str1, result1));
    expect_to_be_true(string_equals(str2, result2));
    expect_to_be_true(string_equals(str3, result3));
    
    // Update a string
    char updated_str[] = "Updated message";
    hashtable_set(&table, "message", updated_str);
    
    char updated_result[32] = {0};
    hashtable_get(&table, "message", updated_result);
    expect_to_be_true(string_equals(updated_str, updated_result));
    
    hashtable_destroy(&table);
    cfree(memory, element_size * element_count, MEMORY_TAG_DARRAY);
    
    return true;
}

// Test with floating point values
u8 test_hashtable_float_values() {
    hashtable table;
    u32 element_count = 32;
    u64 element_size = sizeof(f32);
    void* memory = callocate(element_size * element_count, MEMORY_TAG_DARRAY);
    
    hashtable_create(element_size, element_count, memory, false, &table);
    
    // Test with float values
    f32 value1 = 3.14159f;
    f32 value2 = 2.71828f;
    f32 value3 = 1.61803f;
    
    hashtable_set(&table, "pi", &value1);
    hashtable_set(&table, "e", &value2);
    hashtable_set(&table, "golden_ratio", &value3);
    
    f32 result1 = 0.0f;
    f32 result2 = 0.0f;
    f32 result3 = 0.0f;
    
    hashtable_get(&table, "pi", &result1);
    hashtable_get(&table, "e", &result2);
    hashtable_get(&table, "golden_ratio", &result3);
    
    expect_float_to_be(value1, result1);
    expect_float_to_be(value2, result2);
    expect_float_to_be(value3, result3);
    
    hashtable_destroy(&table);
    cfree(memory, element_size * element_count, MEMORY_TAG_DARRAY);
    
    return true;
}

// Test with struct values
u8 test_hashtable_struct_values() {
    typedef struct test_vector3 {
        f32 x;
        f32 y;
        f32 z;
    } test_vector3;
    
    hashtable table;
    u32 element_count = 32;
    u64 element_size = sizeof(test_vector3);
    void* memory = callocate(element_size * element_count, MEMORY_TAG_DARRAY);
    
    hashtable_create(element_size, element_count, memory, false, &table);
    
    // Create some test vectors
    test_vector3 pos1 = {1.0f, 2.0f, 3.0f};
    test_vector3 pos2 = {4.0f, 5.0f, 6.0f};
    test_vector3 pos3 = {7.0f, 8.0f, 9.0f};
    
    hashtable_set(&table, "player_pos", &pos1);
    hashtable_set(&table, "enemy_pos", &pos2);
    hashtable_set(&table, "target_pos", &pos3);
    
    test_vector3 result1 = {0};
    test_vector3 result2 = {0};
    test_vector3 result3 = {0};
    
    hashtable_get(&table, "player_pos", &result1);
    hashtable_get(&table, "enemy_pos", &result2);
    hashtable_get(&table, "target_pos", &result3);
    
    // Verify all struct fields
    expect_float_to_be(pos1.x, result1.x);
    expect_float_to_be(pos1.y, result1.y);
    expect_float_to_be(pos1.z, result1.z);
    
    expect_float_to_be(pos2.x, result2.x);
    expect_float_to_be(pos2.y, result2.y);
    expect_float_to_be(pos2.z, result2.z);
    
    expect_float_to_be(pos3.x, result3.x);
    expect_float_to_be(pos3.y, result3.y);
    expect_float_to_be(pos3.z, result3.z);
    
    // Update a struct
    test_vector3 updated_pos = {10.0f, 11.0f, 12.0f};
    hashtable_set(&table, "player_pos", &updated_pos);
    
    test_vector3 updated_result = {0};
    hashtable_get(&table, "player_pos", &updated_result);
    
    expect_float_to_be(updated_pos.x, updated_result.x);
    expect_float_to_be(updated_pos.y, updated_result.y);
    expect_float_to_be(updated_pos.z, updated_result.z);
    
    hashtable_destroy(&table);
    cfree(memory, element_size * element_count, MEMORY_TAG_DARRAY);
    
    return true;
}

// Register all hashtable tests
void hashtable_register_tests() {
    test_manager_register_test(test_hashtable_create_primitive, "Hashtable creation for primitive types");
    test_manager_register_test(test_hashtable_create_pointers, "Hashtable creation for pointer types");
    test_manager_register_test(test_hashtable_set_get_primitive, "Hashtable set/get operations for primitive types");
    test_manager_register_test(test_hashtable_set_get_pointers, "Hashtable set/get operations for pointer types");
    test_manager_register_test(test_hashtable_collisions, "Hashtable collision handling");
    test_manager_register_test(test_hashtable_fill, "Hashtable fill operation");
    test_manager_register_test(test_hashtable_error_handling, "Hashtable error handling");
    test_manager_register_test(test_hashtable_string_values, "Hashtable with string values");
    test_manager_register_test(test_hashtable_float_values, "Hashtable with floating point values");
    test_manager_register_test(test_hashtable_struct_values, "Hashtable with struct values");
}
