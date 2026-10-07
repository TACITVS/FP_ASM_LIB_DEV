// fpasm.glsl — GLSL side of the FP-ASM data contract (OpenGL / Vulkan).
//
// Paste or #include (GL_ARB_shading_language_include / shaderc) into shaders.
// Mirrors the library's memory layout and conventions; links to nothing.
//
//   Vec3f       16 bytes = vec4 (x, y, z, pad); pad == 1 after an affine
//                          fp_mat4_mul_vec3_batch, so it is a position vec4.
//   Quaternion  16 bytes = vec4 (x, y, z, w), w scalar.
//   Mat4        64 bytes = mat4, column-major (GLSL's native layout), v' = M * v.
// All three are std140/std430 compatible as-is.
//
// Projections: fp_mat4_perspective_gfx(..., FP_GFX_OPENGL) gives depth in
// [-1, 1]; FP_GFX_VULKAN gives [0, 1] with clip-space Y pointing down, so no
// extra flip is needed in the shader or the viewport.

#ifndef FPASM_GLSL
#define FPASM_GLSL

vec4 fp_transform(mat4 m, vec4 v)        { return m * v; }
vec4 fp_transform_point(mat4 m, vec3 p)  { return m * vec4(p, 1.0); }
vec3 fp_transform_dir(mat4 m, vec3 d)    { return mat3(m) * d; }

// Same rotation as fp_quat_rotate_vec3 (q * v * conj(q), unit q).
vec3 fp_quat_rotate(vec4 q, vec3 v) {
    vec3 t = 2.0 * cross(q.xyz, v);
    return v + q.w * t + cross(q.xyz, t);
}

#endif // FPASM_GLSL
