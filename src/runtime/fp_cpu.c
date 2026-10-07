/*
 * fp_cpu.c — CPUID/XGETBV feature detection and diagnostics.
 *
 * Compiled for the plain x86-64 baseline (the build systems override -march
 * for src/runtime/ except fp_build_info.c), so it runs on any x86-64 CPU and
 * can explain a mismatch before an AVX2/AVX-512 instruction would fault.
 */
#include "fp_cpu.h"
#include "fp_once.h"

#include <stdlib.h>
#include <string.h>

#if defined(_MSC_VER) && !defined(__clang__)
#  include <intrin.h>
static void cpuid_live(unsigned leaf, unsigned sub, unsigned r[4]) {
    int v[4];
    __cpuidex(v, (int)leaf, (int)sub);
    r[0] = (unsigned)v[0]; r[1] = (unsigned)v[1];
    r[2] = (unsigned)v[2]; r[3] = (unsigned)v[3];
}
static uint64_t xgetbv0(void) { return _xgetbv(0); }
#else
#  include <cpuid.h>
static void cpuid_live(unsigned leaf, unsigned sub, unsigned r[4]) {
    __cpuid_count(leaf, sub, r[0], r[1], r[2], r[3]);
}
/* Encoded by hand so no -mxsave is needed to compile this file. */
static uint64_t xgetbv0(void) {
    unsigned lo, hi;
    __asm__ volatile(".byte 0x0f, 0x01, 0xd0" : "=a"(lo), "=d"(hi) : "c"(0));
    return ((uint64_t)hi << 32) | lo;
}
#endif

#define B(f) FP_CPU_BIT(FP_CPU_##f)
#define BIT(reg, n) (((reg) >> (n)) & 1u)

/* XCR0 state components */
#define XCR0_SSE      (1ull << 1)
#define XCR0_AVX      (1ull << 2)
#define XCR0_AVX512   ((1ull << 5) | (1ull << 6) | (1ull << 7))  /* opmask, ZMM_Hi256, Hi16_ZMM */
#define XCR0_AMX      ((1ull << 17) | (1ull << 18))              /* TILECFG, TILEDATA */

static const char* const feature_names[FP_CPU_FEATURE_COUNT] = {
    "SSE2", "SSE3", "SSSE3", "SSE4.1", "SSE4.2", "POPCNT", "PCLMUL",
    "AVX", "F16C", "FMA", "AVX2", "BMI1", "BMI2", "LZCNT", "MOVBE", "ADX",
    "SHA", "GFNI", "VAES", "VPCLMULQDQ",
    "AVX-VNNI", "AVX-VNNI-INT8", "AVX-IFMA",
    "AVX512F", "AVX512DQ", "AVX512CD", "AVX512BW", "AVX512VL",
    "AVX512-VNNI", "AVX512-IFMA", "AVX512-VBMI", "AVX512-VBMI2",
    "AVX512-BITALG", "AVX512-VPOPCNTDQ", "AVX512-BF16", "AVX512-FP16",
    "AVX10", "AMX-TILE", "AMX-INT8", "AMX-BF16",
    "ERMS", "FSRM", "HYBRID",
};

const char* fp_cpu_feature_name(fp_cpu_feature f) {
    return ((int)f >= 0 && (int)f < FP_CPU_FEATURE_COUNT) ? feature_names[f] : "?";
}

/* ---- microarchitecture guess (display only; never used for dispatch) ---- */
static const char* intel_uarch(unsigned family, unsigned model) {
    if (family != 6) return family == 19 ? "Intel family 19 (Diamond Rapids/Nova Lake class)" : "Intel (unknown)";
    switch (model) {
    case 0x3C: case 0x3F: case 0x45: case 0x46: return "Haswell";
    case 0x3D: case 0x47: case 0x4F: case 0x56: return "Broadwell";
    case 0x4E: case 0x5E: return "Skylake";
    case 0x55: return "Skylake-SP / Cascade Lake / Cooper Lake";
    case 0x8E: case 0x9E: return "Kaby / Coffee / Whiskey Lake";
    case 0xA5: case 0xA6: return "Comet Lake";
    case 0x66: return "Cannon Lake";
    case 0x6A: case 0x6C: return "Ice Lake-SP";
    case 0x7D: case 0x7E: return "Ice Lake";
    case 0x8C: case 0x8D: return "Tiger Lake";
    case 0xA7: return "Rocket Lake";
    case 0x97: case 0x9A: case 0xBE: return "Alder Lake (hybrid)";
    case 0xB7: case 0xBA: case 0xBF: return "Raptor Lake (hybrid)";
    case 0xAA: case 0xAC: return "Meteor Lake (hybrid)";
    case 0xC5: case 0xC6: return "Arrow Lake (hybrid)";
    case 0xBD: return "Lunar Lake (hybrid)";
    case 0xCC: return "Panther Lake (hybrid)";
    case 0x8F: return "Sapphire Rapids";
    case 0xCF: return "Emerald Rapids";
    case 0xAD: case 0xAE: return "Granite Rapids";
    case 0xAF: return "Sierra Forest";
    case 0xB6: return "Grand Ridge";
    default: return "Intel family 6 (unlisted model)";
    }
}

