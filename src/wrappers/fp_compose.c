/**
 * fp_compose.c - Function Composition & Pipeline Implementation
 *
 * Implements Haskell-style composition, pipelines, transducers, and lazy evaluation.
 * Maintains functional programming principles while optimizing for performance.
 */

#include "../../include/fp_compose.h"
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * BASIC COMBINATORS (Haskell/ML equivalents)
 * ============================================================================ */

// Identity functions
static double fp_id_impl_f64(double x) {
    return x;
}

static int64_t fp_id_impl_i64(int64_t x) {
    return x;
}

// Global identity combinator
const fp_id_t fp_id = {
    .f64 = fp_id_impl_f64,
    .i64 = fp_id_impl_i64
};

// Constant constructors
fp_const_t fp_const_f64(double value) {
    fp_const_t result = {
        .value_f64 = value,
        .value_i64 = 0
    };
    return result;
}

fp_const_t fp_const_i64(int64_t value) {
    fp_const_t result = {
        .value_f64 = 0.0,
        .value_i64 = value
    };
    return result;
}

// Flip implementation structures
typedef struct {
    fp_binary_f64_t original;
} fp_flipped_f64_t;

typedef struct {
    fp_binary_i64_t original;
} fp_flipped_i64_t;

static double fp_flipped_call_f64(double x, double y, void* ctx) {
    fp_flipped_f64_t* flipped = (fp_flipped_f64_t*)ctx;
    return flipped->original(y, x);
}

static int64_t fp_flipped_call_i64(int64_t x, int64_t y, void* ctx) {
    fp_flipped_i64_t* flipped = (fp_flipped_i64_t*)ctx;
    return flipped->original(y, x);
}

// Note: These return function pointers that require context
// In practice, users should use the flipped_call functions directly
fp_binary_f64_t fp_flip_f64(fp_binary_f64_t fn) {
    // This is a simplified version - actual usage requires context passing
    return fn;  // Placeholder - full implementation needs closure support
}

fp_binary_i64_t fp_flip_i64(fp_binary_i64_t fn) {
    return fn;  // Placeholder - full implementation needs closure support
}

/* ============================================================================
 * FUNCTION COMPOSITION (f . g)
 * ============================================================================ */

// Composition context structures
typedef struct {
    double (*f)(double);
    double (*g)(double);
} fp_compose_ctx_f64_t;

typedef struct {
    int64_t (*f)(int64_t);
    int64_t (*g)(int64_t);
} fp_compose_ctx_i64_t;

// Composed function wrappers
static double fp_composed_f64(double x, void* ctx) {
    fp_compose_ctx_f64_t* comp = (fp_compose_ctx_f64_t*)ctx;
    return comp->f(comp->g(x));
}

static int64_t fp_composed_i64(int64_t x, void* ctx) {
    fp_compose_ctx_i64_t* comp = (fp_compose_ctx_i64_t*)ctx;
    return comp->f(comp->g(x));
}

fp_composed_unary_t fp_compose_f64(double (*f)(double), double (*g)(double)) {
    fp_compose_ctx_f64_t* ctx = malloc(sizeof(fp_compose_ctx_f64_t));
    if (!ctx) {
        fp_composed_unary_t empty = {0};
        return empty;
    }

    ctx->f = f;
    ctx->g = g;

    fp_composed_unary_t result = {
        .unary_f64 = NULL,  // Will be set by caller
        .unary_i64 = NULL,
        .context = ctx
    };
    return result;
}

fp_composed_unary_t fp_compose_i64(int64_t (*f)(int64_t), int64_t (*g)(int64_t)) {
    fp_compose_ctx_i64_t* ctx = malloc(sizeof(fp_compose_ctx_i64_t));
    if (!ctx) {
        fp_composed_unary_t empty = {0};
        return empty;
    }

    ctx->f = f;
    ctx->g = g;

    fp_composed_unary_t result = {
        .unary_f64 = NULL,
        .unary_i64 = NULL,
        .context = ctx
    };
    return result;
}

