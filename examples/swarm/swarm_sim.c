/* swarm_sim.c — see swarm_sim.h. */
#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#  define _POSIX_C_SOURCE 200112L   /* clock_gettime, posix_memalign */
#endif
#include "swarm_sim.h"
#include "fp_core.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#else
#  include <time.h>
#endif

const char* const swarm_stage_names[SWARM_STAGES] = { "motion", "turbulence", "analysis", "projection" };
const char* const swarm_mode_names[2] = { "FP-ASM", "plain C" };

#define PI_F 3.14159265f
#define CHUNK_ALIGN 64            /* elements */

double swarm_now_ms(void) {
#ifdef _WIN32
    LARGE_INTEGER c, f;
    QueryPerformanceCounter(&c);
    QueryPerformanceFrequency(&f);
    return (double)c.QuadPart * 1000.0 / (double)f.QuadPart;
#else
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1e3 + t.tv_nsec * 1e-6;
#endif
}

/* ------------------------------------------------------------------ RNG */
typedef struct { uint64_t s; } rng_t;
static uint64_t rng_u64(rng_t* r) {     /* xorshift64* */
    r->s ^= r->s >> 12; r->s ^= r->s << 25; r->s ^= r->s >> 27;
    return r->s * 2685821657736338717ull;
}
static float rng_unit(rng_t* r) { return (float)(rng_u64(r) >> 40) * (1.0f / 16777216.0f); }
static float rng_gauss(rng_t* r) {
    float u = rng_unit(r) + 1e-7f, v = rng_unit(r);
    return sqrtf(-2.0f * logf(u)) * cosf(2.0f * PI_F * v);
}

/* ---------------------------------------------------------- generation */
#define GALAXY_R 10.0f

static void* aalloc(size_t bytes) {
#ifdef _WIN32
    return _aligned_malloc(bytes, 64);
#else
    void* p = NULL;
    return posix_memalign(&p, 64, bytes) == 0 ? p : NULL;
#endif
}
static void afree(void* p) {
#ifdef _WIN32
    _aligned_free(p);
#else
    free(p);
#endif
}

