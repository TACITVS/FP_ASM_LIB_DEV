/* Thread-safety test: the library keeps no mutable global state apart from
 * two run-once caches (CPU detection, dispatch table), so threads calling
 * kernels on their own buffers must get exactly the single-threaded results.
 *
 *   - 16 threads start together through a barrier, so their FIRST calls race
 *     the lazy CPU detection and dispatch resolution;
 *   - each thread runs dispatched kernels (reductions, dot products), the
 *     batch transform, convention-aware matrices and fp_stream_copy on its
 *     own buffers, many times, checking every repetition is identical;
 *   - afterwards the main thread recomputes everything single-threaded and
 *     requires bit-for-bit equality, and checks all threads saw the same
 *     CPU info and dispatch choice.
 * `make test-tsan` runs this under ThreadSanitizer as well. */
#include "fp_core.h"
#include "fp_cpu.h"
#include "fp_dispatch.h"
#include "fp_gfx.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#  include <windows.h>
typedef HANDLE thread_t;
#else
#  include <pthread.h>
typedef pthread_t thread_t;
#endif

#define NTHREADS 16
#define REPS     60
#define N        3001     /* odd: exercises every tail path */
#define NV       257

typedef struct {
    float    sum_f32, dot_f32, sq_f32;
    double   dot_f64;
    int8_t   dot_i8;
    int16_t  dot_i16;
    Vec3f    transformed[NV];
    Mat4     proj, view;
    unsigned char copied[1024];
    const fp_cpu_info_t* cpu;
    const char* tier_dotp_i8;
} result_t;

typedef struct {
    int      id;
    float    a[N], b[N];
    double   da[N], db[N];
    int8_t   ia[N], ib[N];
    int16_t  sa[N], sb[N];
    Vec3f    verts[NV];
    unsigned char src[1024];
    result_t res;
    int      mismatches;     /* repetitions that differed from the first */
} work_t;

static work_t g_work[NTHREADS];
static long   g_arrived;      /* start barrier */

static void fill(work_t* w) {
    uint32_t x = 2463534242u + (uint32_t)w->id * 7919u;
    int i;
#define NEXT() (x ^= x << 13, x ^= x >> 17, x ^= x << 5, x)
    for (i = 0; i < N; i++) {
        w->a[i] = (float)((int)(NEXT() % 2001) - 1000) * 1e-3f;
        w->b[i] = (float)((int)(NEXT() % 2001) - 1000) * 1e-3f;
        w->da[i] = (double)w->a[i] * 1.5;
        w->db[i] = (double)w->b[i] * 0.5;
        w->ia[i] = (int8_t)NEXT();
        w->ib[i] = (int8_t)NEXT();
        w->sa[i] = (int16_t)NEXT();
        w->sb[i] = (int16_t)NEXT();
    }
    for (i = 0; i < NV; i++) {
        w->verts[i].x = (float)(NEXT() % 100) * 0.1f;
        w->verts[i].y = (float)(NEXT() % 100) * 0.1f;
        w->verts[i].z = -(float)(NEXT() % 100) * 0.1f - 1.0f;
        w->verts[i]._pad = 1.0f;
    }
    for (i = 0; i < 1024; i++) w->src[i] = (unsigned char)NEXT();
#undef NEXT
}

/* Everything one thread computes; pure function of the thread's inputs. */
static void compute(const work_t* w, result_t* r) {
    fp_gfx_conventions c;
    Mat4 vp;
    r->sum_f32 = fp_reduce_add_f32(w->a, N);
    r->dot_f32 = fp_fold_dotp_f32(w->a, w->b, N);
    r->sq_f32  = fp_fold_sumsq_f32(w->a, N);
    r->dot_f64 = fp_fold_dotp_f64(w->da, w->db, N);
    r->dot_i8  = fp_fold_dotp_i8(w->ia, w->ib, N);
    r->dot_i16 = fp_fold_dotp_i16(w->sa, w->sb, N);
    fp_gfx_conventions_init(&c, w->id % 2 ? FP_GFX_D3D11 : FP_GFX_VULKAN, w->id % 3 == 0);
    fp_mat4_perspective_gfx(&r->proj, 1.0f + 0.01f * (float)w->id, 1.5f, 0.1f, 100.0f, &c);
    fp_mat4_lookat_gfx(&r->view, (float)w->id, 2, 5, 0, 0, 0, 0, 1, 0, &c);
    fp_mat4_mul(&vp, &r->proj, &r->view);
    fp_mat4_mul_vec3_batch(r->transformed, &vp, w->verts, NV);
    fp_stream_copy(r->copied, w->src + (w->id % 7), sizeof r->copied - 8);
    r->cpu = fp_cpu_info();
    r->tier_dotp_i8 = fp_dispatch_selected("fp_fold_dotp_i8");
}

