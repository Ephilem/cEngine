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
                f32 x,r,s,u;
            };
            union {
                f32 y,g,t,v;
            };
            union {
                f32 z,b,p,w;
            };
            union {
                f32 w,a,q;
            };
        };
    };
} vec4;

typedef vec4 quat;


