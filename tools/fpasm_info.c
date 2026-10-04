/* fpasm_info.c — `fpasm-info`: CPU / build / dispatch diagnostics.
 *
 *   fpasm-info            full report: CPU, OS state, features, build, dispatch
 *   fpasm-info --check    exit 0 if this build runs on this CPU, 1 otherwise
 *                         (prints the reason; used by `make test`)
 *   fpasm-info --tier     print the best usable tier (avx2|avxvnni|avx512)
 *   fpasm-info --json     machine-readable summary
 *   fpasm-info --bench    time every dispatched kernel in every tier this CPU
 *                         supports (pin to one core for stable numbers)
 *
 * Compiled for the x86-64 baseline (it must run to explain why the library
 * would not), and it only calls kernels of tiers the CPU supports.
 */
#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#  define _POSIX_C_SOURCE 199309L   /* clock_gettime under -std=c11 */
#endif
#include "fp_cpu.h"
#include "fp_dispatch.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#  include <windows.h>
#endif

/* We print our own diagnostics; skip the library's load-time warning. */
const int fp_cpu_no_load_check = 1;

static void usage(void) {
    puts("usage: fpasm-info [--check | --tier | --json | --bench [N] | --help]");
}

static void print_json(void) {
    const fp_cpu_info_t* ci = fp_cpu_info();
    const fp_build_info_t* bi = fp_build_info();
    int i, first = 1;
    printf("{\n  \"vendor\": \"%s\",\n  \"brand\": \"%s\",\n  \"uarch\": \"%s\",\n",
           ci->vendor, ci->brand, ci->uarch);
    printf("  \"family\": %u, \"model\": %u, \"stepping\": %u,\n", ci->family, ci->model, ci->stepping);
    printf("  \"isa_level\": %d, \"hybrid\": %s, \"xcr0\": %llu,\n", ci->isa_level,
           ci->hybrid ? "true" : "false", (unsigned long long)ci->xcr0);
    printf("  \"features\": [");
    for (i = 0; i < FP_CPU_FEATURE_COUNT; i++)
        if (ci->features & FP_CPU_BIT(i)) {
            printf("%s\"%s\"", first ? "" : ", ", fp_cpu_feature_name((fp_cpu_feature)i));
            first = 0;
        }
    printf("],\n  \"build\": {\"isa\": \"%s\", \"march\": \"%s\", \"compiler\": \"%s\", \"dispatch\": %s},\n",
           bi->isa, bi->march, bi->compiler, bi->dispatch ? "true" : "false");
    printf("  \"missing\": [");
    first = 1;
    for (i = 0; i < FP_CPU_FEATURE_COUNT; i++)
        if (fp_cpu_missing() & FP_CPU_BIT(i)) {
            printf("%s\"%s\"", first ? "" : ", ", fp_cpu_feature_name((fp_cpu_feature)i));
            first = 0;
        }
    printf("],\n  \"best_tier\": \"%s\",\n  \"compatible\": %s\n}\n",
           fp_tier_name(fp_dispatch_best_tier()), fp_cpu_missing() ? "false" : "true");
}

/* ---------------------------------------------------------------- bench */
static double now_ns(void) {
#ifdef _WIN32
    LARGE_INTEGER c, f;
    QueryPerformanceCounter(&c);
    QueryPerformanceFrequency(&f);
    return (double)c.QuadPart * 1e9 / (double)f.QuadPart;
#else
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1e9 + t.tv_nsec;
#endif
}
static volatile double g_sink;

typedef double (*bench_fn)(const void* a, const void* b, size_t n, int tier);

