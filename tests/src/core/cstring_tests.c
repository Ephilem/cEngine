#include "cstring_tests.h"

#include <core/cstring.h>
#include "../test_manager.h"
#include "../expect.h"
#include <core/logger.h>
#include <core/cmemory.h>
#include <math/cmath.h>

// Test string_length function
u8 test_string_length() {
    const char* empty = "";
    const char* short_str = "Hello";
    const char* long_str = "This is a much longer string to test the string_length function";
    
    expect_should_be(0, string_length(empty));
    expect_should_be(5, string_length(short_str));
    expect_should_be(63, string_length(long_str));
    
    return true;
}

// Test string_duplicate function
u8 test_string_duplicate() {
    const char* original = "Test string for duplication";
    
    char* duplicate = string_duplicate(original);
    
    // Check that the duplicate is not NULL
    expect_to_be_true(duplicate != NULL);
    
    // Check that the content is identical
    expect_to_be_true(string_equals(original, duplicate));
    
    // Make sure they are different memory addresses
    expect_to_be_true(original != duplicate);
    
    // Check length is the same
    expect_should_be(string_length(original), string_length(duplicate));
    
    // Clean up
    cfree(duplicate, string_length(original) + 1, MEMORY_TAG_STRING);
    
    return true;
}

// Test string_equals function
u8 test_string_equals() {
    const char* str1 = "Hello World";
    const char* str2 = "Hello World";
    const char* str3 = "hello world"; // Different case
    const char* str4 = "Hello"; // Different content
    
    // Same strings should be equal
    expect_to_be_true(string_equals(str1, str2));
    
    // Different case should not be equal
    expect_to_be_false(string_equals(str1, str3));
    
    // Different content should not be equal
    expect_to_be_false(string_equals(str1, str4));
    
    // String should equal itself
    expect_to_be_true(string_equals(str1, str1));
    
    return true;
}

// Test string_equals_case function
u8 test_string_equals_case() {
    const char* str1 = "Hello World";
    const char* str2 = "hello world"; // Different case
    const char* str3 = "HELLO WORLD"; // All caps
    const char* str4 = "Hello"; // Different content
    
    // Same strings with different case should be equal
    expect_to_be_true(string_equals_case(str1, str2));
    expect_to_be_true(string_equals_case(str1, str3));
    
    // Different content should not be equal
    expect_to_be_false(string_equals_case(str1, str4));
    
    // String should equal itself
    expect_to_be_true(string_equals_case(str1, str1));
    
    return true;
}

// Test string_format function
u8 test_string_format() {
    char buffer[128];
    
    // Test with basic format
    i32 result1 = string_format(buffer, "Basic string");
    expect_to_be_true(string_equals(buffer, "Basic string"));
    expect_should_be(12, result1); // Length of "Basic string"
    
    // Test with integer format
    i32 result2 = string_format(buffer, "Value: %d", 42);
    expect_to_be_true(string_equals(buffer, "Value: 42"));
    expect_should_be(9, result2); // Length of "Value: 42"
    
    // Test with multiple format specifiers
    i32 result3 = string_format(buffer, "%s %d %.2f", "Number", 123, 3.14159f);
    expect_to_be_true(string_equals(buffer, "Number 123 3.14"));
    expect_should_be(15, result3); // Length of "Number 123 3.14"
    
    // Test with NULL destination (should return -1)
    i32 result4 = string_format(NULL, "Test");
    expect_should_be(-1, result4);
    
    return true;
}

// Test string_copy and string_ncopy functions
u8 test_string_copy() {
    char dest1[32] = {0};
    char dest2[10] = {0};
    const char* src = "Hello World";
    
    // Test string_copy
    char* result1 = string_copy(dest1, src);
    expect_to_be_true(string_equals(dest1, src));
    expect_should_be(dest1, result1); // Return value should be the destination
    
    // Test string_ncopy with limit
    char* result2 = string_ncopy(dest2, src, 5);
    expect_to_be_true(string_equals(dest2, "Hello")); // Only first 5 chars
    expect_should_be(dest2, result2); // Return value should be the destination
    
    return true;
}

