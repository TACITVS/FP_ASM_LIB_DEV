#include "fp_core.h"
#include "fp_3d_math_wrapper.h" // For C-kernels and reference implementations
#include "fp_generic.h" // For Layer 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include <malloc.h>
#define ALIGNED_MALLOC(size, alignment) _aligned_malloc(size, alignment)
#define ALIGNED_FREE(ptr) _aligned_free(ptr)

#define BENCH_N (1024 * 1024) // Number of vectors
#define ITERATIONS 100

// --- Test Data Initialization ---
ALIGN16 Mat4 g_matrix = {{
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    10.0f, 20.0f, 30.0f, 1.0f // Translate +10, +20, +30
}};

ALIGN16 Quaternion g_quat = { 0.0f, 0.707107f, 0.0f, 0.707107f }; // 90 deg rot around Y

Vec3f* g_in_a;
Vec3f* g_in_b;
Vec3f* g_out_ref; // Output from C-reference
Vec3f* g_out_asm; // Output from Assembly

void init_data() {
    g_in_a = ALIGNED_MALLOC(BENCH_N * sizeof(Vec3f), 16);
    g_in_b = ALIGNED_MALLOC(BENCH_N * sizeof(Vec3f), 16);
    g_out_ref = ALIGNED_MALLOC(BENCH_N * sizeof(Vec3f), 16);
    g_out_asm = ALIGNED_MALLOC(BENCH_N * sizeof(Vec3f), 16);

    for (size_t i = 0; i < BENCH_N; i++) {
        g_in_a[i] = (Vec3f){(float)i, (float)(i*2), (float)(i*3), 0.0f};
        g_in_b[i] = (Vec3f){(float)i, (float)i, (float)i, 0.0f};
    }
}

void cleanup_data() {
    ALIGNED_FREE(g_in_a);
    ALIGNED_FREE(g_in_b);
    ALIGNED_FREE(g_out_ref);
    ALIGNED_FREE(g_out_asm);
}

// --- Benchmarking Utilities ---
double get_time_ms() {
    return (double)clock() / CLOCKS_PER_SEC * 1000.0;
}

void run_benchmark(const char* name, void (*func)(), int n_ops) {
    double start_time = get_time_ms();
    for (int i = 0; i < ITERATIONS; i++) {
        func();
    }
    double end_time = get_time_ms();
    double elapsed_ms = (end_time - start_time) / ITERATIONS;
    double ops_per_sec = (double)n_ops / (elapsed_ms / 1000.0);
    printf("% -30s: %8.3f ms/iter | %10.2f ops/sec\n", name, elapsed_ms, ops_per_sec);
}

// --- Benchmark Functions ---

void bench_transform_ref() {
    ref_map_transform_vec3_f32(g_in_a, g_out_ref, BENCH_N, &g_matrix);
}

void bench_transform_asm() {
    fp_map_transform_vec3_f32(g_in_a, g_out_asm, BENCH_N, &g_matrix);
}

void bench_zip_add_ref() {
    ref_zipWith_vec3_add_f32(g_in_a, g_in_b, g_out_ref, BENCH_N);
}

void bench_zip_add_asm() {
    fp_zipWith_vec3_add_f32(g_in_a, g_in_b, g_out_asm, BENCH_N);
}

void bench_reduce_add_ref() {
    Vec3f sum;
    ref_reduce_vec3_add_f32(g_in_a, BENCH_N, &sum);
}

void bench_reduce_add_asm() {
    Vec3f sum;
    fp_reduce_vec3_add_f32(g_in_a, BENCH_N, &sum);
}

void bench_fold_dot_ref() {
    ref_fold_vec3_dot_f32(g_in_a, g_in_b, BENCH_N);
}

void bench_fold_dot_asm() {
    fp_fold_vec3_dot_f32(g_in_a, g_in_b, BENCH_N);
}

void bench_quat_rotate_ref() {
    ref_map_quat_rotate_vec3_f32(g_in_a, g_out_ref, BENCH_N, &g_quat);
}

void bench_quat_rotate_asm() {
    fp_map_quat_rotate_vec3_f32(g_in_a, g_out_asm, BENCH_N, &g_quat);
}

// --- Main Benchmark Runner ---
int main(void) {
    printf("--- Running 3D Math Benchmarks (N=%d, Iterations=%d) ---\n", BENCH_N, ITERATIONS);
    init_data();

    run_benchmark("Transform Vec3 (Ref)", bench_transform_ref, BENCH_N);
    run_benchmark("Transform Vec3 (ASM)", bench_transform_asm, BENCH_N);
    printf("\n");

    run_benchmark("ZipAdd Vec3 (Ref)", bench_zip_add_ref, BENCH_N);
    run_benchmark("ZipAdd Vec3 (ASM)", bench_zip_add_asm, BENCH_N);
    printf("\n");

    run_benchmark("ReduceAdd Vec3 (Ref)", bench_reduce_add_ref, BENCH_N);
    run_benchmark("ReduceAdd Vec3 (ASM)", bench_reduce_add_asm, BENCH_N);
    printf("\n");

    run_benchmark("FoldDot Vec3 (Ref)", bench_fold_dot_ref, BENCH_N);
    run_benchmark("FoldDot Vec3 (ASM)", bench_fold_dot_asm, BENCH_N);
    printf("\n");

    run_benchmark("Quat Rotate Vec3 (Ref)", bench_quat_rotate_ref, BENCH_N);
    run_benchmark("Quat Rotate Vec3 (ASM) (Stub)", bench_quat_rotate_asm, BENCH_N);
    printf("\n");

    cleanup_data();
    printf("------------------------------------------------\n");
    
    return 0;
}