// Compose N functions
fp_composed_chain_t fp_compose_chain_f64(double (**functions)(double), size_t count) {
    fp_composed_chain_t chain = {
        .functions_f64 = malloc(sizeof(double (*)(double)) * count),
        .functions_i64 = NULL,
        .count = count
    };

    if (chain.functions_f64 && functions) {
        memcpy(chain.functions_f64, functions, sizeof(double (*)(double)) * count);
    }

    return chain;
}

fp_composed_chain_t fp_compose_chain_i64(int64_t (**functions)(int64_t), size_t count) {
    fp_composed_chain_t chain = {
        .functions_f64 = NULL,
        .functions_i64 = malloc(sizeof(int64_t (*)(int64_t)) * count),
        .count = count
    };

    if (chain.functions_i64 && functions) {
        memcpy(chain.functions_i64, functions, sizeof(int64_t (*)(int64_t)) * count);
    }

    return chain;
}

double fp_apply_composed_f64(fp_composed_chain_t* chain, double x) {
    if (!chain || !chain->functions_f64 || chain->count == 0) {
        return x;
    }

    double result = x;
    // Apply functions in reverse order: (f . g . h)(x) = f(g(h(x)))
    for (int i = (int)chain->count - 1; i >= 0; i--) {
        result = chain->functions_f64[i](result);
    }

    return result;
}

int64_t fp_apply_composed_i64(fp_composed_chain_t* chain, int64_t x) {
    if (!chain || !chain->functions_i64 || chain->count == 0) {
        return x;
    }

    int64_t result = x;
    for (int i = (int)chain->count - 1; i >= 0; i--) {
        result = chain->functions_i64[i](result);
    }

    return result;
}

/* ============================================================================
 * PIPELINE BUILDER (Fluent API)
 * ============================================================================ */

// Forward declarations for pipeline methods
static fp_pipeline_f64_t* pipeline_map_f64(fp_pipeline_f64_t* self, double (*fn)(double, void*), void* ctx);
static fp_pipeline_f64_t* pipeline_filter_f64(fp_pipeline_f64_t* self, bool (*pred)(double, void*), void* ctx);
static fp_pipeline_f64_t* pipeline_take_f64(fp_pipeline_f64_t* self, size_t n);
static fp_pipeline_f64_t* pipeline_drop_f64(fp_pipeline_f64_t* self, size_t n);
static double pipeline_reduce_f64(fp_pipeline_f64_t* self, double init, double (*fn)(double, double, void*), void* ctx);
static size_t pipeline_to_array_f64(fp_pipeline_f64_t* self, double* output, size_t max_size);
static void pipeline_foreach_f64(fp_pipeline_f64_t* self, void (*fn)(double, void*), void* ctx);

static fp_pipeline_i64_t* pipeline_map_i64(fp_pipeline_i64_t* self, int64_t (*fn)(int64_t, void*), void* ctx);
static fp_pipeline_i64_t* pipeline_filter_i64(fp_pipeline_i64_t* self, bool (*pred)(int64_t, void*), void* ctx);
static fp_pipeline_i64_t* pipeline_take_i64(fp_pipeline_i64_t* self, size_t n);
static fp_pipeline_i64_t* pipeline_drop_i64(fp_pipeline_i64_t* self, size_t n);
static int64_t pipeline_reduce_i64(fp_pipeline_i64_t* self, int64_t init, int64_t (*fn)(int64_t, int64_t, void*), void* ctx);
static size_t pipeline_to_array_i64(fp_pipeline_i64_t* self, int64_t* output, size_t max_size);
static void pipeline_foreach_i64(fp_pipeline_i64_t* self, void (*fn)(int64_t, void*), void* ctx);

