/* Checks the FFI surface (include/fp_ffi.h):
 *   - fp_ffi.h is self-contained (included first) and its prototypes match
 *     the regular headers exactly (included after it: any mismatch is a
 *     compile error);
 *   - ABI version and type sizes as documented;
 *   - the documented "out may equal in" for the element-wise maps;
 *   - the shared library, loaded at runtime like an FFI does (dlopen /
 *     LoadLibrary), exports every function the header lists, and calling one
 *     through the looked-up address gives the statically linked result. */
#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#  define _POSIX_C_SOURCE 200809L   /* readlink under -std=c11 */
#endif
#include "fp_ffi.h"        /* first: must not depend on anything else */
#include "fp_core.h"
#include "fp_cpu.h"
#include "fp_dispatch.h"
#include "fp_gfx.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#if defined(_WIN32)
#  include <windows.h>
#elif defined(__linux__)
#  include <dlfcn.h>
#  include <limits.h>
#  include <unistd.h>
#endif

static int failures = 0, checks = 0;
#define CHECK(cond, ...) do { checks++; if (!(cond)) { failures++; printf("FAIL %s:%d ", __FILE__, __LINE__); \
                              printf(__VA_ARGS__); printf("\n"); } } while (0)

/* Every function fp_ffi.h declares (its own section plus the fp_gfx.h ones it
 * points to). Keep in sync with the header; the address table below makes
 * the static link fail if one is missing. */
static const char* const ffi_names[] = {
    "fp_ffi_abi_version", "fp_ffi_type_size", "fp_dispatch_init", "fp_cpu_check",
    "fp_reduce_add_f32", "fp_reduce_min_f32", "fp_reduce_max_f32",
    "fp_reduce_add_f64", "fp_reduce_min_f64", "fp_reduce_max_f64",
    "fp_reduce_add_i32", "fp_reduce_min_i32", "fp_reduce_max_i32",
    "fp_reduce_add_i64", "fp_reduce_min_i64", "fp_reduce_max_i64",
    "fp_fold_dotp_f32", "fp_fold_sumsq_f32", "fp_fold_dotp_f64", "fp_fold_dotp_i32",
    "fp_fold_dotp_i16", "fp_fold_dotp_i8", "fp_fold_dotp_u8",
    "fp_map_scale_f32", "fp_map_offset_f32", "fp_map_axpy_f32", "fp_zip_add_f32",
    "fp_map_scale_f64", "fp_map_offset_f64", "fp_map_axpy_f64", "fp_zip_add_f64",
    "fp_mat4_mul_vec3_batch", "fp_map_transform_vec3_f32", "fp_map_quat_rotate_vec3_f32",
    "fp_zipWith_vec3_add_f32", "fp_reduce_vec3_add_f32", "fp_fold_vec3_dot_f32",
    "fp_quat_normalize_batch",
    "fp_mat4_identity", "fp_mat4_mul", "fp_mat4_transpose", "fp_mat4_inverse",
    "fp_mat4_translation", "fp_mat4_scale", "fp_mat4_rotation_axis", "fp_mat4_rotation_euler",
    "fp_quat_mul", "fp_quat_rotate_vec3", "fp_quat_normalize", "fp_quat_to_mat4_pure_c",
    "fp_gfx_preset", "fp_gfx_default", "fp_gfx_conventions_init",
    "fp_mat4_perspective_gfx", "fp_mat4_ortho_gfx", "fp_mat4_lookat_gfx", "fp_mat4_upload_gfx",
    "fp_gfx_depth_clear_value", "fp_gfx_depth_compare", "fp_gfx_cbuffer_size", "fp_stream_copy",
};
typedef void (*fn_t)(void);
static const fn_t ffi_addrs[] = {
    (fn_t)fp_ffi_abi_version, (fn_t)fp_ffi_type_size, (fn_t)fp_dispatch_init, (fn_t)fp_cpu_check,
    (fn_t)fp_reduce_add_f32, (fn_t)fp_reduce_min_f32, (fn_t)fp_reduce_max_f32,
    (fn_t)fp_reduce_add_f64, (fn_t)fp_reduce_min_f64, (fn_t)fp_reduce_max_f64,
    (fn_t)fp_reduce_add_i32, (fn_t)fp_reduce_min_i32, (fn_t)fp_reduce_max_i32,
    (fn_t)fp_reduce_add_i64, (fn_t)fp_reduce_min_i64, (fn_t)fp_reduce_max_i64,
    (fn_t)fp_fold_dotp_f32, (fn_t)fp_fold_sumsq_f32, (fn_t)fp_fold_dotp_f64, (fn_t)fp_fold_dotp_i32,
    (fn_t)fp_fold_dotp_i16, (fn_t)fp_fold_dotp_i8, (fn_t)fp_fold_dotp_u8,
    (fn_t)fp_map_scale_f32, (fn_t)fp_map_offset_f32, (fn_t)fp_map_axpy_f32, (fn_t)fp_zip_add_f32,
    (fn_t)fp_map_scale_f64, (fn_t)fp_map_offset_f64, (fn_t)fp_map_axpy_f64, (fn_t)fp_zip_add_f64,
    (fn_t)fp_mat4_mul_vec3_batch, (fn_t)fp_map_transform_vec3_f32, (fn_t)fp_map_quat_rotate_vec3_f32,
    (fn_t)fp_zipWith_vec3_add_f32, (fn_t)fp_reduce_vec3_add_f32, (fn_t)fp_fold_vec3_dot_f32,
    (fn_t)fp_quat_normalize_batch,
    (fn_t)fp_mat4_identity, (fn_t)fp_mat4_mul, (fn_t)fp_mat4_transpose, (fn_t)fp_mat4_inverse,
    (fn_t)fp_mat4_translation, (fn_t)fp_mat4_scale, (fn_t)fp_mat4_rotation_axis, (fn_t)fp_mat4_rotation_euler,
    (fn_t)fp_quat_mul, (fn_t)fp_quat_rotate_vec3, (fn_t)fp_quat_normalize, (fn_t)fp_quat_to_mat4_pure_c,
    (fn_t)fp_gfx_preset, (fn_t)fp_gfx_default, (fn_t)fp_gfx_conventions_init,
    (fn_t)fp_mat4_perspective_gfx, (fn_t)fp_mat4_ortho_gfx, (fn_t)fp_mat4_lookat_gfx, (fn_t)fp_mat4_upload_gfx,
    (fn_t)fp_gfx_depth_clear_value, (fn_t)fp_gfx_depth_compare, (fn_t)fp_gfx_cbuffer_size, (fn_t)fp_stream_copy,
};
#define N_FFI (sizeof ffi_names / sizeof ffi_names[0])
typedef char names_match_addrs[(sizeof ffi_names / sizeof ffi_names[0]) ==
                               (sizeof ffi_addrs / sizeof ffi_addrs[0]) ? 1 : -1];