static int same(const result_t* x, const result_t* y) {
    /* bitwise for floats: same inputs, same kernel -> same bits */
    return memcmp(&x->sum_f32, &y->sum_f32, sizeof x->sum_f32) == 0 &&
           memcmp(&x->dot_f32, &y->dot_f32, sizeof x->dot_f32) == 0 &&
           memcmp(&x->sq_f32, &y->sq_f32, sizeof x->sq_f32) == 0 &&
           memcmp(&x->dot_f64, &y->dot_f64, sizeof x->dot_f64) == 0 &&
           x->dot_i8 == y->dot_i8 && x->dot_i16 == y->dot_i16 &&
           memcmp(x->transformed, y->transformed, sizeof x->transformed) == 0 &&
           memcmp(&x->proj, &y->proj, sizeof x->proj) == 0 &&
           memcmp(&x->view, &y->view, sizeof x->view) == 0 &&
           memcmp(x->copied, y->copied, sizeof x->copied - 8) == 0 &&
           x->cpu == y->cpu && strcmp(x->tier_dotp_i8, y->tier_dotp_i8) == 0;
}

static void barrier_wait(void) {
    __atomic_add_fetch(&g_arrived, 1, __ATOMIC_ACQ_REL);
    while (__atomic_load_n(&g_arrived, __ATOMIC_ACQUIRE) < NTHREADS) { /* spin */ }
}

#ifdef _WIN32
static DWORD WINAPI worker(LPVOID p)
#else
static void* worker(void* p)
#endif
{
    work_t* w = (work_t*)p;
    result_t r;
    int k;
    barrier_wait();                     /* everyone's first call races the lazy init */
    compute(w, &w->res);
    for (k = 1; k < REPS; k++) {
        memset(&r, 0, sizeof r);
        compute(w, &r);
        if (!same(&r, &w->res)) w->mismatches++;
    }
    return 0;
}

int main(void) {
    thread_t th[NTHREADS];
    int t, failures = 0, checks = 0;

    for (t = 0; t < NTHREADS; t++) { g_work[t].id = t; fill(&g_work[t]); }

    /* No library call before this point: the threads trigger the lazy init. */
    for (t = 0; t < NTHREADS; t++) {
#ifdef _WIN32
        th[t] = CreateThread(NULL, 0, worker, &g_work[t], 0, NULL);
        if (!th[t]) { puts("FAIL CreateThread"); return 2; }
#else
        if (pthread_create(&th[t], NULL, worker, &g_work[t]) != 0) { puts("FAIL pthread_create"); return 2; }
#endif
    }
    for (t = 0; t < NTHREADS; t++) {
#ifdef _WIN32
        WaitForSingleObject(th[t], INFINITE);
        CloseHandle(th[t]);
#else
        pthread_join(th[t], NULL);
#endif
    }

    for (t = 0; t < NTHREADS; t++) {
        result_t ref;
        compute(&g_work[t], &ref);      /* single-threaded, after the fact */
        checks += 3;
        if (g_work[t].mismatches) {
            printf("FAIL thread %2d: %d of %d repetitions differed\n", t, g_work[t].mismatches, REPS - 1);
            failures++;
        }
        if (!same(&g_work[t].res, &ref)) {
            printf("FAIL thread %2d: result differs from the single-threaded run\n", t);
            failures++;
        }
        if (g_work[t].res.cpu != g_work[0].res.cpu || strcmp(g_work[t].res.tier_dotp_i8, g_work[0].res.tier_dotp_i8) != 0) {
            printf("FAIL thread %2d: saw different CPU info / dispatch choice\n", t);
            failures++;
        }
    }
    printf("%d threads x %d repetitions; dispatch tier for dotp_i8: %s; CPU: %s\n",
           NTHREADS, REPS, g_work[0].res.tier_dotp_i8, g_work[0].res.cpu->uarch);
    printf("\n%s (%d checks, %d failures)\n", failures ? "SOME FAILED" : "ALL PASS", checks, failures);
    return failures != 0;
}