// Pipeline constructors
fp_pipeline_f64_t* fp_pipeline_f64(const double* input, size_t n) {
    fp_pipeline_f64_t* pipeline = malloc(sizeof(fp_pipeline_f64_t));
    if (!pipeline) return NULL;

    pipeline->input = input;
    pipeline->input_size = n;
    pipeline->first_op = NULL;
    pipeline->last_op = NULL;

    // Assign method pointers
    pipeline->map = pipeline_map_f64;
    pipeline->filter = pipeline_filter_f64;
    pipeline->take = pipeline_take_f64;
    pipeline->drop = pipeline_drop_f64;
    pipeline->reduce = pipeline_reduce_f64;
    pipeline->to_array = pipeline_to_array_f64;
    pipeline->foreach = pipeline_foreach_f64;

    return pipeline;
}

fp_pipeline_i64_t* fp_pipeline_i64(const int64_t* input, size_t n) {
    fp_pipeline_i64_t* pipeline = malloc(sizeof(fp_pipeline_i64_t));
    if (!pipeline) return NULL;

    pipeline->input = input;
    pipeline->input_size = n;
    pipeline->first_op = NULL;
    pipeline->last_op = NULL;

    // Assign method pointers
    pipeline->map = pipeline_map_i64;
    pipeline->filter = pipeline_filter_i64;
    pipeline->take = pipeline_take_i64;
    pipeline->drop = pipeline_drop_i64;
    pipeline->reduce = pipeline_reduce_i64;
    pipeline->to_array = pipeline_to_array_i64;
    pipeline->foreach = pipeline_foreach_i64;

    return pipeline;
}

// Helper to add operation to pipeline
static void add_operation(fp_pipe_op_t** first, fp_pipe_op_t** last, fp_pipe_op_t* op) {
    if (*first == NULL) {
        *first = op;
        *last = op;
    } else {
        (*last)->next = op;
        *last = op;
    }
}

// Pipeline methods - f64
static fp_pipeline_f64_t* pipeline_map_f64(fp_pipeline_f64_t* self, double (*fn)(double, void*), void* ctx) {
    fp_pipe_op_t* op = malloc(sizeof(fp_pipe_op_t));
    if (!op) return self;

    op->type = FP_PIPE_MAP;
    op->function = (void*)fn;
    op->context = ctx;
    op->next = NULL;

    add_operation(&self->first_op, &self->last_op, op);
    return self;
}

static fp_pipeline_f64_t* pipeline_filter_f64(fp_pipeline_f64_t* self, bool (*pred)(double, void*), void* ctx) {
    fp_pipe_op_t* op = malloc(sizeof(fp_pipe_op_t));
    if (!op) return self;

    op->type = FP_PIPE_FILTER;
    op->function = (void*)pred;
    op->context = ctx;
    op->next = NULL;

    add_operation(&self->first_op, &self->last_op, op);
    return self;
}

static fp_pipeline_f64_t* pipeline_take_f64(fp_pipeline_f64_t* self, size_t n) {
    fp_pipe_op_t* op = malloc(sizeof(fp_pipe_op_t));
    if (!op) return self;

    op->type = FP_PIPE_TAKE;
    op->function = NULL;
    op->context = NULL;
    op->params.count = n;
    op->next = NULL;

    add_operation(&self->first_op, &self->last_op, op);
    return self;
}

static fp_pipeline_f64_t* pipeline_drop_f64(fp_pipeline_f64_t* self, size_t n) {
    fp_pipe_op_t* op = malloc(sizeof(fp_pipe_op_t));
    if (!op) return self;

    op->type = FP_PIPE_DROP;
    op->function = NULL;
    op->context = NULL;
    op->params.count = n;
    op->next = NULL;

    add_operation(&self->first_op, &self->last_op, op);
    return self;
}

