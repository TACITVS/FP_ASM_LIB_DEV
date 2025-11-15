#ifndef FP_MATH_H
#define FP_MATH_H

#include "fp_graphics_engine.h"
#include <math.h>

static inline Vec3f vec3f_add(Vec3f a, Vec3f b) { return (Vec3f){a.x + b.x, a.y + b.y, a.z + b.z}; }
static inline Vec3f vec3f_sub(Vec3f a, Vec3f b) { return (Vec3f){a.x - b.x, a.y - b.y, a.z - b.z}; }
static inline Vec3f vec3f_scale(Vec3f v, float s) { return (Vec3f){v.x * s, v.y * s, v.z * s}; }
static inline float vec3f_length(Vec3f v) { return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z); }
static inline Vec3f vec3f_normalize(Vec3f v) { float l = vec3f_length(v); return l > 0 ? vec3f_scale(v, 1.0f / l) : v; }
static inline Vec3f vec3f_cross(Vec3f a, Vec3f b) {
    return (Vec3f){
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

#endif // FP_MATH_H
