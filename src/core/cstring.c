#include "core/cstring.h"
#include "core/cmemory.h"

#include <string.h>

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