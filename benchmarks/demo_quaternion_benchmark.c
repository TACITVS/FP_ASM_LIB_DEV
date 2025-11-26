#include "fp_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include <malloc.h>
#define ALIGNED_MALLOC(size, alignment) ((void*)_aligned_malloc(size, alignment))
#define ALIGNED_FREE(ptr) _aligned_free(ptr)

#define BENCH_N    100000   // quaternions per iteration
#define ITERATIONS 200      // outer repetitions

static Quaternion* g_in;
static Quaternion* g_out_lib;   /* library batch normalize */
static Quaternion* g_out_c;     /* scalar C baseline */

// -----------------------------------------------------------------------------
// Baseline C implementation (matches fp_quat_normalize_pure_c)
// -----------------------------------------------------------------------------
static void quat_normalize_baseline(Quaternion* out, const Quaternion* q) {
    float len_sq = q->x * q->x + q->y * q->y + q->z * q->z + q->w * q->w;
    if (len_sq < 1e-8f) {
        out->x = 0.0f;
        out->y = 0.0f;
        out->z = 0.0f;
        out->w = 1.0f;
        return;
    }
    float inv_len = 1.0f / sqrtf(len_sq);
    out->x = q->x * inv_len;
    out->y = q->y * inv_len;
    out->z = q->z * inv_len;
    out->w = q->w * inv_len;
}

// -----------------------------------------------------------------------------
// Benchmark harness
// -----------------------------------------------------------------------------
static double get_time_ms(void) {
    return (double)clock() / CLOCKS_PER_SEC * 1000.0;
}

static void run_benchmark(const char* name, void (*func)(void), int n_ops) {
    double start = get_time_ms();
    for (int i = 0; i < ITERATIONS; ++i) {
        func();
    }
    double end = get_time_ms();
    double elapsed_ms = (end - start) / ITERATIONS;
    double ops_per_sec = (double)n_ops / (elapsed_ms / 1000.0);
    printf("%-32s: %8.3f ms/iter | %10.2f ops/sec\n",
           name, elapsed_ms, ops_per_sec);
}

static void init_data(void) {
    g_in      = (Quaternion*)ALIGNED_MALLOC(sizeof(Quaternion) * BENCH_N, 16);
    g_out_lib = (Quaternion*)ALIGNED_MALLOC(sizeof(Quaternion) * BENCH_N, 16);
    g_out_c   = (Quaternion*)ALIGNED_MALLOC(sizeof(Quaternion) * BENCH_N, 16);

    srand(42);
    for (int i = 0; i < BENCH_N; ++i) {
        float x = (float)(rand() % 1000) / 100.0f;
        float y = (float)(rand() % 1000) / 100.0f;
        float z = (float)(rand() % 1000) / 100.0f;
        float w = (float)(rand() % 1000) / 100.0f;
        g_in[i].x = x;
        g_in[i].y = y;
        g_in[i].z = z;
        g_in[i].w = w;
    }
}

static void cleanup_data(void) {
    ALIGNED_FREE(g_in);
    ALIGNED_FREE(g_out_lib);
    ALIGNED_FREE(g_out_c);
}

// -----------------------------------------------------------------------------
// Benchmarked functions
// -----------------------------------------------------------------------------
static void bench_quat_normalize_lib(void) {
    fp_quat_normalize_batch(g_out_lib, g_in, BENCH_N);
}

static void bench_quat_normalize_c(void) {
    for (int i = 0; i < BENCH_N; ++i) {
        quat_normalize_baseline(&g_out_c[i], &g_in[i]);
    }
}

int main(void) {
    printf("--- Quaternion Normalize Benchmark ---\n");
    printf("N = %d, iterations = %d\n\n", BENCH_N, ITERATIONS);

    init_data();

    run_benchmark("Library  fp_quat_normalize", bench_quat_normalize_lib, BENCH_N);
    run_benchmark("C base  quat_normalize_baseline", bench_quat_normalize_c, BENCH_N);

    cleanup_data();

    return 0;
}
