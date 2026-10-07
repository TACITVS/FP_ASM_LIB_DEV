/*
 * fp_gfx.c — renderer conventions and convention-aware matrices.
 * See include/fp_gfx.h. Pure functions over immutable presets.
 *
 * Derivation (column-major, v_clip = M * v). Let s = +1 for left-handed and
 * -1 for right-handed view space, so the distance in front of the camera is
 * d = s * z_view, in [near, far]. Perspective puts d in w:
 *     w_clip = s * z_view            -> m[11] = s
 *     z_clip = A * z_view + B        -> m[10] = A, m[14] = B
 *     z_ndc  = A*s + B/d
 * and A, B are solved from where near and far must land:
 *     depth [0,1]  standard:  near->0, far->1    A = s*f/(f-n)        B = -n*f/(f-n)
 *     depth [0,1]  reversed:  near->1, far->0    A = -s*n/(f-n)       B =  n*f/(f-n)
 *     depth [-1,1] standard:  near->-1, far->1   A = s*(f+n)/(f-n)    B = -2nf/(f-n)
 *     depth [-1,1] reversed:  near->1, far->-1   A = -s*(f+n)/(f-n)   B =  2nf/(f-n)
 * Infinite far is the limit f -> inf of the same formulas.
 * Orthographic is the linear analogue: z_ndc = A*z_view + B with w = 1.
 */
#include "fp_gfx.h"

#include <math.h>
#include <string.h>

#ifndef FPASM_GFX_DEFAULT
#  define FPASM_GFX_DEFAULT FP_GFX_OPENGL
#endif

/* The GPU-facing layout promises in fp_gfx.h. */
typedef char fp_gfx_vec3f_is_16_bytes[sizeof(Vec3f) == FP_GFX_VEC3F_STRIDE ? 1 : -1];
typedef char fp_gfx_quat_is_16_bytes[sizeof(Quaternion) == FP_GFX_QUATERNION_STRIDE ? 1 : -1];
typedef char fp_gfx_mat4_is_64_bytes[sizeof(Mat4) == FP_GFX_MAT4_SIZE ? 1 : -1];

#define CONV(depth, hand, y, preset) \
    { (depth), (hand), FP_GFX_Z_STANDARD, (y), FP_GFX_UPLOAD_COLUMN_MAJOR, (preset) }

static const fp_gfx_conventions presets[FP_GFX_PRESET_COUNT] = {
    CONV(FP_GFX_DEPTH_NEG_ONE_TO_ONE, FP_GFX_RIGHT_HANDED, FP_GFX_CLIP_Y_UP,   FP_GFX_OPENGL),
    CONV(FP_GFX_DEPTH_ZERO_TO_ONE,    FP_GFX_RIGHT_HANDED, FP_GFX_CLIP_Y_UP,   FP_GFX_D3D11),
    CONV(FP_GFX_DEPTH_ZERO_TO_ONE,    FP_GFX_LEFT_HANDED,  FP_GFX_CLIP_Y_UP,   FP_GFX_D3D11_LH),
    CONV(FP_GFX_DEPTH_ZERO_TO_ONE,    FP_GFX_RIGHT_HANDED, FP_GFX_CLIP_Y_DOWN, FP_GFX_VULKAN),
    CONV(FP_GFX_DEPTH_ZERO_TO_ONE,    FP_GFX_RIGHT_HANDED, FP_GFX_CLIP_Y_UP,   FP_GFX_METAL),
    CONV(FP_GFX_DEPTH_ZERO_TO_ONE,    FP_GFX_RIGHT_HANDED, FP_GFX_CLIP_Y_UP,   FP_GFX_D3D12),
};

static const char* const preset_names[FP_GFX_PRESET_COUNT] = {
    "opengl", "d3d11", "d3d11-lh", "vulkan", "metal", "d3d12",
};

const fp_gfx_conventions* fp_gfx_preset(int32_t preset) {
    return (preset >= 0 && preset < FP_GFX_PRESET_COUNT) ? &presets[preset] : NULL;
}

const fp_gfx_conventions* fp_gfx_default(void) {
    return &presets[FPASM_GFX_DEFAULT];
}

const char* fp_gfx_preset_name(int32_t preset) {
    return (preset >= 0 && preset < FP_GFX_PRESET_COUNT) ? preset_names[preset] : "custom";
}

int32_t fp_gfx_preset_from_name(const char* name) {
    int32_t i;
    if (!name) return -1;
    for (i = 0; i < FP_GFX_PRESET_COUNT; i++)
        if (strcmp(name, preset_names[i]) == 0) return i;
    if (strcmp(name, "generic") == 0 || strcmp(name, "gl") == 0) return FP_GFX_OPENGL;
    if (strcmp(name, "d3d11_lh") == 0) return FP_GFX_D3D11_LH;
    if (strcmp(name, "dx11") == 0) return FP_GFX_D3D11;
    if (strcmp(name, "dx12") == 0) return FP_GFX_D3D12;
    return -1;
}

int fp_gfx_conventions_init(fp_gfx_conventions* out, int32_t preset, int32_t reversed_z) {
    const fp_gfx_conventions* p = fp_gfx_preset(preset);
    if (!p || !out) return -1;
    *out = *p;
    out->depth_order = reversed_z ? FP_GFX_Z_REVERSED : FP_GFX_Z_STANDARD;
    return 0;
}

static void zero(Mat4* out) { memset(out->m, 0, sizeof out->m); }

static float hand_sign(const fp_gfx_conventions* c) {
    return c->handedness == FP_GFX_LEFT_HANDED ? 1.0f : -1.0f;
}

