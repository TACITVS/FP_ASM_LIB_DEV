#include "../../include/fp_3d_math.h"

#if defined(_MSC_VER)
#    include <malloc.h>
#    define FP_STACK_ALLOC(bytes) _alloca(bytes)
#else
#    include <alloca.h>
#    define FP_STACK_ALLOC(bytes) alloca(bytes)
#endif

static void transform_to_matrix(const Transform* transform, Mat4f* out_matrix) {
    const Vec3f* position = &transform->position;
    const QuatF32* rotation = &transform->rotation;
    const Vec3f* scale = &transform->scale;

    const float x = rotation->x;
    const float y = rotation->y;
    const float z = rotation->z;
    const float w = rotation->w;

    const float x2 = x + x;
    const float y2 = y + y;
    const float z2 = z + z;

    const float xx = x * x2;
    const float xy = x * y2;
    const float xz = x * z2;
    const float yy = y * y2;
    const float yz = y * z2;
    const float zz = z * z2;
    const float wx = w * x2;
    const float wy = w * y2;
    const float wz = w * z2;

    float* m = out_matrix->m;

    m[0] = 1.0f - (yy + zz);
    m[1] = xy + wz;
    m[2] = xz - wy;
    m[3] = 0.0f;

    m[4] = xy - wz;
    m[5] = 1.0f - (xx + zz);
    m[6] = yz + wx;
    m[7] = 0.0f;

    m[8] = xz + wy;
    m[9] = yz - wx;
    m[10] = 1.0f - (xx + yy);
    m[11] = 0.0f;

    m[12] = position->x;
    m[13] = position->y;
    m[14] = position->z;
    m[15] = 1.0f;

    m[0] *= scale->x;
    m[1] *= scale->x;
    m[2] *= scale->x;

    m[4] *= scale->y;
    m[5] *= scale->y;
    m[6] *= scale->y;

    m[8] *= scale->z;
    m[9] *= scale->z;
    m[10] *= scale->z;
}

/* -------------------------------------------------------------------------- */
/*                               Layer 1 Kernels                              */
/* -------------------------------------------------------------------------- */

void kernel_vec3_add(void* out, const void* a, const void* b, void* ctx) {
    (void)ctx;
    const Vec3f* va = (const Vec3f*)a;
    const Vec3f* vb = (const Vec3f*)b;
    Vec3f* v_out = (Vec3f*)out;

    v_out->x = va->x + vb->x;
    v_out->y = va->y + vb->y;
    v_out->z = va->z + vb->z;
}

void kernel_transform_vec3(void* out, const void* in, void* ctx) {
    const Mat4f* m = (const Mat4f*)ctx;
    const Vec3f* v_in = (const Vec3f*)in;
    Vec3f* v_out = (Vec3f*)out;
    const float w = 1.0f;

    v_out->x = v_in->x * m->m[0] + v_in->y * m->m[4] + v_in->z * m->m[8] + w * m->m[12];
    v_out->y = v_in->x * m->m[1] + v_in->y * m->m[5] + v_in->z * m->m[9] + w * m->m[13];
    v_out->z = v_in->x * m->m[2] + v_in->y * m->m[6] + v_in->z * m->m[10] + w * m->m[14];
}

void kernel_quat_rotate_vec3(void* out, const void* in, void* ctx) {
    const QuatF32* quat = (const QuatF32*)ctx;
    const Vec3f* vec_in = (const Vec3f*)in;
    Vec3f* vec_out = (Vec3f*)out;

    const float qx = quat->x;
    const float qy = quat->y;
    const float qz = quat->z;
    const float qw = quat->w;

    const float vx = vec_in->x;
    const float vy = vec_in->y;
    const float vz = vec_in->z;

    const float tx = 2.0f * (qy * vz - qz * vy);
    const float ty = 2.0f * (qz * vx - qx * vz);
    const float tz = 2.0f * (qx * vy - qy * vx);

    const float s_tx = qw * tx;
    const float s_ty = qw * ty;
    const float s_tz = qw * tz;

    const float cx = qy * tz - qz * ty;
    const float cy = qz * tx - qx * tz;
    const float cz = qx * ty - qy * tx;

    vec_out->x = vx + s_tx + cx;
    vec_out->y = vy + s_ty + cy;
    vec_out->z = vz + s_tz + cz;
}

/* -------------------------------------------------------------------------- */
/*                     Layer 1 Reference Implementations                      */
/* -------------------------------------------------------------------------- */

void FP_API_MSABI ref_zipWith_vec3_add_f32(const Vec3f* a, const Vec3f* b, Vec3f* out, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        out[i].x = a[i].x + b[i].x;
        out[i].y = a[i].y + b[i].y;
        out[i].z = a[i].z + b[i].z;
    }
}

void FP_API_MSABI ref_map_transform_vec3_f32(const Vec3f* in_vecs, Vec3f* out_vecs, size_t n, const Mat4f* matrix) {
    for (size_t i = 0; i < n; ++i) {
        kernel_transform_vec3(&out_vecs[i], &in_vecs[i], (void*)matrix);
    }
}

void FP_API_MSABI ref_map_quat_rotate_vec3_f32(const Vec3f* in_vecs, Vec3f* out_vecs, size_t n, const QuatF32* quat) {
    for (size_t i = 0; i < n; ++i) {
        kernel_quat_rotate_vec3(&out_vecs[i], &in_vecs[i], (void*)quat);
    }
}

float FP_API_MSABI ref_fold_vec3_dot_f32(const Vec3f* a, const Vec3f* b, size_t n) {
    float acc = 0.0f;
    for (size_t i = 0; i < n; ++i) {
        acc += a[i].x * b[i].x;
        acc += a[i].y * b[i].y;
        acc += a[i].z * b[i].z;
    }
    return acc;
}

/* -------------------------------------------------------------------------- */
/*                 High-Level Composition / Example Function                  */
/* -------------------------------------------------------------------------- */

void fp_calculate_world_positions(
    const Vertex* local_vertices,
    const Transform* object_transforms,
    size_t n,
    Vec3f* out_world_positions
) {
    if (n == 0) {
        return;
    }

    Mat4f* world_matrices = (Mat4f*)FP_STACK_ALLOC(n * sizeof(Mat4f));
    Vec3f* local_positions = (Vec3f*)FP_STACK_ALLOC(n * sizeof(Vec3f));

    for (size_t i = 0; i < n; ++i) {
        transform_to_matrix(&object_transforms[i], &world_matrices[i]);
        local_positions[i] = local_vertices[i].position;
    }

    for (size_t i = 0; i < n; ++i) {
        fp_map_transform_vec3_f32(&local_positions[i], &out_world_positions[i], 1, &world_matrices[i]);
    }
}
