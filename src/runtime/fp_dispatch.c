/*
 * fp_dispatch.c — runtime kernel selection (see include/fp_dispatch.h).
 *
 * Each dispatched kernel has a function-pointer slot. With FP_DISPATCH the
 * public symbol (e.g. fp_fold_dotp_f32) is defined here and jumps through
 * its slot; the slot starts at a stub that resolves every slot on first
 * call. Without FP_DISPATCH the assembly exports the public names directly
 * (AVX2) and this file only provides the reporting API.
 *
 * Compiled for the plain x86-64 baseline, like fp_cpu.c.
 */
#include "fp_dispatch.h"
#include "fp_cpu.h"
#include "fp_core.h"   /* public prototypes: the definitions below must match */

#include <stdlib.h>
#include <string.h>

typedef void (*fp_fn)(void);

#if defined(__GNUC__) || defined(__clang__)
#  define SLOT_LOAD(p)     __atomic_load_n(&(p), __ATOMIC_ACQUIRE)
#  define SLOT_STORE(p, v) __atomic_store_n(&(p), (v), __ATOMIC_RELEASE)
#else
#  define SLOT_LOAD(p)     (p)
#  define SLOT_STORE(p, v) ((p) = (v))
#endif

#define B(f) FP_CPU_BIT(FP_CPU_##f)
#define REQ_AVX2    FP_CPU_ASM_BASELINE
#define REQ_AVXVNNI (REQ_AVX2 | B(AVX_VNNI))
#define REQ_AVX512  (REQ_AVX2 | B(BMI2) | B(AVX512F) | B(AVX512BW) | B(AVX512DQ) | B(AVX512VL))
#define REQ_AVX512_VNNI (REQ_AVX512 | B(AVX512_VNNI))

/*  X(name, return type, params, args, use AVX-VNNI variant, AVX-512 requirement)
 *  i16/u16: the AVX-VNNI variants exist (fp_fold_dotp_i16_avxvnni, ...) but
 *  are not selected: on Raptor Lake (i7-13700HX) they measured 0.94-0.97x
 *  of AVX2, which already does one vpmaddwd-free multiply-add per element
 *  and is memory-bound. i8/u8 gain 14-16x there. */
#define FP_DISPATCH_KERNELS(X) \
    X(fp_reduce_add_f32, float,    (const float* a, size_t n),                     (a, n),    0, REQ_AVX512) \
    X(fp_reduce_add_f64, double,   (const double* a, size_t n),                    (a, n),    0, REQ_AVX512) \
    X(fp_fold_sumsq_f32, float,    (const float* a, size_t n),                     (a, n),    0, REQ_AVX512) \
    X(fp_fold_dotp_f32,  float,    (const float* a, const float* b, size_t n),     (a, b, n), 0, REQ_AVX512) \
    X(fp_fold_dotp_f64,  double,   (const double* a, const double* b, size_t n),   (a, b, n), 0, REQ_AVX512) \
    X(fp_fold_dotp_i8,   int8_t,   (const int8_t* a, const int8_t* b, size_t n),   (a, b, n), 1, REQ_AVX512_VNNI) \
    X(fp_fold_dotp_u8,   uint8_t,  (const uint8_t* a, const uint8_t* b, size_t n), (a, b, n), 1, REQ_AVX512_VNNI) \
    X(fp_fold_dotp_i16,  int16_t,  (const int16_t* a, const int16_t* b, size_t n), (a, b, n), 0, REQ_AVX512_VNNI) \
    X(fp_fold_dotp_u16,  uint16_t, (const uint16_t* a, const uint16_t* b, size_t n), (a, b, n), 0, REQ_AVX512_VNNI)

#define VNNI_IMPL_0(name) NULL
#define VNNI_IMPL_1(name) (fp_fn)name##_avxvnni

