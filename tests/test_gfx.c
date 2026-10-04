/* Validates the renderer-convention API (fp_gfx.h):
 *   - the OpenGL preset reproduces the existing fp_mat4_perspective /
 *     fp_mat4_ortho / fp_mat4_lookat exactly (backward compatibility);
 *   - for every preset, standard and reversed-Z, finite and infinite far:
 *     points on the near / far planes land on the right NDC depth, points in
 *     front of the camera have w > 0, Vulkan flips clip-space Y;
 *   - left-handed view matrices match DirectXMath's LookAtLH;
 *   - upload layout, depth state and cbuffer helpers. */
#include "fp_core.h"
#include "fp_gfx.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(__linux__)
#  include <sys/mman.h>
#  include <unistd.h>
#endif

static int failures = 0, checks = 0;

#define CHECK(cond, ...) do { checks++; if (!(cond)) { failures++; printf("FAIL %s:%d ", __FILE__, __LINE__); \
                              printf(__VA_ARGS__); printf("\n"); } } while (0)

static int near_eq(float a, float b, float tol) { return fabsf(a - b) <= tol * (1.0f + fabsf(b)); }

static void mul(const Mat4* m, const float v[4], float out[4]) {
    int r;
    for (r = 0; r < 4; r++)
        out[r] = m->m[r] * v[0] + m->m[4 + r] * v[1] + m->m[8 + r] * v[2] + m->m[12 + r] * v[3];
}

static void same_mat(const char* what, const Mat4* a, const Mat4* b) {
    int i;
    for (i = 0; i < 16; i++)
        CHECK(near_eq(a->m[i], b->m[i], 1e-6f), "%s m[%d]: %g vs %g", what, i, a->m[i], b->m[i]);
}

/* Project a view-space point; return NDC z and w. */
static void project(const Mat4* p, float x, float y, float z, float* ndc_y, float* ndc_z, float* w) {
    float v[4] = { x, y, z, 1.0f }, o[4];
    mul(p, v, o);
    *w = o[3];
    *ndc_y = o[1] / o[3];
    *ndc_z = o[2] / o[3];
}

static void check_projection(int32_t preset, int reversed, int infinite, int ortho) {
    fp_gfx_conventions c;
    Mat4 p;
    const float n = 0.5f, f = 200.0f;
    float y, z, w, want_near, want_far;
    float s;   /* view-space z of "in front of the camera" */
    char tag[96];

    fp_gfx_conventions_init(&c, preset, reversed);
    s = c.handedness == FP_GFX_LEFT_HANDED ? 1.0f : -1.0f;
    snprintf(tag, sizeof tag, "%s%s%s%s", fp_gfx_preset_name(preset), reversed ? "+revZ" : "",
             infinite ? "+inf" : "", ortho ? " ortho" : " persp");

    if (ortho) fp_mat4_ortho_gfx(&p, -4, 4, -3, 3, n, f, &c);
    else       fp_mat4_perspective_gfx(&p, 1.0f, 16.0f / 9.0f, n, infinite ? INFINITY : f, &c);

    if (c.depth_range == FP_GFX_DEPTH_ZERO_TO_ONE) { want_near = reversed ? 1.0f : 0.0f; want_far = reversed ? 0.0f : 1.0f; }
    else                                            { want_near = reversed ? 1.0f : -1.0f; want_far = reversed ? -1.0f : 1.0f; }

    project(&p, 0.1f, 0.2f, s * n, &y, &z, &w);
    CHECK(w > 0, "%s: near point has w=%g (would be clipped)", tag, w);
    CHECK(near_eq(z, want_near, 1e-5f), "%s: near plane -> z_ndc %g, want %g", tag, z, want_near);

    if (infinite) {
        project(&p, 0.0f, 0.0f, s * 1e7f, &y, &z, &w);
        CHECK(near_eq(z, want_far, 1e-4f), "%s: very far point -> z_ndc %g, want ~%g", tag, z, want_far);
    } else {
        project(&p, 0.0f, 0.0f, s * f, &y, &z, &w);
        CHECK(near_eq(z, want_far, 1e-4f), "%s: far plane -> z_ndc %g, want %g", tag, z, want_far);
        /* midway is strictly between, and depth is monotonic */
        {
            float z1, z2;
            project(&p, 0, 0, s * 10.0f, &y, &z1, &w);
            project(&p, 0, 0, s * 20.0f, &y, &z2, &w);
            CHECK(reversed ? z2 < z1 : z2 > z1, "%s: depth not monotonic (%g, %g)", tag, z1, z2);
        }
    }

    /* a point above the view axis: +y in GL/D3D/Metal, -y in Vulkan clip space */
    project(&p, 0.0f, 1.0f, s * 5.0f, &y, &z, &w);
    CHECK(c.clip_y == FP_GFX_CLIP_Y_DOWN ? y < 0 : y > 0, "%s: clip y sign wrong (%g)", tag, y);

    /* depth state that goes with it */
    CHECK(fp_gfx_depth_clear_value(&c) == (reversed ? 0.0f : 1.0f), "%s: clear value", tag);
    CHECK(fp_gfx_depth_compare(&c) == (reversed ? FP_GFX_COMPARE_GREATER : FP_GFX_COMPARE_LESS), "%s: compare", tag);
}