static const char* amd_uarch(unsigned family, unsigned model) {
    switch (family) {
    case 0x15: return model >= 0x60 ? "Excavator" : "Bulldozer family";
    case 0x16: return "Jaguar / Puma";
    case 0x17: return model >= 0x30 ? "Zen 2" : "Zen / Zen+";
    case 0x19:
        if (model <= 0x0F || (model >= 0x20 && model <= 0x5F)) return "Zen 3";
        return "Zen 4";
    case 0x1A: return "Zen 5";
    default: return "AMD (unknown)";
    }
}

/* ---- detection ----
 * detect() reads CPUID through a source so the same decoder serves the live
 * CPU and recorded dumps (fp_cpu_decode, used by tests/test_cpu_decode.c). */
typedef struct {
    const fp_cpuid_leaf* dump;   /* NULL = live CPU */
    size_t               n;
    uint64_t             xcr0;   /* dump only */
} cpuid_src;

/* src == NULL (or no dump) reads the live CPU. Passed explicitly, never via
 * a global, so decoding a dump cannot interfere with live detection running
 * on another thread. */
static void cpuid(const cpuid_src* src, unsigned leaf, unsigned sub, unsigned r[4]) {
    size_t i;
    if (!src || !src->dump) { cpuid_live(leaf, sub, r); return; }
    r[0] = r[1] = r[2] = r[3] = 0;
    for (i = 0; i < src->n; i++)
        if (src->dump[i].leaf == leaf && src->dump[i].sub == sub) {
            r[0] = src->dump[i].eax; r[1] = src->dump[i].ebx;
            r[2] = src->dump[i].ecx; r[3] = src->dump[i].edx;
            return;
        }
}

static uint64_t read_xcr0(const cpuid_src* src) {
    return (src && src->dump) ? src->xcr0 : xgetbv0();
}

static fp_cpu_info_t g_info;
static long          g_info_once;   /* fp_once.h state */

