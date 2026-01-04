#include "fp_hnsw.h"
#include "fp_query.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

// Helper: Random level generation for HNSW
static int32_t random_level(int32_t M) {
    double r = (double)rand() / RAND_MAX;
    double mult = 1.0 / log(M);
    int32_t lvl = (int32_t)(-log(r) * mult);
    return lvl;
}

// Helper: Get vector from columnar DB
static void get_vector(const double** columns, size_t dim, int32_t idx, double* out) {
    for (size_t d = 0; d < dim; d++) {
        out[d] = columns[d][idx];
    }
}

// Helper: Distance calculation (Cosine similarity converted to distance)
// Since MerkleDB vectors are normalized, dot product is cosine similarity.
// Distance = 1.0 - similarity
static double get_dist(const double* a, const double* b, int32_t dim) {
    // We should use our SIMD kernels here
    // For now, simple implementation, TODO: call ASM kernel
    double dot = 0;
    for (int32_t i = 0; i < dim; i++) {
        dot += a[i] * b[i];
    }
    return 1.0 - dot;
}

fp_hnsw_index* fp_hnsw_create(int32_t dim, int32_t M, int32_t ef_construction, size_t capacity) {
    fp_hnsw_index* index = (fp_hnsw_index*)malloc(sizeof(fp_hnsw_index));
    if (!index) return NULL;

    index->dim = dim;
    index->M = M;
    index->ef_construction = ef_construction;
    index->capacity = capacity;
    index->count = 0;
    index->max_layers = 16; // Heuristic
    index->current_max_layer = -1;
    index->entry_point = -1;

    index->nodes = (fp_hnsw_node*)malloc(capacity * sizeof(fp_hnsw_node));
    if (!index->nodes) {
        free(index);
        return NULL;
    }

    return index;
}

void fp_hnsw_free(fp_hnsw_index* index) {
    if (!index) return;
    for (size_t i = 0; i < index->count; i++) {
        fp_hnsw_node* node = &index->nodes[i];
        for (int32_t l = 0; l < node->num_layers; l++) {
            free(node->neighbors[l]);
        }
        free(node->neighbors);
        free(node->neighbor_counts);
    }
    free(index->nodes);
    free(index);
}

// Search one layer greedily
static int32_t search_layer(
    const fp_hnsw_index* index,
    const double* query_vec,
    int32_t entry_point_node_idx,
    int32_t layer,
    const double** db_columns,
    double* curr_dist_out
) {
    int32_t curr_node_idx = entry_point_node_idx;
    
    double* node_vec = malloc(index->dim * sizeof(double));
    get_vector(db_columns, index->dim, index->nodes[curr_node_idx].vector_idx, node_vec);
    double curr_dist = get_dist(query_vec, node_vec, index->dim);
    
    int32_t changed = 1;
    while (changed) {
        changed = 0;
        fp_hnsw_node* node = &index->nodes[curr_node_idx];
        int32_t count = node->neighbor_counts[layer];
        int32_t* neighbors = node->neighbors[layer];
        
        for (int32_t i = 0; i < count; i++) {
            int32_t neighbor_node_idx = neighbors[i];
            get_vector(db_columns, index->dim, index->nodes[neighbor_node_idx].vector_idx, node_vec);
            double d = get_dist(query_vec, node_vec, index->dim);
            if (d < curr_dist) {
                curr_dist = d;
                curr_node_idx = neighbor_node_idx;
                changed = 1;
            }
        }
    }
    
    free(node_vec);
    if (curr_dist_out) *curr_dist_out = curr_dist;
    return curr_node_idx;
}

void fp_hnsw_insert(fp_hnsw_index* index, int32_t vector_idx, const double* vector, const double** db_columns, size_t db_count) {
    if (index->count >= index->capacity) return;

    int32_t node_idx = (int32_t)index->count;
    fp_hnsw_node* new_node = &index->nodes[node_idx];
    new_node->vector_idx = vector_idx;
    
    int32_t target_level = random_level(index->M);
    new_node->num_layers = target_level + 1;
    new_node->neighbors = (int32_t**)malloc(new_node->num_layers * sizeof(int32_t*));
    new_node->neighbor_counts = (int32_t*)malloc(new_node->num_layers * sizeof(int32_t));
    
    for (int32_t l = 0; l < new_node->num_layers; l++) {
        // Allocate space for up to M neighbors (bottom layer M*2 usually)
        int32_t max_m = (l == 0) ? index->M * 2 : index->M;
        new_node->neighbors[l] = (int32_t*)malloc(max_m * sizeof(int32_t));
        new_node->neighbor_counts[l] = 0;
    }

    if (index->entry_point == -1) {
        index->entry_point = node_idx;
        index->current_max_layer = target_level;
        index->count++;
        return;
    }

    int32_t curr_entry_node = index->entry_point;
    
    // 1. Search layers above target_level
    for (int32_t l = index->current_max_layer; l > target_level; l--) {
        curr_entry_node = search_layer(index, vector, curr_entry_node, l, db_columns, NULL);
    }
    
    // 2. Insert into target_level and below
    // Note: This is a simplified version. Real HNSW uses a priority queue for ef_construction.
    for (int32_t l = (target_level < index->current_max_layer ? target_level : index->current_max_layer); l >= 0; l--) {
        curr_entry_node = search_layer(index, vector, curr_entry_node, l, db_columns, NULL);
        
        // Add bidirectional connections
        fp_hnsw_node* neighbor_node = &index->nodes[curr_entry_node];
        int32_t max_m = (l == 0) ? index->M * 2 : index->M;
        
        if (new_node->neighbor_counts[l] < max_m) {
            new_node->neighbors[l][new_node->neighbor_counts[l]++] = curr_entry_node;
        }
        
        if (neighbor_node->neighbor_counts[l] < max_m) {
            neighbor_node->neighbors[l][neighbor_node->neighbor_counts[l]++] = node_idx;
        }
    }
    
    if (target_level > index->current_max_layer) {
        index->entry_point = node_idx;
        index->current_max_layer = target_level;
    }
    
    index->count++;
}

size_t fp_hnsw_search(
    const fp_hnsw_index* index,
    const double* query_vec,
    int32_t k,
    int32_t ef_search,
    const double** db_columns,
    size_t db_count,
    int32_t* results_indices,
    double* results_scores
) {
    if (index->count == 0) return 0;

    int32_t curr_node = index->entry_point;
    double curr_dist = 0;
    
    // 1. Greedy search to bottom layer
    for (int32_t l = index->current_max_layer; l > 0; l--) {
        curr_node = search_layer(index, query_vec, curr_node, l, db_columns, &curr_dist);
    }
    
    // 2. Search bottom layer (simplified: should use priority queue)
    curr_node = search_layer(index, query_vec, curr_node, 0, db_columns, &curr_dist);
    
    // Return top 1 for now (simplified)
    results_indices[0] = index->nodes[curr_node].vector_idx;
    results_scores[0] = 1.0 - curr_dist; // Convert back to similarity
    
    return 1;
}
