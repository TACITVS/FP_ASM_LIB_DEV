#ifndef FP_3DMATH_WRAPPER_H
#define FP_3DMATH_WRAPPER_H

#include "fp_core.h" // For Vec3f, Mat4f, QuatF32

// C-kernels for fp_generic.h
void kernel_vec3_add(void* out, const void* a, const void* b, void* ctx);
void kernel_transform_vec3(void* out, const void* in, void* ctx);
void kernel_quat_rotate_vec3(void* out, const void* in, void* ctx);
void kernel_vec3_sum(void* acc, const void* elem, void* ctx);

// C-reference implementations for testing
void ref_map_transform_vec3_f32(const Vec3f* in_vecs, Vec3f* out_vecs, size_t n, const Mat4* matrix);
void ref_zipWith_vec3_add_f32(const Vec3f* in_a, const Vec3f* in_b, Vec3f* out_vecs, size_t n);
void ref_map_quat_rotate_vec3_f32(const Vec3f* in_vecs, Vec3f* out_vecs, size_t n, const Quaternion* quat);
void ref_reduce_vec3_add_f32(const Vec3f* in_vecs, size_t n, Vec3f* out_sum);
float ref_fold_vec3_dot_f32(const Vec3f* in_a, const Vec3f* in_b, size_t n);

#endif // FP_3DMATH_WRAPPER_H
