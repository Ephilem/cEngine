#include "core/cstring.h"
#include "core/cmemory.h"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h> // use for isspace

u64 string_length(const char* str) {
    return strlen(str);
}

char* string_duplicate(const char* str) {
    u64 length = string_length(str);
    char* copy = callocate(length + 1, MEMORY_TAG_STRING);
    ccopy_memory(copy, str, length + 1);
    return copy;
}

b8 string_equals(const char* a, const char* b) {
    return strcmp(a, b) == 0;
}

b8 string_equals_case(const char *a, const char *b) {
#if defined(__GNUC__)
    return strcasecmp(a, b) == 0;
#elif defined(_MSC_VER)
    return _strcmpi(a, b) == 0;
#endif
}

i32 string_format(char* dest, const char* format, ...) {
    if (dest) {
        __builtin_va_list arg_ptr;
        va_start(arg_ptr, format);
        i32 written = string_format_v(dest, format, arg_ptr);
        va_end(arg_ptr);
        return written;
    }
    return -1;
}

i32 string_format_v(char* dest, const char* format, void* va_listp) {
    if (dest) {
        // Big, but can fit on the stack.
        char buffer[32000];
        i32 written = vsnprintf(buffer, 32000, format, va_listp);
        buffer[written] = 0;
        ccopy_memory(dest, buffer, written + 1);

        return written;
    }
    return -1;
}

char* string_empty(char* str) {
    if (str) {
        str[0] = 0;
    }

    return str;
}

char* string_copy(char* dest, const char* src) {
    return strcpy(dest, src);
}

char* string_ncopy(char* dest, const char* src, i32 len) {
    return strncpy(dest, src, len);
}

char* string_trim(char* str) {
    while (isspace((unsigned char)*str)) {
        str++;
    }
    if (*str) {
        // goto the end of the string
        char* p = str;
        while (*p) {
            p++;
        }
        // backward loop to find the last non-space character
        while (isspace((unsigned char)*(--p)))
            ;

        p[1] = '\0';
    }

    return str;
}

void string_mid(char* dest, const char* source, i32 start, i32 length) {
    if (length == 0) {
        return;
    }

    u64 src_length = string_length(source);
    if (start >= src_length) {
        dest[0] = 0;
        return;
    }
    if (length > 0) {
        // keep copy within bounds
        for (u64 i = start, j = 0; j < length && i < src_length; ++i, ++j) {
            dest[j] = source[i];
        }
        dest[start + length] = 0; // null-terminate (end of string)
    } else {
        // if a negative value is passed, proceeed to the end of the string)
        u64 j = 0;
        for (u64 i = start; source[i]; ++i, ++j) {
            dest[j] = source[i];
        }
        dest[start + j] = 0;
    }
}

i32 string_index_of(char* str, char c) {
    if (!str) {
        return -1;
    }
    u32 length = string_length(str);
    if (length > 0) {
        for (u32 i = 0; i < length; ++i) {
            if (str[i] == c) {
                return i;
            }
        }
    }
    return -1;
}

b8 string_to_vec4(char* str, vec4* out_vec4) {
    if (!str || !out_vec4) {
        return false;
    }

    czero_memory(out_vec4, sizeof(*out_vec4));
    i32 result = sscanf(str, "%f %f %f %f", &out_vec4->x, &out_vec4->y, &out_vec4->z, &out_vec4->w);
    return result != -1;
}

b8 string_to_vec3(char* str, vec3* out_vec3) {
    if (!str || !out_vec3) {
        return false;
    }

    czero_memory(out_vec3, sizeof(*out_vec3));
    i32 result = sscanf(str, "%f %f %f", &out_vec3->x, &out_vec3->y, &out_vec3->z);
    return result != -1;
}

b8 string_to_vec2(char* str, vec2* out_vec2) {
    if (!str || !out_vec2) {
        return false;
    }

    czero_memory(out_vec2, sizeof(*out_vec2));
    i32 result = sscanf(str, "%f %f", &out_vec2->x, &out_vec2->y);
    return result != -1;
}

b8 string_to_f32(char* str, f32* out_f32) {
    if (!str || !out_f32) {
        return false;
    }

    *out_f32 = 0;
    i32 result = sscanf(str, "%f", out_f32);
    return result != -1;
}

b8 string_to_f64(char* str, f64* out_f64) {
    if (!str || !out_f64) {
        return false;
    }

    *out_f64 = 0;
    i32 result = sscanf(str, "%lf", out_f64);
    return result != -1;
}

b8 sintrg_to_i8(char* str, i8* out_i8) {
    if (!str || !out_i8) {
        return false;
    }

    *out_i8 = 0;
    i32 result = sscanf(str, "%hhi", out_i8);
    return result != -1;
}

b8 string_to_i16(char* str, i16* out_i16) {
    if (!str || !out_i16) {
        return false;
    }

    *out_i16 = 0;
    i32 result = sscanf(str, "%hi", out_i16);
    return result != -1;
}

b8 string_to_i32(char* str, i32* out_i32) {
    if (!str || !out_i32) {
        return false;
    }

    *out_i32 = 0;
    i32 result = sscanf(str, "%i", out_i32);
    return result != -1;
}

b8 string_to_i64(char* str, i64* out_i64) {
    if (!str || !out_i64) {
        return false;
    }

    *out_i64 = 0;
    i32 result = sscanf(str, "%lli", out_i64);
    return result != -1;
}

b8 string_to_u8(char* str, u8* out_u8) {
    if (!str || !out_u8) {
        return false;
    }

    *out_u8 = 0;
    i32 result = sscanf(str, "%hhu", out_u8);
    return result != -1;
}

b8 string_to_u16(char* str, u16* out_u16) {
    if (!str || !out_u16) {
        return false;
    }

    *out_u16 = 0;
    i32 result = sscanf(str, "%hu", out_u16);
    return result != -1;
}

b8 string_to_u32(char* str, u32* out_u32) {
    if (!str || !out_u32) {
        return false;
    }

    *out_u32 = 0;
    i32 result = sscanf(str, "%u", out_u32);
    return result != -1;
}

b8 string_to_u64(char* str, u64* out_u64) {
    if (!str || !out_u64) {
        return false;
    }

    *out_u64 = 0;
    i32 result = sscanf(str, "%llu", out_u64);
    return result != -1;
}

b8 string_to_bool(char* str, b8* out_bool) {
    if (!str || !out_bool) {
        return false;
    }

    return string_equals(str, "true") || string_equals(str, "1") ? (*out_bool = true, true) : (*out_bool = false, false);
}

