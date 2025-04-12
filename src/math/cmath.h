#pragma once

#include <stdlib.h>

#include "define.h"
#include "math_types.h"
#include "core/cmemory.h"

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

f32 c_sinf(f32 x);
f32 c_cosf(f32 x);
f32 c_tanf(f32 x);
f32 c_acosf(f32 x);
f32 c_sqrtf(f32 x);
f32 c_absf(f32 x);


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
    return c_sqrtf(vec2_length_squared(a));
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
    if (abs(a.x - b.x) > tolerence) return false;
    if (abs(a.y - b.y) > tolerence) return false;
    return true;
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
    return c_sqrtf(vec3_length_squared(a));
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
    if (abs(a.x - b.x) > tolerence) return false;
    if (abs(a.y - b.y) > tolerence) return false;
    if (abs(a.z - b.z) > tolerence) return false;
    return true;
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
    return c_sqrtf(vec4_length_squared(a));
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

////////

cINLINE mat4 mat4_identity() {
    mat4 result = {0};
    czero_memory(result.data, sizeof(result.data));
    result.data[0] = 1.0f;
    result.data[5] = 1.0f;
    result.data[10] = 1.0f;
    result.data[15] = 1.0f;
    return result;
}

cINLINE mat4 mat4_multiply(mat4 a, mat4 b) {
    mat4 result = mat4_identity();

    const f32* m1_ptr = a.data;
    const f32* m2_ptr = b.data;
    f32* dst_ptr = result.data;

    for (i32 i = 0; i < 4; ++i) {
        for (i32 j = 0; j < 4; ++j) {
            *dst_ptr =
                m1_ptr[0] * m2_ptr[0 + j] +
                m1_ptr[1] * m2_ptr[4 + j] +
                m1_ptr[2] * m2_ptr[8 + j] +
                m1_ptr[3] * m2_ptr[12 + j];
            dst_ptr++;
        }
        m1_ptr += 4;
    }

    return result;
}

cINLINE mat4 mat4_orthographic(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far) {
    mat4 result = mat4_identity();

    f32 lr = 1.0f / (left - right);
    f32 bt = 1.0f / (bottom - top);
    f32 nf = 1.0f / (near - far);

    result.data[0] = -2.0f * lr;
    result.data[5] = -2.0f * bt;
    result.data[10] = 2.0f * nf;

    result.data[12] = (left + right) * lr;
    result.data[13] = (top + bottom) * bt;
    result.data[14] = (far + near) * nf;

    return result;
}

cINLINE mat4 mat4_perspective(f32 fov, f32 aspect_ratio, f32 near, f32 far) {
    mat4 result;
    czero_memory(result.data, sizeof(result.data));

    f32 half_tan_fov = c_tanf(fov * 0.5f);

    result.data[0] = 1.0f / (aspect_ratio * half_tan_fov);
    result.data[5] = 1.0f / half_tan_fov;
    result.data[10] = -((far + near) / (far - near));
    result.data[11] = -1.0f;
    result.data[14] = -((2.0f * far * near) / (far - near));

    return result;
}

/**
 * Return a matrix looking at target from the perspective of postition
 */
cINLINE mat4 mat4_look_at(vec3 position, vec3 target, vec3 up) {
    mat4 result;

    vec3 z_axis;
    z_axis.x = target.x - position.x;
    z_axis.y = target.y - position.y;
    z_axis.z = target.z - position.z;

    z_axis = vec3_normalized(z_axis);
    vec3 x_axis = vec3_normalized(vec3_cross(z_axis, up));
    vec3 y_axis = vec3_cross(x_axis, z_axis);

    // Assigner individuellement les éléments de la matrice
    result.data[0] = x_axis.x;
    result.data[1] = y_axis.x;
    result.data[2] = -z_axis.x;
    result.data[3] = 0.0f;

    result.data[4] = x_axis.y;
    result.data[5] = y_axis.y;
    result.data[6] = -z_axis.y;
    result.data[7] = 0.0f;

    result.data[8] = x_axis.z;
    result.data[9] = y_axis.z;
    result.data[10] = -z_axis.z;
    result.data[11] = 0.0f;

    result.data[12] = -vec3_dot(x_axis, position);
    result.data[13] = -vec3_dot(y_axis, position);
    result.data[14] = vec3_dot(z_axis, position);
    result.data[15] = 1.0f;

    return result;
}

