/*
 * fp_build_info.c — records how the library was compiled.
 *
 * Unlike the rest of src/runtime/, this file is compiled with the SAME flags
 * as the library (the ISA preset's -march), so the compiler's predefined
 * feature macros below describe exactly which instruction sets the C code
 * of this build may contain. It holds data only — no code that could itself
 * use those instructions.
 */
#include "fp_cpu.h"

#define FP_XSTR_(x) #x
#define FP_XSTR(x)  FP_XSTR_(x)

#ifdef FPASM_ISA_NAME
#  define FP_ISA_STR FP_XSTR(FPASM_ISA_NAME)
#else
#  define FP_ISA_STR "unspecified"
#endif
#ifdef FPASM_MARCH
#  define FP_MARCH_STR FP_XSTR(FPASM_MARCH)
#else
#  define FP_MARCH_STR "compiler default"
#endif
#ifdef FP_DISPATCH
#  define FP_DISPATCH_ON 1
#else
#  define FP_DISPATCH_ON 0
#endif

#if defined(__clang__)
#  define FP_COMPILER "clang " __clang_version__
#elif defined(__GNUC__)
#  define FP_COMPILER "gcc " __VERSION__
#elif defined(_MSC_VER)
#  define FP_COMPILER "msvc " FP_XSTR(_MSC_VER)
#else
#  define FP_COMPILER "unknown"
#endif

#define B(f) FP_CPU_BIT(FP_CPU_##f)


/* Read by fp_build_info() in fp_cpu.c. */
const fp_build_info_t fp_build_info_data = {
    FP_ISA_STR,
    FP_MARCH_STR,
    FP_COMPILER,
    FP_DISPATCH_ON,
    /* The assembly needs the AVX2 baseline; the C code needs whatever the
     * -march in effect let the compiler use. */
    FP_CPU_ASM_BASELINE
#ifdef __SSE2__
        | B(SSE2)
#endif
#ifdef __SSE3__
        | B(SSE3)
#endif
#ifdef __SSSE3__
        | B(SSSE3)
#endif
#ifdef __SSE4_1__
        | B(SSE41)
#endif
#ifdef __SSE4_2__
        | B(SSE42)
#endif
#ifdef __POPCNT__
        | B(POPCNT)
#endif
#ifdef __PCLMUL__
        | B(PCLMUL)
#endif
#ifdef __AVX__
        | B(AVX)
#endif
#ifdef __F16C__
        | B(F16C)
#endif
#ifdef __FMA__
        | B(FMA)
#endif
#ifdef __AVX2__
        | B(AVX2)
#endif
#ifdef __BMI__
        | B(BMI1)
#endif
#ifdef __BMI2__
        | B(BMI2)
#endif
#ifdef __LZCNT__
        | B(LZCNT)
#endif
#ifdef __MOVBE__
        | B(MOVBE)
#endif
#ifdef __ADX__
        | B(ADX)
#endif
#ifdef __SHA__
        | B(SHA)
#endif
#ifdef __GFNI__
        | B(GFNI)
#endif
#ifdef __VAES__
        | B(VAES)
#endif
#ifdef __VPCLMULQDQ__
        | B(VPCLMULQDQ)
#endif
#ifdef __AVXVNNI__
        | B(AVX_VNNI)
#endif
#ifdef __AVXVNNIINT8__
        | B(AVX_VNNI_INT8)
#endif
#ifdef __AVXIFMA__
        | B(AVX_IFMA)
#endif
#ifdef __AVX512F__
        | B(AVX512F)
#endif
#ifdef __AVX512DQ__
        | B(AVX512DQ)
#endif
#ifdef __AVX512CD__
        | B(AVX512CD)
#endif
#ifdef __AVX512BW__
        | B(AVX512BW)
#endif
#ifdef __AVX512VL__
        | B(AVX512VL)
#endif
#ifdef __AVX512VNNI__
        | B(AVX512_VNNI)
#endif
#ifdef __AVX512IFMA__
        | B(AVX512_IFMA)
#endif
#ifdef __AVX512VBMI__
        | B(AVX512_VBMI)
#endif
#ifdef __AVX512VBMI2__
        | B(AVX512_VBMI2)
#endif
#ifdef __AVX512BITALG__
        | B(AVX512_BITALG)
#endif
#ifdef __AVX512VPOPCNTDQ__
        | B(AVX512_VPOPCNTDQ)
#endif
#ifdef __AVX512BF16__
        | B(AVX512_BF16)
#endif
#ifdef __AVX512FP16__
        | B(AVX512_FP16)
#endif
};
