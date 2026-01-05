#include "fp_hnsw.h"
#include "fp_query.h"
#include "fp_core.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

typedef struct {
    int32_t node_idx;
    double dist;
} hnsw_candidate;

static void swap_candidate(hnsw_candidate* a, hnsw_candidate* b) {
    hnsw_candidate tmp = *a;
    *a = *b;
    *b = tmp;
}

static void min_heap_sift_up(hnsw_candidate* heap, size_t idx) {
    while (idx > 0) {
        size_t parent = (idx - 1) / 2;
        if (heap[parent].dist <= heap[idx].dist) break;
        swap_candidate(&heap[parent], &heap[idx]);
        idx = parent;
    }
}

static void min_heap_sift_down(hnsw_candidate* heap, size_t size, size_t idx) {
    while (1) {
        size_t left = idx * 2 + 1;
        size_t right = idx * 2 + 2;
        size_t smallest = idx;

        if (left < size && heap[left].dist < heap[smallest].dist) smallest = left;
        if (right < size && heap[right].dist < heap[smallest].dist) smallest = right;
        if (smallest == idx) break;

        swap_candidate(&heap[idx], &heap[smallest]);
        idx = smallest;
    }
}

static void max_heap_sift_up(hnsw_candidate* heap, size_t idx) {
    while (idx > 0) {
        size_t parent = (idx - 1) / 2;
        if (heap[parent].dist >= heap[idx].dist) break;
        swap_candidate(&heap[parent], &heap[idx]);
        idx = parent;
    }
}

static void max_heap_sift_down(hnsw_candidate* heap, size_t size, size_t idx) {
    while (1) {
        size_t left = idx * 2 + 1;
        size_t right = idx * 2 + 2;
        size_t largest = idx;

        if (left < size && heap[left].dist > heap[largest].dist) largest = left;
        if (right < size && heap[right].dist > heap[largest].dist) largest = right;
        if (largest == idx) break;

        swap_candidate(&heap[idx], &heap[largest]);
        idx = largest;
    }
}

static void min_heap_push(hnsw_candidate* heap, size_t* size, hnsw_candidate item) {
    heap[*size] = item;
    min_heap_sift_up(heap, *size);
    (*size)++;
}

static hnsw_candidate min_heap_pop(hnsw_candidate* heap, size_t* size) {
    hnsw_candidate result = heap[0];
    (*size)--;
    if (*size > 0) {
        heap[0] = heap[*size];
        min_heap_sift_down(heap, *size, 0);
    }
    return result;
}

static void max_heap_push(hnsw_candidate* heap, size_t* size, hnsw_candidate item) {
    heap[*size] = item;
    max_heap_sift_up(heap, *size);
    (*size)++;
}

static void max_heap_replace_root(hnsw_candidate* heap, size_t size, hnsw_candidate item) {
    if (size == 0) return;
    heap[0] = item;
    max_heap_sift_down(heap, size, 0);
}

static hnsw_candidate max_heap_pop(hnsw_candidate* heap, size_t* size) {
    hnsw_candidate result = heap[0];
    (*size)--;
    if (*size > 0) {
        heap[0] = heap[*size];
        max_heap_sift_down(heap, *size, 0);
    }
    return result;
}

static int ensure_candidate_capacity(hnsw_candidate** heap, size_t* capacity, size_t needed) {
    if (needed <= *capacity) return 1;
    size_t new_capacity = (*capacity == 0) ? 32 : *capacity;
    while (new_capacity < needed) {
        new_capacity *= 2;
    }
    hnsw_candidate* resized = (hnsw_candidate*)realloc(*heap, new_capacity * sizeof(hnsw_candidate));
    if (!resized) return 0;
    *heap = resized;
    *capacity = new_capacity;
    return 1;
}

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
    double dot = fp_fold_dotp_f64(a, b, (size_t)dim);
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

