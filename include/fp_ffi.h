/*
 * fp_ffi.h — the stable surface for foreign-function interfaces.
 *
 * A curated subset of the library meant to be bound from other languages
 * (Common Lisp CFFI, SBCL sb-alien, Chez Scheme foreign-procedure, Rust,
 * Zig, Python ctypes, ...). Everything here follows the same rules:
 *
 *   - Batch functions: a pointer and an element count. One call does the
 *     work for a whole array, so the per-call FFI cost is amortized.
 *   - Plain C types only: fixed-width integers, float, double, size_t
 *     (= uint64 on x86-64), and pointers. Structs are passed by POINTER,
 *     never by value, and nothing takes a callback.
 *   - Pure: functions read their inputs and write only their output
 *     arguments. No global state is touched, so any number of threads may
 *     call them at once as long as their OUTPUT buffers do not overlap.
 *     Call fp_dispatch_init() once at start-up (before starting threads).
 *   - Memory layout (all little-endian, no padding beyond what is listed):
 *       Vec3f       16 bytes: float x, y, z, pad   (use pad = 1 for positions)
 *       Quaternion  16 bytes: float x, y, z, w      (w = scalar part)
 *       Mat4        64 bytes: float[16], column-major (m[12..14] = translation)
 *       fp_gfx_conventions  6 x int32 (see fp_gfx.h)
 *     fp_ffi_type_size() reports these at runtime so a binding can verify
 *     its own struct definitions against the library it actually loaded.
 *   - Buffers handed to the library must not move during the call: in a
 *     garbage-collected language use non-moving memory (foreign
 *     allocation, static-vectors in Common Lisp, pinned arrays, ...).
 *
 * Every prototype here is identical to the one in fp_core.h / fp_gfx.h /
 * fp_dispatch.h / fp_cpu.h (tests/test_ffi.c includes them together, which
 * fails to compile on any mismatch), so C code may include either.
 * The ABI version below changes whenever this surface changes incompatibly.
 */
#ifndef FP_FFI_H
#define FP_FFI_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "fp_types.h"
#include "fp_gfx.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FP_FFI_ABI_VERSION 1u

/* ---- runtime ---------------------------------------------------------- */
uint32_t fp_ffi_abi_version(void);           /* == FP_FFI_ABI_VERSION of the loaded library */

enum { FP_FFI_TYPE_VEC3F = 0, FP_FFI_TYPE_QUATERNION = 1, FP_FFI_TYPE_MAT4 = 2,
       FP_FFI_TYPE_GFX_CONVENTIONS = 3 };
uint32_t fp_ffi_type_size(int32_t type);     /* sizeof, or 0 for an unknown id */

void fp_dispatch_init(void);                 /* pick CPU-specific kernels now (idempotent) */
int  fp_cpu_check(FILE* err);                /* 0 = this build runs on this CPU; pass NULL
                                                for no message */

/* ---- reductions and folds (n = element count) -------------------------- */
float   fp_reduce_add_f32(const float* in, size_t n);
float   fp_reduce_min_f32(const float* in, size_t n);
float   fp_reduce_max_f32(const float* in, size_t n);
double  fp_reduce_add_f64(const double* in, size_t n);
double  fp_reduce_min_f64(const double* in, size_t n);
double  fp_reduce_max_f64(const double* in, size_t n);
int32_t fp_reduce_add_i32(const int32_t* in, size_t n);
int32_t fp_reduce_min_i32(const int32_t* in, size_t n);
int32_t fp_reduce_max_i32(const int32_t* in, size_t n);
int64_t fp_reduce_add_i64(const int64_t* in, size_t n);
int64_t fp_reduce_min_i64(const int64_t* in, size_t n);
int64_t fp_reduce_max_i64(const int64_t* in, size_t n);