static void detect(fp_cpu_info_t* ci, const cpuid_src* src) {
    unsigned r[4], max_leaf, max_ext;
    uint64_t f = 0;
    memset(ci, 0, sizeof *ci);

    cpuid(src, 0, 0, r);
    max_leaf = r[0];
    memcpy(ci->vendor + 0, &r[1], 4);
    memcpy(ci->vendor + 4, &r[3], 4);
    memcpy(ci->vendor + 8, &r[2], 4);
    ci->vendor[12] = '\0';

    cpuid(src, 1, 0, r);
    {
        unsigned eax = r[0];
        unsigned base_fam = (eax >> 8) & 0xF, base_mod = (eax >> 4) & 0xF;
        ci->stepping = eax & 0xF;
        ci->family   = base_fam == 0xF ? base_fam + ((eax >> 20) & 0xFF) : base_fam;
        ci->model    = (base_fam == 0x6 || base_fam == 0xF)
                     ? (((eax >> 16) & 0xF) << 4) | base_mod : base_mod;
    }
    {
        unsigned c = r[2], d = r[3];
        if (BIT(d, 26)) f |= B(SSE2);
        if (BIT(c, 0))  f |= B(SSE3);
        if (BIT(c, 1))  f |= B(PCLMUL);
        if (BIT(c, 9))  f |= B(SSSE3);
        if (BIT(c, 12)) f |= B(FMA);
        if (BIT(c, 19)) f |= B(SSE41);
        if (BIT(c, 20)) f |= B(SSE42);
        if (BIT(c, 22)) f |= B(MOVBE);
        if (BIT(c, 23)) f |= B(POPCNT);
        if (BIT(c, 28)) f |= B(AVX);
        if (BIT(c, 29)) f |= B(F16C);
        if (BIT(c, 27)) ci->xcr0 = read_xcr0(src);   /* OSXSAVE */
        ci->hypervisor = BIT(c, 31);
    }

    if (max_leaf >= 7) {
        unsigned max_sub;
        cpuid(src, 7, 0, r);
        max_sub = r[0];
        {
            unsigned b = r[1], c = r[2], d = r[3];
            if (BIT(b, 3))  f |= B(BMI1);
            if (BIT(b, 5))  f |= B(AVX2);
            if (BIT(b, 8))  f |= B(BMI2);
            if (BIT(b, 9))  f |= B(ERMS);
            if (BIT(b, 16)) f |= B(AVX512F);
            if (BIT(b, 17)) f |= B(AVX512DQ);
            if (BIT(b, 19)) f |= B(ADX);
            if (BIT(b, 21)) f |= B(AVX512_IFMA);
            if (BIT(b, 28)) f |= B(AVX512CD);
            if (BIT(b, 29)) f |= B(SHA);
            if (BIT(b, 30)) f |= B(AVX512BW);
            if (BIT(b, 31)) f |= B(AVX512VL);
            if (BIT(c, 1))  f |= B(AVX512_VBMI);
            if (BIT(c, 6))  f |= B(AVX512_VBMI2);
            if (BIT(c, 8))  f |= B(GFNI);
            if (BIT(c, 9))  f |= B(VAES);
            if (BIT(c, 10)) f |= B(VPCLMULQDQ);
            if (BIT(c, 11)) f |= B(AVX512_VNNI);
            if (BIT(c, 12)) f |= B(AVX512_BITALG);
            if (BIT(c, 14)) f |= B(AVX512_VPOPCNTDQ);
            if (BIT(d, 4))  f |= B(FSRM);
            if (BIT(d, 15)) f |= B(HYBRID);
            if (BIT(d, 22)) f |= B(AMX_BF16);
            if (BIT(d, 23)) f |= B(AVX512_FP16);
            if (BIT(d, 24)) f |= B(AMX_TILE);
            if (BIT(d, 25)) f |= B(AMX_INT8);
        }
        if (max_sub >= 1) {
            cpuid(src, 7, 1, r);
            if (BIT(r[0], 4))  f |= B(AVX_VNNI);
            if (BIT(r[0], 5))  f |= B(AVX512_BF16);
            if (BIT(r[0], 23)) f |= B(AVX_IFMA);
            if (BIT(r[3], 4))  f |= B(AVX_VNNI_INT8);
            if (BIT(r[3], 19)) f |= B(AVX10);
        }
    }
    if ((f & B(AVX10)) && max_leaf >= 0x24) {
        cpuid(src, 0x24, 0, r);
        ci->avx10_version = (int)(r[1] & 0xFF);
    }

    cpuid(src, 0x80000000u, 0, r);
    max_ext = r[0];
    if (max_ext >= 0x80000001u) {
        cpuid(src, 0x80000001u, 0, r);
        if (BIT(r[2], 5)) f |= B(LZCNT);
    }
    if (max_ext >= 0x80000004u) {
        unsigned i;
        for (i = 0; i < 3; i++) {
            cpuid(src, 0x80000002u + i, 0, r);
            memcpy(ci->brand + 16 * i, r, 16);
        }
        ci->brand[48] = '\0';
        {   /* trim leading spaces some CPUs pad with */
            char* p = ci->brand;
            while (*p == ' ') p++;
            memmove(ci->brand, p, strlen(p) + 1);
        }
    } else {
        strcpy(ci->brand, "(no brand string)");
    }

    ci->cpuid_features = f;
    ci->hybrid = (f & B(HYBRID)) != 0;

    /* Mask out what the OS has not enabled. */
    {
        const uint64_t avx_dep = B(AVX) | B(F16C) | B(FMA) | B(AVX2) | B(VAES) |
                                 B(VPCLMULQDQ) | B(AVX_VNNI) | B(AVX_VNNI_INT8) |
                                 B(AVX_IFMA);
        const uint64_t avx512_dep = B(AVX512F) | B(AVX512DQ) | B(AVX512CD) |
                                    B(AVX512BW) | B(AVX512VL) | B(AVX512_VNNI) |
                                    B(AVX512_IFMA) | B(AVX512_VBMI) | B(AVX512_VBMI2) |
                                    B(AVX512_BITALG) | B(AVX512_VPOPCNTDQ) |
                                    B(AVX512_BF16) | B(AVX512_FP16) | B(AVX10);
        const uint64_t amx_dep = B(AMX_TILE) | B(AMX_INT8) | B(AMX_BF16);
        if ((ci->xcr0 & (XCR0_SSE | XCR0_AVX)) != (XCR0_SSE | XCR0_AVX))
            f &= ~avx_dep;
        if ((ci->xcr0 & (XCR0_SSE | XCR0_AVX | XCR0_AVX512)) !=
            (XCR0_SSE | XCR0_AVX | XCR0_AVX512))
            f &= ~avx512_dep;
        if ((ci->xcr0 & XCR0_AMX) != XCR0_AMX)
            f &= ~amx_dep;
    }
    ci->features = f;

    {   /* x86-64 psABI micro-architecture levels */
        const uint64_t v2 = B(SSE3) | B(SSSE3) | B(SSE41) | B(SSE42) | B(POPCNT);
        const uint64_t v3 = v2 | B(AVX) | B(AVX2) | B(BMI1) | B(BMI2) | B(F16C) |
                            B(FMA) | B(LZCNT) | B(MOVBE);
        const uint64_t v4 = v3 | B(AVX512F) | B(AVX512BW) | B(AVX512CD) |
                            B(AVX512DQ) | B(AVX512VL);
        ci->isa_level = (f & v4) == v4 ? 4 : (f & v3) == v3 ? 3 : (f & v2) == v2 ? 2 : 1;
    }

    if (strcmp(ci->vendor, "GenuineIntel") == 0)
        ci->uarch = intel_uarch(ci->family, ci->model);
    else if (strcmp(ci->vendor, "AuthenticAMD") == 0 || strcmp(ci->vendor, "HygonGenuine") == 0)
        ci->uarch = amd_uarch(ci->family, ci->model);
    else
        ci->uarch = "unknown vendor";
}

