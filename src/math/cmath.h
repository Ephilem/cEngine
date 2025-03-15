#pragma once

#include <stdlib.h>

#include "define.h"
#include "math_types.h"

#define PI 3.14159265358979323846f
#define PI_2 2.0f * PI
#define HALF_PI 0.5f * PI
#define QUARTER_PI 0.25f * PI
#define ONE_OVER_PI 1.0f / PI
#define ONE_OVER_TWO_PI 1.0f / PI_2
#define SQRT_TWO 1.41421356237309504880f
#define SQRT_THREE 1.73205080756887729352f
#define SQRT_ONE_OVER_TWO 0.70710678118654752440f
#define SQRT_ONE_OVER_THREE 0.57735026918962576450f
#define DEG2RAD_MULTIPLIER PI / 180.0f
#define RAD2DEG_MULTIPLIER 180.0f / PI

#define SEC_TO_MS_MULTIPLIER 1000.0f
#define MS_TO_SEC_MULTIPLIER 0.001f

#define INFINITY 1e30f
#define FLOAT_EPSILON 1.192092896e-07f

f32 sinf(f32 x);
f32 cosf(f32 x);
f32 tanf(f32 x);
f32 acosf(f32 x);
f32 sqrtf(f32 x);
f32 absf(f32 x);


cINLINE b8 is_power_of_2(u64 value) {
    return (value != 0) && ((value & (value - 1)) == 0);
}

i32 crandom();
i32 crandom_in_range(i32 min, i32 max);

f32 fcrandom();
f32 fcrandom_in_range(f32 min, f32 max);



///// VECTOR 2 /////
/// Constructors
cINLINE vec2 vec2_create(f32 x, f32 y) {
    return (vec2){{x, y}};
}
cINLINE vec2 vec2_create_from_scalar(f32 scalar) {
    return (vec2){{scalar, scalar}};
}
cINLINE vec2 vec2_zero() {
    return (vec2){{0.0f, 0.0f}};
}
cINLINE vec2 vec2_one() {
    return (vec2){{1.0f, 1.0f}};
}
cINLINE vec2 vec2_up() {
    return (vec2){{0.0f, 1.0f}};
}
cINLINE vec2 vec2_down() {
    return (vec2){{0.0f, -1.0f}};
}
cINLINE vec2 vec2_left() {
    return (vec2){{-1.0f, 0.0f}};
}
cINLINE vec2 vec2_right() {
    return (vec2){{1.0f, 0.0f}};
}

/// Operations
cINLINE vec2 vec2_add(vec2 a, vec2 b) {
    return vec2_create(a.x + b.x, a.y + b.y);
}
cINLINE vec2 vec2_subtract(vec2 a, vec2 b) {
    return vec2_create(a.x - b.x, a.y - b.y);
}
cINLINE vec2 vec2_multiply(vec2 a, vec2 b) {
    return vec2_create(a.x * b.x, a.y * b.y);
}
cINLINE vec2 vec2_divide(vec2 a, vec2 b) {
    return vec2_create(a.x / b.x, a.y / b.y);
}
cINLINE vec2 vec2_scale(vec2 a, f32 scalar) {
    return vec2_create(a.x * scalar, a.y * scalar);
}
cINLINE f32 vec2_length_squared(vec2 a) {
    return a.x * a.x + a.y * a.y;
}
cINLINE f32 vec2_length(vec2 a) {
    return sqrtf(vec2_length_squared(a));
}
cINLINE void vec2_normalize(vec2* a) {
    f32 length = vec2_length(*a);
    a->x /= length;
    a->y /= length;
}
cINLINE vec2 vec2_normalized(vec2 a) {
    vec2 result = a;
    vec2_normalize(&result);
    return result;
}
cINLINE b8 vec2_compare(vec2 a, vec2 b, f32 tolerence) {
    if (abs(a.x - b.x) > tolerence) return FALSE;
    if (abs(a.y - b.y) > tolerence) return FALSE;
    return TRUE;
}
cINLINE f32 vec2_distance(vec2 a, vec2 b) {
    const vec2 d = (vec2){
        a.x - b.x,
        a.y - b.y
    };
    return vec2_length(d);
}


///// VECTOR 3 /////
/// Constructors
cINLINE vec3 vec3_create(f32 x, f32 y, f32 z) {
    return (vec3){{x, y, z}};
}
cINLINE vec3 vec3_create_from_scalar(f32 scalar) {
    return (vec3){{scalar, scalar, scalar}};
}
cINLINE vec3 vec3_zero() {
    return (vec3){{0.0f, 0.0f, 0.0f}};
}
cINLINE vec3 vec3_one() {
    return (vec3){{1.0f, 1.0f, 1.0f}};
}
cINLINE vec3 vec3_up() {
    return (vec3){{0.0f, 1.0f, 0.0f}};
}
cINLINE vec3 vec3_down() {
    return (vec3){{0.0f, -1.0f, 0.0f}};
}
cINLINE vec3 vec3_left() {
    return (vec3){{-1.0f, 0.0f, 0.0f}};
}
cINLINE vec3 vec3_right() {
    return (vec3){{1.0f, 0.0f, 0.0f}};
}
cINLINE vec3 vec3_forward() {
    return (vec3){{0.0f, 0.0f, 1.0f}};
}
cINLINE vec3 vec3_backward() {
    return (vec3){{0.0f, 0.0f, -1.0f}};
}

