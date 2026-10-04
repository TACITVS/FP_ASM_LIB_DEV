/*
 * fp_gfx.h — renderer conventions as plain, immutable data.
 *
 * The library never talks to a graphics API. It only produces numbers
 * (matrices, transformed vertices) that a renderer uploads. What differs
 * between Direct3D, OpenGL, Vulkan and Metal is a handful of CONVENTIONS:
 * the clip-space depth range, the view-space handedness, the clip-space Y
 * direction, the matrix layout the shaders expect, and (optionally)
 * reversed-Z. fp_gfx_conventions captures exactly that, as a value.
 *
 * Design rules (same as the rest of the library):
 *   - Pure: every function reads its inputs and writes only its `out`
 *     argument. No global state; presets are immutable `const` data.
 *   - Renderer-agnostic: no D3D/GL/Vulkan headers or libraries. A Direct3D 11
 *     renderer passes fp_gfx_preset(FP_GFX_D3D11); an OpenGL one passes
 *     FP_GFX_OPENGL; nothing else changes.
 *   - FFI-friendly: fixed-size int32 fields, structs passed by pointer.
 *
 * Matrices are column-major Mat4 (fp_types.h) used as  v_clip = M * v,
 * as everywhere else in the library. The existing fp_mat4_perspective /
 * fp_mat4_ortho / fp_mat4_lookat keep their OpenGL behaviour; the *_gfx
 * versions below take the conventions explicitly.
 *
 * The build may choose a default (make GFX=d3d11 / -DFPASM_GFX=d3d11),
 * returned by fp_gfx_default(). It is only a default value: passing a
 * different preset always works, whatever the build chose.
 */
#ifndef FP_GFX_H
#define FP_GFX_H

#include <stddef.h>
#include <stdint.h>
#include "fp_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- convention fields (int32 for FFI) ---- */
enum { FP_GFX_DEPTH_NEG_ONE_TO_ONE = 0,   /* OpenGL: z_ndc in [-1, 1]            */
       FP_GFX_DEPTH_ZERO_TO_ONE    = 1 }; /* D3D, Vulkan, Metal: z_ndc in [0, 1] */

enum { FP_GFX_RIGHT_HANDED = 0,           /* camera looks down -Z (GL, glTF)      */
       FP_GFX_LEFT_HANDED  = 1 };         /* camera looks down +Z (DirectXMath LH) */

enum { FP_GFX_Z_STANDARD = 0,             /* near -> min depth, far -> max depth  */
       FP_GFX_Z_REVERSED = 1 };           /* near -> 1, far -> 0: far better float
                                             precision with a [0,1] depth range   */

enum { FP_GFX_CLIP_Y_UP   = 0,            /* GL, D3D, Metal                       */
       FP_GFX_CLIP_Y_DOWN = 1 };          /* Vulkan                               */

enum { FP_GFX_UPLOAD_COLUMN_MAJOR = 0,    /* HLSL default packing / GLSL; shader
                                             multiplies mul(M, v) / M * v          */
       FP_GFX_UPLOAD_ROW_MAJOR    = 1 };  /* shader declares row_major or uses
                                             mul(v, M)                             */

enum { FP_GFX_COMPARE_LESS    = 0,        /* depth test to use with the projection */
       FP_GFX_COMPARE_GREATER = 1 };

typedef struct {
    int32_t depth_range;    /* FP_GFX_DEPTH_*   */
    int32_t handedness;     /* FP_GFX_*_HANDED  */
    int32_t depth_order;    /* FP_GFX_Z_*       */
    int32_t clip_y;         /* FP_GFX_CLIP_Y_*  */
    int32_t upload_layout;  /* FP_GFX_UPLOAD_*  */
    int32_t preset;         /* FP_GFX_* preset it came from (informational) */
} fp_gfx_conventions;

