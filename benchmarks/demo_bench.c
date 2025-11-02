// FP-ASM-DemoBench-v4
// Build against fp_core_win64.asm and fp.h.
//
// CHANGES (v4):
// - Added a 'volatile int64_t sink' variable.
// - Added the result of each benchmarked operation to the sink
//   (e.g., sink += c_sum) INSIDE the timing loop.
// - This prevents the -O3 compiler from performing Dead Code Elimination
//   on the C benchmarks, ensuring a fair comparison against the ASM.

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <windows.h>
#include "../include/fp.h"

// --- (Timing functions are unchanged) ---
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
    const double dt = (double)(now.QuadPart - t->t0.QuadPart);
    return (1000.0 * dt) / (double)t->freq.QuadPart;
}

// --- (Baseline C functions are unchanged) ---
static size_t c_map_i64(const int64_t* in, int64_t* out, size_t n, fp_unary_i64 f) {
    for (size_t i = 0; i < n; ++i) out[i] = f(in[i]);
    return n;
}

static int64_t c_reduce_i64(const int64_t* a, size_t n, int64_t init, fp_binary_i64 op) {
    int64_t acc = init;
    for (size_t i = 0; i < n; ++i) acc = op(acc, a[i]);
    return acc;
}

static size_t c_map_square_i64(const int64_t* in, int64_t* out, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        out[i] = in[i] * in[i];
    }
    return n;
}

static int64_t c_reduce_add_i64(const int64_t* a, size_t n, int64_t init) {
    int64_t acc = init;
    for (size_t i = 0; i < n; ++i) {
        acc += a[i];
    }
    return acc;
}

static int64_t c_foldmap_sumsq_i64(const int64_t* in, size_t n, int64_t init) {
    int64_t acc = init;
    for (size_t i = 0; i < n; ++i) {
        acc += in[i] * in[i];
    }
    return acc;
}

// --- (Sample user functions are unchanged) ---
static int64_t square_i64(int64_t x) { return x * x; }
static int64_t add_i64(int64_t a, int64_t b) { return a + b; }

// --- (Helpers are unchanged) ---
static void* xmalloc(size_t bytes) {
    void* p = malloc(bytes);
    if (!p) {
        fprintf(stderr, "Out of memory requesting %zu bytes\n", bytes);
        exit(EXIT_FAILURE);
    }
    return p;
}

static void usage(const char* exe) {
    fprintf(stderr,
        "Usage: %s <start> <end> [iters]\n"
        "  Computes out[i] = square(in[i]) for i in [start..end], then sum(out).\n"
        "  Benchmarks ASM vs C baselines.\n"
        "  Example: %s 1 10000000 5\n", exe, exe);
}

