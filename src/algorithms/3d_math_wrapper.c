#include "fp_3d_math_wrapper.h"
#include <string.h> // for memcpy
#include <stdio.h>

/*
 * ============================================================================
 * SECTION 1: C-KERNELS (for fp_generic.h)
 * ============================================================================
 */

void kernel_vec3_add(void* out, const void* a, const void* b, void* ctx) {
    (void)ctx;
    fp_zipWith_vec3_add_f32((const Vec3f*)a, (const Vec3f*)b, (Vec3f*)out, 1);
}

void kernel_transform_vec3(void* out, const void* in, void* ctx) {
    fp_map_transform_vec3_f32((const Vec3f*)in, (Vec3f*)out, 1, (const Mat4*)ctx);
}

void kernel_quat_rotate_vec3(void* out, const void* in, void* ctx) {
    fp_map_quat_rotate_vec3_f32((const Vec3f*)in, (Vec3f*)out, 1, (const Quaternion*)ctx);
}

void kernel_vec3_sum(void* acc, const void* elem, void* ctx) {
    (void)ctx;
    Vec3f temp;
    fp_zipWith_vec3_add_f32((const Vec3f*)acc, (const Vec3f*)elem, &temp, 1);
    *(Vec3f*)acc = temp;
}

/*
 * ============================================================================
 * SECTION 2: C-REFERENCE IMPLEMENTATIONS (for testing)
 * ============================================================================
 */

// Helper for quaternion math (pure C baseline)
static void quat_mul_vec3(const Quaternion* q, const Vec3f* v, Quaternion* out) {
    out->w = -q->x * v->x - q->y * v->y - q->z * v->z;
    out->x =  q->w * v->x + q->y * v->z - q->z * v->y;
    out->y =  q->w * v->y - q->x * v->z + q->z * v->x;
    out->z =  q->w * v->z + q->x * v->y - q->y * v->x;
}

static void quat_mul_quat(const Quaternion* q1, const Quaternion* q2, Quaternion* out) {
    out->w = q1->w * q2->w - q1->x * q2->x - q1->y * q2->y - q1->z * q2->z;
    out->x = q1->w * q2->x + q1->x * q2->w + q1->y * q2->z - q1->z * q2->y;
    out->y = q1->w * q2->y - q1->x * q2->z + q1->y * q2->w + q1->z * q2->x;
    out->z = q1->w * q2->z + q1->x * q2->y - q1->y * q2->x + q1->z * q2->w;
}

void ref_map_transform_vec3_f32(const Vec3f* in_vecs, Vec3f* out_vecs, size_t n, const Mat4* matrix) {
    for (size_t i = 0; i < n; i++) {
        const Vec3f* v_in = &in_vecs[i];
        Vec3f* v_out = &out_vecs[i];
        float w = 1.0f;

        float x = v_in->x * matrix->m[0] + v_in->y * matrix->m[4] + v_in->z * matrix->m[8]  + w * matrix->m[12];
        float y = v_in->x * matrix->m[1] + v_in->y * matrix->m[5] + v_in->z * matrix->m[9]  + w * matrix->m[13];
        float z = v_in->x * matrix->m[2] + v_in->y * matrix->m[6] + v_in->z * matrix->m[10] + w * matrix->m[14];
        float w_out = v_in->x * matrix->m[3] + v_in->y * matrix->m[7] + v_in->z * matrix->m[11] + w * matrix->m[15];

        if (w_out != 0.0f && w_out != 1.0f) {
            x /= w_out;
            y /= w_out;
            z /= w_out;
        }

        v_out->x = x;
        v_out->y = y;
        v_out->z = z;
        v_out->_pad = 0.0f;
    }
}

void ref_zipWith_vec3_add_f32(
    const Vec3f* in_a,
    const Vec3f* in_b,
    Vec3f* out_vecs,
    size_t n
) {
    for (size_t i = 0; i < n; i++) {
        out_vecs[i].x = in_a[i].x + in_b[i].x;
        out_vecs[i].y = in_a[i].y + in_b[i].y;
        out_vecs[i].z = in_a[i].z + in_b[i].z;
        out_vecs[i]._pad = 0.0f;
    }
}

void ref_map_quat_rotate_vec3_f32(const Vec3f* in_vecs, Vec3f* out_vecs, size_t n, const Quaternion* quat) {
    for (size_t i = 0; i < n; i++) {
        Quaternion q_conj = { -quat->x, -quat->y, -quat->z, quat->w };
        Quaternion temp, result_q;
        quat_mul_vec3(quat, &in_vecs[i], &temp);
        quat_mul_quat(&temp, &q_conj, &result_q);
        out_vecs[i].x = result_q.x;
        out_vecs[i].y = result_q.y;
        out_vecs[i].z = result_q.z;
        out_vecs[i]._pad = 0.0f;
    }
}

void ref_reduce_vec3_add_f32(
    const Vec3f* in_vecs,
    size_t n,
    Vec3f* out_sum
) {
    out_sum->x = 0.0f;
    out_sum->y = 0.0f;
    out_sum->z = 0.0f;
    for (size_t i = 0; i < n; i++) {
        out_sum->x += in_vecs[i].x;
        out_sum->y += in_vecs[i].y;
        out_sum->z += in_vecs[i].z;
    }
}

float ref_fold_vec3_dot_f32(const Vec3f* in_a, const Vec3f* in_b, size_t n) {
    float sum = 0.0f;
    for (size_t i = 0; i < n; ++i) {
        sum += (in_a[i].x * in_b[i].x +
                in_a[i].y * in_b[i].y +
                in_a[i].z * in_b[i].z);
    }
    return sum;
}