/* C transcription of fp_quat_rotate() in shaders/fpasm.hlsli and fpasm.glsl:
 *   t = 2 cross(q.xyz, v);  v + q.w t + cross(q.xyz, t)                      */
static void shader_quat_rotate(const Quaternion* q, const float v[3], float o[3]) {
    float t[3], c[3];
    t[0] = 2.0f * (q->y * v[2] - q->z * v[1]);
    t[1] = 2.0f * (q->z * v[0] - q->x * v[2]);
    t[2] = 2.0f * (q->x * v[1] - q->y * v[0]);
    c[0] = q->y * t[2] - q->z * t[1];
    c[1] = q->z * t[0] - q->x * t[2];
    c[2] = q->x * t[1] - q->y * t[0];
    o[0] = v[0] + q->w * t[0] + c[0];
    o[1] = v[1] + q->w * t[1] + c[1];
    o[2] = v[2] + q->w * t[2] + c[2];
}

int main(void) {
    Mat4 a, b;
    int32_t p;
    int rev, inf;
    const fp_gfx_conventions* gl = fp_gfx_preset(FP_GFX_OPENGL);

    /* 1. OpenGL preset == the existing functions */
    fp_mat4_perspective(&a, 1.1f, 1.5f, 0.1f, 100.0f);
    fp_mat4_perspective_gfx(&b, 1.1f, 1.5f, 0.1f, 100.0f, gl);
    same_mat("perspective(opengl)", &a, &b);
    fp_mat4_ortho(&a, -3, 5, -2, 4, 0.5f, 50.0f);
    fp_mat4_ortho_gfx(&b, -3, 5, -2, 4, 0.5f, 50.0f, gl);
    same_mat("ortho(opengl)", &a, &b);
    fp_mat4_lookat(&a, 1, 2, 3, -4, 0.5f, 2, 0, 1, 0);
    fp_mat4_lookat_gfx(&b, 1, 2, 3, -4, 0.5f, 2, 0, 1, 0, gl);
    same_mat("lookat == lookat_gfx(opengl)", &a, &b);

    /* 1b. view matrices by behaviour, for arbitrary cameras: the eye maps to
     *     the origin, the target lies straight ahead (-z RH, +z LH), the
     *     camera's right maps to +x and its up to +y, and the rotation is
     *     orthonormal. (The old fp_mat4_lookat failed this: transposed.) */
    {
        static const float cams[][6] = {
            { 5, 0, 0, 0, 0, 0 }, { 1, 2, 3, -4, 0.5f, 2 }, { -3, 7, -2, 4, -1, 9 }, { 0, 10, 0.01f, 0, 0, 0 },
        };
        int k, hand;
        for (hand = 0; hand < 2; hand++)
            for (k = 0; k < 4; k++) {
                const float* e = cams[k];
                const fp_gfx_conventions* cv = fp_gfx_preset(hand ? FP_GFX_D3D11_LH : FP_GFX_D3D11);
                float fwd[3] = { e[3] - e[0], e[4] - e[1], e[5] - e[2] }, len, right[3], up[3], o[4], pt[4];
                float sz = hand ? 1.0f : -1.0f;
                int i, j;
                len = sqrtf(fwd[0] * fwd[0] + fwd[1] * fwd[1] + fwd[2] * fwd[2]);
                for (i = 0; i < 3; i++) fwd[i] /= len;
                /* right = fwd x worldup (RH) or worldup x fwd (LH); up is the same for both */
                right[0] = -fwd[2]; right[1] = 0; right[2] = fwd[0];
                { float rl = sqrtf(right[0] * right[0] + right[2] * right[2]); right[0] /= rl; right[2] /= rl; }
                if (hand) { right[0] = -right[0]; right[2] = -right[2]; }   /* LH: right = up x fwd */
                {   /* up = rightRH x fwd, with rightRH = (-fwd.z, 0, fwd.x) normalized */
                    float rx = hand ? -right[0] : right[0], rz = hand ? -right[2] : right[2];
                    up[0] = -rz * fwd[1];
                    up[1] = rz * fwd[0] - rx * fwd[2];
                    up[2] = rx * fwd[1];
                }
                fp_mat4_lookat_gfx(&a, e[0], e[1], e[2], e[3], e[4], e[5], 0, 1, 0, cv);

                pt[0] = e[0]; pt[1] = e[1]; pt[2] = e[2]; pt[3] = 1; mul(&a, pt, o);
                CHECK(fabsf(o[0]) < 1e-4f && fabsf(o[1]) < 1e-4f && fabsf(o[2]) < 1e-4f,
                      "lookat cam %d hand %d: eye -> (%g %g %g)", k, hand, o[0], o[1], o[2]);
                pt[0] = e[3]; pt[1] = e[4]; pt[2] = e[5]; mul(&a, pt, o);
                CHECK(fabsf(o[0]) < 1e-4f && fabsf(o[1]) < 1e-4f && near_eq(o[2], sz * len, 1e-4f),
                      "lookat cam %d hand %d: target -> (%g %g %g)", k, hand, o[0], o[1], o[2]);
                for (i = 0; i < 3; i++) pt[i] = e[i] + right[i];
                mul(&a, pt, o);
                CHECK(near_eq(o[0], 1.0f, 1e-4f) && fabsf(o[1]) < 1e-4f,
                      "lookat cam %d hand %d: camera right -> (%g %g %g)", k, hand, o[0], o[1], o[2]);
                for (i = 0; i < 3; i++) pt[i] = e[i] + up[i];
                mul(&a, pt, o);
                CHECK(near_eq(o[1], 1.0f, 1e-4f) && fabsf(o[0]) < 1e-4f,
                      "lookat cam %d hand %d: camera up -> (%g %g %g)", k, hand, o[0], o[1], o[2]);
                for (i = 0; i < 3; i++)
                    for (j = 0; j < 3; j++) {
                        float d = a.m[i] * a.m[j] + a.m[4 + i] * a.m[4 + j] + a.m[8 + i] * a.m[8 + j];
                        CHECK(fabsf(d - (i == j)) < 1e-5f, "lookat cam %d: rows %d,%d not orthonormal", k, i, j);
                    }
            }
    }

    /* 2. every preset x reversed-Z x infinite far, perspective and ortho */
    for (p = 0; p < FP_GFX_PRESET_COUNT; p++)
        for (rev = 0; rev <= 1; rev++) {
            for (inf = 0; inf <= 1; inf++) check_projection(p, rev, inf, 0);
            check_projection(p, rev, 0, 1);
        }

    /* 3. D3D11 RH vs LH view matrices (DirectXMath LookAtRH / LookAtLH):
     *    eye (0,0,-5) looking at the origin: LH puts the origin at z=+5,
     *    RH (eye (0,0,5)) at z=-5; both keep x right and y up. */
    {
        float v[4] = { 0, 0, 0, 1 }, o[4], px[4] = { 1, 0, 0, 1 };
        fp_mat4_lookat_gfx(&a, 0, 0, -5, 0, 0, 0, 0, 1, 0, fp_gfx_preset(FP_GFX_D3D11_LH));
        mul(&a, v, o);
        CHECK(near_eq(o[2], 5.0f, 1e-6f) && fabsf(o[0]) < 1e-6f, "LH lookat: origin at z=%g", o[2]);
        mul(&a, px, o);
        CHECK(near_eq(o[0], 1.0f, 1e-6f), "LH lookat: +x maps to x=%g", o[0]);
        fp_mat4_lookat_gfx(&a, 0, 0, 5, 0, 0, 0, 0, 1, 0, fp_gfx_preset(FP_GFX_D3D11));
        mul(&a, v, o);
        CHECK(near_eq(o[2], -5.0f, 1e-6f), "RH lookat: origin at z=%g", o[2]);
        mul(&a, px, o);
        CHECK(near_eq(o[0], 1.0f, 1e-6f), "RH lookat: +x maps to x=%g", o[0]);
        fp_mat4_lookat_gfx(&a, 1, 1, 1, 1, 1, 1, 0, 1, 0, fp_gfx_preset(FP_GFX_D3D11));
        CHECK(a.m[0] == 1 && a.m[5] == 1 && a.m[10] == 1 && a.m[15] == 1 && a.m[12] == 0,
              "degenerate lookat must be identity");
    }

    /* 4. end to end: world point in front of a D3D11 camera lands in the
     *    [0,1] depth range and inside the viewport */
    {
        Mat4 view, proj, vp;
        float v[4] = { 0.3f, -0.2f, 0.0f, 1 }, o[4];
        const fp_gfx_conventions* d3d = fp_gfx_preset(FP_GFX_D3D11);
        fp_mat4_lookat_gfx(&view, 0, 1, 6, 0, 0, 0, 0, 1, 0, d3d);
        fp_mat4_perspective_gfx(&proj, 1.0f, 1.0f, 0.1f, 100.0f, d3d);
        fp_mat4_mul(&vp, &proj, &view);
        mul(&vp, v, o);
        CHECK(o[3] > 0 && o[2] / o[3] > 0 && o[2] / o[3] < 1 && fabsf(o[0] / o[3]) < 1 && fabsf(o[1] / o[3]) < 1,
              "d3d11 end-to-end: clip (%g %g %g %g)", o[0], o[1], o[2], o[3]);
    }

    /* 5. upload layout */
    {
        float dst[16];
        fp_gfx_conventions rm;
        int i;
        for (i = 0; i < 16; i++) a.m[i] = (float)i;
        fp_mat4_upload_gfx(dst, &a, fp_gfx_preset(FP_GFX_D3D11));
        CHECK(memcmp(dst, a.m, sizeof dst) == 0, "column-major upload is a copy");
        fp_gfx_conventions_init(&rm, FP_GFX_D3D11, 0);
        rm.upload_layout = FP_GFX_UPLOAD_ROW_MAJOR;
        fp_mat4_upload_gfx(dst, &a, &rm);
        for (i = 0; i < 16; i++)
            CHECK(dst[i] == a.m[(i % 4) * 4 + i / 4], "row-major upload is the transpose (i=%d)", i);
    }

    /* 5b. the shader-side quaternion helper agrees with the library */
    {
        int k;
        for (k = 0; k < 50; k++) {
            float ax = (float)(k % 7) - 3, ay = (float)(k % 5) - 2, az = (float)(k % 3) + 0.5f;
            float al = sqrtf(ax * ax + ay * ay + az * az), ang = 0.37f * (float)k, sh = sinf(ang * 0.5f);
            Quaternion q = { ax / al * sh, ay / al * sh, az / al * sh, cosf(ang * 0.5f) };
            Vec3f v = { 1.5f - (float)k * 0.1f, 0.25f * (float)k, -2.0f, 0 }, lib;
            float in[3] = { v.x, v.y, v.z }, sh3[3];
            fp_quat_rotate_vec3(&lib, &q, &v);
            shader_quat_rotate(&q, in, sh3);
            CHECK(near_eq(lib.x, sh3[0], 1e-5f) && near_eq(lib.y, sh3[1], 1e-5f) && near_eq(lib.z, sh3[2], 1e-5f),
                  "shader quat rotate k=%d: lib (%g %g %g) shader (%g %g %g)", k, lib.x, lib.y, lib.z, sh3[0], sh3[1], sh3[2]);
        }
    }

    /* 5c. GPU contract of the batch transform: with an affine matrix the
     *     4th lane is written as 1.0 (whatever the input padding holds), so
     *     the output is a ready-to-use position float4 for shaders. */
    {
        Mat4 t, r, m;
        Vec3f in[37], out[37];
        int k;
        fp_mat4_translation(&t, 1, 2, 3);
        fp_mat4_rotation_y(&r, 0.7f);
        fp_mat4_mul(&m, &t, &r);
        for (k = 0; k < 37; k++) { in[k].x = (float)k; in[k].y = 1; in[k].z = -2; in[k]._pad = 123.0f + (float)k; }
        fp_mat4_mul_vec3_batch(out, &m, in, 37);
        for (k = 0; k < 37; k++) CHECK(out[k]._pad == 1.0f, "batch transform w[%d] = %g, want 1", k, out[k]._pad);
    }

    /* 5d. fp_stream_copy: every size 0..600 at every src/dst misalignment
     *     0..31 equals memcpy and writes nothing outside [dst, dst+n). */
    {
        static unsigned char src[700], dst[800], ref[800];
        size_t n, i;
        int so, d0;
        for (i = 0; i < sizeof src; i++) src[i] = (unsigned char)(i * 131u + 7u);
        for (n = 0; n <= 600; n += (n < 140 ? 1 : 37))
            for (so = 0; so < 32; so += 3)
                for (d0 = 0; d0 < 32; d0++) {
                    unsigned char* d = dst + 64 + d0;
                    memset(dst, 0xAB, sizeof dst);
                    memcpy(ref, dst, sizeof ref);
                    memcpy(ref + 64 + d0, src + so, n);
                    fp_stream_copy(d, src + so, n);
                    checks++;
                    if (memcmp(dst, ref, sizeof dst) != 0) {
                        failures++;
                        printf("FAIL fp_stream_copy n=%zu src+%d dst+%d\n", n, so, d0);
                        so = 99; break;
                    }
                }
#if defined(__linux__)
        {   /* src and dst both end exactly at an unmapped page */
            long pg = sysconf(_SC_PAGESIZE);
            unsigned char* m = mmap(NULL, (size_t)pg * 4, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            if (m != MAP_FAILED && mprotect(m + pg, (size_t)pg, PROT_NONE) == 0 &&
                mprotect(m + 3 * (size_t)pg, (size_t)pg, PROT_NONE) == 0) {
                for (n = 0; n < 300; n++) {
                    memset(m, (int)n, (size_t)pg);
                    fp_stream_copy(m + 3 * (size_t)pg - n, m + (size_t)pg - n, n);
                    checks++;
                    if (memcmp(m + 3 * (size_t)pg - n, m + (size_t)pg - n, n) != 0) { failures++; printf("FAIL guard copy n=%zu\n", n); }
                }
            }
            if (m != MAP_FAILED) munmap(m, (size_t)pg * 4);
        }
#endif
    }

    /* 6. helpers, presets, purity of presets */
    CHECK(fp_gfx_cbuffer_size(0) == 0 && fp_gfx_cbuffer_size(1) == 16 && fp_gfx_cbuffer_size(64) == 64 &&
          fp_gfx_cbuffer_size(65) == 80, "cbuffer size rounding");
    CHECK(sizeof(Vec3f) == FP_GFX_VEC3F_STRIDE && sizeof(Mat4) == FP_GFX_MAT4_SIZE, "GPU layout sizes");
    for (p = 0; p < FP_GFX_PRESET_COUNT; p++) {
        CHECK(fp_gfx_preset_from_name(fp_gfx_preset_name(p)) == p, "name round trip %d", p);
        CHECK(fp_gfx_preset(p)->preset == p, "preset id %d", p);
        CHECK(fp_gfx_preset(p)->depth_order == FP_GFX_Z_STANDARD, "presets are standard-Z");
    }
    CHECK(fp_gfx_preset(-1) == NULL && fp_gfx_preset(FP_GFX_PRESET_COUNT) == NULL, "unknown preset -> NULL");
    CHECK(fp_gfx_preset_from_name("dx11") == FP_GFX_D3D11 && fp_gfx_preset_from_name("nope") == -1, "aliases");
    {
        fp_gfx_conventions c;
        CHECK(fp_gfx_conventions_init(&c, 99, 1) == -1, "init rejects unknown preset");
        fp_gfx_conventions_init(&c, FP_GFX_D3D11, 1);
        CHECK(fp_gfx_preset(FP_GFX_D3D11)->depth_order == FP_GFX_Z_STANDARD, "init must not touch the preset");
    }
    printf("build default conventions: %s\n", fp_gfx_preset_name(fp_gfx_default()->preset));

    printf("\n%s (%d checks, %d failures)\n", failures ? "SOME FAILED" : "ALL PASS", checks, failures);
    return failures != 0;
}