static double pipeline_reduce_f64(fp_pipeline_f64_t* self, double init, double (*fn)(double, double, void*), void* ctx) {
    if (!self || !self->input) return init;

    // Allocate working buffer for pipeline operations
    double* buffer1 = malloc(self->input_size * sizeof(double));
    double* buffer2 = malloc(self->input_size * sizeof(double));
    if (!buffer1 || !buffer2) {
        free(buffer1);
        free(buffer2);
        return init;
    }

    // Copy input to buffer1
    memcpy(buffer1, self->input, self->input_size * sizeof(double));
    size_t current_size = self->input_size;
    double* current = buffer1;
    double* next = buffer2;

    // Execute pipeline operations
    fp_pipe_op_t* op = self->first_op;
    while (op) {
        switch (op->type) {
            case FP_PIPE_MAP: {
                double (*map_fn)(double, void*) = (double (*)(double, void*))op->function;
                for (size_t i = 0; i < current_size; i++) {
                    next[i] = map_fn(current[i], op->context);
                }
                break;
            }

            case FP_PIPE_FILTER: {
                bool (*pred)(double, void*) = (bool (*)(double, void*))op->function;
                size_t write_idx = 0;
                for (size_t i = 0; i < current_size; i++) {
                    if (pred(current[i], op->context)) {
                        next[write_idx++] = current[i];
                    }
                }
                current_size = write_idx;
                break;
            }

            case FP_PIPE_TAKE: {
                size_t take_count = (op->params.count < current_size) ? op->params.count : current_size;
                memcpy(next, current, take_count * sizeof(double));
                current_size = take_count;
                break;
            }

            case FP_PIPE_DROP: {
                if (op->params.count < current_size) {
                    size_t remaining = current_size - op->params.count;
                    memcpy(next, current + op->params.count, remaining * sizeof(double));
                    current_size = remaining;
                } else {
                    current_size = 0;
                }
                break;
            }

            default:
                break;
        }

        // Swap buffers
        double* temp = current;
        current = next;
        next = temp;

        op = op->next;
    }

    // Perform reduction
    double result = init;
    if (fn) {
        for (size_t i = 0; i < current_size; i++) {
            result = fn(result, current[i], ctx);
        }
    }

    free(buffer1);
    free(buffer2);
    return result;
}

static size_t pipeline_to_array_f64(fp_pipeline_f64_t* self, double* output, size_t max_size) {
    if (!self || !self->input || !output) return 0;

    double* buffer1 = malloc(self->input_size * sizeof(double));
    double* buffer2 = malloc(self->input_size * sizeof(double));
    if (!buffer1 || !buffer2) {
        free(buffer1);
        free(buffer2);
        return 0;
    }

    memcpy(buffer1, self->input, self->input_size * sizeof(double));
    size_t current_size = self->input_size;
    double* current = buffer1;
    double* next = buffer2;

    // Execute pipeline (same as reduce, but return array)
    fp_pipe_op_t* op = self->first_op;
    while (op) {
        switch (op->type) {
            case FP_PIPE_MAP: {
                double (*map_fn)(double, void*) = (double (*)(double, void*))op->function;
                for (size_t i = 0; i < current_size; i++) {
                    next[i] = map_fn(current[i], op->context);
                }
                break;
            }

            case FP_PIPE_FILTER: {
                bool (*pred)(double, void*) = (bool (*)(double, void*))op->function;
                size_t write_idx = 0;
                for (size_t i = 0; i < current_size; i++) {
                    if (pred(current[i], op->context)) {
                        next[write_idx++] = current[i];
                    }
                }
                current_size = write_idx;
                break;
            }

            case FP_PIPE_TAKE: {
                size_t take_count = (op->params.count < current_size) ? op->params.count : current_size;
                memcpy(next, current, take_count * sizeof(double));
                current_size = take_count;
                break;
            }

            case FP_PIPE_DROP: {
                if (op->params.count < current_size) {
                    size_t remaining = current_size - op->params.count;
                    memcpy(next, current + op->params.count, remaining * sizeof(double));
                    current_size = remaining;
                } else {
                    current_size = 0;
                }
                break;
            }

            default:
                break;
        }

        double* temp = current;
        current = next;
        next = temp;

        op = op->next;
    }

    size_t copy_size = (current_size < max_size) ? current_size : max_size;
    memcpy(output, current, copy_size * sizeof(double));

    free(buffer1);
    free(buffer2);
    return copy_size;
}