/* ---- runtime loading, as an FFI does ---- */
#if defined(_WIN32)
typedef HMODULE lib_t;
static lib_t lib_open(const char* dir) {
    char p[4096 + 32];
    snprintf(p, sizeof p, "%s\\libfpasm.dll", dir);
    return LoadLibraryA(p);
}
static void* lib_sym(lib_t l, const char* n) { return (void*)GetProcAddress(l, n); }
static int exe_dir(char* out, size_t cap) {
    DWORD len = GetModuleFileNameA(NULL, out, (DWORD)cap);
    char* s;
    if (len == 0 || len >= cap) return 0;
    s = strrchr(out, '\\');
    if (!s) s = strrchr(out, '/');
    if (!s) return 0;
    *s = '\0';
    return 1;
}
#elif defined(__linux__)
typedef void* lib_t;
static lib_t lib_open(const char* dir) {
    char p[PATH_MAX + 32];
    snprintf(p, sizeof p, "%s/libfpasm.so", dir);
    return dlopen(p, RTLD_NOW | RTLD_LOCAL);
}
static void* lib_sym(lib_t l, const char* n) { return dlsym(l, n); }
static int exe_dir(char* out, size_t cap) {
    ssize_t len = readlink("/proc/self/exe", out, cap - 1);
    char* s;
    if (len <= 0) return 0;
    out[len] = '\0';
    s = strrchr(out, '/');
    if (!s) return 0;
    *s = '\0';
    return 1;
}
#endif

