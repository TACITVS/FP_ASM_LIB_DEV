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

/* ========================================================================
 * PHASE 1: Critical Functions for Game Engine
 * ======================================================================== */

/* ------------------------------------------------------------------------
 * Pure C Baseline Implementations (for benchmarking)
 * ------------------------------------------------------------------------ */

/**
 * Normalize a quaternion to unit length - PURE C VERSION.
 * Used as baseline for benchmarking L0-optimized version.
 */
void fp_quat_normalize_pure_c(Quaternion* out, const Quaternion* q) {
    float len_sq = q->x * q->x + q->y * q->y + q->z * q->z + q->w * q->w;

    if (len_sq < 1e-8f) {
        out->x = 0.0f;
        out->y = 0.0f;
        out->z = 0.0f;
        out->w = 1.0f;
        return;
    }

    float inv_len = 1.0f / sqrtf(len_sq);
    // Use temporaries to handle aliasing (in-place normalization: out == q)
    float temp_x = q->x * inv_len;
    float temp_y = q->y * inv_len;
    float temp_z = q->z * inv_len;
    float temp_w = q->w * inv_len;
    out->x = temp_x;
    out->y = temp_y;
    out->z = temp_z;
    out->w = temp_w;
}

/* ------------------------------------------------------------------------
 * L0-Optimized Implementations (using assembly primitives)
 * ------------------------------------------------------------------------ */

/**
 * Normalize a quaternion to unit length (library entry point).
 * Inline implementation to avoid function call overhead.
 *
 * @param out Pointer to output normalized quaternion
 * @param q   Pointer to input quaternion
 */
void fp_quat_normalize(Quaternion* out, const Quaternion* q) {
    float len_sq = q->x * q->x + q->y * q->y + q->z * q->z + q->w * q->w;

    if (len_sq < 1e-8f) {
        out->x = 0.0f;
        out->y = 0.0f;
        out->z = 0.0f;
        out->w = 1.0f;
        return;
    }

    float inv_len = 1.0f / sqrtf(len_sq);
    // Use temporaries to handle aliasing (in-place normalization: out == q)
    float temp_x = q->x * inv_len;
    float temp_y = q->y * inv_len;
    float temp_z = q->z * inv_len;
    float temp_w = q->w * inv_len;
    out->x = temp_x;
    out->y = temp_y;
    out->z = temp_z;
    out->w = temp_w;
}

/**
 * Convert Euler angles to quaternion - PURE C VERSION.
 * Uses ZYX rotation order to match fp_mat4_rotation_euler (Rz * Ry * Rx).
 * This is the standard yaw-pitch-roll convention.
 */
void fp_euler_to_quat_pure_c(Quaternion* out, float pitch_x, float yaw_y, float roll_z) {
    float cx = cosf(pitch_x * 0.5f);
    float sx = sinf(pitch_x * 0.5f);
    float cy = cosf(yaw_y * 0.5f);
    float sy = sinf(yaw_y * 0.5f);
    float cz = cosf(roll_z * 0.5f);
    float sz = sinf(roll_z * 0.5f);

    // ZYX order: Rz * Ry * Rx (matches fp_mat4_rotation_euler)
    out->w = cx * cy * cz + sx * sy * sz;
    out->x = sx * cy * cz - cx * sy * sz;
    out->y = cx * sy * cz + sx * cy * sz;
    out->z = cx * cy * sz - sx * sy * cz;
}

/**
 * Convert Euler angles to quaternion.
 * Uses ZYX rotation order (Rz * Ry * Rx) to match fp_mat4_rotation_euler.
 * This is the standard yaw-pitch-roll convention.
 * Avoids gimbal lock inherent in Euler angle representations.
 *
 * Inline implementation to avoid function call overhead.
 *
 * @param out      Pointer to output quaternion
 * @param pitch_x  Rotation around X axis (radians)
 * @param yaw_y    Rotation around Y axis (radians)
 * @param roll_z   Rotation around Z axis (radians)
 */
void fp_euler_to_quat(Quaternion* out, float pitch_x, float yaw_y, float roll_z) {
    float cx = cosf(pitch_x * 0.5f);
    float sx = sinf(pitch_x * 0.5f);
    float cy = cosf(yaw_y * 0.5f);
    float sy = sinf(yaw_y * 0.5f);
    float cz = cosf(roll_z * 0.5f);
    float sz = sinf(roll_z * 0.5f);

    // ZYX order: Rz * Ry * Rx (matches fp_mat4_rotation_euler)
    out->w = cx * cy * cz + sx * sy * sz;
    out->x = sx * cy * cz - cx * sy * sz;
    out->y = cx * sy * cz + sx * cy * sz;
    out->z = cx * cy * sz - sx * sy * cz;
}

/**
 * Convert quaternion to 4x4 rotation matrix - PURE C VERSION.
 * Optimized formula with only 15 multiplications (vs 27 naive).
 */
void fp_quat_to_mat4_pure_c(Mat4* out, const Quaternion* q) {
    float xx = q->x * q->x, yy = q->y * q->y, zz = q->z * q->z;
    float xy = q->x * q->y, xz = q->x * q->z, yz = q->y * q->z;
    float wx = q->w * q->x, wy = q->w * q->y, wz = q->w * q->z;

    // Column-major layout (OpenGL convention)
    out->m[0] = 1.0f - 2.0f * (yy + zz);
    out->m[1] = 2.0f * (xy + wz);
    out->m[2] = 2.0f * (xz - wy);
    out->m[3] = 0.0f;

    out->m[4] = 2.0f * (xy - wz);
    out->m[5] = 1.0f - 2.0f * (xx + zz);
    out->m[6] = 2.0f * (yz + wx);
    out->m[7] = 0.0f;

    out->m[8] = 2.0f * (xz + wy);
    out->m[9] = 2.0f * (yz - wx);
    out->m[10] = 1.0f - 2.0f * (xx + yy);
    out->m[11] = 0.0f;

    out->m[12] = 0.0f;
    out->m[13] = 0.0f;
    out->m[14] = 0.0f;
    out->m[15] = 1.0f;
}

/**
 * Batched quaternion normalization.
 *
 * This is the preferred entry point for engine-scale workloads where many
 * quaternions must be normalized in one pass. It delegates to the scalar
 * fp_quat_normalize (currently pure C), allowing the compiler and/or future
 * L0 primitives to optimize the inner kernel while amortizing call overhead.
 *
 * @param out  Output array of normalized quaternions (size n)
 * @param in   Input array of quaternions (size n)
 * @param n    Number of quaternions
 */
static void fp_quat_normalize_batch_recursive(
    Quaternion* out,
    const Quaternion* in,
    size_t n,
    size_t idx
) {
    if (idx >= n) {
        return;
    }

    fp_quat_normalize(&out[idx], &in[idx]);

    /* Tail-recursive call; with -O3 and sibling-call optimization this
     * compiles down to a loop but keeps the user-facing code in a
     * functional style without explicit for-loops.
     */
    fp_quat_normalize_batch_recursive(out, in, n, idx + 1);
}

void fp_quat_normalize_batch(Quaternion* out, const Quaternion* in, size_t n) {
    fp_quat_normalize_batch_recursive(out, in, n, 0);
}