static void pipeline_foreach_f64(fp_pipeline_f64_t* self, void (*fn)(double, void*), void* ctx) {
    if (!self || !self->input || !fn) return;

    double* buffer = malloc(self->input_size * sizeof(double));
    if (!buffer) return;

    size_t result_size = pipeline_to_array_f64(self, buffer, self->input_size);

    for (size_t i = 0; i < result_size; i++) {
        fn(buffer[i], ctx);
    }

    free(buffer);
}

// Pipeline methods - i64 (similar structure to f64)
static fp_pipeline_i64_t* pipeline_map_i64(fp_pipeline_i64_t* self, int64_t (*fn)(int64_t, void*), void* ctx) {
    fp_pipe_op_t* op = malloc(sizeof(fp_pipe_op_t));
    if (!op) return self;

    op->type = FP_PIPE_MAP;
    op->function = (void*)fn;
    op->context = ctx;
    op->next = NULL;

    add_operation(&self->first_op, &self->last_op, op);
    return self;
}

static fp_pipeline_i64_t* pipeline_filter_i64(fp_pipeline_i64_t* self, bool (*pred)(int64_t, void*), void* ctx) {
    fp_pipe_op_t* op = malloc(sizeof(fp_pipe_op_t));
    if (!op) return self;

    op->type = FP_PIPE_FILTER;
    op->function = (void*)pred;
    op->context = ctx;
    op->next = NULL;

    add_operation(&self->first_op, &self->last_op, op);
    return self;
}

static fp_pipeline_i64_t* pipeline_take_i64(fp_pipeline_i64_t* self, size_t n) {
    fp_pipe_op_t* op = malloc(sizeof(fp_pipe_op_t));
    if (!op) return self;

    op->type = FP_PIPE_TAKE;
    op->function = NULL;
    op->context = NULL;
    op->params.count = n;
    op->next = NULL;

    add_operation(&self->first_op, &self->last_op, op);
    return self;
}

static fp_pipeline_i64_t* pipeline_drop_i64(fp_pipeline_i64_t* self, size_t n) {
    fp_pipe_op_t* op = malloc(sizeof(fp_pipe_op_t));
    if (!op) return self;

    op->type = FP_PIPE_DROP;
    op->function = NULL;
    op->context = NULL;
    op->params.count = n;
    op->next = NULL;

    add_operation(&self->first_op, &self->last_op, op);
    return self;
}

static int64_t pipeline_reduce_i64(fp_pipeline_i64_t* self, int64_t init, int64_t (*fn)(int64_t, int64_t, void*), void* ctx) {
    if (!self || !self->input) return init;

    int64_t* buffer1 = malloc(self->input_size * sizeof(int64_t));
    int64_t* buffer2 = malloc(self->input_size * sizeof(int64_t));
    if (!buffer1 || !buffer2) {
        free(buffer1);
        free(buffer2);
        return init;
    }

    memcpy(buffer1, self->input, self->input_size * sizeof(int64_t));
    size_t current_size = self->input_size;
    int64_t* current = buffer1;
    int64_t* next = buffer2;

    fp_pipe_op_t* op = self->first_op;
    while (op) {
        switch (op->type) {
            case FP_PIPE_MAP: {
                int64_t (*map_fn)(int64_t, void*) = (int64_t (*)(int64_t, void*))op->function;
                for (size_t i = 0; i < current_size; i++) {
                    next[i] = map_fn(current[i], op->context);
                }
                break;
            }

            case FP_PIPE_FILTER: {
                bool (*pred)(int64_t, void*) = (bool (*)(int64_t, void*))op->function;
                size_t write_idx = 0;
                for (size_t i = 0; i < current_size; i++) {
                    if (pred(current[i], op->context)) {
                        next[write_idx++] = current[i];
                    }
                }
                current_size = write_idx;
                break;
            }

            case FP_PIPE_TAKE: {
                size_t take_count = (op->params.count < current_size) ? op->params.count : current_size;
                memcpy(next, current, take_count * sizeof(int64_t));
                current_size = take_count;
                break;
            }

            case FP_PIPE_DROP: {
                if (op->params.count < current_size) {
                    size_t remaining = current_size - op->params.count;
                    memcpy(next, current + op->params.count, remaining * sizeof(int64_t));
                    current_size = remaining;
                } else {
                    current_size = 0;
                }
                break;
            }

            default:
                break;
        }

        int64_t* temp = current;
        current = next;
        next = temp;

        op = op->next;
    }

    int64_t result = init;
    if (fn) {
        for (size_t i = 0; i < current_size; i++) {
            result = fn(result, current[i], ctx);
        }
    }

    free(buffer1);
    free(buffer2);
    return result;
}