cINLINE mat4 mat4_transposed(mat4 a) {
    mat4 result = mat4_identity();
    result.data[0] = a.data[0];
    result.data[1] = a.data[4];
    result.data[2] = a.data[8];
    result.data[3] = a.data[12];
    result.data[4] = a.data[1];
    result.data[5] = a.data[5];
    result.data[6] = a.data[9];
    result.data[7] = a.data[13];
    result.data[8] = a.data[2];
    result.data[9] = a.data[6];
    result.data[10] = a.data[10];
    result.data[11] = a.data[14];
    result.data[12] = a.data[3];
    result.data[13] = a.data[7];
    result.data[14] = a.data[11];
    result.data[15] = a.data[15];
    return result;
}

cINLINE mat4 mat4_inverse(mat4 a) {
    const f32* m = a.data;

    const f32 t0 = m[10] * m[15];
    const f32 t1 = m[14] * m[11];
    const f32 t2 = m[6] * m[15];
    const f32 t3 = m[14] * m[7];
    const f32 t4 = m[6] * m[11];
    const f32 t5 = m[10] * m[7];
    const f32 t6 = m[2] * m[15];
    const f32 t7 = m[14] * m[3];
    const f32 t8 = m[2] * m[11];
    const f32 t9 = m[10] * m[3];
    const f32 t10 = m[2] * m[7];
    const f32 t11 = m[6] * m[3];
    const f32 t12 = m[8] * m[13];
    const f32 t13 = m[12] * m[9];
    const f32 t14 = m[4] * m[13];
    const f32 t15 = m[12] * m[5];
    const f32 t16 = m[4] * m[9];
    const f32 t17 = m[8] * m[5];
    const f32 t18 = m[0] * m[13];
    const f32 t19 = m[12] * m[1];
    const f32 t20 = m[0] * m[9];
    const f32 t21 = m[8] * m[1];
    const f32 t22 = m[0] * m[5];
    const f32 t23 = m[4] * m[1];

    mat4 result;
    f32* o = result.data;

    o[0] = (t0 * m[5] + t3 * m[9] + t4 * m[13]) - (t1 * m[5] + t2 * m[9] + t5 * m[13]);
    o[1] = (t1 * m[1] + t6 * m[9] + t9 * m[13]) - (t0 * m[1] + t7 * m[9] + t8 * m[13]);
    o[2] = (t2 * m[1] + t7 * m[5] + t10 * m[13]) - (t3 * m[1] + t6 * m[5] + t11 * m[13]);
    o[3] = (t5 * m[1] + t8 * m[5] + t11 * m[9]) - (t4 * m[1] + t9 * m[5] + t10 * m[9]);

    f32 d = 1.0f / (m[0] * o[0] + m[4] * o[1] + m[8] * o[2] + m[12] * o[3]);

    o[0] = o[0] * d;
    o[1] = o[1] * d;
    o[2] = o[2] * d;
    o[3] = o[3] * d;

    o[4] = d * ((t1 * m[4] + t2 * m[8] + t5 * m[12]) - (t0 * m[4] + t3 * m[8] + t4 * m[12]));
    o[5] = d * ((t0 * m[0] + t7 * m[8] + t8 * m[12]) - (t1 * m[0] + t6 * m[8] + t9 * m[12]));
    o[6] = d * ((t3 * m[0] + t6 * m[4] + t11 * m[12]) - (t2 * m[0] + t7 * m[4] + t10 * m[12]));
    o[7] = d * ((t4 * m[0] + t9 * m[4] + t10 * m[8]) - (t5 * m[0] + t8 * m[4] + t11 * m[8]));
    o[8] = d * ((t12 * m[7] + t15 * m[11] + t16 * m[15]) - (t13 * m[7] + t14 * m[11] + t17 * m[15]));
    o[9] = d * ((t13 * m[3] + t18 * m[11] + t21 * m[15]) - (t12 * m[3] + t19 * m[11] + t20 * m[15]));
    o[10] = d * ((t14 * m[3] + t19 * m[7] + t22 * m[15]) - (t15 * m[3] + t18 * m[7] + t23 * m[15]));
    o[11] = d * ((t17 * m[3] + t20 * m[7] + t23 * m[11]) - (t16 * m[3] + t21 * m[7] + t22 * m[11]));
    o[12] = d * ((t14 * m[10] + t17 * m[14] + t13 * m[6]) - (t16 * m[14] + t12 * m[6] + t15 * m[10]));
    o[13] = d * ((t20 * m[14] + t12 * m[2] + t19 * m[10]) - (t18 * m[10] + t21 * m[14] + t13 * m[2]));
    o[14] = d * ((t18 * m[6] + t23 * m[14] + t15 * m[2]) - (t22 * m[14] + t14 * m[2] + t19 * m[6]));
    o[15] = d * ((t22 * m[10] + t16 * m[2] + t21 * m[6]) - (t20 * m[6] + t23 * m[10] + t17 * m[2]));

    return result;
}