/// Operations
cINLINE vec3 vec3_add(vec3 a, vec3 b) {
    return vec3_create(a.x + b.x, a.y + b.y, a.z + b.z);
}
cINLINE vec3 vec3_subtract(vec3 a, vec3 b) {
    return vec3_create(a.x - b.x, a.y - b.y, a.z - b.z);
}
cINLINE vec3 vec3_multiply(vec3 a, vec3 b) {
    return vec3_create(a.x * b.x, a.y * b.y, a.z * b.z);
}
cINLINE vec3 vec3_divide(vec3 a, vec3 b) {
    return vec3_create(a.x / b.x, a.y / b.y, a.z / b.z);
}
cINLINE vec3 vec3_scale(vec3 a, f32 scalar) {
    return vec3_create(a.x * scalar, a.y * scalar, a.z * scalar);
}
cINLINE f32 vec3_length_squared(vec3 a) {
    return a.x * a.x + a.y * a.y + a.z * a.z;
}
cINLINE f32 vec3_length(vec3 a) {
    return sqrtf(vec3_length_squared(a));
}
cINLINE void vec3_normalize(vec3* a) {
    f32 length = vec3_length(*a);
    a->x /= length;
    a->y /= length;
    a->z /= length;
}
cINLINE vec3 vec3_normalized(vec3 a) {
    vec3 result = a;
    vec3_normalize(&result);
    return result;
}
cINLINE b8 vec3_compare(vec3 a, vec3 b, f32 tolerence) {
    if (abs(a.x - b.x) > tolerence) return FALSE;
    if (abs(a.y - b.y) > tolerence) return FALSE;
    if (abs(a.z - b.z) > tolerence) return FALSE;
    return TRUE;
}
cINLINE f32 vec3_distance(vec3 a, vec3 b) {
    const vec3 d = (vec3){
        a.x - b.x,
        a.y - b.y,
        a.z - b.z
    };
    return vec3_length(d);
}
cINLINE f32 vec3_dot(vec3 a, vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
cINLINE vec3 vec3_cross(vec3 a, vec3 b) {
    return (vec3){
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}


///// VECTOR 4 /////
/// Constructors
cINLINE vec4 vec4_create(f32 x, f32 y, f32 z, f32 w) {
    vec4 out_vector;
#if defined(cUSE_SIMD)
    out_vector.data = _mm_set_ps(w, z, y, x);
#else
    out_vector.x = x;
    out_vector.y = y;
    out_vector.z = z;
    out_vector.w = w;
#endif
    return out_vector;
}

cINLINE vec3 vec4_to_vec3(vec4 a) {
    return (vec3){{a.x, a.y, a.z}};
}

cINLINE vec4 vec4_from_vec3(vec3 a, f32 w) {
#if defined(cUSE_SIMD)
    vec4 out_vector;
    out_vector.data = _mm_set_ps(w, a.z, a.y, a.x);
    return out_vector;
#else
    return (vec4){{a.x, a.y, a.z, w}};
#endif
}

cINLINE vec4 vec4_zero() {
    return vec4_create(0.0f, 0.0f, 0.0f, 0.0f);
}

cINLINE vec4 vec4_one() {
    return vec4_create(1.0f, 1.0f, 1.0f, 1.0f);
}

cINLINE vec4 vec4_add(vec4 a, vec4 b) {
    vec4 result;
    for (u64 i = 0; i < 4; ++i) {
        result.elements[i] = a.elements[i] + b.elements[i];
    }
    return result;
}

cINLINE vec4 vec4_subtract(vec4 a, vec4 b) {
    vec4 result;
    for (u64 i = 0; i < 4; ++i) {
        result.elements[i] = a.elements[i] - b.elements[i];
    }
    return result;
}

cINLINE vec4 vec4_multiply(vec4 a, vec4 b) {
    vec4 result;
    for (u64 i = 0; i < 4; ++i) {
        result.elements[i] = a.elements[i] * b.elements[i];
    }
    return result;
}

cINLINE vec4 vec4_divide(vec4 a, vec4 b) {
    vec4 result;
    for (u64 i = 0; i < 4; ++i) {
        result.elements[i] = a.elements[i] / b.elements[i];
    }
    return result;
}

cINLINE vec4 vec4_scale(vec4 a, f32 scalar) {
    vec4 result;
    for (u64 i = 0; i < 4; ++i) {
        result.elements[i] = a.elements[i] * scalar;
    }
    return result;
}

cINLINE f32 vec4_length_squared(vec4 a) {
    return a.x * a.x + a.y * a.y + a.z * a.z + a.w * a.w;
}

cINLINE f32 vec4_length(vec4 a) {
    return sqrtf(vec4_length_squared(a));
}

cINLINE void vec4_normalize(vec4* a) {
    f32 length = vec4_length(*a);
    a->x /= length;
    a->y /= length;
    a->z /= length;
    a->w /= length;
}

cINLINE vec4 vec4_normalized(vec4 a) {
    vec4 result = a;
    vec4_normalize(&result);
    return result;
}









