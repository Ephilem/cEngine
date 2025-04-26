#pragma once

#include <stdalign.h>

#include "define.h"

typedef union vec2_u {
    f32 elements[2];
    struct {
        union {
            f32 x,r,s,u;
        };
        union {
            f32 y,g,t,v;
        };
    };
} vec2;

typedef union vec3_u {
    f32 elements[3];
    struct {
        union {
            f32 x,r,s,u;
        };
        union {
            f32 y,g,t,v;
        };
        union {
            f32 z,b,p,w;
        };
    };
} vec3;

typedef union vec4_u {
#if defined(cUSE_SIMD)
    alignas(16) _m128 data;
#endif

    alignas(16) f32 elements[4];

    union {
        struct {
            union {
                f32 x,r,s;
            };
            union {
                f32 y,g,t;
            };
            union {
                f32 z,b,p;
            };
            union {
                f32 w,a,q;
            };
        };
    };
} vec4;

typedef vec4 quat;

typedef union mat4_u {
    alignas(16) f32 data[16];

#if defined(cUSE_SIMD)
    alignas(16) _m128 rows[4];
#endif
} mat4;

typedef struct vertex_3d {
    vec3 position;
    vec2 texcoord;
    //vec3 normal;
} vertex_3d;