cINLINE mat4 mat4_translation(vec3 position) {
    mat4 result = mat4_identity();
    result.data[12] = position.x;
    result.data[13] = position.y;
    result.data[14] = position.z;
    return result;
}

cINLINE mat4 mat4_scale(vec3 scale) {
    mat4 result = mat4_identity();
    result.data[0] = scale.x;
    result.data[5] = scale.y;
    result.data[10] = scale.z;
    return result;
}

cINLINE mat4 mat4_euler_x(f32 angle) {
    mat4 result = mat4_identity();
    f32 c = c_cosf(angle);
    f32 s = c_sinf(angle);

    result.data[5] = c;
    result.data[6] = s;
    result.data[9] = -s;
    result.data[10] = c;
    return result;
}

cINLINE mat4 mat4_euler_y(f32 angle) {
    mat4 result = mat4_identity();
    f32 c = c_cosf(angle);
    f32 s = c_sinf(angle);

    result.data[0] = c;
    result.data[2] = -s;
    result.data[8] = s;
    result.data[10] = c;
    return result;
}

cINLINE mat4 mat4_euler_z(f32 angle) {
    mat4 result = mat4_identity();
    f32 c = c_cosf(angle);
    f32 s = c_sinf(angle);

    result.data[0] = c;
    result.data[1] = s;
    result.data[4] = -s;
    result.data[5] = c;
    return result;
}

cINLINE mat4 mat4_euler_xyz(f32 x, f32 y, f32 z) {
    return mat4_multiply(mat4_multiply(mat4_euler_x(x), mat4_euler_y(y)), mat4_euler_z(z));
}

cINLINE vec3 mat4_forward(mat4 a) {
    vec3 forward;
    forward.x = -a.data[2];
    forward.y = -a.data[6];
    forward.z = -a.data[10];
    vec3_normalize(&forward);
    return forward;
}

cINLINE vec3 mat4_backward(mat4 a) {
    vec3 backward;
    backward.x = a.data[2];
    backward.y = a.data[6];
    backward.z = a.data[10];
    vec3_normalize(&backward);
    return backward;
}

cINLINE vec3 mat4_up(mat4 a) {
    vec3 up;
    up.x = a.data[1];
    up.y = a.data[5];
    up.z = a.data[9];
    vec3_normalize(&up);
    return up;
}

cINLINE vec3 mat4_down(mat4 a) {
    vec3 down;
    down.x = -a.data[1];
    down.y = -a.data[5];
    down.z = -a.data[9];
    vec3_normalize(&down);
    return down;
}

cINLINE vec3 mat4_left(mat4 a) {
    vec3 left;
    left.x = -a.data[0];
    left.y = -a.data[4];
    left.z = -a.data[8];
    vec3_normalize(&left);
    return left;
}

cINLINE vec3 mat4_right(mat4 a) {
    vec3 right;
    right.x = a.data[0];
    right.y = a.data[4];
    right.z = a.data[8];
    vec3_normalize(&right);
    return right;
}

// --------------------
// Quaternion functions
// --------------------

cINLINE quat quat_identity() {
    return (quat){{0.0f, 0.0f, 0.0f, 1.0f}};
}

