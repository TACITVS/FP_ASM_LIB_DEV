// fpasm.hlsli — HLSL side of the FP-ASM data contract (Direct3D 11/12).
//
// Include from your shaders:   #include "fpasm.hlsli"
// Nothing here links against the library; it documents and mirrors the
// memory layout and math conventions of what the library writes, so
// buffers produced on the CPU are consumed without conversion.
//
// Layout (checked at compile time in src/algorithms/fp_gfx.c):
//   Vec3f       16 bytes  = float4 (x, y, z, pad). After fp_mat4_mul_vec3_batch
//                           with an affine matrix, the 4th lane is 1.0, so the
//                           output is usable directly as a position float4.
//                           Input layout: DXGI_FORMAT_R32G32B32A32_FLOAT (or
//                           R32G32B32_FLOAT with a 16-byte stride).
//   Quaternion  16 bytes  = float4 (x, y, z, w), w is the scalar part.
//   Mat4        64 bytes  = float4x4, column-major in memory.
//
// Matrices: upload with fp_mat4_upload_gfx() using FP_GFX_D3D11 (a plain
// copy) and keep HLSL's default column_major packing; multiply mul(M, v).
// If your shaders use mul(v, M) or declare row_major, set the conventions'
// upload_layout to FP_GFX_UPLOAD_ROW_MAJOR instead.
//
// Depth: projections from fp_mat4_perspective_gfx(..., FP_GFX_D3D11) map
// depth to [0, 1]. With reversed-Z (fp_gfx_conventions_init(&c, FP_GFX_D3D11, 1))
// clear depth to fp_gfx_depth_clear_value(&c) == 0 and use
// D3D11_COMPARISON_GREATER (fp_gfx_depth_compare(&c) == FP_GFX_COMPARE_GREATER).

#ifndef FPASM_HLSLI
#define FPASM_HLSLI

typedef float4   fp_vec3f;       // xyz + pad (position: pad == 1 after an affine transform)
typedef float4   fp_quaternion;  // xyz vector part, w scalar part
typedef float4x4 fp_mat4;        // column-major, use fp_transform()

// v_clip = M * v, matching the library's column-vector convention.
float4 fp_transform(fp_mat4 m, float4 v) { return mul(m, v); }
float4 fp_transform_point(fp_mat4 m, float3 p) { return mul(m, float4(p, 1.0)); }
float3 fp_transform_dir(fp_mat4 m, float3 d) { return mul((float3x3)m, d); }

// Same rotation as fp_quat_rotate_vec3 (q * v * conj(q), unit q).
float3 fp_quat_rotate(fp_quaternion q, float3 v) {
    float3 t = 2.0 * cross(q.xyz, v);
    return v + q.w * t + cross(q.xyz, t);
}

// Constant-buffer sizes must be multiples of 16 bytes (fp_gfx_cbuffer_size).
// A typical per-frame buffer filled with fp_mat4_upload_gfx:
//   cbuffer FpFrame : register(b0) { fp_mat4 fp_view_proj; };

#endif // FPASM_HLSLI