static size_t search_layer_best(
    const fp_hnsw_index* index,
    const double* query_vec,
    int32_t entry_point_node_idx,
    int32_t layer,
    int32_t ef,
    const double** db_columns,
    int32_t* out_node_indices,
    double* out_dists
) {
    if (index->count == 0) return 0;
    if (ef < 1) ef = 1;
    if ((size_t)ef > index->count) ef = (int32_t)index->count;

    uint8_t* visited = (uint8_t*)calloc(index->count, 1);
    if (!visited) return 0;

    hnsw_candidate* candidates = NULL;
    size_t candidate_size = 0;
    size_t candidate_capacity = 0;

    hnsw_candidate* results = (hnsw_candidate*)malloc((size_t)ef * sizeof(hnsw_candidate));
    if (!results) {
        free(visited);
        return 0;
    }
    size_t result_size = 0;

    double* node_vec = (double*)malloc(index->dim * sizeof(double));
    if (!node_vec) {
        free(visited);
        free(results);
        return 0;
    }

    get_vector(db_columns, index->dim, index->nodes[entry_point_node_idx].vector_idx, node_vec);
    double entry_dist = get_dist(query_vec, node_vec, index->dim);

    visited[entry_point_node_idx] = 1;
    if (!ensure_candidate_capacity(&candidates, &candidate_capacity, 1)) {
        free(visited);
        free(results);
        free(node_vec);
        return 0;
    }

    min_heap_push(candidates, &candidate_size, (hnsw_candidate){entry_point_node_idx, entry_dist});
    max_heap_push(results, &result_size, (hnsw_candidate){entry_point_node_idx, entry_dist});

    while (candidate_size > 0) {
        hnsw_candidate current = min_heap_pop(candidates, &candidate_size);
        if (result_size >= (size_t)ef && current.dist > results[0].dist) {
            break;
        }

        fp_hnsw_node* node = &index->nodes[current.node_idx];
        int32_t count = node->neighbor_counts[layer];
        int32_t* neighbors = node->neighbors[layer];

        for (int32_t i = 0; i < count; i++) {
            int32_t neighbor_node_idx = neighbors[i];
            if (visited[neighbor_node_idx]) continue;
            visited[neighbor_node_idx] = 1;

            get_vector(db_columns, index->dim, index->nodes[neighbor_node_idx].vector_idx, node_vec);
            double d = get_dist(query_vec, node_vec, index->dim);

            if (result_size < (size_t)ef) {
                if (!ensure_candidate_capacity(&candidates, &candidate_capacity, candidate_size + 1)) {
                    free(visited);
                    free(results);
                    free(node_vec);
                    free(candidates);
                    return 0;
                }
                min_heap_push(candidates, &candidate_size, (hnsw_candidate){neighbor_node_idx, d});
                max_heap_push(results, &result_size, (hnsw_candidate){neighbor_node_idx, d});
            } else if (d < results[0].dist) {
                if (!ensure_candidate_capacity(&candidates, &candidate_capacity, candidate_size + 1)) {
                    free(visited);
                    free(results);
                    free(node_vec);
                    free(candidates);
                    return 0;
                }
                min_heap_push(candidates, &candidate_size, (hnsw_candidate){neighbor_node_idx, d});
                max_heap_replace_root(results, result_size, (hnsw_candidate){neighbor_node_idx, d});
            }
        }
    }

    size_t out_count = result_size;
    for (size_t i = out_count; i > 0; i--) {
        hnsw_candidate item = max_heap_pop(results, &result_size);
        out_node_indices[i - 1] = item.node_idx;
        if (out_dists) out_dists[i - 1] = item.dist;
    }

    free(visited);
    free(results);
    free(node_vec);
    free(candidates);

    return out_count;
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
    
    // 2. Insert into target_level and below using ef_construction search
    int32_t max_search_level = target_level < index->current_max_layer ? target_level : index->current_max_layer;
    int32_t ef = index->ef_construction;

    int32_t* candidate_nodes = NULL;
    double* candidate_dists = NULL;
    if (ef < index->M * 2) ef = index->M * 2;

    if (ef > 0) {
        candidate_nodes = (int32_t*)malloc((size_t)ef * sizeof(int32_t));
        candidate_dists = (double*)malloc((size_t)ef * sizeof(double));
    }

    for (int32_t l = max_search_level; l >= 0; l--) {
        size_t found = 0;
        if (candidate_nodes && candidate_dists) {
            found = search_layer_best(index, vector, curr_entry_node, l, ef, db_columns, candidate_nodes, candidate_dists);
        }

        if (found == 0) {
            curr_entry_node = search_layer(index, vector, curr_entry_node, l, db_columns, NULL);
            fp_hnsw_node* neighbor_node = &index->nodes[curr_entry_node];
            int32_t max_m = (l == 0) ? index->M * 2 : index->M;

            if (new_node->neighbor_counts[l] < max_m) {
                new_node->neighbors[l][new_node->neighbor_counts[l]++] = curr_entry_node;
            }

            if (neighbor_node->neighbor_counts[l] < max_m) {
                neighbor_node->neighbors[l][neighbor_node->neighbor_counts[l]++] = node_idx;
            }
            continue;
        }

        int32_t max_m = (l == 0) ? index->M * 2 : index->M;
        size_t connect_count = found < (size_t)max_m ? found : (size_t)max_m;

        for (size_t i = 0; i < connect_count; i++) {
            int32_t neighbor_idx = candidate_nodes[i];
            fp_hnsw_node* neighbor_node = &index->nodes[neighbor_idx];

            if (new_node->neighbor_counts[l] < max_m) {
                new_node->neighbors[l][new_node->neighbor_counts[l]++] = neighbor_idx;
            }

            if (neighbor_node->neighbor_counts[l] < max_m) {
                neighbor_node->neighbors[l][neighbor_node->neighbor_counts[l]++] = node_idx;
            }
        }

        curr_entry_node = candidate_nodes[0];
    }

    free(candidate_nodes);
    free(candidate_dists);
    
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
    if (index->count == 0 || k <= 0) return 0;

    if ((size_t)k > index->count) {
        k = (int32_t)index->count;
    }

    int32_t curr_node = index->entry_point;
    double curr_dist = 0;

    // 1. Greedy search to bottom layer
    for (int32_t l = index->current_max_layer; l > 0; l--) {
        curr_node = search_layer(index, query_vec, curr_node, l, db_columns, &curr_dist);
    }

    int32_t ef = ef_search;
    if (ef < k) ef = k;
    if ((size_t)ef > index->count) ef = (int32_t)index->count;

    int32_t* candidate_nodes = (int32_t*)malloc((size_t)ef * sizeof(int32_t));
    double* candidate_dists = (double*)malloc((size_t)ef * sizeof(double));
    if (!candidate_nodes || !candidate_dists) {
        free(candidate_nodes);
        free(candidate_dists);
        return 0;
    }

    size_t found = search_layer_best(index, query_vec, curr_node, 0, ef, db_columns, candidate_nodes, candidate_dists);
    if (found == 0) {
        free(candidate_nodes);
        free(candidate_dists);
        return 0;
    }

    size_t result_count = found < (size_t)k ? found : (size_t)k;
    for (size_t i = 0; i < result_count; i++) {
        int32_t node_idx = candidate_nodes[i];
        results_indices[i] = index->nodes[node_idx].vector_idx;
        results_scores[i] = 1.0 - candidate_dists[i];
    }

    free(candidate_nodes);
    free(candidate_dists);

    return result_count;
}
