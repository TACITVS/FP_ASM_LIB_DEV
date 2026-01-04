#ifndef FP_HNSW_H
#define FP_HNSW_H

#include <stdint.h>
#include <stddef.h>

/**
 * HNSW Node structure.
 * Each node represents a vector in the database.
 */
typedef struct {
    int32_t vector_idx;     // Index into the database's columnar storage
    int32_t num_layers;     // Number of layers this node belongs to
    int32_t** neighbors;    // Array of neighbor indices per layer
    int32_t* neighbor_counts; // Number of neighbors per layer
} fp_hnsw_node;

/**
 * HNSW Index structure.
 */
typedef struct {
    int32_t dim;            // Vector dimension
    int32_t M;              // Max number of neighbors per node
    int32_t ef_construction; // Search depth during construction
    int32_t max_layers;     // Max layers allowed
    int32_t entry_point;    // Node index of the entry point
    int32_t current_max_layer;
    
    fp_hnsw_node* nodes;    // Array of nodes
    size_t count;           // Number of nodes currently in index
    size_t capacity;        // Max nodes
} fp_hnsw_index;

/**
 * Initialize a new HNSW index.
 */
fp_hnsw_index* fp_hnsw_create(int32_t dim, int32_t M, int32_t ef_construction, size_t capacity);

/**
 * Free HNSW index.
 */
void fp_hnsw_free(fp_hnsw_index* index);

/**
 * Insert a vector into the HNSW index.
 * Note: Caller must provide the vector data since HNSW only stores indices.
 */
void fp_hnsw_insert(fp_hnsw_index* index, int32_t vector_idx, const double* vector, const double** db_columns, size_t db_count);

/**
 * Search the HNSW index.
 */
size_t fp_hnsw_search(
    const fp_hnsw_index* index,
    const double* query_vec,
    int32_t k,
    int32_t ef_search,
    const double** db_columns,
    size_t db_count,
    int32_t* results_indices,
    double* results_scores
);

#endif /* FP_HNSW_H */