static size_t pipeline_to_array_i64(fp_pipeline_i64_t* self, int64_t* output, size_t max_size) {
    if (!self || !self->input || !output) return 0;

    int64_t* buffer1 = malloc(self->input_size * sizeof(int64_t));
    int64_t* buffer2 = malloc(self->input_size * sizeof(int64_t));
    if (!buffer1 || !buffer2) {
        free(buffer1);
        free(buffer2);
        return 0;
    }

    memcpy(buffer1, self->input, self->input_size * sizeof(int64_t));
    size_t current_size = self->input_size;
    int64_t* current = buffer1;
    int64_t* next = buffer2;

    fp_pipe_op_t* op = self->first_op;
    while (op) {
        switch (op->type) {
            case FP_PIPE_MAP: {
                int64_t (*map_fn)(int64_t, void*) = (int64_t (*)(int64_t, void*))op->function;
                for (size_t i = 0; i < current_size; i++) {
                    next[i] = map_fn(current[i], op->context);
                }
                break;
            }

            case FP_PIPE_FILTER: {
                bool (*pred)(int64_t, void*) = (bool (*)(int64_t, void*))op->function;
                size_t write_idx = 0;
                for (size_t i = 0; i < current_size; i++) {
                    if (pred(current[i], op->context)) {
                        next[write_idx++] = current[i];
                    }
                }
                current_size = write_idx;
                break;
            }

            case FP_PIPE_TAKE: {
                size_t take_count = (op->params.count < current_size) ? op->params.count : current_size;
                memcpy(next, current, take_count * sizeof(int64_t));
                current_size = take_count;
                break;
            }

            case FP_PIPE_DROP: {
                if (op->params.count < current_size) {
                    size_t remaining = current_size - op->params.count;
                    memcpy(next, current + op->params.count, remaining * sizeof(int64_t));
                    current_size = remaining;
                } else {
                    current_size = 0;
                }
                break;
            }

            default:
                break;
        }

        int64_t* temp = current;
        current = next;
        next = temp;

        op = op->next;
    }

    size_t copy_size = (current_size < max_size) ? current_size : max_size;
    memcpy(output, current, copy_size * sizeof(int64_t));

    free(buffer1);
    free(buffer2);
    return copy_size;
}

static void pipeline_foreach_i64(fp_pipeline_i64_t* self, void (*fn)(int64_t, void*), void* ctx) {
    if (!self || !self->input || !fn) return;

    int64_t* buffer = malloc(self->input_size * sizeof(int64_t));
    if (!buffer) return;

    size_t result_size = pipeline_to_array_i64(self, buffer, self->input_size);

    for (size_t i = 0; i < result_size; i++) {
        fn(buffer[i], ctx);
    }

    free(buffer);
}

// Pipeline cleanup
void fp_pipeline_free_f64(fp_pipeline_f64_t* pipeline) {
    if (!pipeline) return;

    // Free operation chain
    fp_pipe_op_t* op = pipeline->first_op;
    while (op) {
        fp_pipe_op_t* next = op->next;
        free(op);
        op = next;
    }

    free(pipeline);
}

void fp_pipeline_free_i64(fp_pipeline_i64_t* pipeline) {
    if (!pipeline) return;

    fp_pipe_op_t* op = pipeline->first_op;
    while (op) {
        fp_pipe_op_t* next = op->next;
        free(op);
        op = next;
    }

    free(pipeline);
}

/* ============================================================================
 * PARTIAL APPLICATION (Currying)
 * ============================================================================ */

