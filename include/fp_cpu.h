/*
 * fp_cpu.h — CPU feature detection and build/CPU compatibility diagnostics.
 *
 * The library can be built for several ISA targets (see README "Build
 * targets"): a portable AVX2 build that runs on every Haswell-or-newer CPU,
 * a build tuned for Alder/Raptor Lake (AVX2 + AVX-VNNI), an AVX-512 build,
 * or -march=native. These functions tell you what the running CPU (and OS)
 * actually supports, what this build of the library requires, and whether
 * the two match — so a mismatch produces a readable message instead of a
 * SIGILL ("Illegal instruction").
 *
 * Everything here is compiled for the plain x86-64 baseline, so it is safe
 * to call on any x86-64 CPU, whatever ISA the rest of the library targets.
 */
#ifndef FP_CPU_H
#define FP_CPU_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Feature indices; test with fp_cpu_has() or FP_CPU_BIT() against a mask.
 * AVX/AVX-512/AMX features are reported only when the OS has also enabled
 * the corresponding register state (XCR0), i.e. when they are usable. */
typedef enum {
    FP_CPU_SSE2 = 0,
    FP_CPU_SSE3,
    FP_CPU_SSSE3,
    FP_CPU_SSE41,
    FP_CPU_SSE42,
    FP_CPU_POPCNT,
    FP_CPU_PCLMUL,
    FP_CPU_AVX,
    FP_CPU_F16C,
    FP_CPU_FMA,
    FP_CPU_AVX2,
    FP_CPU_BMI1,
    FP_CPU_BMI2,
    FP_CPU_LZCNT,
    FP_CPU_MOVBE,
    FP_CPU_ADX,
    FP_CPU_SHA,
    FP_CPU_GFNI,
    FP_CPU_VAES,
    FP_CPU_VPCLMULQDQ,
    FP_CPU_AVX_VNNI,
    FP_CPU_AVX_VNNI_INT8,
    FP_CPU_AVX_IFMA,
    FP_CPU_AVX512F,
    FP_CPU_AVX512DQ,
    FP_CPU_AVX512CD,
    FP_CPU_AVX512BW,
    FP_CPU_AVX512VL,
    FP_CPU_AVX512_VNNI,
    FP_CPU_AVX512_IFMA,
    FP_CPU_AVX512_VBMI,
    FP_CPU_AVX512_VBMI2,
    FP_CPU_AVX512_BITALG,
    FP_CPU_AVX512_VPOPCNTDQ,
    FP_CPU_AVX512_BF16,
    FP_CPU_AVX512_FP16,
    FP_CPU_AVX10,
    FP_CPU_AMX_TILE,
    FP_CPU_AMX_INT8,
    FP_CPU_AMX_BF16,
    FP_CPU_ERMS,
    FP_CPU_FSRM,
    FP_CPU_HYBRID,
    FP_CPU_FEATURE_COUNT
} fp_cpu_feature;

#define FP_CPU_BIT(f) (1ull << (f))

/* What the assembly kernels themselves need (the AVX2 baseline). */
#define FP_CPU_ASM_BASELINE \
    (FP_CPU_BIT(FP_CPU_AVX) | FP_CPU_BIT(FP_CPU_AVX2) | FP_CPU_BIT(FP_CPU_FMA))

typedef enum {
    FP_CORE_UNKNOWN = 0,  /* not a hybrid CPU, or not reported */
    FP_CORE_EFFICIENCY,   /* Intel E-core (Atom: Gracemont, Crestmont, ...) */
    FP_CORE_PERFORMANCE   /* Intel P-core (Golden Cove, Raptor Cove, ...) */
} fp_core_type;

typedef struct {
    char     vendor[13];        /* "GenuineIntel", "AuthenticAMD", ... */
    char     brand[49];         /* marketing name, e.g. "13th Gen Intel(R) Core(TM) i7-13700K" */
    unsigned family, model, stepping;   /* display family/model */
    const char* uarch;          /* best-effort microarchitecture name */
    uint64_t features;          /* FP_CPU_BIT() mask of usable features */
    uint64_t cpuid_features;    /* same, but ignoring OS (XCR0) support */
    uint64_t xcr0;              /* OS-enabled register state (0 if no XSAVE) */
    int      isa_level;         /* x86-64 psABI level: 1, 2, 3 (AVX2) or 4 (AVX-512) */
    int      avx10_version;     /* 0 if no AVX10 */
    int      hybrid;            /* 1 on P-core/E-core hybrid parts */
    int      hypervisor;        /* 1 under a VM / WSL2 / Windows VBS (Hyper-V) */
} fp_cpu_info_t;

/* One recorded CPUID result (e.g. transcribed from a CPU-Z "Thread dumps"
 * section), for fp_cpu_decode(). */
typedef struct {
    unsigned leaf, sub;
    unsigned eax, ebx, ecx, edx;
} fp_cpuid_leaf;

/* Description of how this copy of the library was compiled. */
typedef struct {
    const char* isa;            /* ISA preset name (ISA= / FPASM_ISA) */
    const char* march;          /* -march value used for the C code */
    const char* compiler;       /* compiler version string */
    int         dispatch;       /* 1 if runtime kernel dispatch is compiled in */
    const char* gfx;            /* default renderer conventions (GFX= / FPASM_GFX) */
    uint64_t    required;       /* FP_CPU_BIT() mask this build needs */
} fp_build_info_t;

/* Detected once (thread-safe enough: idempotent) and cached. */
const fp_cpu_info_t*   fp_cpu_info(void);
int                    fp_cpu_has(fp_cpu_feature f);
const char*            fp_cpu_feature_name(fp_cpu_feature f);
fp_core_type           fp_cpu_current_core_type(void); /* of the calling thread, right now */

const fp_build_info_t* fp_build_info(void);

/* Decode a recorded CPUID dump (missing leaves read as zero) with the given
 * XCR0 as if it were the running CPU. Lets you check what the library would
 * select on a machine you don't have. Not thread-safe. Returns 0 on success. */
int fp_cpu_decode(const fp_cpuid_leaf* dump, size_t n, uint64_t xcr0, fp_cpu_info_t* out);

/* Features this build needs that the CPU/OS lacks (0 = compatible). */
uint64_t fp_cpu_missing(void);

/* Check build vs. CPU. Returns 0 if compatible; otherwise prints an
 * explanation (with a suggested rebuild) to `err` if non-NULL and returns
 * the number of missing features. */
int fp_cpu_check(FILE* err);

/* Full human-readable diagnostic report: CPU, OS state, every feature,
 * what this build requires, and the compatibility verdict. */
void fp_cpu_report(FILE* out);

#ifdef __cplusplus
}
#endif
#endif /* FP_CPU_H */