float   fp_fold_dotp_f32(const float* a, const float* b, size_t n);
float   fp_fold_sumsq_f32(const float* in, size_t n);
double  fp_fold_dotp_f64(const double* a, const double* b, size_t n);
int32_t fp_fold_dotp_i32(const int32_t* a, const int32_t* b, size_t n);
int16_t fp_fold_dotp_i16(const int16_t* a, const int16_t* b, size_t n);  /* wraps mod 2^16 */
int8_t  fp_fold_dotp_i8(const int8_t* a, const int8_t* b, size_t n);     /* wraps mod 2^8  */
uint8_t fp_fold_dotp_u8(const uint8_t* a, const uint8_t* b, size_t n);   /* wraps mod 2^8  */

/* ---- element-wise maps (out may equal in) ------------------------------ */
void fp_map_scale_f32(const float* in, float* out, size_t n, float c);
void fp_map_offset_f32(const float* in, float* out, size_t n, float c);
void fp_map_axpy_f32(const float* x, const float* y, float* out, size_t n, float c);  /* c*x + y */
void fp_zip_add_f32(const float* a, const float* b, float* out, size_t n);
void fp_map_scale_f64(const double* in, double* out, size_t n, double c);
void fp_map_offset_f64(const double* in, double* out, size_t n, double c);
void fp_map_axpy_f64(const double* x, const double* y, double* out, size_t n, double c);
void fp_zip_add_f64(const double* a, const double* b, double* out, size_t n);

/* ---- 3D batches (the per-frame hot paths) ------------------------------ */
void  fp_mat4_mul_vec3_batch(Vec3f* output, const Mat4* m, const Vec3f* input, int count);
void  fp_map_transform_vec3_f32(const Vec3f* in_vecs, Vec3f* out_vecs, size_t n, const Mat4* matrix);
void  fp_map_quat_rotate_vec3_f32(const Vec3f* in_vecs, Vec3f* out_vecs, size_t n, const Quaternion* quat);  /* unit q; in-place ok */
void  fp_zipWith_vec3_add_f32(const Vec3f* in_a, const Vec3f* in_b, Vec3f* out_vecs, size_t n);
void  fp_reduce_vec3_add_f32(const Vec3f* in_vecs, size_t n, Vec3f* out_sum);
float fp_fold_vec3_dot_f32(const Vec3f* in_a, const Vec3f* in_b, size_t n);
void  fp_quat_normalize_batch(Quaternion* out, const Quaternion* in, size_t n);

/* ---- single matrices / quaternions (once per object, not per vertex) --- */
void fp_mat4_identity(Mat4* output);
void fp_mat4_mul(Mat4* output, const Mat4* a, const Mat4* b);
void fp_mat4_transpose(Mat4* output, const Mat4* m);
int  fp_mat4_inverse(Mat4* out, const Mat4* m);              /* 1 = ok, 0 = singular */
void fp_mat4_translation(Mat4* out, float x, float y, float z);
void fp_mat4_scale(Mat4* out, float sx, float sy, float sz);
void fp_mat4_rotation_axis(Mat4* out, float x, float y, float z, float angle_radians);
void fp_mat4_rotation_euler(Mat4* out, float pitch_x, float yaw_y, float roll_z);
void fp_quat_mul(Quaternion* out, const Quaternion* a, const Quaternion* b);
void fp_quat_rotate_vec3(Vec3f* out, const Quaternion* q, const Vec3f* v);
void fp_quat_normalize(Quaternion* out, const Quaternion* q);
/* fp_core.h's fp_quat_to_mat4 is a static inline (not exported); bind this: */
void fp_quat_to_mat4_pure_c(Mat4* out, const Quaternion* q);

/* ---- renderer conventions (fp_gfx.h) ----------------------------------- */
/* fp_gfx_preset, fp_gfx_default, fp_gfx_conventions_init,
 * fp_mat4_perspective_gfx, fp_mat4_ortho_gfx, fp_mat4_lookat_gfx,
 * fp_mat4_upload_gfx, fp_gfx_depth_clear_value, fp_gfx_depth_compare,
 * fp_gfx_cbuffer_size and fp_stream_copy are all FFI-ready as declared in
 * fp_gfx.h (included above). */

#ifdef __cplusplus
}
#endif
#endif /* FP_FFI_H */
