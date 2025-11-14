#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "../include/fp_3d_math.h"

#if defined(_MSC_VER) || defined(_WIN32)
#    include <malloc.h>
#endif

#if defined(_WIN32)
#    include <windows.h>
typedef struct {
    LARGE_INTEGER freq;
    LARGE_INTEGER t0;
} hi_timer_t;

static hi_timer_t timer_start(void) {
    hi_timer_t t;
    QueryPerformanceFrequency(&t.freq);
    QueryPerformanceCounter(&t.t0);
    return t;
}

static double timer_ms_since(const hi_timer_t* t) {
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    const double delta = (double)(now.QuadPart - t->t0.QuadPart);
    return (1000.0 * delta) / (double)t->freq.QuadPart;
}
#else
#    include <time.h>
typedef struct {
    struct timespec t0;
} hi_timer_t;

static hi_timer_t timer_start(void) {
    hi_timer_t t;
    clock_gettime(CLOCK_MONOTONIC, &t.t0);
    return t;
}

static double timer_ms_since(const hi_timer_t* t) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    const double sec = (double)(now.tv_sec - t->t0.tv_sec);
    const double nsec = (double)(now.tv_nsec - t->t0.tv_nsec);
    return sec * 1000.0 + nsec / 1.0e6;
}
#endif

static void* alloc_aligned16(size_t bytes) {
#if defined(_MSC_VER) || defined(_WIN32)
    void* ptr = _aligned_malloc(bytes, 16);
    if (!ptr) {
        fprintf(stderr, "_aligned_malloc failed for %zu bytes\n", bytes);
        exit(EXIT_FAILURE);
    }
    return ptr;
#else
    void* ptr = NULL;
    if (posix_memalign(&ptr, 16, bytes) != 0) {
        fprintf(stderr, "posix_memalign failed for %zu bytes\n", bytes);
        exit(EXIT_FAILURE);
    }
    return ptr;
#endif
}

static void free_aligned16(void* ptr) {
#if defined(_MSC_VER) || defined(_WIN32)
    _aligned_free(ptr);
#else
    free(ptr);
#endif
}

static void fill_vectors(Vec3f* vecs, size_t n, float ax, float ay, float az) {
    for (size_t i = 0; i < n; ++i) {
        vecs[i].x = ax * (float)i + 0.11f;
        vecs[i].y = ay * (float)i - 0.07f;
        vecs[i].z = az * (float)i + 0.33f;
        vecs[i]._padding = 0.0f;
    }
}

static bool nearly_equal(float ref, float got, float tol) {
    const float diff = fabsf(ref - got);
    const float scale = fmaxf(fabsf(ref), fmaxf(fabsf(got), 1.0f));
    return diff <= tol * scale;
}

static bool vec3_arrays_match(const Vec3f* a, const Vec3f* b, size_t n, float tol) {
    for (size_t i = 0; i < n; ++i) {
        if (!nearly_equal(a[i].x, b[i].x, tol) ||
            !nearly_equal(a[i].y, b[i].y, tol) ||
            !nearly_equal(a[i].z, b[i].z, tol)) {
            return false;
        }
    }
    return true;
}

