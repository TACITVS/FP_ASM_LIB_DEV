#ifndef FP_QUERY_H
#define FP_QUERY_H

#include <stddef.h>
#include <stdint.h>

/**
 * Columnar matrix-vector multiply for vector similarity search.
 * Computes scores = columns^T * query in a single pass.
 *
 * @param columns     Array of dim column pointers, each pointing to count doubles
 * @param query       Query vector (dim doubles, should be normalized)
 * @param scores_out  Output buffer (count doubles)
 * @param count       Number of vectors (rows)
 * @param dim         Vector dimension (number of columns)
 */
void fp_query_gemv_columnar_f64(
    const double** columns,
    const double* query,
    double* scores_out,
    size_t count,
    size_t dim
);

/**
 * Batch version of columnar GEMV similarity search.
 * Computes scores for multiple query vectors in a single pass over the data.
 * This is much more efficient than multiple calls as it improves cache reuse.
 *
 * @param columns       Array of dim column pointers
 * @param queries       Array of query vectors (batch_count * dim doubles)
 * @param batch_count   Number of query vectors
 * @param scores_out    Output buffer (batch_count * count doubles)
 * @param count         Number of vectors in DB
 * @param dim           Vector dimension
 */
void fp_query_gemv_columnar_batch_f64(
    const double** columns,
    const double* queries,
    size_t batch_count,
    double* scores_out,
    size_t count,
    size_t dim
);

/**
 * Same as above but with candidate mask for IVF search.
 * Only computes scores for indices where mask[i] != 0.
 * Other scores are set to -INFINITY.
 *
 * @param columns     Array of dim column pointers
 * @param query       Query vector (dim doubles)
 * @param mask        Candidate mask (count bytes, non-zero = compute)
 * @param scores_out  Output buffer (count doubles)
 * @param count       Number of vectors
 * @param dim         Vector dimension
 */
void fp_query_gemv_masked_f64(
    const double** columns,
    const double* query,
    const uint8_t* mask,
    double* scores_out,
    size_t count,
    size_t dim
);

/**
 * Flat binary layout version - columns stored as single contiguous binary.
 * columns_flat layout: [col0_vec0, col0_vec1, ..., col0_vecN, col1_vec0, ...]
 *
 * @param columns_flat  Contiguous column data (dim * count doubles)
 * @param query         Query vector (dim doubles)
 * @param scores_out    Output buffer (count doubles)
 * @param count         Number of vectors
 * @param dim           Vector dimension
 */
void fp_query_gemv_flat_f64(
    const double* columns_flat,
    const double* query,
    double* scores_out,
    size_t count,
    size_t dim
);

/**
 * Indexed GEMV for IVF search - only compute for specified indices.
 * This is O(num_indices * dim) instead of O(count * dim).
 *
 * @param columns       Array of dim column pointers
 * @param query         Query vector (dim doubles)
 * @param indices       Array of candidate row indices
 * @param num_indices   Number of candidate indices
 * @param scores_out    Output buffer (num_indices doubles, in same order as indices)
 * @param count         Total number of vectors (for bounds checking)
 * @param dim           Vector dimension
 */
void fp_query_gemv_indexed_f64(
    const double** columns,
    const double* query,
    const int32_t* indices,
    size_t num_indices,
    double* scores_out,
    size_t count,
    size_t dim
);

/**
 * Top-K selection from scores array.
 * Returns indices of top K scores in descending order.
 *
 * @param scores      Score array (count doubles)
 * @param count       Number of scores
 * @param k           Number of top results to return
 * @param threshold   Minimum score threshold (scores below this are ignored)
 * @param indices_out Output buffer for indices (k int32s)
 * @param scores_out  Output buffer for scores (k doubles)
 * @return            Actual number of results (may be < k if not enough above threshold)
 */
size_t fp_query_topk_f64(
    const double* scores,
    size_t count,
    size_t k,
    double threshold,
    int32_t* indices_out,
    double* scores_out
);

/**
 * Quantize a float64 array to uint8.
 * out[i] = (in[i] - min_val) * inv_scale
 *
 * @param in         Input array (count doubles)
 * @param out        Output array (count bytes)
 * @param count      Number of elements
 * @param min_val    Minimum value (mapped to 0)
 * @param inv_scale  1.0 / (scale), factor to multiply
 */
void fp_quantize_f64_to_u8(
    const double* in,
    uint8_t* out,
    size_t count,
    double min_val,
    double inv_scale
);

/**
 * Columnar GEMV for Quantized (u8) Data.
 *
 * @param columns_u8    Array of dim column pointers (u8 data)
 * @param scaled_query  Query vector pre-scaled by quantization scale factor (dim doubles)
 * @param bias          Precomputed bias: sum(query[d] * min[d])
 * @param scores_out    Output scores (accumulates into existing values or use memset 0)
 * @param count         Number of vectors
 * @param dim           Dimensions
 */
void fp_query_gemv_quantized_f64_u8(
    const uint8_t** columns_u8,
    const double* scaled_query,
    double bias,
    double* scores_out,
    size_t count,
    size_t dim
);

/**
 * Sparse dot product.
 * Computes dot product between two sparse vectors.
 * Indices must be sorted.
 */
double fp_sparse_dotp_f64(
    const int32_t* indices_a,
    const double* values_a,
    size_t len_a,
    const int32_t* indices_b,
    const double* values_b,
    size_t len_b
);

// ========== Float32 (f32) Versions ==========

/**
 * Columnar matrix-vector multiply for f32 vectors.
 */
void fp_query_gemv_columnar_f32(
    const float** columns,
    const float* query,
    float* scores_out,
    size_t count,
    size_t dim
);

/**
 * Batch GEMV for f32 vectors.
 *
 * @param vectors_flat  Flat array of vectors (count * dim floats, row-major)
 * @param queries       Array of query vectors (batch_count * dim floats)
 * @param batch_count   Number of query vectors
 * @param scores_out    Output scores (batch_count * count floats)
 * @param count         Number of database vectors
 * @param dim           Vector dimension
 */
void fp_query_gemv_f32_batch(
    const float* vectors_flat,
    const float* queries,
    size_t batch_count,
    float* scores_out,
    size_t count,
    size_t dim
);

/**
 * Vector sum for f32.
 * Computes element-wise sum of vectors.
 *
 * @param vectors   Array of vectors (count * dim floats)
 * @param result    Output sum vector (dim floats)
 * @param count     Number of vectors
 * @param dim       Vector dimension
 */
void fp_vector_sum_f32(
    const float* vectors,
    float* result,
    size_t count,
    size_t dim
);

/**
 * Quantize f32 array to uint8.
 */
void fp_quantize_f32_to_u8(
    const float* in,
    uint8_t* out,
    size_t count,
    float min_val,
    float inv_scale
);

/**
 * Quantized columnar GEMV for f32.
 */
void fp_query_gemv_quantized_f32_u8(
    const uint8_t** columns_u8,
    const float* scaled_query,
    float bias,
    float* scores_out,
    size_t count,
    size_t dim
);

/**
 * Bitmasked columnar GEMV for f32.
 * Similar to masked version but uses bitmap instead of byte array.
 *
 * @param columns       Array of dim column pointers
 * @param query         Query vector (dim floats)
 * @param bitmap        Bit array where 1 = compute, 0 = skip
 * @param scores_out    Output scores (count floats)
 * @param count         Number of vectors
 * @param dim           Vector dimension
 */
void fp_query_gemv_bitmasked_f32(
    const float** columns,
    const float* query,
    const uint64_t* bitmap,
    float* scores_out,
    size_t count,
    size_t dim
);

#endif /* FP_QUERY_H */