fp_partial_map_f64_t fp_curry_map_f64(double (*fn)(double, void*), void* ctx) {
    fp_partial_map_f64_t partial = {
        .transform = fn,
        .context = ctx
    };
    return partial;
}

void fp_apply_partial_map_f64(fp_partial_map_f64_t partial, const double* in, double* out, size_t n) {
    if (!in || !out || !partial.transform) return;

    for (size_t i = 0; i < n; i++) {
        out[i] = partial.transform(in[i], partial.context);
    }
}

fp_partial_filter_f64_t fp_curry_filter_f64(bool (*pred)(double, void*), void* ctx) {
    fp_partial_filter_f64_t partial = {
        .predicate = pred,
        .context = ctx
    };
    return partial;
}

size_t fp_apply_partial_filter_f64(fp_partial_filter_f64_t partial, const double* in, double* out, size_t n) {
    if (!in || !out || !partial.predicate) return 0;

    size_t write_idx = 0;
    for (size_t i = 0; i < n; i++) {
        if (partial.predicate(in[i], partial.context)) {
            out[write_idx++] = in[i];
        }
    }

    return write_idx;
}

/* ============================================================================
 * LAZY EVALUATION
 * ============================================================================ */

// Lazy sequence state structures
typedef struct {
    double current;
    double end;
    double step;
} fp_lazy_range_state_t;

typedef struct {
    double current;
    double (*fn)(double);
} fp_lazy_iterate_state_t;

typedef struct {
    const double* array;
    size_t size;
    size_t index;
} fp_lazy_array_state_t;

// Lazy range implementation
static double lazy_range_next(fp_lazy_seq_t* seq) {
    fp_lazy_range_state_t* state = (fp_lazy_range_state_t*)seq->state;
    double result = state->current;
    state->current += state->step;
    return result;
}

static bool lazy_range_has_next(fp_lazy_seq_t* seq) {
    fp_lazy_range_state_t* state = (fp_lazy_range_state_t*)seq->state;
    return (state->step > 0) ? (state->current < state->end) : (state->current > state->end);
}

static void lazy_range_cleanup(fp_lazy_seq_t* seq) {
    if (seq && seq->state) {
        free(seq->state);
    }
}

fp_lazy_seq_t* fp_lazy_range_f64(double start, double end, double step) {
    fp_lazy_seq_t* seq = malloc(sizeof(fp_lazy_seq_t));
    if (!seq) return NULL;

    fp_lazy_range_state_t* state = malloc(sizeof(fp_lazy_range_state_t));
    if (!state) {
        free(seq);
        return NULL;
    }

    state->current = start;
    state->end = end;
    state->step = step;

    seq->next = lazy_range_next;
    seq->has_next = lazy_range_has_next;
    seq->state = state;
    seq->cleanup = lazy_range_cleanup;

    return seq;
}

// Lazy iterate implementation
static double lazy_iterate_next(fp_lazy_seq_t* seq) {
    fp_lazy_iterate_state_t* state = (fp_lazy_iterate_state_t*)seq->state;
    double result = state->current;
    state->current = state->fn(state->current);
    return result;
}

static bool lazy_iterate_has_next(fp_lazy_seq_t* seq) {
    return true;  // Infinite sequence
}

static void lazy_iterate_cleanup(fp_lazy_seq_t* seq) {
    if (seq && seq->state) {
        free(seq->state);
    }
}

fp_lazy_seq_t* fp_lazy_iterate_f64(double init, double (*fn)(double)) {
    if (!fn) return NULL;

    fp_lazy_seq_t* seq = malloc(sizeof(fp_lazy_seq_t));
    if (!seq) return NULL;

    fp_lazy_iterate_state_t* state = malloc(sizeof(fp_lazy_iterate_state_t));
    if (!state) {
        free(seq);
        return NULL;
    }

    state->current = init;
    state->fn = fn;

    seq->next = lazy_iterate_next;
    seq->has_next = lazy_iterate_has_next;
    seq->state = state;
    seq->cleanup = lazy_iterate_cleanup;

    return seq;
}

