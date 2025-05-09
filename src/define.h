#pragma once

#define _DEBUG 1

// Unsigned int types.
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

// Signed int types.
typedef signed char i8;
typedef signed short i16;
typedef signed int i32;
typedef signed long long i64;

// Floating point types
typedef float f32;
typedef double f64;

// Boolean types
typedef int b32;
typedef _Bool b8;

// Properly define static assertions.
#if defined(__clang__) || defined(__gcc__)
#define STATIC_ASSERT _Static_assert
#else
#define STATIC_ASSERT static_assert
#endif

// Ensure all types are of the correct size.
// STATIC_ASSERT(sizeof(u8) == 1, "Expected u8 to be 1 byte.");
// STATIC_ASSERT(sizeof(u16) == 2, "Expected u16 to be 2 bytes.");
// STATIC_ASSERT(sizeof(u32) == 4, "Expected u32 to be 4 bytes.");
// STATIC_ASSERT(sizeof(u64) == 8, "Expected u64 to be 8 bytes.");
//
// STATIC_ASSERT(sizeof(i8) == 1, "Expected i8 to be 1 byte.");
// STATIC_ASSERT(sizeof(i16) == 2, "Expected i16 to be 2 bytes.");
// STATIC_ASSERT(sizeof(i32) == 4, "Expected i32 to be 4 bytes.");
// STATIC_ASSERT(sizeof(i64) == 8, "Expected i64 to be 8 bytes.");
//
// STATIC_ASSERT(sizeof(f32) == 4, "Expected f32 to be 4 bytes.");
// STATIC_ASSERT(sizeof(f64) == 8, "Expected f64 to be 8 bytes.");

#define true 1
#define false 0

#define INVALID_ID 4294967295U // 0xFFFFFFFF


// Platform detection
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#define cPLATFORM_WINDOWS 1
#ifndef _WIN64
#error "64-bit is required on Windows!"
#endif
#elif defined(__linux__) || defined(__gnu_linux__)
// Linux OS
#define cPLATFORM_LINUX 1
#if defined(__ANDROID__)
#define cPLATFORM_ANDROID 1
#endif
#elif defined(__unix__)
// Catch anything not caught by the above.
#define cPLATFORM_UNIX 1
#elif defined(_POSIX_VERSION)
// Posix
#define cPLATFORM_POSIX 1
#elif __APPLE__
// Apple platforms
#define cPLATFORM_APPLE 1
#include <TargetConditionals.h>
#if TARGET_IPHONE_SIMULATOR
// iOS Simulator
#define cPLATFORM_IOS 1
#define cPLATFORM_IOS_SIMULATOR 1
#elif TARGET_OS_IPHONE
#define cPLATFORM_IOS 1
// iOS device
#elif TARGET_OS_MAC
// Other kinds of Mac OS
#else
#error "Unknown Apple platform"
#endif
#else
#error "Unknown platform!"
#endif

#ifdef KEXPORT
// Exports
#ifdef _MSC_VER
#define KAPI __declspec(dllexport)
#else
#define KAPI __attribute__((visibility("default")))
#endif
#else
// Imports
#ifdef _MSC_VER
#define KAPI __declspec(dllimport)
#else
#define KAPI
#endif
#endif

#define cCLAMP(value, min, max) ((value) < (min) ? (min) : (value) > (max) ? (max) : (value))

#ifdef _MSC_VER
#define cINLINE __forceinline
#define cNOINLINE __declspec(noinline)
#else
#define cINLINE static inline
#define cNOINLINE __attribute__((noinline))
#endif