int fp_cpu_decode(const fp_cpuid_leaf* dump, size_t n, uint64_t xcr0, fp_cpu_info_t* out) {
    cpuid_src src;
    if (!dump || !out) return -1;
    src.dump = dump; src.n = n; src.xcr0 = xcr0;
    detect(out, &src);
    return 0;
}

const fp_cpu_info_t* fp_cpu_info(void) {
    if (fp_once_begin(&g_info_once)) {   /* exactly one thread detects */
        detect(&g_info, NULL);
        fp_once_end(&g_info_once);
    }
    return &g_info;
}

int fp_cpu_has(fp_cpu_feature f) {
    if ((int)f < 0 || (int)f >= FP_CPU_FEATURE_COUNT) return 0;
    return (fp_cpu_info()->features & FP_CPU_BIT(f)) != 0;
}

fp_core_type fp_cpu_current_core_type(void) {
    unsigned r[4];
    const fp_cpu_info_t* ci = fp_cpu_info();
    if (!ci->hybrid) return FP_CORE_UNKNOWN;
    cpuid(NULL, 0, 0, r);
    if (r[0] < 0x1A) return FP_CORE_UNKNOWN;
    cpuid(NULL, 0x1A, 0, r);
    switch (r[0] >> 24) {
    case 0x20: return FP_CORE_EFFICIENCY;
    case 0x40: return FP_CORE_PERFORMANCE;
    default:   return FP_CORE_UNKNOWN;
    }
}

extern const fp_build_info_t fp_build_info_data;  /* fp_build_info.c */

const fp_build_info_t* fp_build_info(void) { return &fp_build_info_data; }

uint64_t fp_cpu_missing(void) {
    return fp_build_info()->required & ~fp_cpu_info()->features;
}

static void print_mask(FILE* out, uint64_t m) {
    int i;
    for (i = 0; i < FP_CPU_FEATURE_COUNT; i++)
        if (m & FP_CPU_BIT(i)) fprintf(out, " %s", feature_names[i]);
}

static const char* suggest_isa(const fp_cpu_info_t* ci) {
    if (ci->isa_level >= 4) return "x86-64-v4";
    if (ci->features & B(AVX_VNNI)) return "alderlake";
    if (ci->isa_level >= 3) return "x86-64-v3";
    return NULL;
}

