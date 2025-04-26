#pragma once

#include "define.h"
#include "math/math_types.h"

u64 string_length(const char* str);

char* string_duplicate(const char* str);

b8 string_equals(const char* a, const char* b);

b8 string_equals_case(const char* a, const char* b);

i32 string_format(char* dest, const char* format, ...);

i32 string_format_v(char* dest, const char* format, void* va_listp);

char* string_empty(char* str);

char* string_copy(char* dest, const char* src);
char* string_ncopy(char* dest, const char* src, i32 len);

char* string_trim(char* str);
void string_mid(char* dest, const char* source, i32 start, i32 length);

/**
 * Get the index of the first occurrence of a character in a string
 * @param str string to be scanned
 * @param c character to look for
 * @return the index of the first occurrence of c in str, or -1 if not found
 */
i32 string_index_of(char* str, char c);

//// Method to parse a string into a vector of a given dimension. values should be space-delimited
b8 string_to_vec4(char* str, vec4* out_vec4);
b8 string_to_vec3(char* str, vec3* out_vec3);
b8 string_to_vec2(char* str, vec2* out_vec2);


b8 string_to_f32(char* str, f32* out_f32);
b8 string_to_f64(char* str, f64* out_f64);

b8 sintrg_to_i8(char* str, i8* out_i8);
b8 string_to_i16(char* str, i16* out_i16);
b8 string_to_i32(char* str, i32* out_i32);
b8 string_to_i64(char* str, i64* out_i64);
b8 string_to_u8(char* str, u8* out_u8);
b8 string_to_u16(char* str, u16* out_u16);
b8 string_to_u32(char* str, u32* out_u32);
b8 string_to_u64(char* str, u64* out_u64);

/**
 * Convert a string to a boolean value.
 * "true" or "1" will be converted to true, anything else will be converted to false.
 * @param str string to be converted
 * @param out_bool pointer to the boolean value to be set
 * @return true if the conversion was successful, false otherwise
 */
b8 string_to_bool(char* str, b8* out_bool);