/* ---- presets ---- */
enum {
    FP_GFX_OPENGL    = 0,   /* RH, depth [-1,1], Y up                         */
    FP_GFX_D3D11     = 1,   /* RH world (like the rest of the library), depth
                               [0,1], Y up, column-major upload + mul(M, v)    */
    FP_GFX_D3D11_LH  = 2,   /* same, left-handed view space (DirectXMath *LH) */
    FP_GFX_VULKAN    = 3,   /* RH, depth [0,1], clip Y down                   */
    FP_GFX_METAL     = 4,   /* RH, depth [0,1], Y up                          */
    FP_GFX_D3D12     = 5,   /* same conventions as FP_GFX_D3D11               */
    FP_GFX_PRESET_COUNT
};

/* Immutable preset (NULL if `preset` is unknown). */
const fp_gfx_conventions* fp_gfx_preset(int32_t preset);

/* The build's default (GFX= / FPASM_GFX); FP_GFX_OPENGL if none was chosen. */
const fp_gfx_conventions* fp_gfx_default(void);

/* Name of a preset ("d3d11", "opengl", ...) or "custom". */
const char* fp_gfx_preset_name(int32_t preset);

/* Preset id from a name; -1 if unknown. Accepts the GFX= spellings. */
int32_t fp_gfx_preset_from_name(const char* name);

/* out = preset, with reversed-Z switched on or off. Returns 0, or -1 for an
 * unknown preset (out is then left untouched). */
int fp_gfx_conventions_init(fp_gfx_conventions* out, int32_t preset, int32_t reversed_z);

/* ---- matrices (pure: write only *out) ---- */

/* Perspective projection. fovy in radians, aspect = width / height,
 * 0 < near < far. far may be INFINITY (infinite far plane; pairs well with
 * reversed-Z). */
void fp_mat4_perspective_gfx(Mat4* out, float fovy_radians, float aspect,
                             float near_z, float far_z, const fp_gfx_conventions* c);

/* Orthographic projection (2D/HUD, shadow maps). far must be finite. */
void fp_mat4_ortho_gfx(Mat4* out, float left, float right, float bottom, float top,
                       float near_z, float far_z, const fp_gfx_conventions* c);

/* View matrix for the conventions' handedness. Degenerate input (eye ==
 * target, or up parallel to the view direction) yields the identity. */
void fp_mat4_lookat_gfx(Mat4* out,
                        float eye_x, float eye_y, float eye_z,
                        float target_x, float target_y, float target_z,
                        float up_x, float up_y, float up_z,
                        const fp_gfx_conventions* c);

/* Writes the 16 floats a shader constant buffer expects for `m` (a copy, or
 * the transpose for FP_GFX_UPLOAD_ROW_MAJOR). `dst` may be mapped GPU
 * memory: it is only written, sequentially. dst and m must not overlap. */
void fp_mat4_upload_gfx(float* dst, const Mat4* m, const fp_gfx_conventions* c);

/* ---- depth state to pair with the projection ---- */
float   fp_gfx_depth_clear_value(const fp_gfx_conventions* c); /* 1 standard, 0 reversed */
int32_t fp_gfx_depth_compare(const fp_gfx_conventions* c);     /* FP_GFX_COMPARE_*        */

/* ---- writing into GPU-mapped memory ---- */
/* Streaming copy for memory from Map(WRITE_DISCARD) / glMapBufferRange /
 * vkMapMemory, which is usually write-combined: dst is written front to back
 * with non-temporal stores (never read), any alignment, followed by sfence.
 * dst and src must not overlap. Kernels whose output is write-only (e.g.
 * fp_mat4_mul_vec3_batch) may also write into mapped memory directly. */
void fp_stream_copy(void* dst, const void* src, size_t bytes);

/* ---- buffer layout helpers ---- */
/* Constant-buffer size rounded up to the 16-byte register size that D3D11
 * requires for ByteWidth (and that std140 rows use). */
size_t fp_gfx_cbuffer_size(size_t bytes);

/* Byte sizes / strides of the library's types as seen by a GPU:
 * Vec3f and Quaternion are 16 bytes (one float4 register), Mat4 is 64
 * (four float4 registers). Checked at compile time in fp_gfx.c. */
#define FP_GFX_VEC3F_STRIDE      16u
#define FP_GFX_QUATERNION_STRIDE 16u
#define FP_GFX_MAT4_SIZE         64u
#define FP_GFX_CBUFFER_ALIGN     16u

#ifdef __cplusplus
}
#endif
#endif /* FP_GFX_H */