int fp_cpu_check(FILE* err) {
    const fp_cpu_info_t* ci = fp_cpu_info();
    const fp_build_info_t* bi = fp_build_info();
    uint64_t missing = fp_cpu_missing();
    int n = 0, i;
    for (i = 0; i < FP_CPU_FEATURE_COUNT; i++)
        if (missing & FP_CPU_BIT(i)) n++;
    if (n && err) {
        const char* s = suggest_isa(ci);
        fprintf(err,
            "FP-ASM: this library was built for ISA '%s' (-march=%s), but this CPU\n"
            "FP-ASM: (%s, %s) lacks:", bi->isa, bi->march, ci->brand, ci->uarch);
        print_mask(err, missing);
        fprintf(err, "\nFP-ASM: calling into the library will likely crash with SIGILL.\n");
        if (missing & ~ci->cpuid_features & FP_CPU_ASM_BASELINE)
            fprintf(err, "FP-ASM: the assembly kernels need AVX2+FMA (Haswell / Zen 1 or newer); "
                         "this CPU cannot run them.\n");
        else if (missing & ci->cpuid_features)
            fprintf(err, "FP-ASM: the CPU has some of these but the OS has not enabled their "
                         "register state (XCR0=0x%llx).\n", (unsigned long long)ci->xcr0);
        if (s)
            fprintf(err, "FP-ASM: rebuild with `make ISA=%s` (or ISA=x86-64-v3 for a build that "
                         "runs on every AVX2 CPU).\n", s);
    }
    return n;
}

static const char* yes_no(int b) { return b ? "yes" : "no"; }