static void ensure_vec3_equal(const Vec3f* ref, const Vec3f* got, size_t n, const char* label) {
    if (!vec3_arrays_match(ref, got, n, 5e-5f)) {
        fprintf(stderr, "%s verification failed.\n", label);
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char** argv) {
    const size_t n = (argc > 1) ? (size_t)strtoull(argv[1], NULL, 10) : (1u << 20);
    const int iterations = (argc > 2) ? (int)strtol(argv[2], NULL, 10) : 10;

    printf("FP-ASM 3D Math Benchmark\n");
    printf("Elements: %zu, Iterations: %d\n", n, iterations);

    Vec3f* src_a = (Vec3f*)alloc_aligned16(n * sizeof(Vec3f));
    Vec3f* src_b = (Vec3f*)alloc_aligned16(n * sizeof(Vec3f));
    Vec3f* dst_ref = (Vec3f*)alloc_aligned16(n * sizeof(Vec3f));
    Vec3f* dst_fp = (Vec3f*)alloc_aligned16(n * sizeof(Vec3f));

    fill_vectors(src_a, n, 0.17f, -0.08f, 0.05f);
    fill_vectors(src_b, n, -0.03f, 0.11f, -0.09f);

    Mat4f matrix = {{
        1.0f, 0.05f, -0.15f, 0.0f,
        0.25f, 0.9f, 0.35f, 0.0f,
        -0.45f, 0.55f, 1.2f, 0.0f,
        1.0f, -2.0f, 0.75f, 1.0f
    }};

    QuatF32 quat = {0.0f, 0.38268343f, 0.0f, 0.92387950f};

    /* Validate correctness before benchmarking */
    ref_map_transform_vec3_f32(src_a, dst_ref, n, &matrix);
    fp_map_transform_vec3_f32(src_a, dst_fp, n, &matrix);
    ensure_vec3_equal(dst_ref, dst_fp, n, "Transform map");

    ref_zipWith_vec3_add_f32(src_a, src_b, dst_ref, n);
    fp_zipWith_vec3_add_f32(src_a, src_b, dst_fp, n);
    ensure_vec3_equal(dst_ref, dst_fp, n, "Vec3 add");

    ref_map_quat_rotate_vec3_f32(src_a, dst_ref, n, &quat);
    fp_map_quat_rotate_vec3_f32(src_a, dst_fp, n, &quat);
    ensure_vec3_equal(dst_ref, dst_fp, n, "Quaternion rotate");

    const float ref_fold = ref_fold_vec3_dot_f32(src_a, src_b, n);
    const float fp_fold = fp_fold_vec3_dot_f32(src_a, src_b, n);
    if (!nearly_equal(ref_fold, fp_fold, 5e-4f)) {
        fprintf(stderr, "Dot product verification failed: %.6f vs %.6f\n", ref_fold, fp_fold);
        exit(EXIT_FAILURE);
    }

    hi_timer_t timer;
    double ref_ms, asm_ms;

    /* Transform benchmark */
    timer = timer_start();
    for (int it = 0; it < iterations; ++it) {
        ref_map_transform_vec3_f32(src_a, dst_ref, n, &matrix);
    }
    ref_ms = timer_ms_since(&timer);

    timer = timer_start();
    for (int it = 0; it < iterations; ++it) {
        fp_map_transform_vec3_f32(src_a, dst_fp, n, &matrix);
    }
    asm_ms = timer_ms_since(&timer);
    printf("Transform map -> ref: %.3f ms, asm: %.3f ms, speedup: %.2fx\n", ref_ms, asm_ms, ref_ms / asm_ms);

    /* Vector add benchmark */
    timer = timer_start();
    for (int it = 0; it < iterations; ++it) {
        ref_zipWith_vec3_add_f32(src_a, src_b, dst_ref, n);
    }
    ref_ms = timer_ms_since(&timer);

    timer = timer_start();
    for (int it = 0; it < iterations; ++it) {
        fp_zipWith_vec3_add_f32(src_a, src_b, dst_fp, n);
    }
    asm_ms = timer_ms_since(&timer);
    printf("Vec3 add -> ref: %.3f ms, asm: %.3f ms, speedup: %.2fx\n", ref_ms, asm_ms, ref_ms / asm_ms);

    /* Quaternion rotate benchmark */
    timer = timer_start();
    for (int it = 0; it < iterations; ++it) {
        ref_map_quat_rotate_vec3_f32(src_a, dst_ref, n, &quat);
    }
    ref_ms = timer_ms_since(&timer);

    timer = timer_start();
    for (int it = 0; it < iterations; ++it) {
        fp_map_quat_rotate_vec3_f32(src_a, dst_fp, n, &quat);
    }
    asm_ms = timer_ms_since(&timer);
    printf("Quaternion rotate -> ref: %.3f ms, asm: %.3f ms, speedup: %.2fx\n", ref_ms, asm_ms, (asm_ms > 0.0) ? ref_ms / asm_ms : 0.0);

    /* Dot product benchmark */
    float sink = 0.0f;
    timer = timer_start();
    for (int it = 0; it < iterations; ++it) {
        sink += ref_fold_vec3_dot_f32(src_a, src_b, n);
    }
    ref_ms = timer_ms_since(&timer);

    timer = timer_start();
    for (int it = 0; it < iterations; ++it) {
        sink += fp_fold_vec3_dot_f32(src_a, src_b, n);
    }
    asm_ms = timer_ms_since(&timer);
    printf("Dot product -> ref: %.3f ms, asm: %.3f ms, speedup: %.2fx\n", ref_ms, asm_ms, ref_ms / asm_ms);

    free_aligned16(src_a);
    free_aligned16(src_b);
    free_aligned16(dst_ref);
    free_aligned16(dst_fp);

    if (sink < 0.0f) {
        printf("Sink=%f\n", sink); /* Prevent optimization */
    }

    return 0;
}