// Test string_trim function
u8 test_string_trim() {
    char str1[] = "  Hello World  ";
    char str2[] = "Hello World";
    char str3[] = "   ";
    
    // Test with spaces on both sides
    char* result1 = string_trim(str1);
    expect_to_be_true(string_equals(result1, "Hello World"));
    
    // Test with no spaces
    char* result2 = string_trim(str2);
    expect_to_be_true(string_equals(result2, "Hello World"));
    
    // Test with only spaces
    char* result3 = string_trim(str3);
    expect_to_be_true(string_equals(result3, ""));
    
    return true;
}

// Test string_mid function
u8 test_string_mid() {
    const char* source = "Hello World";
    char dest[32];
    
    // Test with start in range and positive length
    string_mid(dest, source, 1, 4);
    expect_to_be_true(string_equals(dest, "ello"));
    
    // Test with start at beginning and positive length
    string_mid(dest, source, 0, 5);
    expect_to_be_true(string_equals(dest, "Hello"));
    
    // Test with negative length (should go to end of string)
    string_mid(dest, source, 6, -1);
    expect_to_be_true(string_equals(dest, "World"));
    
    // Test with start out of range
    string_mid(dest, source, 20, 5);
    expect_to_be_true(string_equals(dest, ""));
    
    // Test with zero length
    string_mid(dest, source, 0, 0);
    // Destination should remain unchanged from previous test
    expect_to_be_true(string_equals(dest, ""));
    
    return true;
}

// Test string_index_of function
u8 test_string_index_of() {
    char str[] = "Hello World";
    
    // Test finding characters
    expect_should_be(0, string_index_of(str, 'H'));
    expect_should_be(1, string_index_of(str, 'e'));
    expect_should_be(6, string_index_of(str, 'W'));
    
    // Test character not in string
    expect_should_be(-1, string_index_of(str, 'z'));
    
    // Test with NULL string
    expect_should_be(-1, string_index_of(NULL, 'a'));
    
    return true;
}

// Test string_to_vec functions
u8 test_string_to_vec() {
    vec2 v2;
    vec3 v3;
    vec4 v4;
    
    // Test vec2 conversion
    expect_to_be_true(string_to_vec2("1.0 2.0", &v2));
    expect_float_to_be(1.0f, v2.x);
    expect_float_to_be(2.0f, v2.y);
    
    // Test vec3 conversion
    expect_to_be_true(string_to_vec3("3.0 4.0 5.0", &v3));
    expect_float_to_be(3.0f, v3.x);
    expect_float_to_be(4.0f, v3.y);
    expect_float_to_be(5.0f, v3.z);
    
    // Test vec4 conversion
    expect_to_be_true(string_to_vec4("6.0 7.0 8.0 9.0", &v4));
    expect_float_to_be(6.0f, v4.x);
    expect_float_to_be(7.0f, v4.y);
    expect_float_to_be(8.0f, v4.z);
    expect_float_to_be(9.0f, v4.w);
    
    // Test with null pointers
    expect_to_be_false(string_to_vec2(NULL, &v2));
    expect_to_be_false(string_to_vec2("1.0 2.0", NULL));
    
    expect_to_be_false(string_to_vec3(NULL, &v3));
    expect_to_be_false(string_to_vec3("1.0 2.0 3.0", NULL));
    
    expect_to_be_false(string_to_vec4(NULL, &v4));
    expect_to_be_false(string_to_vec4("1.0 2.0 3.0 4.0", NULL));
    
    return true;
}

// Test string_to_f32 and string_to_f64 functions
u8 test_string_to_float() {
    f32 f32_value;
    f64 f64_value;
    
    // Test f32 conversion
    expect_to_be_true(string_to_f32("3.14159", &f32_value));
    expect_float_to_be(3.14159f, f32_value);
    
    // Test f64 conversion
    expect_to_be_true(string_to_f64("2.71828", &f64_value));
    expect_should_be(1, c_absf(2.71828 - f64_value) < 0.0001);
    
    // Test invalid inputs
    expect_to_be_false(string_to_f32(NULL, &f32_value));
    expect_to_be_false(string_to_f32("3.14", NULL));
    
    expect_to_be_false(string_to_f64(NULL, &f64_value));
    expect_to_be_false(string_to_f64("2.718", NULL));
    
    return true;
}