int main(void) {
    size_t i, n;

    /* ABI description */
    CHECK(fp_ffi_abi_version() == FP_FFI_ABI_VERSION, "abi version %u", fp_ffi_abi_version());
    CHECK(fp_ffi_type_size(FP_FFI_TYPE_VEC3F) == 16 && fp_ffi_type_size(FP_FFI_TYPE_QUATERNION) == 16 &&
          fp_ffi_type_size(FP_FFI_TYPE_MAT4) == 64 && fp_ffi_type_size(FP_FFI_TYPE_GFX_CONVENTIONS) == 24 &&
          fp_ffi_type_size(99) == 0, "type sizes");
    for (i = 0; i < N_FFI; i++) CHECK(ffi_addrs[i] != NULL, "%s has no address", ffi_names[i]);

    /* out may equal in for the element-wise maps */
    for (n = 0; n <= 70; n++) {
        float  f[80], g[80], ref[80];
        double d[80], e[80], dref[80];
        for (i = 0; i < n; i++) { f[i] = (float)i * 0.5f - 7; g[i] = (float)(i % 5); d[i] = f[i]; e[i] = g[i]; }
        for (i = 0; i < n; i++) ref[i] = f[i] * 3.0f;
        fp_map_scale_f32(f, f, n, 3.0f);
        CHECK(memcmp(f, ref, n * sizeof *f) == 0, "in-place scale_f32 n=%zu", n);
        for (i = 0; i < n; i++) ref[i] = f[i] + 1.5f;
        fp_map_offset_f32(f, f, n, 1.5f);
        CHECK(memcmp(f, ref, n * sizeof *f) == 0, "in-place offset_f32 n=%zu", n);
        for (i = 0; i < n; i++) ref[i] = 2.0f * f[i] + g[i];
        fp_map_axpy_f32(f, g, f, n, 2.0f);
        for (i = 0; i < n; i++) CHECK(fabsf(f[i] - ref[i]) <= 1e-5f * (1 + fabsf(ref[i])), "in-place axpy_f32 n=%zu i=%zu", n, i);
        for (i = 0; i < n; i++) ref[i] = f[i] + g[i];
        fp_zip_add_f32(f, g, f, n);
        CHECK(memcmp(f, ref, n * sizeof *f) == 0, "in-place zip_add_f32 n=%zu", n);
        for (i = 0; i < n; i++) dref[i] = d[i] * 3.0;
        fp_map_scale_f64(d, d, n, 3.0);
        CHECK(memcmp(d, dref, n * sizeof *d) == 0, "in-place scale_f64 n=%zu", n);
        for (i = 0; i < n; i++) dref[i] = d[i] + 1.5;
        fp_map_offset_f64(d, d, n, 1.5);
        CHECK(memcmp(d, dref, n * sizeof *d) == 0, "in-place offset_f64 n=%zu", n);
        for (i = 0; i < n; i++) dref[i] = 2.0 * d[i] + e[i];
        fp_map_axpy_f64(d, e, d, n, 2.0);
        for (i = 0; i < n; i++) CHECK(fabs(d[i] - dref[i]) <= 1e-12 * (1 + fabs(dref[i])), "in-place axpy_f64 n=%zu", n);
        for (i = 0; i < n; i++) dref[i] = d[i] + e[i];
        fp_zip_add_f64(d, e, d, n);
        CHECK(memcmp(d, dref, n * sizeof *d) == 0, "in-place zip_add_f64 n=%zu", n);
    }

    /* the exported quaternion->matrix equals the header's inline one */
    {
        Quaternion q = { 0.1825742f, 0.3651484f, 0.5477226f, 0.7302967f };
        Mat4 a, b;
        fp_quat_to_mat4(&a, &q);
        fp_quat_to_mat4_pure_c(&b, &q);
        for (i = 0; i < 16; i++) CHECK(fabsf(a.m[i] - b.m[i]) < 1e-6f, "quat_to_mat4 m[%zu]", i);
    }

    /* load the shared library the way a foreign language would */
#if defined(_WIN32) || defined(__linux__)
    {
        char dir[4096];
        lib_t lib = exe_dir(dir, sizeof dir) ? lib_open(dir) : NULL;
        if (!lib) {
            printf("shared library: SKIP (no libfpasm shared library next to this test)\n");
        } else {
            int missing = 0;
            for (i = 0; i < N_FFI; i++)
                if (!lib_sym(lib, ffi_names[i])) { printf("FAIL shared library does not export %s\n", ffi_names[i]); missing++; }
            checks++;
            if (missing) failures++;
            {
                float (*dot)(const float*, const float*, size_t) =
                    (float (*)(const float*, const float*, size_t))lib_sym(lib, "fp_fold_dotp_f32");
                uint32_t (*ver)(void) = (uint32_t (*)(void))lib_sym(lib, "fp_ffi_abi_version");
                float a[37], b[37];
                for (i = 0; i < 37; i++) { a[i] = (float)i; b[i] = 0.5f; }
                CHECK(dot && dot(a, b, 37) == fp_fold_dotp_f32(a, b, 37), "dotp via the loaded library");
                CHECK(ver && ver() == FP_FFI_ABI_VERSION, "abi version via the loaded library");
            }
            printf("shared library: %zu FFI symbols resolved by name\n", N_FFI);
        }
    }
#endif

    printf("\n%s (%d checks, %d failures)\n", failures ? "SOME FAILED" : "ALL PASS", checks, failures);
    return failures != 0;
}
