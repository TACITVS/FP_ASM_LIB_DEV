/*
 * fp_dispatch.h — runtime selection of ISA-specific kernels.
 *
 * A few hot kernels have faster variants for newer CPUs. With dispatch
 * enabled (the default; DISPATCH=0 / -DFPASM_DISPATCH=OFF turns it off) the
 * public functions in fp_core.h pick the best variant the running CPU
 * supports, once, on first use:
 *
 *   tier     needs                          typical CPUs
 *   avx2     AVX2 + FMA                     Haswell+, Zen 1+ (always present)
 *   avxvnni  AVX-VNNI                       Alder/Raptor Lake (12-14th gen), Meteor/Arrow Lake, Zen 5
 *   avx512   AVX-512 F/BW/DQ/VL + BMI2      Ice Lake, Sapphire Rapids, Zen 4/5
 *            (+ AVX512-VNNI for int dots)
 *
 * The variant functions are also exported under explicit names (below), so
 * tests and benchmarks can compare them directly. Calling a variant the CPU
 * does not support raises SIGILL — check fp_dispatch_tier_available() first.
 *
 * Environment overrides (read at first use):
 *   FPASM_TIER=avx2|avxvnni|avx512|auto   cap the tier (e.g. A/B benchmarks)
 *   FPASM_VERBOSE=1                       print the dispatch table to stderr
 */
#ifndef FP_DISPATCH_H
#define FP_DISPATCH_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    FP_TIER_AUTO    = -1,
    FP_TIER_AVX2    = 0,
    FP_TIER_AVXVNNI = 1,
    FP_TIER_AVX512  = 2,
    FP_TIER_COUNT   = 3
} fp_tier;

const char* fp_tier_name(fp_tier t);
fp_tier     fp_tier_from_name(const char* s);   /* FP_TIER_AUTO if unknown */

/* 1 if this build routes the public symbols through the dispatcher. */
int fp_dispatch_enabled(void);

/* 1 if the CPU+OS can run kernels of tier `t`. */
int fp_dispatch_tier_available(fp_tier t);

/* Highest tier usable on this CPU, respecting the current cap. */
fp_tier fp_dispatch_best_tier(void);

/* Cap the tier and re-resolve every kernel. Returns the previous cap.
 * FP_TIER_AUTO removes the cap. Not meant to race with kernel calls on
 * other threads (they would just see either variant). */
fp_tier fp_dispatch_set_max_tier(fp_tier t);

/* (Re)initialise now instead of on first call. Idempotent. */
void fp_dispatch_init(void);

/* Print the kernel -> variant table. */
void fp_dispatch_report(FILE* out);

/* Name of the variant currently selected for a public kernel, e.g.
 * fp_dispatch_selected("fp_fold_dotp_f32") -> "avx512". NULL if unknown. */
const char* fp_dispatch_selected(const char* kernel);

/* ---- explicit variants ---------------------------------------------- */
float    fp_reduce_add_f32_avx2(const float* in, size_t n);
float    fp_reduce_add_f32_avx512(const float* in, size_t n);
double   fp_reduce_add_f64_avx2(const double* in, size_t n);
double   fp_reduce_add_f64_avx512(const double* in, size_t n);
float    fp_fold_sumsq_f32_avx2(const float* in, size_t n);
float    fp_fold_sumsq_f32_avx512(const float* in, size_t n);
float    fp_fold_dotp_f32_avx2(const float* a, const float* b, size_t n);
float    fp_fold_dotp_f32_avx512(const float* a, const float* b, size_t n);
double   fp_fold_dotp_f64_avx2(const double* a, const double* b, size_t n);
double   fp_fold_dotp_f64_avx512(const double* a, const double* b, size_t n);

int8_t   fp_fold_dotp_i8_avx2(const int8_t* a, const int8_t* b, size_t n);
int8_t   fp_fold_dotp_i8_avxvnni(const int8_t* a, const int8_t* b, size_t n);
int8_t   fp_fold_dotp_i8_avx512(const int8_t* a, const int8_t* b, size_t n);
uint8_t  fp_fold_dotp_u8_avx2(const uint8_t* a, const uint8_t* b, size_t n);
uint8_t  fp_fold_dotp_u8_avxvnni(const uint8_t* a, const uint8_t* b, size_t n);
uint8_t  fp_fold_dotp_u8_avx512(const uint8_t* a, const uint8_t* b, size_t n);
int16_t  fp_fold_dotp_i16_avx2(const int16_t* a, const int16_t* b, size_t n);
int16_t  fp_fold_dotp_i16_avxvnni(const int16_t* a, const int16_t* b, size_t n);
int16_t  fp_fold_dotp_i16_avx512(const int16_t* a, const int16_t* b, size_t n);
uint16_t fp_fold_dotp_u16_avx2(const uint16_t* a, const uint16_t* b, size_t n);
uint16_t fp_fold_dotp_u16_avxvnni(const uint16_t* a, const uint16_t* b, size_t n);
uint16_t fp_fold_dotp_u16_avx512(const uint16_t* a, const uint16_t* b, size_t n);

#ifdef __cplusplus
}
#endif
#endif /* FP_DISPATCH_H */
