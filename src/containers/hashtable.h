#pragma once

#include "define.h"

/**
 * Represents a hashtable. memberes of this structure
 * should not be modified outsite the functions associated with it (like darray).
 *
 * For non-pointer tpyes :
 * - table retains a copy of the value
 *
 * For pointer types :
 * - make sure to use the _ptr setter and getter. Table
 *   does not take ownership of pointers or assocaited memory allocations and
 *   should be managed externally
 */
typedef struct hashtable {
    u64 element_size;
    u32 element_count;
    b8 is_pointer_type;
    void* memory;
} hashtable;


void hashtable_create(u64 element_size, u32 element_count, void* memory, b8 is_pointer_type, hashtable* out_table);
void hashtable_destroy(hashtable* table);

b8 hashtable_set(hashtable* table, const char* name, void* value); //copy version
b8 hashtable_set_ptr(hashtable* table, const char* name, void** value); //pointer version

b8 hashtable_get(hashtable* table, const char* name, void* out_value);
b8 hashtable_get_ptr(hashtable* table, const char* name, void** out_value);

// fill all entries with the value. useful for default values
b8 hashtable_fill(hashtable* table, void* value);

