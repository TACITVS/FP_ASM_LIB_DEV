#include "fp_query.h"
#include <math.h>
#include <float.h>
#include <string.h>
#include <stdio.h>

#ifdef __AVX2__
#include <immintrin.h>
#endif

/**
 * Columnar GEMV: scores = sum_d(columns[d] * query[d])
 * AVX2-optimized for 4 doubles at a time.
 */
void fp_query_gemv_columnar_f64(
    const double** columns,
    const double* query,
    double* scores_out,
    size_t count,
    size_t dim
) {
    // Zero output
    memset(scores_out, 0, count * sizeof(double));

#ifdef __AVX2__
    size_t vec_count = count & ~3ULL;  // Process 4 at a time

    for (size_t d = 0; d < dim; d++) {
        const double* col = columns[d];
        double q = query[d];
        __m256d q_vec = _mm256_set1_pd(q);

        // Vectorized loop
        for (size_t i = 0; i < vec_count; i += 4) {
            __m256d c = _mm256_loadu_pd(&col[i]);
            __m256d acc = _mm256_loadu_pd(&scores_out[i]);
#if defined(__FMA__)
            acc = _mm256_fmadd_pd(c, q_vec, acc);
#else
            acc = _mm256_add_pd(acc, _mm256_mul_pd(c, q_vec));
#endif
            _mm256_storeu_pd(&scores_out[i], acc);
        }

        // Scalar tail
        for (size_t i = vec_count; i < count; i++) {
            scores_out[i] += col[i] * q;
        }
    }
#else
    // Scalar fallback
    for (size_t d = 0; d < dim; d++) {
        const double* col = columns[d];
        double q = query[d];
        for (size_t i = 0; i < count; i++) {
            scores_out[i] += col[i] * q;
        }
    }
#endif
}

/**
 * Batch GEMV: process multiple queries in one pass.
 * Optimized for cache locality by processing data blocks.
 */
void fp_query_gemv_columnar_batch_f64(
    const double** columns,
    const double* queries,
    size_t batch_count,
    double* scores_out,
    size_t count,
    size_t dim
) {
    printf("Kernel: batch=%llu, count=%llu, dim=%llu\n", (unsigned long long)batch_count, (unsigned long long)count, (unsigned long long)dim);
    fflush(stdout);
    // Zero all output buffers
    memset(scores_out, 0, batch_count * count * sizeof(double));

#ifdef __AVX2__
    size_t vec_count = count & ~3ULL;
    printf("Kernel: AVX mode, vec_count=%llu\n", (unsigned long long)vec_count);
    fflush(stdout);

    // Process in blocks of vectors to keep 'col' in L1 cache
    // while looping over queries.
    for (size_t d = 0; d < dim; d++) {
        const double* col = columns[d];

        for (size_t b = 0; b < batch_count; b++) {
            double q = queries[b * dim + d];
            __m256d q_vec = _mm256_set1_pd(q);
            double* b_scores = &scores_out[b * count];

            for (size_t i = 0; i < vec_count; i += 4) {
                __m256d c = _mm256_loadu_pd(&col[i]);
                __m256d acc = _mm256_loadu_pd(&b_scores[i]);
#if defined(__FMA__)
                acc = _mm256_fmadd_pd(c, q_vec, acc);
#else
                acc = _mm256_add_pd(acc, _mm256_mul_pd(c, q_vec));
#endif
                _mm256_storeu_pd(&b_scores[i], acc);
            }

            for (size_t i = vec_count; i < count; i++) {
                b_scores[i] += col[i] * q;
            }
        }
    }
#else
    printf("Kernel: Scalar mode\n");
    fflush(stdout);
    for (size_t d = 0; d < dim; d++) {
        const double* col = columns[d];
        for (size_t b = 0; b < batch_count; b++) {
            double q = queries[b * dim + d];
            double* b_scores = &scores_out[b * count];
            for (size_t i = 0; i < count; i++) {
                b_scores[i] += col[i] * q;
            }
        }
    }
#endif
    printf("Kernel Finished.\n");
    fflush(stdout);
}

/**
 * Masked GEMV for IVF search - only compute for candidates.
 */
void fp_query_gemv_masked_f64(
    const double** columns,
    const double* query,
    const uint8_t* mask,
    double* scores_out,
    size_t count,
    size_t dim
) {
    // Initialize: -infinity for non-candidates, 0 for candidates
    for (size_t i = 0; i < count; i++) {
        scores_out[i] = mask[i] ? 0.0 : -INFINITY;
    }

#ifdef __AVX2__
    for (size_t d = 0; d < dim; d++) {
        const double* col = columns[d];
        double q = query[d];

        // Only compute for masked entries
        for (size_t i = 0; i < count; i++) {
            if (mask[i]) {
                scores_out[i] += col[i] * q;
            }
        }
    }
#else
    for (size_t d = 0; d < dim; d++) {
        const double* col = columns[d];
        double q = query[d];
        for (size_t i = 0; i < count; i++) {
            if (mask[i]) {
                scores_out[i] += col[i] * q;
            }
        }
    }
#endif
}