int swarm_galaxy_create(swarm_galaxy* g, size_t n, uint32_t seed) {
    rng_t rng = { 0x9E3779B97F4A7C15ull ^ seed };
    Vec3f* pos = malloc(n * sizeof *pos);
    Vec3f* jit = malloc(n * sizeof *jit);
    float* col = malloc(n * 4 * sizeof *col);
    int*   band = malloc(n * sizeof *band);
    size_t i, *fill;
    int b;

    memset(g, 0, sizeof *g);
    g->n = n;
    g->bands = 96;
    g->band_start = calloc((size_t)g->bands + 1, sizeof *g->band_start);
    g->band_omega = malloc((size_t)g->bands * sizeof *g->band_omega);
    g->base   = aalloc(n * sizeof(Vec3f));
    g->jitter = aalloc(n * sizeof(Vec3f));
    g->world  = aalloc(n * sizeof(Vec3f));
    g->prev   = aalloc(n * sizeof(Vec3f));
    g->delta  = aalloc(n * sizeof(Vec3f));
    g->color  = aalloc(n * 4 * sizeof(float));
    g->chunks = (int)((n + 8191) / 8192);
    if (g->chunks < 1) g->chunks = 1;
    if (g->chunks > 512) g->chunks = 512;
    g->part = malloc((size_t)g->chunks * 5 * sizeof *g->part);
    if (!pos || !jit || !col || !band || !g->base || !g->jitter || !g->world || !g->prev || !g->delta ||
        !g->color || !g->part || !g->band_start || !g->band_omega) return -1;

    for (i = 0; i < n; i++) {
        float r, th, y, bright, size, cr, cg, cb, pick = rng_unit(&rng);
        if (pick < 0.16f) {                                  /* bulge: warm, dense, bright */
            r = fabsf(rng_gauss(&rng)) * 1.1f;
            th = rng_unit(&rng) * 2.0f * PI_F;
            y = rng_gauss(&rng) * 0.55f * expf(-r * 0.4f);
            cr = 1.0f; cg = 0.82f + 0.1f * rng_unit(&rng); cb = 0.55f + 0.15f * rng_unit(&rng);
            bright = 0.35f + 0.65f * rng_unit(&rng);
            size = 0.035f + 0.04f * rng_unit(&rng);
        } else {                                             /* disk with 4 logarithmic arms */
            int arm = (int)(rng_unit(&rng) * 4.0f) & 3;
            float jitter_arm;
            r = 0.6f + -logf(rng_unit(&rng) + 1e-6f) * 2.6f;
            if (r > GALAXY_R) r = GALAXY_R * rng_unit(&rng) + 0.6f;
            jitter_arm = rng_gauss(&rng) * (0.18f + 0.05f * r);
            th = (float)arm * (PI_F * 0.5f) + logf(r) * 2.3f + jitter_arm;
            y = rng_gauss(&rng) * 0.12f * expf(-r * 0.12f);
            pick = rng_unit(&rng);
            if (pick < 0.35f)      { cr = 0.55f; cg = 0.7f;  cb = 1.0f; }              /* young, blue */
            else if (pick < 0.75f) { cr = 1.0f;  cg = 0.95f; cb = 0.88f; }             /* white */
            else if (pick < 0.92f) { cr = 1.0f;  cg = 0.75f; cb = 0.5f; }              /* old, orange */
            else                   { cr = 0.9f;  cg = 0.35f; cb = 0.45f; }             /* nebula pink */
            bright = 0.15f + 0.85f * rng_unit(&rng) * expf(-fabsf(jitter_arm) * 2.0f);
            size = 0.02f + 0.05f * rng_unit(&rng);
            if (rng_unit(&rng) < 0.004f) { size *= 4.0f; bright = 1.0f; }             /* rare giants */
        }
        pos[i].x = r * cosf(th); pos[i].z = r * sinf(th); pos[i].y = y; pos[i]._pad = 0.0f;
        jit[i].x = rng_gauss(&rng) * 0.05f; jit[i].y = rng_gauss(&rng) * 0.05f;
        jit[i].z = rng_gauss(&rng) * 0.05f; jit[i]._pad = 0.0f;
        col[4 * i + 0] = cr * bright; col[4 * i + 1] = cg * bright;
        col[4 * i + 2] = cb * bright; col[4 * i + 3] = size;
        b = (int)(r / (GALAXY_R + 1.0f) * (float)g->bands);
        band[i] = b < 0 ? 0 : b >= g->bands ? g->bands - 1 : b;
    }

    /* sort stars by band (counting sort) so each band is one contiguous batch */
    for (i = 0; i < n; i++) g->band_start[band[i] + 1]++;
    for (b = 0; b < g->bands; b++) g->band_start[b + 1] += g->band_start[b];
    fill = malloc((size_t)g->bands * sizeof *fill);
    for (b = 0; b < g->bands; b++) fill[b] = g->band_start[b];
    for (i = 0; i < n; i++) {
        size_t d = fill[band[i]]++;
        g->base[d] = pos[i];
        g->jitter[d] = jit[i];
        memcpy(g->color + 4 * d, col + 4 * i, 4 * sizeof(float));
    }
    /* angular speed: solid-body core, flattening outwards (inner bands faster) */
    for (b = 0; b < g->bands; b++) {
        float r = ((float)b + 0.5f) / (float)g->bands * (GALAXY_R + 1.0f);
        g->band_omega[b] = 0.06f + 0.42f / (1.0f + r * 0.45f);
    }
    memcpy(g->world, g->base, n * sizeof(Vec3f));
    memcpy(g->prev, g->base, n * sizeof(Vec3f));
    free(fill); free(pos); free(jit); free(col); free(band);
    return 0;
}

void swarm_galaxy_destroy(swarm_galaxy* g) {
    afree(g->base); afree(g->jitter); afree(g->world); afree(g->prev); afree(g->delta); afree(g->color);
    free(g->band_start); free(g->band_omega); free(g->part);
    memset(g, 0, sizeof *g);
}