#define VARIANT_CALL(ret, base, tier, ...)                                     \
    (tier == FP_TIER_AVX512 ? (double)(ret)base##_avx512(__VA_ARGS__)          \
                            : (double)(ret)base##_avx2(__VA_ARGS__))
#define VARIANT_CALL3(ret, base, tier, ...)                                    \
    (tier == FP_TIER_AVX512  ? (double)(ret)base##_avx512(__VA_ARGS__)  :      \
     tier == FP_TIER_AVXVNNI ? (double)(ret)base##_avxvnni(__VA_ARGS__) :      \
                               (double)(ret)base##_avx2(__VA_ARGS__))

static double b_add_f32(const void* a, const void* b, size_t n, int t) { (void)b; return VARIANT_CALL(float, fp_reduce_add_f32, t, a, n); }
static double b_add_f64(const void* a, const void* b, size_t n, int t) { (void)b; return VARIANT_CALL(double, fp_reduce_add_f64, t, a, n); }
static double b_sq_f32 (const void* a, const void* b, size_t n, int t) { (void)b; return VARIANT_CALL(float, fp_fold_sumsq_f32, t, a, n); }
static double b_dot_f32(const void* a, const void* b, size_t n, int t) { return VARIANT_CALL(float, fp_fold_dotp_f32, t, a, b, n); }
static double b_dot_f64(const void* a, const void* b, size_t n, int t) { return VARIANT_CALL(double, fp_fold_dotp_f64, t, a, b, n); }
static double b_dot_i8 (const void* a, const void* b, size_t n, int t) { return VARIANT_CALL3(int8_t, fp_fold_dotp_i8, t, a, b, n); }
static double b_dot_u8 (const void* a, const void* b, size_t n, int t) { return VARIANT_CALL3(uint8_t, fp_fold_dotp_u8, t, a, b, n); }
static double b_dot_i16(const void* a, const void* b, size_t n, int t) { return VARIANT_CALL3(int16_t, fp_fold_dotp_i16, t, a, b, n); }
static double b_dot_u16(const void* a, const void* b, size_t n, int t) { return VARIANT_CALL3(uint16_t, fp_fold_dotp_u16, t, a, b, n); }

static void run_bench(size_t n) {
    static const struct {
        const char* name; bench_fn fn; size_t esize; int inputs; int has_vnni;
    } K[] = {
        { "fp_reduce_add_f32", b_add_f32, 4, 1, 0 },
        { "fp_reduce_add_f64", b_add_f64, 8, 1, 0 },
        { "fp_fold_sumsq_f32", b_sq_f32,  4, 1, 0 },
        { "fp_fold_dotp_f32",  b_dot_f32, 4, 2, 0 },
        { "fp_fold_dotp_f64",  b_dot_f64, 8, 2, 0 },
        { "fp_fold_dotp_i8",   b_dot_i8,  1, 2, 1 },
        { "fp_fold_dotp_u8",   b_dot_u8,  1, 2, 1 },
        { "fp_fold_dotp_i16",  b_dot_i16, 2, 2, 1 },
        { "fp_fold_dotp_u16",  b_dot_u16, 2, 2, 1 },
    };
    unsigned char *a = malloc(n * 8 + 64), *b = malloc(n * 8 + 64);
    size_t k, i;
    fp_core_type ct = fp_cpu_current_core_type();
    if (!a || !b) { fprintf(stderr, "out of memory\n"); exit(2); }
    for (i = 0; i < n * 8; i++) { a[i] = (unsigned char)(i * 7 + 1); b[i] = (unsigned char)(i * 13 + 5); }
    /* make the float views sane numbers (bit patterns above are arbitrary) */
    for (i = 0; i < n; i++) { ((float*)a)[i] = (float)(i % 1000) * 0.01f; ((float*)b)[i] = (float)(i % 777) * 0.013f; }
    if (n * 8 >= n * 4 * 2)
        for (i = 0; i < n; i++) { ((double*)a)[i] = (double)(i % 1000) * 0.01; ((double*)b)[i] = (double)(i % 777) * 0.013; }

    printf("Micro-benchmark: n = %zu elements per call, best of 50 runs", n);
    if (ct != FP_CORE_UNKNOWN) printf(" (started on a %s)", ct == FP_CORE_PERFORMANCE ? "P-core" : "E-core");
    printf("\n%-20s %-8s %12s %10s %9s\n", "kernel", "tier", "ns/call", "GB/s", "vs avx2");
    for (k = 0; k < sizeof K / sizeof K[0]; k++) {
        double base_ns = 0;
        int t;
        /* the float views alias different bytes for f64; refill per kernel */
        if (K[k].esize == 8)
            for (i = 0; i < n; i++) { ((double*)a)[i] = (double)(i % 1000) * 0.01; ((double*)b)[i] = (double)(i % 777) * 0.013; }
        else if (K[k].esize == 4)
            for (i = 0; i < n; i++) { ((float*)a)[i] = (float)(i % 1000) * 0.01f; ((float*)b)[i] = (float)(i % 777) * 0.013f; }
        for (t = FP_TIER_AVX2; t < FP_TIER_COUNT; t++) {
            double best = 1e30;
            int r;
            if (t == FP_TIER_AVXVNNI && !K[k].has_vnni) continue;
            if (!fp_dispatch_tier_available((fp_tier)t)) continue;
            if (t == FP_TIER_AVX512 && K[k].esize <= 2 && !fp_cpu_has(FP_CPU_AVX512_VNNI)) continue;
            for (r = 0; r < 50; r++) {
                double t0 = now_ns();
                g_sink = K[k].fn(a, b, n, t);
                t0 = now_ns() - t0;
                if (t0 < best) best = t0;
            }
            if (t == FP_TIER_AVX2) base_ns = best;
            printf("%-20s %-8s %12.0f %10.2f %8.2fx\n", K[k].name, fp_tier_name((fp_tier)t), best,
                   (double)(n * K[k].esize * K[k].inputs) / best, base_ns / best);
        }
    }
    printf("\nTips: pin to one core (Linux: taskset -c 2 ./fpasm-info --bench); on hybrid CPUs\n"
           "compare a P-core against an E-core. FPASM_TIER caps what the public symbols use.\n");
    free(a);
    free(b);
}

int main(int argc, char** argv) {
    if (argc > 1) {
        if (!strcmp(argv[1], "--check")) {
            int n = fp_cpu_check(stderr);
            if (!n) printf("fpasm-info: OK — build '%s' runs on %s (%s)\n",
                           fp_build_info()->isa, fp_cpu_info()->brand, fp_cpu_info()->uarch);
            return n ? 1 : 0;
        }
        if (!strcmp(argv[1], "--tier")) { puts(fp_tier_name(fp_dispatch_best_tier())); return 0; }
        if (!strcmp(argv[1], "--json")) { print_json(); return 0; }
        if (!strcmp(argv[1], "--bench")) {
            size_t n = argc > 2 ? (size_t)strtoull(argv[2], NULL, 10) : 1u << 16;
            if (!fp_dispatch_tier_available(FP_TIER_AVX2)) { fp_cpu_check(stderr); return 1; }
            run_bench(n ? n : 1u << 16);
            return 0;
        }
        usage();
        return strcmp(argv[1], "--help") && strcmp(argv[1], "-h") ? 2 : 0;
    }
    puts("FP-ASM diagnostics");
    puts("==================");
    fp_cpu_report(stdout);
    puts("");
    fp_dispatch_report(stdout);
    return fp_cpu_missing() ? 1 : 0;
}
