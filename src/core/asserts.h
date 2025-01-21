#pragma once

#include "define.h"

#define cASSERTIONS_ENABLED

#ifdef cASSERTIONS_ENABLED

#if _MSC_VER
#include <intrin.h>
#define cDebugBreak() __debugbreak()
#else
#define cDebugBreak() __builtin_trap()
#endif

void report_assertion_failure(const char* expression, const char* message, const char* file, i32 line);

#define cASSERT(expr) \
    { \
        if (expr) {} else { \
            report_assertion_failure(#expr, "", __FILE__, __LINE__); \
            cDebugBreak(); \
        } \
    }

#define cASSERT_MSG(expr, message) \
    { \
        if (expr) {} else { \
            report_assertion_failure(#expr, message, __FILE__, __LINE__); \
            cDebugBreak(); \
        } \
    }


#else

#define cASSERT(expr)
#define cASSERT_MSG(expr, message)

#endif