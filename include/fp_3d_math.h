#ifndef FP_3D_MATH_H
#define FP_3D_MATH_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_MSC_VER)
#    define FP_ALIGN16 __declspec(align(16))
#else
#    define FP_ALIGN16 __attribute__((aligned(16)))
#endif


#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
#    define FP_API_MSABI
#elif defined(__GNUC__) || defined(__clang__)
#    define FP_API_MSABI __attribute__((ms_abi))
#else
#    define FP_API_MSABI
#endif

/* --- Core Math Types --- */

typedef struct FP_ALIGN16 {
    float x, y;
    float _padding[2];
} Vec2f;

typedef struct FP_ALIGN16 {
    float x, y, z;
    float _padding;
} Vec3f;

typedef struct FP_ALIGN16 {
    float x, y, z, w;
} QuatF32;

typedef struct FP_ALIGN16 {
    float m[16];
} Mat4f;

/* --- Core Scene/Render Types --- */

typedef struct FP_ALIGN16 {
    Vec3f position;
    Vec3f normal;
    Vec2f uv;
} Vertex;

typedef struct FP_ALIGN16 {
    Vec3f position;
    QuatF32 rotation;
    Vec3f scale;
} Transform;

/* --- Layer 1 Kernels --- */
void kernel_vec3_add(void* out, const void* a, const void* b, void* ctx);
void kernel_transform_vec3(void* out, const void* in, void* ctx);
void kernel_quat_rotate_vec3(void* out, const void* in, void* ctx);

/* --- Layer 1 Reference Implementations --- */
void FP_API_MSABI ref_zipWith_vec3_add_f32(const Vec3f* a, const Vec3f* b, Vec3f* out, size_t n);
void FP_API_MSABI ref_map_transform_vec3_f32(const Vec3f* in_vecs, Vec3f* out_vecs, size_t n, const Mat4f* matrix);
void FP_API_MSABI ref_map_quat_rotate_vec3_f32(const Vec3f* in_vecs, Vec3f* out_vecs, size_t n, const QuatF32* quat);
float FP_API_MSABI ref_fold_vec3_dot_f32(const Vec3f* a, const Vec3f* b, size_t n);

/* --- Layer 2 Specialized Assembly Functions --- */
void FP_API_MSABI fp_zipWith_vec3_add_f32(const Vec3f* in_a, const Vec3f* in_b, Vec3f* out_vecs, size_t n);
void FP_API_MSABI fp_map_transform_vec3_f32(const Vec3f* in_vecs, Vec3f* out_vecs, size_t n, const Mat4f* matrix);
void FP_API_MSABI fp_map_quat_rotate_vec3_f32(const Vec3f* in_vecs, Vec3f* out_vecs, size_t n, const QuatF32* quat);
float FP_API_MSABI fp_fold_vec3_dot_f32(const Vec3f* in_a, const Vec3f* in_b, size_t n);

/* --- High-Level Composition Example --- */
void fp_calculate_world_positions(const Vertex* local_vertices, const Transform* object_transforms, size_t n, Vec3f* out_world_positions);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* FP_3D_MATH_H */
