#pragma once

#include "define.h"

u64 string_length(const char* str);

char* string_duplicate(const char* str);

b8 string_equals(const char* a, const char* b);

i32 string_format(char* dest, const char* format, ...);

i32 string_format_v(char* dest, const char* format, void* va_listp);