// Lazy from array implementation
static double lazy_array_next(fp_lazy_seq_t* seq) {
    fp_lazy_array_state_t* state = (fp_lazy_array_state_t*)seq->state;
    return state->array[state->index++];
}

static bool lazy_array_has_next(fp_lazy_seq_t* seq) {
    fp_lazy_array_state_t* state = (fp_lazy_array_state_t*)seq->state;
    return state->index < state->size;
}

static void lazy_array_cleanup(fp_lazy_seq_t* seq) {
    if (seq && seq->state) {
        free(seq->state);
    }
}

fp_lazy_seq_t* fp_lazy_from_array_f64(const double* array, size_t n) {
    if (!array) return NULL;

    fp_lazy_seq_t* seq = malloc(sizeof(fp_lazy_seq_t));
    if (!seq) return NULL;

    fp_lazy_array_state_t* state = malloc(sizeof(fp_lazy_array_state_t));
    if (!state) {
        free(seq);
        return NULL;
    }

    state->array = array;
    state->size = n;
    state->index = 0;

    seq->next = lazy_array_next;
    seq->has_next = lazy_array_has_next;
    seq->state = state;
    seq->cleanup = lazy_array_cleanup;

    return seq;
}

// Lazy map/filter/take (simplified - would need more complex state management for full implementation)
fp_lazy_seq_t* fp_lazy_map_f64(fp_lazy_seq_t* seq, double (*fn)(double)) {
    // TODO: Implement lazy map transformation
    return seq;  // Placeholder
}

fp_lazy_seq_t* fp_lazy_filter_f64(fp_lazy_seq_t* seq, bool (*pred)(double)) {
    // TODO: Implement lazy filter transformation
    return seq;  // Placeholder
}

fp_lazy_seq_t* fp_lazy_take_f64(fp_lazy_seq_t* seq, size_t n) {
    // TODO: Implement lazy take transformation
    return seq;  // Placeholder
}

// Force evaluation
double* fp_lazy_to_array_f64(fp_lazy_seq_t* seq, size_t max_size, size_t* out_size) {
    if (!seq || !out_size) return NULL;

    double* result = malloc(max_size * sizeof(double));
    if (!result) return NULL;

    size_t count = 0;
    while (seq->has_next(seq) && count < max_size) {
        result[count++] = seq->next(seq);
    }

    *out_size = count;
    return result;
}

void fp_lazy_free_f64(fp_lazy_seq_t* seq) {
    if (!seq) return;

    if (seq->cleanup) {
        seq->cleanup(seq);
    }

    free(seq);
}

/* ============================================================================
 * TRANSDUCERS (Single-pass composition)
 * Note: Full transducer implementation is complex - providing simplified version
 * ============================================================================ */

fp_transducer_t fp_mapping_f64(double (*fn)(double)) {
    fp_transducer_t trans = {
        .state = (void*)fn,
        .step = NULL,
        .complete = NULL
    };
    return trans;
}

fp_transducer_t fp_filtering_f64(bool (*pred)(double)) {
    fp_transducer_t trans = {
        .state = (void*)pred,
        .step = NULL,
        .complete = NULL
    };
    return trans;
}

fp_transducer_t fp_taking_f64(size_t n) {
    fp_transducer_t trans = {
        .state = (void*)(uintptr_t)n,
        .step = NULL,
        .complete = NULL
    };
    return trans;
}

fp_transducer_t fp_compose_transducers(fp_transducer_t* transducers, size_t count) {
    // Simplified - full implementation would chain transducers
    fp_transducer_t composed = {
        .state = transducers,
        .step = NULL,
        .complete = NULL
    };
    return composed;
}

void fp_transduce_f64(const double* input, size_t n, fp_transducer_t transducer,
                      double init, double (*reducer)(double acc, double x)) {
    // Simplified transducer application
    // Full implementation would properly chain transformations
    if (!input || !reducer) return;

    double acc = init;
    for (size_t i = 0; i < n; i++) {
        acc = reducer(acc, input[i]);
    }
}
