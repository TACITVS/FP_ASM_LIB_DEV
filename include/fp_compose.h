/**
 * fp_compose.h - Function Composition & Pipeline Utilities
 * 
 * Brings Haskell-style composition and pipelines to C.
 * Enables beautiful, declarative data transformations.
 * 
 * Features:
 * - Function composition (f . g)
 * - Pipeline builders (data |> transform |> reduce)
 * - Transducers (single-pass multi-operation)
 * - Common combinators (id, const, flip)
 * 
 * Example:
 *   auto pipeline = fp_pipeline_f64(data, n)
 *       ->map(square)
 *       ->filter(is_positive)
 *       ->reduce(fp_reduce_add_f64);
 *   double result = pipeline->execute();
 */

#ifndef FP_COMPOSE_H
#define FP_COMPOSE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* ============================================================================
 * BASIC COMBINATORS (Haskell/ML equivalents)
 * ============================================================================ */

// Identity: id x = x
typedef struct {
    double (*f64)(double);
    int64_t (*i64)(int64_t);
} fp_id_t;

extern const fp_id_t fp_id;

// Constant: const x y = x
typedef struct {
    double value_f64;
    int64_t value_i64;
} fp_const_t;

fp_const_t fp_const_f64(double value);
fp_const_t fp_const_i64(int64_t value);

// Flip: flip f x y = f y x
// Note: C lacks closures, so flip returns a struct with context
typedef double (*fp_binary_f64_t)(double, double);
typedef int64_t (*fp_binary_i64_t)(int64_t, int64_t);

// Flipped function container (holds original function for context-based call)
typedef struct {
    fp_binary_f64_t original;
} fp_flipped_f64_t;

typedef struct {
    fp_binary_i64_t original;
} fp_flipped_i64_t;

// Create flipped function wrapper
fp_flipped_f64_t fp_flip_f64(fp_binary_f64_t fn);
fp_flipped_i64_t fp_flip_i64(fp_binary_i64_t fn);

// Apply flipped function: fp_apply_flip(flip(f), x, y) == f(y, x)
double fp_apply_flip_f64(fp_flipped_f64_t flipped, double x, double y);
int64_t fp_apply_flip_i64(fp_flipped_i64_t flipped, int64_t x, int64_t y);

/* ============================================================================
 * FUNCTION COMPOSITION (f . g)
 * ============================================================================ */

// Unary composition: (f . g)(x) = f(g(x))
typedef struct {
    double (*unary_f64)(double);
    int64_t (*unary_i64)(int64_t);
    void* context;
} fp_composed_unary_t;

fp_composed_unary_t fp_compose_f64(
    double (*f)(double),
    double (*g)(double)
);

fp_composed_unary_t fp_compose_i64(
    int64_t (*f)(int64_t),
    int64_t (*g)(int64_t)
);

// Compose N functions: f . g . h . ...
typedef struct {
    double (**functions_f64)(double);
    int64_t (**functions_i64)(int64_t);
    size_t count;
} fp_composed_chain_t;

fp_composed_chain_t fp_compose_chain_f64(double (**functions)(double), size_t count);
fp_composed_chain_t fp_compose_chain_i64(int64_t (**functions)(int64_t), size_t count);

double fp_apply_composed_f64(fp_composed_chain_t* chain, double x);
int64_t fp_apply_composed_i64(fp_composed_chain_t* chain, int64_t x);

/* ============================================================================
 * PIPELINE BUILDER (Fluent API)
 * ============================================================================ */

// Pipeline operation types
typedef enum {
    FP_PIPE_MAP,
    FP_PIPE_FILTER,
    FP_PIPE_TAKE,
    FP_PIPE_DROP,
    FP_PIPE_REDUCE,
    FP_PIPE_SCAN
} fp_pipe_op_type_t;

// Forward declarations
typedef struct fp_pipeline_f64_t fp_pipeline_f64_t;
typedef struct fp_pipeline_i64_t fp_pipeline_i64_t;

// Pipeline operation
typedef struct fp_pipe_op_t {
    fp_pipe_op_type_t type;
    void* function;
    void* context;
    union {
        size_t count;  // For take/drop
        double init_f64;
        int64_t init_i64;
    } params;
    struct fp_pipe_op_t* next;
} fp_pipe_op_t;

// f64 Pipeline
struct fp_pipeline_f64_t {
    const double* input;
    size_t input_size;
    fp_pipe_op_t* first_op;
    fp_pipe_op_t* last_op;
    
    // Fluent API methods
    fp_pipeline_f64_t* (*map)(fp_pipeline_f64_t*, double (*fn)(double, void*), void* ctx);
    fp_pipeline_f64_t* (*filter)(fp_pipeline_f64_t*, bool (*pred)(double, void*), void* ctx);
    fp_pipeline_f64_t* (*take)(fp_pipeline_f64_t*, size_t n);
    fp_pipeline_f64_t* (*drop)(fp_pipeline_f64_t*, size_t n);
    
