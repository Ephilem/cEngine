#pragma once

#include "define.h"


enum {
	DARRAY_CAPACITY,
	DARRAY_LENGTH,
	DARRAY_STRIDE,
	DARRAY_FIELD_LENGTH
};


// Underlying interface for dynamic array
void* _darray_create(u64 length, u64 stride);
void  _darray_destroy(void* array);

u64 _darray_field_get(void* array, u64 field);
void _darray_field_set(void* array, u64 field, u64 value);

void* _darray_resize(void* array);

void* _darray_push(void* array, const void* value_ptr);
void _darray_pop(void* array, void* dest);

void* _darray_pop_at(void* array, u64 index, void* dest);
void* _darray_insert_at(void* array, u64 index, const void* value_ptr);

#define DARRAY_DEFAULT_CAPACITY 1
#define DARRAY_RESIZE_FACTOR 2 // Size the array will be multiplied by when resizing

#define darray_create(type) \
	_darray_create(DARRAY_DEFAULT_CAPACITY, sizeof(type))

#define darray_reserve(type, length) \
	_darray_create(length, sizeof(type))

#define darray_destroy(array) \
	_darray_destroy(array)

#define darray_push(array, value) \
	{\
		typeof(value) _value = value;\
		array = _darray_push(array, &_value);\
	}

#define darray_pop(array, value_ptr) \
	_darray_pop(array, value_ptr)

#define darray_insert_at(array, index, value) \
	{\
		typeof(value) _value = value;\
		array = _darray_insert_at(array, index, &_value);\
	}

#define darray_pop_at(array, index, value_ptr) \
	_darray_pop_at(array, index, value_ptr)

// set the internal length field to 0
#define darray_clear(array) \
	_darray_field_set(array, DARRAY_LENGTH, 0)

#define darray_capacity(array) \
	_darray_field_get(array, DARRAY_CAPACITY)

#define darray_length(array) \
	_darray_field_get(array, DARRAY_LENGTH)

#define darray_stride(array) \
	_darray_field_get(array, DARRAY_STRIDE)

#define darray_length_set(array) \
	_darray_field_set(array, DARRAY_LENGTH, length)