// -------------------- Main ---------------------------------
int main(int argc, char** argv) {
    if (argc < 3 || argc > 4) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    char* endp = NULL;
    int64_t start = strtoll(argv[1], &endp, 10);
    if (*endp) { usage(argv[0]); return EXIT_FAILURE; }
    int64_t end = strtoll(argv[2], &endp, 10);
    if (*endp) { usage(argv[0]); return EXIT_FAILURE; }
    int iters = (argc == 4) ? (int)strtol(argv[3], &endp, 10) : 5;
    if (argc == 4 && *endp) { usage(argv[0]); return EXIT_FAILURE; }
    if (iters < 1) iters = 1;

    if (end < start) {
        int64_t tmp = start; start = end; end = tmp;
    }

    const uint64_t n64 = (uint64_t)(end - start + 1);
    if (n64 > (uint64_t)SIZE_MAX) {
        fprintf(stderr, "Range too large for this build (n=%" PRIu64 ")\n", n64);
        return EXIT_FAILURE;
    }
    const size_t n = (size_t)n64;

    int64_t* in    = (int64_t*)xmalloc(n * sizeof(int64_t));
    int64_t* out_asm = (int64_t*)xmalloc(n * sizeof(int64_t));
    int64_t* out_c   = (int64_t*)xmalloc(n * sizeof(int64_t));

    for (size_t i = 0; i < n; ++i) in[i] = start + (int64_t)i;

    printf("Range: [%" PRId64 ", %" PRId64 "]  n=%zu   iters=%d\n", start, end, n, iters);
    
    // ---------------- Warm-ups -----------------------------
    (void)fp_map_square_i64(in, out_asm, n);
    (void)c_map_square_i64(in, out_c, n);
    (void)fp_reduce_add_i64(out_asm, n, 0);
    (void)c_reduce_add_i64(out_c, n, 0);
    (void)fp_foldmap_sumsq_i64(in, n, 0);
    (void)c_foldmap_sumsq_i64(in, n, 0);
    
    // ---------------- Correctness check -----------------------
    size_t mism = 0;
    c_map_square_i64(in, out_c, n);
    fp_map_square_i64(in, out_asm, n);
    for (size_t i = 0; i < n; ++i) if (out_asm[i] != out_c[i]) { mism = i + 1; break; }

    if (mism) {
        printf("MAP CHECK: MISMATCH at index %zu (asm=%" PRId64 ", c=%" PRId64 ")\n",
               mism - 1, out_asm[mism-1], out_c[mism-1]);
    } else {
        printf("MAP CHECK: OK\n");
    }

    int64_t asm_sum_fused = fp_foldmap_sumsq_i64(in, n, 0);
    int64_t c_sum_fused = c_foldmap_sumsq_i64(in, n, 0);
    printf("REDUCE CHECK: asm_fused=%" PRId64 "  c_fused=%" PRId64 "  %s\n",
           asm_sum_fused, c_sum_fused, (asm_sum_fused == c_sum_fused ? "OK" : "MISMATCH"));
    
    if (mism || (asm_sum_fused != c_sum_fused)) {
        fprintf(stderr, "Correctness check FAILED. Halting benchmark.\n");
        free(in); free(out_asm); free(out_c);
        return EXIT_FAILURE;
    }

    // ===================================================================
    // THE FIX: A volatile "sink" to prevent dead code elimination
    // ===================================================================
    volatile int64_t sink = 0;

    // ---------------- Bench: ASM map (specialized) ----------------------
    double asm_map_ms = 0.0;
    for (int k = 0; k < iters; ++k) {
        hi_timer_t t = timer_start();
        (void)fp_map_square_i64(in, out_asm, n);
        asm_map_ms += timer_ms_since(&t);
        sink += out_asm[0]; // Use the result
    }
    asm_map_ms /= (double)iters;

    // ---------------- Bench: C map (specialized) ------------------------
    double c_map_ms = 0.0;
    for (int k = 0; k < iters; ++k) {
        hi_timer_t t = timer_start();
        (void)c_map_square_i64(in, out_c, n);
        c_map_ms += timer_ms_since(&t);
        sink += out_c[0]; // Use the result
    }
    c_map_ms /= (double)iters;

    // ---------------- Bench: ASM reduce (specialized) -------------------
    double asm_red_ms = 0.0;
    int64_t asm_sum = 0;
    for (int k = 0; k < iters; ++k) {
        hi_timer_t t = timer_start();
        asm_sum = fp_reduce_add_i64(out_asm, n, 0);
        asm_red_ms += timer_ms_since(&t);
        sink += asm_sum; // Use the result
    }
    asm_red_ms /= (double)iters;

    // ---------------- Bench: C reduce (specialized) ---------------------
    double c_red_ms = 0.0;
    int64_t c_sum = 0;
    for (int k = 0; k < iters; ++k) {
        hi_timer_t t = timer_start();
        c_sum = c_reduce_add_i64(out_c, n, 0);
        c_red_ms += timer_ms_since(&t);
        sink += c_sum; // Use the result
    }
    c_red_ms /= (double)iters;

    // ---------------- Bench: ASM fused map+reduce -----------------------
    double asm_fused_ms = 0.0;
    for (int k = 0; k < iters; ++k) {
        hi_timer_t t = timer_start();
        asm_sum_fused = fp_foldmap_sumsq_i64(in, n, 0);
        asm_fused_ms += timer_ms_since(&t);
        sink += asm_sum_fused; // Use the result
    }
    asm_fused_ms /= (double)iters;

    // ---------------- Bench: C fused map+reduce -------------------------
    double c_fused_ms = 0.0;
    for (int k = 0; k < iters; ++k) {
        hi_timer_t t = timer_start();
        c_sum_fused = c_foldmap_sumsq_i64(in, n, 0);
        c_fused_ms += timer_ms_since(&t);
        sink += c_sum_fused; // Use the result
    }
    c_fused_ms /= (double)iters;


    // ---------------- Results ----------------------------------------------
    const double asm_map_mels = (n / 1e6) / (asm_map_ms / 1000.0);
    const double  c_map_mels  = (n / 1e6) / (c_map_ms  / 1000.0);
    const double asm_red_mels = (n / 1e6) / (asm_red_ms / 1000.0);
    const double  c_red_mels  = (n / 1e6) / (c_red_ms  / 1000.0);
    const double asm_fused_mels = (n / 1e6) / (asm_fused_ms / 1000.0);
    const double  c_fused_mels  = (n / 1e6) / (c_fused_ms / 1000.0);
    const double c_total_ms   = c_map_ms + c_red_ms;
    const double asm_total_ms = asm_map_ms + asm_red_ms;

    printf("\n== Map (out[i]=square(in[i])) == [Specialized]\n");
    printf("ASM : %8.3f ms   %8.3f M elems/s\n", asm_map_ms, asm_map_mels);
    printf("C   : %8.3f ms   %8.3f M elems/s  (Speedup: %.2fx)\n",  c_map_ms,  c_map_mels, c_map_ms / asm_map_ms);

    printf("\n== Reduce (sum of squares) == [Specialized]\n");
    printf("ASM : %8.3f ms   %8.3f M elems/s\n", asm_red_ms, asm_red_mels);
    printf("C   : %8.3f ms   %8.3f M elems/s  (Speedup: %.2fx)\n",  c_red_ms,  c_red_mels, c_red_ms / asm_red_ms);
    
    printf("\n== Fused (sum(in[i]*in[i])) == [Fused]\n");
    printf("ASM : %8.3f ms   %8.3f M elems/s\n", asm_fused_ms, asm_fused_mels);
    printf("C   : %8.3f ms   %8.3f M elems/s  (Speedup: %.2fx)\n",  c_fused_ms,  c_fused_mels, c_fused_ms / asm_fused_ms);
    
    printf("\n== Totals (Map + Reduce) ==\n");
    printf("ASM (Separate) : %8.3f ms\n", asm_total_ms);
    printf("C   (Separate) : %8.3f ms\n", c_total_ms);
    printf("ASM (Fused)    : %8.3f ms (Speedup vs C-Separate: %.2fx)\n", asm_fused_ms, c_total_ms / asm_fused_ms);
    printf("C   (Fused)    : %8.3f ms (Speedup vs C-Separate: %.2fx)\n", c_fused_ms, c_total_ms / c_fused_ms);


    // Small warning about overflow if range is very large
    if ((end - start + 1) > 0 && end > 3037000499LL) {
        puts("\n[Note] Potential int64 overflow in squares for very large inputs.");
    }
    
    // Final "use" of the sink just to be 100% sure
    if (sink == 0x123456789ABCDEFLL) {
        printf("Unexpected sink value!\n");
    }

    free(in);
    free(out_asm);
    free(out_c);
    return EXIT_SUCCESS;
}