void fp_cpu_report(FILE* out) {
    const fp_cpu_info_t* ci = fp_cpu_info();
    const fp_build_info_t* bi = fp_build_info();
    uint64_t missing = fp_cpu_missing();
    int i;

    fprintf(out, "CPU\n");
    fprintf(out, "  vendor        : %s\n", ci->vendor);
    fprintf(out, "  brand         : %s\n", ci->brand);
    fprintf(out, "  family/model  : 0x%X / 0x%X, stepping %u  ->  %s\n",
            ci->family, ci->model, ci->stepping, ci->uarch);
    fprintf(out, "  x86-64 level  : v%d%s\n", ci->isa_level,
            ci->isa_level >= 4 ? " (AVX-512)" : ci->isa_level == 3 ? " (AVX2/FMA/BMI2)" : "");
    if (ci->hybrid) {
        fp_core_type ct = fp_cpu_current_core_type();
        fprintf(out, "  hybrid        : yes (P-cores + E-cores); this thread is on %s right now\n",
                ct == FP_CORE_PERFORMANCE ? "a P-core" :
                ct == FP_CORE_EFFICIENCY  ? "an E-core" : "an unknown core type");
    } else {
        if (ci->hypervisor && strstr(ci->uarch, "(hybrid)"))
            fprintf(out, "  hybrid        : not reported (the hypervisor hides it), but this model is a\n"
                         "                  P-core + E-core design; core type per thread is unknown here\n");
        else
            fprintf(out, "  hybrid        : no\n");
    }
    if (ci->avx10_version)
        fprintf(out, "  AVX10         : version %d\n", ci->avx10_version);
    if (ci->hypervisor) {
        unsigned r[4];
        char hv[13];
        cpuid(NULL, 0x40000000u, 0, r);
        memcpy(hv, &r[1], 4); memcpy(hv + 4, &r[2], 4); memcpy(hv + 8, &r[3], 4);
        hv[12] = '\0';
        fprintf(out, "  hypervisor    : yes (\"%s\") — VM, WSL2 or Windows VBS; features are what it exposes\n", hv);
    }
    fprintf(out, "  OS reg. state : XCR0=0x%llx  SSE:%s  AVX/YMM:%s  AVX-512/ZMM:%s  AMX:%s\n",
            (unsigned long long)ci->xcr0,
            yes_no((ci->xcr0 & XCR0_SSE) != 0), yes_no((ci->xcr0 & XCR0_AVX) != 0),
            yes_no((ci->xcr0 & XCR0_AVX512) == XCR0_AVX512),
            yes_no((ci->xcr0 & XCR0_AMX) == XCR0_AMX));

    fprintf(out, "\nFeatures (cpu = usable on this CPU+OS, build = required by this library build)\n");
    for (i = 0; i < FP_CPU_FEATURE_COUNT; i++) {
        uint64_t b = FP_CPU_BIT(i);
        int has = (ci->features & b) != 0, need = (bi->required & b) != 0;
        const char* note = "";
        if (need && !has) note = "  <-- MISSING";
        else if (!has && (ci->cpuid_features & b)) note = "  (CPU has it, OS disabled)";
        fprintf(out, "  %-17s cpu:%-3s  build:%-3s%s\n", feature_names[i], yes_no(has),
                need ? "yes" : "-", note);
    }

    fprintf(out, "\nBuild\n");
    fprintf(out, "  ISA preset    : %s\n", bi->isa);
    fprintf(out, "  C -march      : %s\n", bi->march);
    fprintf(out, "  compiler      : %s\n", bi->compiler);
    fprintf(out, "  gfx default   : %s (fp_gfx_default(); every preset usable at runtime)\n", bi->gfx);
    fprintf(out, "  dispatch      : %s\n", bi->dispatch
            ? "runtime (best kernel per CPU: AVX-512 > AVX-VNNI > AVX2)"
            : "off (public symbols are the AVX2 kernels)");
    fprintf(out, "  requires      :");
    print_mask(out, bi->required);
    fprintf(out, "\n");

    fprintf(out, "\nVerdict\n");
    if (missing) {
        fprintf(out, "  INCOMPATIBLE — see below\n");
        fp_cpu_check(out);
    } else {
        fprintf(out, "  OK — this build runs on this CPU.\n");
    }

    fprintf(out, "\nNotes\n");
    int hybrid_model = ci->hybrid || strstr(ci->uarch, "(hybrid)") != NULL;
    if (hybrid_model && !(ci->cpuid_features & B(AVX512F)))
        fprintf(out, "  - Hybrid Intel CPU without AVX-512: Alder/Raptor Lake and later client chips\n"
                     "    disable AVX-512 because the E-cores lack it. The library uses AVX2 and,\n"
                     "    for integer dot products, AVX-VNNI%s.\n",
                     (ci->features & B(AVX_VNNI)) ? " (available here)" : "");
    if (hybrid_model)
        fprintf(out, "  - P-cores and E-cores differ ~2x in SIMD throughput. For stable benchmark\n"
                     "    numbers pin to P-cores (Linux: taskset -c 0-7 ...; check lscpu --extended).\n");
    if ((ci->cpuid_features & B(AVX512F)) && !(ci->features & B(AVX512F)))
        fprintf(out, "  - The CPU reports AVX-512 but the OS has not enabled ZMM state; it cannot be used.\n");
    if (ci->isa_level == 4 && !(bi->required & B(AVX512F)))
        fprintf(out, "  - This CPU supports AVX-512; dispatched kernels use it at runtime%s.\n"
                     "    The C code was compiled for '%s'; ISA=native or ISA=x86-64-v4 lets the\n"
                     "    compiler use AVX-512 there too (the result will not run on AVX2-only CPUs).\n",
                     bi->dispatch ? "" : " only if built with dispatch", bi->march);
    else if ((ci->features & B(AVX_VNNI)) && !(bi->required & B(AVX_VNNI)))
        fprintf(out, "  - This CPU supports AVX-VNNI (Alder/Raptor Lake class). ISA=raptorlake or\n"
                     "    ISA=native tunes the C code for it (the result will not run on older CPUs).\n");
    if (strcmp(bi->march, "native") == 0)
        fprintf(out, "  - Built with -march=native: the C code matches the build machine and may not\n"
                     "    run on older CPUs. For a library you ship, use ISA=x86-64-v3 (any AVX2 CPU);\n"
                     "    dispatch still picks AVX-VNNI/AVX-512 kernels at runtime.\n");
    if (!missing && (bi->required & ~(FP_CPU_ASM_BASELINE | B(SSE2))) == 0)
        fprintf(out, "  - C code is built for a generic baseline; this is the most portable build.\n");
}

/* Load-time check: print a clear diagnostic instead of dying on SIGILL.
 *   FPASM_CPU_CHECK=0       disable
 *   FPASM_CPU_CHECK=strict  abort() on mismatch
 *   FPASM_CPU_CHECK=report  always print the full report to stderr */
#if defined(__GNUC__) || defined(__clang__)
#  if defined(__ELF__)
/* A program that reports on its own (fpasm-info) defines this to 1. */
extern const int fp_cpu_no_load_check __attribute__((weak));
#  endif
__attribute__((constructor))
static void fp_cpu_load_check(void) {
    const char* e = getenv("FPASM_CPU_CHECK");
#  if defined(__ELF__)
    if (&fp_cpu_no_load_check && fp_cpu_no_load_check) return;
#  endif
    if (e && strcmp(e, "0") == 0) return;
    if (e && strcmp(e, "report") == 0) fp_cpu_report(stderr);
    if (fp_cpu_check(stderr) && e && strcmp(e, "strict") == 0) abort();
}
#endif