static float y_sign(const fp_gfx_conventions* c) {
    return c->clip_y == FP_GFX_CLIP_Y_DOWN ? -1.0f : 1.0f;
}

void fp_mat4_perspective_gfx(Mat4* out, float fovy, float aspect,
                             float n, float f, const fp_gfx_conventions* c) {
    const float s = hand_sign(c);
    const float g = 1.0f / tanf(fovy * 0.5f);
    const int zero_one = c->depth_range == FP_GFX_DEPTH_ZERO_TO_ONE;
    const int reversed = c->depth_order == FP_GFX_Z_REVERSED;
    float A, B;

    if (isinf(f)) {
        if (zero_one) { A = reversed ? 0.0f : s;  B = reversed ? n : -n; }
        else          { A = reversed ? -s : s;    B = reversed ? 2.0f * n : -2.0f * n; }
    } else {
        const float r = 1.0f / (f - n);
        if (zero_one) {
            A = reversed ? -s * n * r : s * f * r;
            B = reversed ? n * f * r : -n * f * r;
        } else {
            A = (reversed ? -s : s) * (f + n) * r;
            B = (reversed ? 2.0f : -2.0f) * n * f * r;
        }
    }

    zero(out);
    out->m[0]  = g / aspect;
    out->m[5]  = g * y_sign(c);
    out->m[10] = A;
    out->m[11] = s;
    out->m[14] = B;
}

void fp_mat4_ortho_gfx(Mat4* out, float l, float r, float b, float t,
                       float n, float f, const fp_gfx_conventions* c) {
    const float s = hand_sign(c);
    const float ys = y_sign(c);
    const float rf = 1.0f / (f - n);
    float As, B;   /* As = A*s: z_ndc = As * d + B, with d = s*z_view */

    if (c->depth_range == FP_GFX_DEPTH_ZERO_TO_ONE) {
        if (c->depth_order == FP_GFX_Z_REVERSED) { As = -rf; B = f * rf; }
        else                                      { As =  rf; B = -n * rf; }
    } else {
        if (c->depth_order == FP_GFX_Z_REVERSED) { As = -2.0f * rf; B =  (f + n) * rf; }
        else                                      { As =  2.0f * rf; B = -(f + n) * rf; }
    }

    zero(out);
    out->m[0]  = 2.0f / (r - l);
    out->m[5]  = 2.0f / (t - b) * ys;
    out->m[10] = As * s;
    out->m[12] = -(r + l) / (r - l);
    out->m[13] = -(t + b) / (t - b) * ys;
    out->m[14] = B;
    out->m[15] = 1.0f;
}

void fp_mat4_lookat_gfx(Mat4* out,
                        float ex, float ey, float ez,
                        float tx, float ty, float tz,
                        float ux, float uy, float uz,
                        const fp_gfx_conventions* c) {
    /* z axis: towards the viewer (RH) or along the view direction (LH);
     * x = up x z, y = z x x. Matches fp_mat4_lookat for RH and
     * DirectXMath XMMatrixLookAtLH for LH. */
    float zx = tx - ex, zy = ty - ey, zz = tz - ez;
    float len = sqrtf(zx * zx + zy * zy + zz * zz);
    float xx, xy, xz, yx, yy, yz;
    if (len < 1e-8f) goto identity;
    if (c->handedness != FP_GFX_LEFT_HANDED) len = -len;
    zx /= len; zy /= len; zz /= len;

    xx = uy * zz - uz * zy;
    xy = uz * zx - ux * zz;
    xz = ux * zy - uy * zx;
    len = sqrtf(xx * xx + xy * xy + xz * xz);
    if (len < 1e-8f) goto identity;
    xx /= len; xy /= len; xz /= len;

    yx = zy * xz - zz * xy;
    yy = zz * xx - zx * xz;
    yz = zx * xy - zy * xx;

    out->m[0] = xx;  out->m[4] = xy;  out->m[8]  = xz;  out->m[12] = -(xx * ex + xy * ey + xz * ez);
    out->m[1] = yx;  out->m[5] = yy;  out->m[9]  = yz;  out->m[13] = -(yx * ex + yy * ey + yz * ez);
    out->m[2] = zx;  out->m[6] = zy;  out->m[10] = zz;  out->m[14] = -(zx * ex + zy * ey + zz * ez);
    out->m[3] = 0;   out->m[7] = 0;   out->m[11] = 0;   out->m[15] = 1.0f;
    return;
identity:
    zero(out);
    out->m[0] = out->m[5] = out->m[10] = out->m[15] = 1.0f;
}

void fp_mat4_upload_gfx(float* dst, const Mat4* m, const fp_gfx_conventions* c) {
    int i;
    if (c->upload_layout == FP_GFX_UPLOAD_ROW_MAJOR) {
        for (i = 0; i < 16; i++) dst[i] = m->m[(i & 3) * 4 + (i >> 2)];
    } else {
        for (i = 0; i < 16; i++) dst[i] = m->m[i];
    }
}

float fp_gfx_depth_clear_value(const fp_gfx_conventions* c) {
    return c->depth_order == FP_GFX_Z_REVERSED ? 0.0f : 1.0f;
}

int32_t fp_gfx_depth_compare(const fp_gfx_conventions* c) {
    return c->depth_order == FP_GFX_Z_REVERSED ? FP_GFX_COMPARE_GREATER : FP_GFX_COMPARE_LESS;
}

size_t fp_gfx_cbuffer_size(size_t bytes) {
    return (bytes + (FP_GFX_CBUFFER_ALIGN - 1)) & ~(size_t)(FP_GFX_CBUFFER_ALIGN - 1);
}