/**
 * Flat layout GEMV: columns stored contiguously.
 */
void fp_query_gemv_flat_f64(
    const double* columns_flat,
    const double* query,
    double* scores_out,
    size_t count,
    size_t dim
) {
    memset(scores_out, 0, count * sizeof(double));

#ifdef __AVX2__
    size_t vec_count = count & ~3ULL;

    for (size_t d = 0; d < dim; d++) {
        const double* col = columns_flat + d * count;
        double q = query[d];
        __m256d q_vec = _mm256_set1_pd(q);

        for (size_t i = 0; i < vec_count; i += 4) {
            __m256d c = _mm256_loadu_pd(&col[i]);
            __m256d acc = _mm256_loadu_pd(&scores_out[i]);
#if defined(__FMA__)
            acc = _mm256_fmadd_pd(c, q_vec, acc);
#else
            acc = _mm256_add_pd(acc, _mm256_mul_pd(c, q_vec));
#endif
            _mm256_storeu_pd(&scores_out[i], acc);
        }

        for (size_t i = vec_count; i < count; i++) {
            scores_out[i] += col[i] * q;
        }
    }
#else
    for (size_t d = 0; d < dim; d++) {
        const double* col = columns_flat + d * count;
        double q = query[d];
        for (size_t i = 0; i < count; i++) {
            scores_out[i] += col[i] * q;
        }
    }
#endif
}

/**
 * Indexed GEMV for IVF - only compute for specified row indices.
 * This is the key optimization: O(num_indices * dim) vs O(count * dim)
 */
void fp_query_gemv_indexed_f64(
    const double** columns,
    const double* query,
    const int32_t* indices,
    size_t num_indices,
    double* scores_out,
    size_t count,
    size_t dim
) {
    // Zero output scores
    memset(scores_out, 0, num_indices * sizeof(double));

    // For each dimension, accumulate only for candidate indices
    for (size_t d = 0; d < dim; d++) {
        const double* col = columns[d];
        double q = query[d];

        for (size_t i = 0; i < num_indices; i++) {
            int32_t idx = indices[i];
            if (idx >= 0 && (size_t)idx < count) {
                scores_out[i] += col[idx] * q;
            }
        }
    }
}

/**
 * Top-K selection using partial quickselect.
 * Returns top K scores above threshold.
 */
size_t fp_query_topk_f64(
    const double* scores,
    size_t count,
    size_t k,
    double threshold,
    int32_t* indices_out,
    double* scores_out
) {
    // Simple approach: collect candidates, sort, take top k
    // For production, use heap-based selection for O(n log k)

    // Count candidates above threshold
    size_t n_candidates = 0;
    for (size_t i = 0; i < count; i++) {
        if (scores[i] >= threshold) {
            n_candidates++;
        }
    }

    if (n_candidates == 0) {
        return 0;
    }

    // Collect candidate indices and scores
    // Use simple insertion sort for small k (common case)
    size_t result_count = 0;

    for (size_t i = 0; i < count; i++) {
        double score = scores[i];
        if (score < threshold) continue;

        // Find insertion position (descending order)
        size_t pos = result_count;
        while (pos > 0 && scores_out[pos - 1] < score) {
            pos--;
        }

        if (pos < k) {
            // Shift elements right
            size_t shift_count = (result_count < k ? result_count : k - 1) - pos;
            if (shift_count > 0) {
                memmove(&indices_out[pos + 1], &indices_out[pos], shift_count * sizeof(int32_t));
                memmove(&scores_out[pos + 1], &scores_out[pos], shift_count * sizeof(double));
            }

            // Insert
            indices_out[pos] = (int32_t)i;
            scores_out[pos] = score;

            if (result_count < k) {
                result_count++;
            }
        }
    }

    return result_count;
}

