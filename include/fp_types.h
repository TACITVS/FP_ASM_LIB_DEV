#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float x;
    float y;
    float z;
    float _pad;
} Vec3f;

typedef struct {
    float x;
    float y;
    float z;
    float w;
} Quaternion;

typedef struct {
    float m[16];
} Mat4;

typedef struct {
    double mean;
    double variance;
    double std_dev;
    double skewness;
    double kurtosis;
} DescriptiveStats;

typedef struct {
    double q1;
    double median;
    double q3;
    double iqr;
} Quartiles;

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
static_assert(sizeof(Vec3f) == 16, "Vec3f must remain 16 bytes");
static_assert(sizeof(Mat4) == 64, "Mat4 must remain 64 bytes");
static_assert(sizeof(Quaternion) == 16, "Quaternion must remain 16 bytes");
#else
_Static_assert(sizeof(Vec3f) == 16, "Vec3f must remain 16 bytes");
_Static_assert(sizeof(Mat4) == 64, "Mat4 must remain 64 bytes");
_Static_assert(sizeof(Quaternion) == 16, "Quaternion must remain 16 bytes");
#endif
