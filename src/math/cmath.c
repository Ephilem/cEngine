#include "cmath.h"

#include "platform/platform.h"
#include <math.h>

static b8 rand_seeded = false;

f32 c_sinf(f32 x) {
    return sinf(x);
}

f32 c_cosf(f32 x) {
    return cosf(x);
}

f32 c_tanf(f32 x) {
    return tanf(x);
}

f32 c_acosf(f32 x) {
    return acosf(x);
}

f32 c_sqrtf(f32 x) {
    return sqrtf(x);
}

f32 c_absf(f32 x) {
    return fabsf(x);
}

i32 crandom() {
    if (!rand_seeded) {
        srand((u32)platform_get_absolute_time());
        rand_seeded = true;
    }
    return rand();
}

i32 crandom_in_range(i32 min, i32 max) {
    return crandom() % (max - min + 1) + min;
}

f32 fcrandom() {
    if (!rand_seeded) {
        srand((u32)platform_get_absolute_time());
        rand_seeded = true;
    }
    return (f32)rand() / (f32)RAND_MAX;
}

f32 fcrandom_in_range(f32 min, f32 max) {
    return fcrandom() * (max - min) + min;
}