void fp_quantize_f64_to_u8(
    const double* in,
    uint8_t* out,
    size_t count,
    double min_val,
    double inv_scale
) {
#ifdef __AVX2__
    size_t vec_count = count & ~7ULL; // 8 at a time (4 per 256-bit register, need 2 regs for 8 doubles -> 1 reg for 8 bytes?)
    // Actually, process 8 doubles -> 8 integers -> pack to 8 bytes.
    // _mm256_cvtpd_epi32 converts 4 doubles to 4 int32s.
    
    __m256d min_vec = _mm256_set1_pd(min_val);
    __m256d scale_vec = _mm256_set1_pd(inv_scale);

    for (size_t i = 0; i < vec_count; i += 8) {
        // Load 8 doubles
        __m256d d0 = _mm256_loadu_pd(&in[i]);
        __m256d d1 = _mm256_loadu_pd(&in[i+4]);

        // (x - min) * scale
        d0 = _mm256_sub_pd(d0, min_vec);
        d0 = _mm256_mul_pd(d0, scale_vec);
        
        d1 = _mm256_sub_pd(d1, min_vec);
        d1 = _mm256_mul_pd(d1, scale_vec);

        // Convert to int32 (truncation or rounding? default rounding is round-to-nearest-even)
        __m128i i0 = _mm256_cvtpd_epi32(d0);
        __m128i i1 = _mm256_cvtpd_epi32(d1);

        // Pack 8 int32s into 8 bytes (with saturation)
        // i0: [a0, a1, a2, a3] (128-bit)
        // i1: [b0, b1, b2, b3] (128-bit)
        // Pack i0, i1 to 16-bit integers
        __m128i p16 = _mm_packus_epi32(i0, i1); // [a0..a3, b0..b3] as u16
        
        // Pack to 8-bit integers
        __m128i p8 = _mm_packus_epi16(p16, p16); // Lower 64 bits contain our 8 bytes
        
        // Store low 64 bits (8 bytes)
        _mm_storel_epi64((__m128i*)&out[i], p8);
    }
    
    for (size_t i = vec_count; i < count; i++) {
        double val = (in[i] - min_val) * inv_scale;
        if (val < 0) val = 0;
        if (val > 255) val = 255;
        out[i] = (uint8_t)(val + 0.5); // simple round
    }
#else
    for (size_t i = 0; i < count; i++) {
        double val = (in[i] - min_val) * inv_scale;
        if (val < 0) val = 0;
        if (val > 255) val = 255;
        out[i] = (uint8_t)(val + 0.5);
    }
#endif
}

void fp_query_gemv_quantized_f64_u8(
    const uint8_t** columns_u8,
    const double* scaled_query,
    double bias,
    double* scores_out,
    size_t count,
    size_t dim
) {
    // Init scores with bias
    for (size_t i = 0; i < count; i++) {
        scores_out[i] = bias;
    }

#ifdef __AVX2__
    size_t vec_count = count & ~3ULL; // Process 4 rows at a time (since we accumulate into doubles)

    for (size_t d = 0; d < dim; d++) {
        const uint8_t* col = columns_u8[d];
        double q_val = scaled_query[d];
        __m256d q_vec = _mm256_set1_pd(q_val);

        for (size_t i = 0; i < vec_count; i += 4) {
            // Load 4 bytes
            int32_t val_i32 = *(int32_t*)&col[i]; // Load 4 bytes into 32-bit int
            
            // Expand to 4 integers (u8 -> i32)
            __m128i v_u8 = _mm_cvtsi32_si128(val_i32);
            __m128i v_i32 = _mm_cvtepu8_epi32(v_u8);
            
            // Convert to 4 doubles
            __m256d v_f64 = _mm256_cvtepi32_pd(v_i32);
            
            // FMA
            __m256d acc = _mm256_loadu_pd(&scores_out[i]);
#if defined(__FMA__)
            acc = _mm256_fmadd_pd(v_f64, q_vec, acc);
#else
            acc = _mm256_add_pd(acc, _mm256_mul_pd(v_f64, q_vec));
#endif
            _mm256_storeu_pd(&scores_out[i], acc);
        }

        for (size_t i = vec_count; i < count; i++) {
            scores_out[i] += (double)col[i] * q_val;
        }
    }
#else
    for (size_t d = 0; d < dim; d++) {
        const uint8_t* col = columns_u8[d];
        double q_val = scaled_query[d];
        for (size_t i = 0; i < count; i++) {
            scores_out[i] += (double)col[i] * q_val;
        }
    }
#endif
}

double fp_sparse_dotp_f64(
    const int32_t* indices_a,
    const double* values_a,
    size_t len_a,
    const int32_t* indices_b,
    const double* values_b,
    size_t len_b
) {
    double dot = 0.0;
    size_t i = 0, j = 0;

    while (i < len_a && j < len_b) {
        if (indices_a[i] == indices_b[j]) {
            dot += values_a[i] * values_b[j];
            i++;
            j++;
        } else if (indices_a[i] < indices_b[j]) {
            i++;
        } else {
            j++;
        }
    }

    return dot;
}
