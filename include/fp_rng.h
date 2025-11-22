/**
 * fp_rng.h - Deterministic Random Number Generator
 *
 * Provides a pure, deterministic RNG for FP-ASM algorithms.
 * Replaces rand()/srand() with seed-based generation.
 *
 * Properties:
 * - Same seed always produces same sequence (deterministic)
 * - No global state (pure function when using fp_rng_t)
 * - Fast xorshift64 algorithm
 * - FP-compliant: immutable state, returns new state
 *
 * Usage:
 *   fp_rng_t rng = fp_rng_create(42);  // Create with seed
 *   double val;
 *   rng = fp_rng_next_f64(&rng, &val); // Get value, advance state
 */

#ifndef FP_RNG_H
#define FP_RNG_H

#include <stdint.h>
#include <stddef.h>
#include <math.h>

/* ============================================================================
 * RNG State Structure
 * ============================================================================ */

typedef struct {
    uint64_t state;
} fp_rng_t;

/* ============================================================================
 * RNG Creation and Core Operations
 * ============================================================================ */

/**
 * Create RNG with seed
 * Same seed always produces same sequence
 */
static inline fp_rng_t fp_rng_create(uint64_t seed) {
    fp_rng_t rng;
    // Avoid zero state (xorshift fails with zero)
    rng.state = seed ? seed : 0x853c49e6748fea9bULL;
    return rng;
}

/**
 * Generate next 64-bit value (xorshift64)
 * Returns new RNG state (pure function)
 */
static inline fp_rng_t fp_rng_next_u64(fp_rng_t rng, uint64_t* out) {
    uint64_t x = rng.state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    rng.state = x;
    *out = x;
    return rng;
}

/**
 * Generate double in [0.0, 1.0)
 */
static inline fp_rng_t fp_rng_next_f64(fp_rng_t rng, double* out) {
    uint64_t x;
    rng = fp_rng_next_u64(rng, &x);
    // Use top 53 bits for full double precision mantissa
    *out = (x >> 11) * (1.0 / 9007199254740992.0);  // 2^53
    return rng;
}

/**
 * Generate double in [min, max)
 */
static inline fp_rng_t fp_rng_next_f64_range(fp_rng_t rng, double min, double max, double* out) {
    double val;
    rng = fp_rng_next_f64(rng, &val);
    *out = min + val * (max - min);
    return rng;
}

/**
 * Generate int64 in [min, max]
 */
static inline fp_rng_t fp_rng_next_i64_range(fp_rng_t rng, int64_t min, int64_t max, int64_t* out) {
    uint64_t x;
    rng = fp_rng_next_u64(rng, &x);
    uint64_t range = (uint64_t)(max - min + 1);
    *out = min + (int64_t)(x % range);
    return rng;
}

/* ============================================================================
 * Batch Generation (fills arrays)
 * ============================================================================ */

/**
 * Fill array with random doubles in [0.0, 1.0)
 */
static inline fp_rng_t fp_rng_fill_f64(fp_rng_t rng, double* out, size_t n) {
    for (size_t i = 0; i < n; i++) {
        rng = fp_rng_next_f64(rng, &out[i]);
    }
    return rng;
}

/**
 * Fill array with random doubles in [min, max)
 */
static inline fp_rng_t fp_rng_fill_f64_range(fp_rng_t rng, double* out, size_t n, double min, double max) {
    for (size_t i = 0; i < n; i++) {
        rng = fp_rng_next_f64_range(rng, min, max, &out[i]);
    }
    return rng;
}

/**
 * Fill array with standard normal distribution (Box-Muller)
 */
static inline fp_rng_t fp_rng_fill_normal_f64(fp_rng_t rng, double* out, size_t n, double mean, double stddev) {
    // Use Box-Muller transform for pairs
    size_t i = 0;
    while (i < n) {
        double u1, u2;
        rng = fp_rng_next_f64(rng, &u1);
        rng = fp_rng_next_f64(rng, &u2);

        // Avoid log(0)
        if (u1 < 1e-10) u1 = 1e-10;

        // Box-Muller transform
        double mag = stddev * sqrt(-2.0 * log(u1));
        double z0 = mag * cos(2.0 * 3.14159265358979323846 * u2) + mean;
        double z1 = mag * sin(2.0 * 3.14159265358979323846 * u2) + mean;

        out[i++] = z0;
        if (i < n) out[i++] = z1;
    }
    return rng;
}

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

/**
 * Shuffle array (Fisher-Yates)
 */
static inline fp_rng_t fp_rng_shuffle_f64(fp_rng_t rng, double* arr, size_t n) {
    for (size_t i = n - 1; i > 0; i--) {
        int64_t j;
        rng = fp_rng_next_i64_range(rng, 0, (int64_t)i, &j);
        // Swap
        double tmp = arr[i];
        arr[i] = arr[j];
        arr[j] = tmp;
    }
    return rng;
}

#endif /* FP_RNG_H */