/* ---- slots, stubs and public entry points ---- */
#ifdef FP_DISPATCH
#  define DEFINE_KERNEL(name, ret, params, args, vnni, req512)                  \
    static ret stub_##name params;                                               \
    static fp_fn slot_##name = (fp_fn)stub_##name;                               \
    ret name params { return ((ret(*) params)SLOT_LOAD(slot_##name)) args; }     \
    static ret stub_##name params {                                              \
        fp_dispatch_init();                                                      \
        return ((ret(*) params)SLOT_LOAD(slot_##name)) args;                     \
    }
#else
#  define DEFINE_KERNEL(name, ret, params, args, vnni, req512)                  \
    static fp_fn slot_##name = (fp_fn)name##_avx2;
#endif
FP_DISPATCH_KERNELS(DEFINE_KERNEL)

/* ---- table ---- */
typedef struct {
    const char* name;
    fp_fn*      slot;
    fp_fn       impl[FP_TIER_COUNT];
    uint64_t    req[FP_TIER_COUNT];
    fp_tier     chosen;
} kentry;

#define TABLE_ENTRY(name, ret, params, args, vnni, req512)                      \
    { #name, &slot_##name,                                                       \
      { (fp_fn)name##_avx2, VNNI_IMPL_##vnni(name), (fp_fn)name##_avx512 },      \
      { REQ_AVX2, REQ_AVXVNNI, (req512) }, FP_TIER_AVX2 },
static kentry g_table[] = { FP_DISPATCH_KERNELS(TABLE_ENTRY) };
#define N_KERNELS (sizeof g_table / sizeof g_table[0])

static fp_tier g_cap = FP_TIER_AUTO;
static int     g_init;

static const char* const tier_names[FP_TIER_COUNT] = { "avx2", "avxvnni", "avx512" };

const char* fp_tier_name(fp_tier t) {
    return (t >= 0 && t < FP_TIER_COUNT) ? tier_names[t] : "auto";
}

fp_tier fp_tier_from_name(const char* s) {
    int t;
    if (!s) return FP_TIER_AUTO;
    for (t = 0; t < FP_TIER_COUNT; t++)
        if (strcmp(s, tier_names[t]) == 0) return (fp_tier)t;
    if (strcmp(s, "avx-vnni") == 0 || strcmp(s, "vnni") == 0) return FP_TIER_AVXVNNI;
    if (strcmp(s, "avx-512") == 0) return FP_TIER_AVX512;
    return FP_TIER_AUTO;
}

int fp_dispatch_enabled(void) {
#ifdef FP_DISPATCH
    return 1;
#else
    return 0;
#endif
}

static int has_all(uint64_t req) {
    return (fp_cpu_info()->features & req) == req;
}

int fp_dispatch_tier_available(fp_tier t) {
    switch (t) {
    case FP_TIER_AVX2:    return has_all(REQ_AVX2);
    case FP_TIER_AVXVNNI: return has_all(REQ_AVXVNNI);
    case FP_TIER_AVX512:  return has_all(REQ_AVX512);
    default:              return 0;
    }
}

fp_tier fp_dispatch_best_tier(void) {
    int t = (g_cap == FP_TIER_AUTO) ? FP_TIER_COUNT - 1 : g_cap;
    for (; t > FP_TIER_AVX2; t--)
        if (fp_dispatch_tier_available((fp_tier)t)) return (fp_tier)t;
    return FP_TIER_AVX2;
}

static void resolve_all(void) {
    size_t i;
    int top = (g_cap == FP_TIER_AUTO) ? FP_TIER_COUNT - 1 : g_cap;
    for (i = 0; i < N_KERNELS; i++) {
        kentry* k = &g_table[i];
        int t = FP_TIER_AVX2;
#ifdef FP_DISPATCH
        for (t = top; t > FP_TIER_AVX2; t--)
            if (k->impl[t] && has_all(k->req[t])) break;
        SLOT_STORE(*k->slot, k->impl[t]);
#else
        (void)top;
#endif
        k->chosen = (fp_tier)t;
    }
}

void fp_dispatch_init(void) {
    const char* v;
    if (g_init) return;
    if (g_cap == FP_TIER_AUTO)
        g_cap = fp_tier_from_name(getenv("FPASM_TIER"));
    resolve_all();
    g_init = 1;
    v = getenv("FPASM_VERBOSE");
    if (v && *v && strcmp(v, "0") != 0) fp_dispatch_report(stderr);
}

fp_tier fp_dispatch_set_max_tier(fp_tier t) {
    fp_tier prev;
    fp_dispatch_init();
    prev = g_cap;
    g_cap = (t >= 0 && t < FP_TIER_COUNT) ? t : FP_TIER_AUTO;
    resolve_all();
    return prev;
}

const char* fp_dispatch_selected(const char* kernel) {
    size_t i;
    fp_dispatch_init();
    for (i = 0; i < N_KERNELS; i++)
        if (strcmp(g_table[i].name, kernel) == 0) return fp_tier_name(g_table[i].chosen);
    return NULL;
}

void fp_dispatch_report(FILE* out) {
    size_t i;
    int t;
    fp_dispatch_init();
    fprintf(out, "Kernel dispatch: %s", fp_dispatch_enabled() ? "enabled" : "DISABLED at build time");
    fprintf(out, "   tier cap: %s", fp_tier_name(g_cap));
    if (getenv("FPASM_TIER")) fprintf(out, " (FPASM_TIER=%s)", getenv("FPASM_TIER"));
    fprintf(out, "\n  tiers on this CPU:");
    for (t = 0; t < FP_TIER_COUNT; t++)
        fprintf(out, " %s=%s", tier_names[t], fp_dispatch_tier_available((fp_tier)t) ? "yes" : "no");
    fprintf(out, "\n");
    for (i = 0; i < N_KERNELS; i++) {
        const kentry* k = &g_table[i];
        fprintf(out, "  %-20s -> %-8s  (variants:", k->name, tier_names[k->chosen]);
        for (t = 0; t < FP_TIER_COUNT; t++)
            if (k->impl[t]) fprintf(out, " %s", tier_names[t]);
        fprintf(out, ")\n");
    }
}