// Test string_to_i* and string_to_u* functions
u8 test_string_to_integer() {
    i8 i8_value;
    i16 i16_value;
    i32 i32_value;
    i64 i64_value;
    
    u8 u8_value;
    u16 u16_value;
    u32 u32_value;
    u64 u64_value;
    
    // Test signed conversions
    expect_to_be_true(sintrg_to_i8("42", &i8_value));
    expect_should_be(42, i8_value);
    
    expect_to_be_true(string_to_i16("1000", &i16_value));
    expect_should_be(1000, i16_value);
    
    expect_to_be_true(string_to_i32("-12345", &i32_value));
    expect_should_be(-12345, i32_value);
    
    expect_to_be_true(string_to_i64("9876543210", &i64_value));
    expect_should_be(9876543210LL, i64_value);
    
    // Test unsigned conversions
    expect_to_be_true(string_to_u8("200", &u8_value));
    expect_should_be(200, u8_value);
    
    expect_to_be_true(string_to_u16("60000", &u16_value));
    expect_should_be(60000, u16_value);
    
    expect_to_be_true(string_to_u32("4000000000", &u32_value));
    expect_should_be(4000000000U, u32_value);
    
    expect_to_be_true(string_to_u64("10000000000", &u64_value));
    expect_should_be(10000000000ULL, u64_value);
    
    // Test invalid inputs
    expect_to_be_false(sintrg_to_i8(NULL, &i8_value));
    expect_to_be_false(sintrg_to_i8("100", NULL));
    
    expect_to_be_false(string_to_u64(NULL, &u64_value));
    expect_to_be_false(string_to_u64("12345", NULL));
    
    return true;
}

// Test string_to_bool function
u8 test_string_to_bool() {
    b8 bool_value;
    
    // Test "true" string
    bool_value = false;
    expect_to_be_true(string_to_bool("true", &bool_value));
    expect_to_be_true(bool_value);
    
    // Test "1" string
    bool_value = false;
    expect_to_be_true(string_to_bool("1", &bool_value));
    expect_to_be_true(bool_value);
    
    // Test other strings (should all be false)
    bool_value = true; // Set to true first to verify it changes
    string_to_bool("false", &bool_value);
    expect_to_be_false(bool_value);
    
    bool_value = true;
    string_to_bool("0", &bool_value);
    expect_to_be_false(bool_value);
    
    bool_value = true;
    string_to_bool("something", &bool_value);
    expect_to_be_false(bool_value);
    
    // Test invalid inputs
    expect_to_be_false(string_to_bool(NULL, &bool_value));
    expect_to_be_false(string_to_bool("true", NULL));
    
    return true;
}

// Register all cstring tests
void cstring_register_tests() {
    test_manager_register_test(test_string_length, "CString length calculation");
    test_manager_register_test(test_string_duplicate, "CString duplication");
    test_manager_register_test(test_string_equals, "CString case-sensitive comparison");
    test_manager_register_test(test_string_equals_case, "CString case-insensitive comparison");
    test_manager_register_test(test_string_format, "CString formatting");
    test_manager_register_test(test_string_copy, "CString copy and limited copy");
    test_manager_register_test(test_string_trim, "CString whitespace trimming");
    test_manager_register_test(test_string_mid, "CString substring extraction");
    test_manager_register_test(test_string_index_of, "CString character index finding");
    test_manager_register_test(test_string_to_vec, "CString to vector conversion");
    test_manager_register_test(test_string_to_float, "CString to float conversion");
    test_manager_register_test(test_string_to_integer, "CString to integer conversion");
    test_manager_register_test(test_string_to_bool, "CString to boolean conversion");
}