cINLINE f32 quat_normal(quat q) {
    return c_sqrtf(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
}

cINLINE quat quat_normalize(quat q) {
    f32 n = quat_normal(q);
    return (quat){{q.x / n, q.y / n, q.z / n, q.w / n}};
}

cINLINE quat quat_conjugate(quat q) {
    return (quat){{-q.x, -q.y, -q.z, q.w}};
}

cINLINE quat quat_inverse(quat q) {
    return quat_normalize(quat_conjugate(q));
}

cINLINE quat quat_mul(quat a, quat b) {
    quat result;

    result.x = a.x * b.w + a.y * b.z - a.z * b.y + a.w * b.x;
    result.y = -a.x * b.z + a.y * b.w + a.z * b.x + a.w * b.y;
    result.z = a.x * b.y - a.y * b.x + a.z * b.w + a.w * b.z;
    result.w = -a.x * b.x - a.y * b.y - a.z * b.z + a.w * b.w;

    return result;
}

cINLINE f32 quat_dot(quat a, quat b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

cINLINE mat4 quat_to_mat4(quat q) {
    mat4 result = mat4_identity();

    quat n = quat_normalize(q);

    result.data[0] = 1.0f - 2.0f * n.y * n.y - 2.0f * n.z * n.z;
    result.data[1] = 2.0f * n.x * n.y + 2.0f * n.z * n.w;
    result.data[2] = 2.0f * n.x * n.z - 2.0f * n.y * n.w;

    result.data[4] = 2.0f * n.x * n.y - 2.0f * n.z * n.w;
    result.data[5] = 1.0f - 2.0f * n.x * n.x - 2.0f * n.z * n.z;
    result.data[6] = 2.0f * n.y * n.z + 2.0f * n.x * n.w;

    result.data[8] = 2.0f * n.x * n.z + 2.0f * n.y * n.w;
    result.data[9] = 2.0f * n.y * n.z - 2.0f * n.x * n.w;
    result.data[10] = 1.0f - 2.0f * n.x * n.x - 2.0f * n.y * n.y;

    return result;
}

// center is the center point of rotation
cINLINE mat4 quat_to_rotation_matrix(quat q, vec3 center) {
    mat4 result;

    f32* o = result.data;
    o[0] = (q.x * q.x) - (q.y * q.y) - (q.z * q.z) + (q.w * q.w);
    o[1] = 2.0f * ((q.x * q.y) + (q.z * q.w));
    o[2] = 2.0f * ((q.x * q.z) - (q.y * q.w));
    o[3] = center.x - center.x * o[0] - center.y * o[1] - center.z * o[2];

    o[4] = 2.0f * ((q.x * q.y) - (q.z * q.w));
    o[5] = -(q.x * q.x) + (q.y * q.y) - (q.z * q.z) + (q.w * q.w);
    o[6] = 2.0f * ((q.y * q.z) + (q.x * q.w));
    o[7] = center.y - center.x * o[4] - center.y * o[5] - center.z * o[6];

    o[8] = 2.0f * ((q.x * q.z) + (q.y * q.w));
    o[9] = 2.0f * ((q.y * q.z) - (q.x * q.w));
    o[10] = -(q.x * q.x) - (q.y * q.y) + (q.z * q.z) + (q.w * q.w);
    o[11] = center.z - center.x * o[8] - center.y * o[9] - center.z * o[10];

    o[12] = 0.0f;
    o[13] = 0.0f;
    o[14] = 0.0f;
    o[15] = 1.0f;

    return result;
}

cINLINE quat quat_from_axis_angle(vec3 axis, f32 angle, b8 normalize) {
    const f32 half_angle = angle * 0.5f;
    f32 s = c_sinf(half_angle);
    f32 c = c_cosf(half_angle);

    quat q = (quat){{axis.x * s, axis.y * s, axis.z * s, c}};
    if (normalize) {
        return quat_normalize(q);
    }
    return q;
}

cINLINE quat quat_slerp(quat a, quat b, f32 t) {
    f32 dot = quat_dot(a, b);
    if (dot < 0.0f) {
        b = (quat){{-b.x, -b.y, -b.z, -b.w}};
        dot = -dot;
    }

    const f32 DOT_THRESHOLD = 0.9995f;
    if (dot > DOT_THRESHOLD) {
        quat result = (quat){{a.x + t * (b.x - a.x), a.y + t * (b.y - a.y), a.z + t * (b.z - a.z), a.w + t * (b.w - a.w)}};
        return quat_normalize(result);
    }

    f32 theta_0 = c_acosf(dot);
    f32 theta = theta_0 * t;
    f32 sin_theta = c_sinf(theta);
    f32 sin_theta_0 = c_sinf(theta_0);

    f32 s0 = c_cosf(theta) - dot * sin_theta / sin_theta_0;
    f32 s1 = sin_theta / sin_theta_0;

    return (quat){{s0 * a.x + s1 * b.x, s0 * a.y + s1 * b.y, s0 * a.z + s1 * b.z, s0 * a.w + s1 * b.w}};
}

cINLINE f32 deg_to_rad(f32 degrees) {
    return degrees * DEG2RAD_MULTIPLIER;
}

cINLINE f32 rad_to_deg(f32 radians) {
    return radians * RAD2DEG_MULTIPLIER;
}