/* ------------------------------------------------- plain C reference */
static void c_rotate(const Vec3f* in, Vec3f* out, size_t n, const Quaternion* q) {
    size_t i;
    for (i = 0; i < n; i++) {
        float tx = 2.0f * (q->y * in[i].z - q->z * in[i].y);
        float ty = 2.0f * (q->z * in[i].x - q->x * in[i].z);
        float tz = 2.0f * (q->x * in[i].y - q->y * in[i].x);
        out[i].x = in[i].x + q->w * tx + (q->y * tz - q->z * ty);
        out[i].y = in[i].y + q->w * ty + (q->z * tx - q->x * tz);
        out[i].z = in[i].z + q->w * tz + (q->x * ty - q->y * tx);
        out[i]._pad = 0.0f;
    }
}
static void c_axpy(const float* x, const float* y, float* o, size_t n, float a) {
    size_t i;
    for (i = 0; i < n; i++) o[i] = a * x[i] + y[i];
}
static void c_vsum(const Vec3f* v, size_t n, Vec3f* out) {
    float x = 0, y = 0, z = 0;
    size_t i;
    for (i = 0; i < n; i++) { x += v[i].x; y += v[i].y; z += v[i].z; }
    out->x = x; out->y = y; out->z = z; out->_pad = 0;
}
static float c_sumsq(const float* a, size_t n) {
    float s = 0;
    size_t i;
    for (i = 0; i < n; i++) s += a[i] * a[i];
    return s;
}
/* sum of (a - b)^2: the single fused loop a C programmer would write */
static float c_sqdiff(const float* a, const float* b, size_t n) {
    float s = 0;
    size_t i;
    for (i = 0; i < n; i++) { float d = a[i] - b[i]; s += d * d; }
    return s;
}
static void c_transform(Vec3f* out, const Mat4* m, const Vec3f* in, size_t n) {
    const float* M = m->m;
    size_t i;
    for (i = 0; i < n; i++) {
        float x = in[i].x, y = in[i].y, z = in[i].z;
        out[i].x    = M[0] * x + M[4] * y + M[8]  * z + M[12];
        out[i].y    = M[1] * x + M[5] * y + M[9]  * z + M[13];
        out[i].z    = M[2] * x + M[6] * y + M[10] * z + M[14];
        out[i]._pad = M[3] * x + M[7] * y + M[11] * z + M[15];
    }
}

/* ------------------------------------------------------------- frame */
typedef struct {
    swarm_galaxy* g;
    swarm_mode    mode;
    double        t;
    float         turb;
    const Mat4*   vp;
    Vec3f*        clip;
} frame_ctx;

static void chunk_range(const swarm_galaxy* g, int c, size_t* a, size_t* b) {
    size_t per = (g->n / (size_t)g->chunks + CHUNK_ALIGN - 1) / CHUNK_ALIGN * CHUNK_ALIGN;
    *a = (size_t)c * per;
    *b = *a + per;
    if (*a > g->n) *a = g->n;
    if (*b > g->n || c == g->chunks - 1) *b = g->n;
}

static void job_motion(void* p, int c) {
    const frame_ctx* f = p;
    const swarm_galaxy* g = f->g;
    size_t a, e;
    int b = 0;
    chunk_range(g, c, &a, &e);
    while (b < g->bands && g->band_start[b + 1] <= a) b++;
    for (; b < g->bands && g->band_start[b] < e; b++) {
        size_t s = g->band_start[b] > a ? g->band_start[b] : a;
        size_t t = g->band_start[b + 1] < e ? g->band_start[b + 1] : e;
        float ang = (float)(g->band_omega[b] * f->t) * 0.5f;
        Quaternion q = { 0.0f, sinf(ang), 0.0f, cosf(ang) };      /* about +y */
        if (t <= s) continue;
        if (f->mode == SWARM_FPASM) fp_map_quat_rotate_vec3_f32(g->base + s, g->world + s, t - s, &q);
        else                        c_rotate(g->base + s, g->world + s, t - s, &q);
    }
}

