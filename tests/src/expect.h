#pragma once

#include <core/logger.h>
#include <math/cmath.h>

#define expect_should_be(expected, actual) \
    { \
        typeof(actual) actual_val = (actual); \
        if (actual_val != expected) { \
            LOG_ERROR("Expected %s to be %s (%d), but got %d", #actual, #expected, expected, actual_val); \
            return false; \
        } \
    }

#define expect_should_not_be(expected, actual) \
    if (actual == expected) { \
        LOG_ERROR("Expected %s to not be %s, but got %s", #actual, #expected, #actual); \
        return false; \
    }

#define expect_float_to_be(expected, actual) \
    if (c_absf(actual - expected) > 0.0001f) { \
        LOG_ERROR("Expected %s to be %s, but got %s", #actual, #expected, #actual); \
        return false; \
    }

#define expect_to_be_true(actual) \
    if (actual != true) { \
        LOG_ERROR("Expected %s to be true, but got false", #actual); \
        return false; \
    }

#define expect_to_be_false(actual) \
    if (actual != false) { \
        LOG_ERROR("Expected %s to be false, but got true", #actual); \
        return false; \
    }



