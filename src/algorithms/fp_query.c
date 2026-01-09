#include "fp_query.h"
#include "fp_core.h"
#include "fp_bitmap.h"
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
    if (!scores_out || !columns || !query || count == 0 || dim == 0) return;
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
    // Zero all output buffers
    memset(scores_out, 0, batch_count * count * sizeof(double));

#ifdef __AVX2__
    size_t vec_count = count & ~3ULL;

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
    size_t result_count = 0;

    for (size_t i = 0; i < count && result_count < k; i++) {
        if (scores[i] >= threshold) {
            indices_out[result_count] = (int32_t)i;
            scores_out[result_count] = scores[i];
            result_count++;
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

void fp_quantize_f32_to_u8(
    const float* in,
    uint8_t* out,
    size_t count,
    float min_val,
    float inv_scale
) {
#ifdef __AVX2__
    size_t vec_count = count & ~15ULL; // Process 16 at a time (8 per YMM register)
    
    __m256 min_vec = _mm256_set1_ps(min_val);
    __m256 scale_vec = _mm256_set1_ps(inv_scale);

    for (size_t i = 0; i < vec_count; i += 16) {
        // Load 16 floats
        __m256 f0 = _mm256_loadu_ps(&in[i]);
        __m256 f1 = _mm256_loadu_ps(&in[i+8]);

        // (x - min) * scale
        f0 = _mm256_sub_ps(f0, min_vec);
        f0 = _mm256_mul_ps(f0, scale_vec);
        
        f1 = _mm256_sub_ps(f1, min_vec);
        f1 = _mm256_mul_ps(f1, scale_vec);

        // Convert to int32 (round to nearest)
        __m256i i0 = _mm256_cvtps_epi32(f0);
        __m256i i1 = _mm256_cvtps_epi32(f1);

        // Pack 16 int32s into 16 bytes
        // Each _mm256_packus_epi32 packs two 256-bit vectors of i32 into one 256-bit vector of u16
        // but it does it per-lane. We need to be careful with the order if we cared about indexing,
        // but since we are just storing it doesn't matter much as long as query kernel matches.
        // Actually, let's use a simpler 128-bit packing for 8 at a time to stay safe.
        __m128i low0 = _mm256_castsi256_si128(i0);
        __m128i high0 = _mm256_extracti128_si256(i0, 1);
        __m128i p16_0 = _mm_packus_epi32(low0, high0);
        
        __m128i low1 = _mm256_castsi256_si128(i1);
        __m128i high1 = _mm256_extracti128_si256(i1, 1);
        __m128i p16_1 = _mm_packus_epi32(low1, high1);

        // Pack u16 to u8
        __m128i p8 = _mm_packus_epi16(p16_0, p16_1);
        
        _mm_storeu_si128((__m128i*)&out[i], p8);
    }
    
    for (size_t i = vec_count; i < count; i++) {
        float val = (in[i] - min_val) * inv_scale;
        if (val < 0) val = 0;
        if (val > 255) val = 255;
        out[i] = (uint8_t)(val + 0.5f);
    }
#else
    for (size_t i = 0; i < count; i++) {
        float val = (in[i] - min_val) * inv_scale;
        if (val < 0) val = 0;
        if (val > 255) val = 255;
        out[i] = (uint8_t)(val + 0.5f);
    }
#endif
}

void fp_query_gemv_quantized_f32_u8(
    const uint8_t** columns_u8,
    const float* scaled_query,
    float bias,
    float* scores_out,
    size_t count,
    size_t dim
) {
    // Init scores with bias
    for (size_t i = 0; i < count; i++) {
        scores_out[i] = bias;
    }

#ifdef __AVX2__
    size_t vec_count = count & ~7ULL; // Process 8 rows at a time (since we accumulate into floats)

    for (size_t d = 0; d < dim; d++) {
        const uint8_t* col = columns_u8[d];
        float q_val = scaled_query[d];
        __m256 q_vec = _mm256_set1_ps(q_val);

        for (size_t i = 0; i < vec_count; i += 8) {
            // Load 8 bytes
            __m128i v_u8 = _mm_loadl_epi64((__m128i*)&col[i]);
            
            // Expand to 8 integers (u8 -> i32)
            __m256i v_i32 = _mm256_cvtepu8_epi32(v_u8);
            
            // Convert to 8 floats
            __m256 v_f32 = _mm256_cvtepi32_ps(v_i32);
            
            // FMA (or mul + add)
            __m256 acc = _mm256_loadu_ps(&scores_out[i]);
#if defined(__FMA__)
            acc = _mm256_fmadd_ps(v_f32, q_vec, acc);
#else
            acc = _mm256_add_ps(acc, _mm256_mul_ps(v_f32, q_vec));
#endif
            _mm256_storeu_ps(&scores_out[i], acc);
        }

        for (size_t i = vec_count; i < count; i++) {
            scores_out[i] += (float)col[i] * q_val;
        }
    }
#else
    for (size_t d = 0; d < dim; d++) {
        const uint8_t* col = columns_u8[d];
        float q_val = scaled_query[d];
        for (size_t i = 0; i < count; i++) {
            scores_out[i] += (float)col[i] * q_val;
        }
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

/**
 * fp_query_gemv_f32_batch
 * Process a contiguous matrix of f32 vectors against a single query.
 */
void fp_query_gemv_f32_batch(
    const float* db_vectors, 
    const float* query, 
    float* scores_out, 
    size_t count, 
    size_t dim
) {
    if (!db_vectors || !query || !scores_out || count == 0 || dim == 0) return;

    for (size_t i = 0; i < count; i++) {
        const float* chunk_ptr = db_vectors + (i * dim);
        scores_out[i] = fp_dot_product_f32_avx2(chunk_ptr, query, dim);
    }
}

void fp_query_gemv_columnar_f32(
    const float** columns,
    const float* query,
    float* scores_out,
    size_t count,
    size_t dim
) {
    if (!scores_out || !columns || !query || count == 0 || dim == 0) return;
    memset(scores_out, 0, count * sizeof(float));

#ifdef __AVX2__
    size_t vec_count = count & ~7ULL; // Process 8 floats at a time

    for (size_t d = 0; d < dim; d++) {
        const float* col = columns[d];
        float q = query[d];
        __m256 q_vec = _mm256_set1_ps(q);

        for (size_t i = 0; i < vec_count; i += 8) {
            __m256 c = _mm256_loadu_ps(&col[i]);
            __m256 acc = _mm256_loadu_ps(&scores_out[i]);
#if defined(__FMA__)
            acc = _mm256_fmadd_ps(c, q_vec, acc);
#else
            acc = _mm256_add_ps(acc, _mm256_mul_ps(c, q_vec));
#endif
            _mm256_storeu_ps(&scores_out[i], acc);
        }

        for (size_t i = vec_count; i < count; i++) {
            scores_out[i] += col[i] * q;
        }
    }
#else
    for (size_t d = 0; d < dim; d++) {
        const float* col = columns[d];
        float q = query[d];
        for (size_t i = 0; i < count; i++) {
            scores_out[i] += col[i] * q;
        }
    }
#endif
}


/**
 * fp_vector_sum_f32
 * Sums an array of vectors into a single vector.
 */
void fp_vector_sum_f32(
    const float* input_vectors,
    float* output,
    size_t count,
    size_t dim
) {
    if (!input_vectors || !output || count == 0 || dim == 0) return;

    // Initialize output with zeros
    memset(output, 0, dim * sizeof(float));

    // Use scalar loop for stability
    for (size_t i = 0; i < count; i++) {
        const float* vec_ptr = input_vectors + (i * dim);
        for (size_t d = 0; d < dim; d++) {
            output[d] += vec_ptr[d];
        }
    }
}

void fp_query_gemv_bitmasked_f32(
    const float** columns,
    const float* query,
    const uint64_t* bitmap,
    float* scores_out,
    size_t count,
    size_t dim
) {
    if (!scores_out || !columns || !query || !bitmap || count == 0 || dim == 0) return;
    
    // Scalar implementation
    for (size_t i = 0; i < count; i++) {
        // Check bit
        size_t word_idx = i / 64;
        size_t bit_idx = i % 64;
        int is_set = (bitmap[word_idx] & (1ULL << bit_idx)) ? 1 : 0;

        if (is_set) {
            float dot = 0.0f;
            for (size_t d = 0; d < dim; d++) {
                dot += columns[d][i] * query[d];
            }
            scores_out[i] = dot;
        } else {
            scores_out[i] = -1.0e30f; // Large negative number instead of -INFINITY
        }
    }
}