    // Terminal operations
    double (*reduce)(fp_pipeline_f64_t*, double init, double (*fn)(double, double, void*), void* ctx);
    size_t (*to_array)(fp_pipeline_f64_t*, double* output, size_t max_size);
    void (*foreach)(fp_pipeline_f64_t*, void (*fn)(double, void*), void* ctx);
};

// i64 Pipeline
struct fp_pipeline_i64_t {
    const int64_t* input;
    size_t input_size;
    fp_pipe_op_t* first_op;
    fp_pipe_op_t* last_op;
    
    // Fluent API methods
    fp_pipeline_i64_t* (*map)(fp_pipeline_i64_t*, int64_t (*fn)(int64_t, void*), void* ctx);
    fp_pipeline_i64_t* (*filter)(fp_pipeline_i64_t*, bool (*pred)(int64_t, void*), void* ctx);
    fp_pipeline_i64_t* (*take)(fp_pipeline_i64_t*, size_t n);
    fp_pipeline_i64_t* (*drop)(fp_pipeline_i64_t*, size_t n);
    
    // Terminal operations
    int64_t (*reduce)(fp_pipeline_i64_t*, int64_t init, int64_t (*fn)(int64_t, int64_t, void*), void* ctx);
    size_t (*to_array)(fp_pipeline_i64_t*, int64_t* output, size_t max_size);
    void (*foreach)(fp_pipeline_i64_t*, void (*fn)(int64_t, void*), void* ctx);
};

// Pipeline constructors
fp_pipeline_f64_t* fp_pipeline_f64(const double* input, size_t n);
fp_pipeline_i64_t* fp_pipeline_i64(const int64_t* input, size_t n);

// Pipeline cleanup
void fp_pipeline_free_f64(fp_pipeline_f64_t* pipeline);
void fp_pipeline_free_i64(fp_pipeline_i64_t* pipeline);

/* ============================================================================
 * TRANSDUCERS (Single-pass composition)
 * ============================================================================ */

// Transducer: composable transformations without intermediate arrays
typedef struct {
    void* state;
    void (*step)(void* state, const void* elem);
    void* (*complete)(void* state);
} fp_transducer_t;

// Create transducers
fp_transducer_t fp_mapping_f64(double (*fn)(double));
fp_transducer_t fp_filtering_f64(bool (*pred)(double));
fp_transducer_t fp_taking_f64(size_t n);

// Compose transducers
fp_transducer_t fp_compose_transducers(fp_transducer_t* transducers, size_t count);

// Apply transducer
void fp_transduce_f64(
    const double* input,
    size_t n,
    fp_transducer_t transducer,
    double init,
    double (*reducer)(double acc, double x)
);

/* ============================================================================
 * PARTIAL APPLICATION (Currying)
 * ============================================================================ */

// Partially applied map
typedef struct {
    double (*transform)(double, void*);
    void* context;
} fp_partial_map_f64_t;

fp_partial_map_f64_t fp_curry_map_f64(double (*fn)(double, void*), void* ctx);
void fp_apply_partial_map_f64(fp_partial_map_f64_t partial, const double* in, double* out, size_t n);

// Partially applied filter
typedef struct {
    bool (*predicate)(double, void*);
    void* context;
} fp_partial_filter_f64_t;

fp_partial_filter_f64_t fp_curry_filter_f64(bool (*pred)(double, void*), void* ctx);
size_t fp_apply_partial_filter_f64(fp_partial_filter_f64_t partial, const double* in, double* out, size_t n);

/* ============================================================================
 * LAZY EVALUATION
 * ============================================================================ */

// Lazy sequence (infinite or finite)
typedef struct fp_lazy_seq_t {
    double (*next)(struct fp_lazy_seq_t*);
    bool (*has_next)(struct fp_lazy_seq_t*);
    void* state;
    void (*cleanup)(struct fp_lazy_seq_t*);
} fp_lazy_seq_t;

// Create lazy sequences
fp_lazy_seq_t* fp_lazy_range_f64(double start, double end, double step);
fp_lazy_seq_t* fp_lazy_iterate_f64(double init, double (*fn)(double));
fp_lazy_seq_t* fp_lazy_from_array_f64(const double* array, size_t n);

// Lazy map/filter (returns new lazy sequence)
fp_lazy_seq_t* fp_lazy_map_f64(fp_lazy_seq_t* seq, double (*fn)(double));
fp_lazy_seq_t* fp_lazy_filter_f64(fp_lazy_seq_t* seq, bool (*pred)(double));
fp_lazy_seq_t* fp_lazy_take_f64(fp_lazy_seq_t* seq, size_t n);

// Force evaluation
double* fp_lazy_to_array_f64(fp_lazy_seq_t* seq, size_t max_size, size_t* out_size);
void fp_lazy_free_f64(fp_lazy_seq_t* seq);

#endif /* FP_COMPOSE_H */