static void job_turbulence(void* p, int c) {
    const frame_ctx* f = p;
    size_t a, e;
    chunk_range(f->g, c, &a, &e);
    if (e <= a) return;
    if (f->mode == SWARM_FPASM)
        fp_map_axpy_f32((const float*)(f->g->jitter + a), (const float*)(f->g->world + a),
                        (float*)(f->g->world + a), 4 * (e - a), f->turb);
    else
        c_axpy((const float*)(f->g->jitter + a), (const float*)(f->g->world + a),
               (float*)(f->g->world + a), 4 * (e - a), f->turb);
}

static void job_analysis(void* p, int c) {
    const frame_ctx* f = p;
    const swarm_galaxy* g = f->g;
    float* out = g->part + 5 * c;
    Vec3f s = { 0, 0, 0, 0 };
    size_t a, e;
    chunk_range(g, c, &a, &e);
    if (e <= a) { memset(out, 0, 5 * sizeof *out); return; }
    if (f->mode == SWARM_FPASM) {
        fp_reduce_vec3_add_f32(g->world + a, e - a, &s);
        out[3] = fp_fold_sumsq_f32((const float*)(g->world + a), 4 * (e - a));
        /* motion energy: sum |world - prev|^2, as delta = -1*prev + world, then sumsq */
        fp_map_axpy_f32((const float*)(g->prev + a), (const float*)(g->world + a),
                        (float*)(g->delta + a), 4 * (e - a), -1.0f);
        out[4] = fp_fold_sumsq_f32((const float*)(g->delta + a), 4 * (e - a));
    } else {
        c_vsum(g->world + a, e - a, &s);
        out[3] = c_sumsq((const float*)(g->world + a), 4 * (e - a));
        out[4] = c_sqdiff((const float*)(g->world + a), (const float*)(g->prev + a), 4 * (e - a));
    }
    out[0] = s.x; out[1] = s.y; out[2] = s.z;
}

static void job_projection(void* p, int c) {
    const frame_ctx* f = p;
    size_t a, e;
    chunk_range(f->g, c, &a, &e);
    if (e <= a) return;
    if (f->mode == SWARM_FPASM) fp_mat4_mul_vec3_batch(f->clip + a, f->vp, f->g->world + a, (int)(e - a));
    else                        c_transform(f->clip + a, f->vp, f->g->world + a, e - a);
}

void swarm_frame(swarm_galaxy* g, swarm_pool* pool, swarm_mode mode, double t,
                 const Mat4* view_proj, Vec3f* clip_out,
                 swarm_stats* stats, swarm_timing* timing) {
    static const swarm_job_fn jobs[SWARM_STAGES] = { job_motion, job_turbulence, job_analysis, job_projection };
    frame_ctx f;
    Vec3f* tmp;
    int s, c;

    tmp = g->prev; g->prev = g->world; g->world = tmp;            /* last frame becomes prev */
    f.g = g; f.mode = mode; f.t = t; f.vp = view_proj; f.clip = clip_out;
    f.turb = 0.6f * sinf((float)t * 0.9f) + 0.3f * sinf((float)t * 2.3f);

    for (s = 0; s < SWARM_STAGES; s++) {
        double t0 = swarm_now_ms();
        if (s == SWARM_STAGE_PROJECTION && (!view_proj || !clip_out)) { if (timing) timing->ms[s] = 0; continue; }
        swarm_pool_run(pool, jobs[s], &f, g->chunks);
        if (timing) timing->ms[s] = swarm_now_ms() - t0;
    }

    {   /* combine partials in chunk order: same result for any thread count */
        double sx = 0, sy = 0, sz = 0, sq = 0, en = 0, inv = 1.0 / (double)g->n;
        for (c = 0; c < g->chunks; c++) {
            const float* q = g->part + 5 * c;
            sx += q[0]; sy += q[1]; sz += q[2]; sq += q[3]; en += q[4];
        }
        if (stats) {
            double cx = sx * inv, cy = sy * inv, cz = sz * inv, r2 = sq * inv - (cx * cx + cy * cy + cz * cz);
            stats->com.x = (float)cx; stats->com.y = (float)cy; stats->com.z = (float)cz; stats->com._pad = 0;
            stats->spread = (float)sqrt(r2 > 0 ? r2 : 0);
            stats->energy = g->frames ? (float)(en * inv) : 0.0f;
        }
        g->frames++;
    }
}
