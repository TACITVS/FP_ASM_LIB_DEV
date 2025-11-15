#include "../../include/fp_core.h"
#include <math.h> // For sinf, cosf, asinf, atan2f, fabsf, copysignf

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

void fp_quat_identity(Quaternion* out) {
    out->x = 0.0f;
    out->y = 0.0f;
    out->z = 0.0f;
    out->w = 1.0f;
}

void fp_quat_from_axis_angle(Quaternion* out, const Vec3f* axis, float angle) {
    float half_angle = angle * 0.5f;
    float s = sinf(half_angle);
    float c = cosf(half_angle);

    Vec3f normalized_axis;
    vec3_normalize(&normalized_axis, axis);

    out->x = normalized_axis.x * s;
    out->y = normalized_axis.y * s;
    out->z = normalized_axis.z * s;
    out->w = c;
}

void fp_quat_mul(Quaternion* out, const Quaternion* a, const Quaternion* b) {
    out->x = a->x * b->w + a->y * b->z - a->z * b->y + a->w * b->x;
    out->y = -a->x * b->z + a->y * b->w + a->z * b->x + a->w * b->y;
    out->z = a->x * b->y - a->y * b->x + a->z * b->w + a->w * b->z;
    out->w = -a->x * b->x - a->y * b->y - a->z * b->z + a->w * b->w;
}

void fp_quat_rotate_vec3(Vec3f* out, const Quaternion* q, const Vec3f* v) {
    Quaternion vec_quat = {v->x, v->y, v->z, 0.0f};
    Quaternion inv_q = {-q->x, -q->y, -q->z, q->w}; // Conjugate for unit quaternion

    Quaternion temp1, temp2;
    fp_quat_mul(&temp1, q, &vec_quat);
    fp_quat_mul(&temp2, &temp1, &inv_q);

    out->x = temp2.x;
    out->y = temp2.y;
    out->z = temp2.z;
    out->_pad = 0.0f;
}

void fp_quat_to_euler(Vec3f* out, const Quaternion* q) {
    // Roll (x-axis rotation)
    float sinr_cosp = 2.0f * (q->w * q->x + q->y * q->z);
    float cosr_cosp = 1.0f - 2.0f * (q->x * q->x + q->y * q->y);
    out->x = atan2f(sinr_cosp, cosr_cosp);

    // Pitch (y-axis rotation)
    float sinp = 2.0f * (q->w * q->y - q->z * q->x);
    if (fabsf(sinp) >= 1)
        out->y = copysignf(M_PI / 2.0f, sinp); // Use 90 degrees if out of range
    else
        out->y = asinf(sinp);

    // Yaw (z-axis rotation)
    float siny_cosp = 2.0f * (q->w * q->z + q->x * q->y);
    float cosy_cosp = 1.0f - 2.0f * (q->y * q->y + q->z * q->z);
    out->z = atan2f(siny_cosp, cosy_cosp);
    out->_pad = 0.0f;
}
