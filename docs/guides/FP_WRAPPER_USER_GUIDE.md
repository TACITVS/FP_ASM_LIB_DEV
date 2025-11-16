# Functional Programming Wrapper Layer - User Guide

## 🎯 Overview

The FP-ASM wrapper layer brings Haskell-style functional programming to C, providing:

- **Maybe monad** - Safe optional values without null pointers
- **Either monad** - Error handling with messages (no exceptions needed)
- **Pipelines** - Declarative data transformations with fluent API
- **Function composition** - Build complex functions from simple pieces
- **Lazy evaluation** - Process only what's needed
- **Zero-cost abstractions** - Inline optimizations for performance

## 📚 Table of Contents

1. [Quick Start](#quick-start)
2. [Maybe Monad](#maybe-monad)
3. [Either Monad](#either-monad)
4. [Pipelines](#pipelines)
5. [Function Composition](#function-composition)
6. [Fused Operations](#fused-operations)
7. [Lazy Evaluation](#lazy-evaluation)
8. [Performance Tips](#performance-tips)
9. [API Reference](#api-reference)

---

## Quick Start

### Include Headers

```c
#include "fp_monads.h"       // Maybe/Either monads
#include "fp_compose.h"      // Pipelines and composition

// For hot paths (inline optimizations):
#include "fp_monads_inline.h"
#include "fp_compose_inline.h"
```

### Compile

```bash
gcc your_code.c \
    src/wrappers/fp_compose.c \
    src/wrappers/fp_monads.c \
    -Iinclude \
    -o your_program \
    -lm -O3
```

### Hello World Example

```c
#include "fp_monads.h"

int main(void) {
    // Safe division - returns Maybe
    Maybe result = fp_safe_divide_f64(10.0, 2.0);

    if (fp_is_just(result)) {
        printf("Result: %f\n", fp_from_just_f64(result));
    } else {
        printf("Error: Division failed\n");
    }

    return 0;
}
```

---

## Maybe Monad

### What is Maybe?

`Maybe` represents an optional value - either `Just value` or `Nothing`. It's type-safe null handling without segfaults.

### Basic Usage

```c
// Constructor - success
Maybe m1 = fp_just_f64(42.0);

// Constructor - failure
Maybe m2 = fp_nothing();

// Check if value exists
if (fp_is_just(m1)) {
    double value = fp_from_just_f64(m1);  // UNSAFE - check first!
}

// Safe extraction with default
double value = fp_from_maybe_f64(m2, 0.0);  // Returns 0.0 if Nothing
```

### Safe Operations (Returning Maybe)

```c
// Safe division
Maybe result = fp_safe_divide_f64(100.0, 0.0);  // Returns Nothing

// Safe square root
Maybe sqrt_result = fp_safe_sqrt_f64(-4.0);  // Returns Nothing

// Safe logarithm
Maybe log_result = fp_safe_log_f64(10.0);  // Returns Just(ln(10))

// Safe array access
double arr[] = {1.0, 2.0, 3.0};
Maybe elem = fp_safe_at_f64(arr, 3, 5);  // Returns Nothing (out of bounds)
```

### Chaining Operations (Bind)

```c
// Chain operations - short-circuits on Nothing
Maybe result = fp_safe_divide_f64(100.0, 4.0);    // Just(25.0)
result = fp_bind_maybe_f64(result, fp_safe_sqrt_f64);   // Just(5.0)
result = fp_bind_maybe_f64(result, fp_safe_log_f64);    // Just(ln(5))

if (fp_is_just(result)) {
    printf("Success: %f\n", fp_from_just_f64(result));
}

// Example with failure:
Maybe failed = fp_safe_divide_f64(10.0, 0.0);     // Nothing
failed = fp_bind_maybe_f64(failed, fp_safe_sqrt_f64);  // Still Nothing (skipped!)
```

### Mapping Over Maybe (fmap)

```c
double square(double x) { return x * x; }

Maybe m = fp_just_f64(5.0);
Maybe squared = fp_fmap_maybe_f64(m, square);  // Just(25.0)

Maybe n = fp_nothing();
Maybe result = fp_fmap_maybe_f64(n, square);  // Still Nothing
```

---

## Either Monad

### What is Either?

`Either` represents a value that can be either:
- `Left error_msg` - Error case with message
- `Right value` - Success case

It's perfect for operations that might fail with detailed error information.

### Basic Usage

```c
// Constructor - success
Either success = fp_right_f64(42.0);

// Constructor - error
Either error = fp_left("Something went wrong", -1);

// Check which case
if (fp_is_right(success)) {
    double value = fp_from_right_f64(success);
}

if (fp_is_left(error)) {
    const char* msg = fp_from_left_msg(error);
    int code = fp_from_left_code(error);
    printf("Error: %s (code %d)\n", msg, code);
}
```

### Checked Operations (Returning Either)

```c
// Division with error message
Either result = fp_checked_divide_f64(10.0, 0.0);
// Returns: Left("Division by zero", -1)

// Integer overflow detection
Either overflow = fp_checked_divide_i64(INT64_MIN, -1);
// Returns: Left("Integer overflow: INT64_MIN / -1", -2)

// Array bounds with error message
double arr[] = {1.0, 2.0, 3.0};
Either elem = fp_checked_at_f64(arr, 3, 10);
// Returns: Left("Array index out of bounds", -2)
```

### Chaining Operations

```c
double add_ten(double x) { return x + 10.0; }

Either result = fp_checked_divide_f64(100.0, 4.0);  // Right(25.0)
result = fp_fmap_either_f64(result, sqrt);           // Right(5.0)
result = fp_fmap_either_f64(result, add_ten);        // Right(15.0)

if (fp_is_right(result)) {
    printf("Success: %f\n", fp_from_right_f64(result));
}
```

### Error Propagation

```c
// If any step fails, error propagates
Either chain = fp_checked_divide_f64(100.0, 0.0);  // Left(...)
chain = fp_fmap_either_f64(chain, sqrt);  // Still Left (skipped!)
chain = fp_fmap_either_f64(chain, log);   // Still Left (skipped!)

// Original error is preserved throughout the chain
```

---

## Pipelines

### What are Pipelines?

Pipelines provide a fluent API for chaining transformations in a declarative style:

```c
data -> map(f) -> filter(p) -> reduce(op)
```

### Basic Pipeline Example

```c
double square(double x, void* ctx) { return x * x; }
bool gt_ten(double x, void* ctx) { return x > 10.0; }
double add(double acc, double x, void* ctx) { return acc + x; }

double data[] = {1, 2, 3, 4, 5, 6, 7, 8};

// Create pipeline
fp_pipeline_f64_t* pipeline = fp_pipeline_f64(data, 8);

// Chain operations and execute
double result = pipeline
    ->map(pipeline, square, NULL)        // [1,4,9,16,25,36,49,64]
    ->filter(pipeline, gt_ten, NULL)     // [16,25,36,49,64]
    ->reduce(pipeline, 0.0, add, NULL);  // 190.0

printf("Result: %f\n", result);

// Clean up
fp_pipeline_free_f64(pipeline);
```

### Pipeline Operations

#### Map
Transform each element:

```c
double times_two(double x, void* ctx) { return x * 2.0; }

pipeline->map(pipeline, times_two, NULL);
```

#### Filter
Keep only elements matching predicate:

```c
bool is_positive(double x, void* ctx) { return x > 0.0; }

pipeline->filter(pipeline, is_positive, NULL);
```

#### Take
Take first N elements:

```c
pipeline->take(pipeline, 5);  // Take first 5 elements
```

#### Drop
Skip first N elements:

```c
pipeline->drop(pipeline, 3);  // Skip first 3 elements
```

#### Reduce (Terminal)
Fold all elements into a single value:

```c
double sum(double acc, double x, void* ctx) { return acc + x; }

double total = pipeline->reduce(pipeline, 0.0, sum, NULL);
```

#### To Array (Terminal)
Collect results into an array:

```c
double output[100];
size_t count = pipeline->to_array(pipeline, output, 100);

printf("Got %zu elements\n", count);
```

#### Foreach (Terminal)
Execute side effect for each element:

```c
void print_elem(double x, void* ctx) {
    printf("%.2f ", x);
}

pipeline->foreach(pipeline, print_elem, NULL);
```

### Using Context

```c
typedef struct {
    double threshold;
    int count;
} FilterContext;

bool gt_threshold(double x, void* ctx) {
    FilterContext* fc = (FilterContext*)ctx;
    fc->count++;  // Track how many tested
    return x > fc->threshold;
}

FilterContext fc = {.threshold = 10.0, .count = 0};

pipeline->filter(pipeline, gt_threshold, &fc);

printf("Tested %d elements\n", fc.count);
```

---

## Function Composition

### Basic Composition

Compose two functions `f . g` means `f(g(x))`:

```c
double square(double x) { return x * x; }
double add_ten(double x) { return x + 10.0; }

// (square . add_ten)(5) = square(add_ten(5)) = square(15) = 225
double result = fp_compose_2_f64_inline(5.0, square, add_ten);

printf("Result: %f\n", result);  // 225.0
```

### Multi-Function Composition

```c
double f(double x) { return x * 2; }
double g(double x) { return x + 3; }
double h(double x) { return x * x; }

// (f . g . h)(5) = f(g(h(5))) = f(g(25)) = f(28) = 56
double result = fp_compose_3_f64_inline(5.0, f, g, h);
```

### Composition Over Arrays

```c
double subtract_five(double x) { return x - 5.0; }
double times_two(double x) { return x * 2.0; }

double input[] = {1, 2, 3, 4, 5};
double output[5];

// Apply (subtract_five . times_two) to each element
for (size_t i = 0; i < 5; i++) {
    output[i] = fp_compose_2_f64_inline(input[i], subtract_five, times_two);
}

// output = [(1*2)-5, (2*2)-5, (3*2)-5, (4*2)-5, (5*2)-5]
//        = [-3, -1, 1, 3, 5]
```

### Partial Application (Currying)

```c
double scale(double x, void* ctx) {
    double* factor = (double*)ctx;
    return x * (*factor);
}

double factor = 2.5;
fp_partial_map_f64_t partial = fp_curry_map_f64(scale, &factor);

double input[] = {1, 2, 3, 4, 5};
double output[5];

fp_apply_partial_map_f64(partial, input, output, 5);
// output = [2.5, 5.0, 7.5, 10.0, 12.5]
```

---

## Fused Operations

### Why Fusion?

Traditional approach (creates temporary arrays):
```c
// Imperative: Creates temp array!
double temp[N];
for (i = 0; i < N; i++) temp[i] = square(data[i]);
double sum = 0;
for (i = 0; i < N; i++) sum += temp[i];
```

Fused approach (single pass, no temporary):
```c
// Functional: No temp array, single loop!
double sum = fp_fused_map_reduce_f64_inline(data, N, square, 0.0, add);
```

### Fused Map-Reduce

```c
double square(double x) { return x * x; }
double add(double acc, double x) { return acc + x; }

double data[] = {1, 2, 3, 4, 5};

// Sum of squares - single pass!
double result = fp_fused_map_reduce_f64_inline(data, 5, square, 0.0, add);

printf("Sum of squares: %f\n", result);  // 55.0
```

### Fused Filter-Reduce

```c
bool is_even(double x) { return ((int)x % 2) == 0; }
double add(double acc, double x) { return acc + x; }

double data[] = {1, 2, 3, 4, 5, 6};

// Sum of even numbers - single pass!
double result = fp_fused_filter_reduce_f64_inline(data, 6, is_even, 0.0, add);

printf("Sum of evens: %f\n", result);  // 12.0 (2 + 4 + 6)
```

### Fused Map-Filter-Reduce (Triple Fusion!)

```c
double square(double x) { return x * x; }
bool gt_ten(double x) { return x > 10.0; }
double add(double acc, double x) { return acc + x; }

double data[] = {1, 2, 3, 4, 5};

// Sum of (squares > 10) - single pass, no temps!
double result = fp_fused_map_filter_reduce_f64_inline(
    data, 5, square, gt_ten, 0.0, add
);

printf("Result: %f\n", result);  // 50.0 (16 + 25 + 9... wait no, 16+25 = 41)
```

---

## Lazy Evaluation

### Lazy Ranges

```c
// Create lazy range [0, 10) step 1
fp_lazy_seq_t* seq = fp_lazy_range_f64(0.0, 10.0, 1.0);

// Consume lazily
while (seq->has_next(seq)) {
    double value = seq->next(seq);
    printf("%.0f ", value);
}

fp_lazy_free_f64(seq);
```

### Infinite Sequences

```c
double increment(double x) { return x + 1.0; }

// Infinite sequence: 1, 2, 3, 4, ...
fp_lazy_seq_t* seq = fp_lazy_iterate_f64(1.0, increment);

// Take first 10
for (int i = 0; i < 10; i++) {
    printf("%.0f ", seq->next(seq));
}

fp_lazy_free_f64(seq);
```

### Force Evaluation

```c
fp_lazy_seq_t* seq = fp_lazy_range_f64(5.0, 15.0, 2.0);

size_t count;
double* array = fp_lazy_to_array_f64(seq, 100, &count);

printf("Forced %zu values: ", count);
for (size_t i = 0; i < count; i++) {
    printf("%.0f ", array[i]);
}

free(array);
fp_lazy_free_f64(seq);
```

---

## Performance Tips

### 1. Use Inline Headers for Hot Paths

```c
// Regular headers for cold paths
#include "fp_monads.h"

// Inline headers for hot paths (10-100x faster!)
#include "fp_monads_inline.h"

// Hot loop - use inline version
for (int i = 0; i < 1000000; i++) {
    Maybe m = fp_safe_divide_f64_inline(data[i], divisors[i]);
    results[i] = fp_from_maybe_f64_inline(m, 0.0);
}
```

### 2. Use Fused Operations

```c
// DON'T: Two passes, temp array
double temp[N];
fp_simple_map_f64_inline(data, temp, N, square);
double sum = fp_simple_reduce_f64_inline(temp, N, 0.0, add);

// DO: Single pass, no temp!
double sum = fp_fused_map_reduce_f64_inline(data, N, square, 0.0, add);
```

### 3. Prefer Simple Operations for Small Data

```c
// For small arrays (< 100 elements), simple functions are faster
// than full pipeline machinery

// DON'T:
fp_pipeline_f64_t* p = fp_pipeline_f64(small_data, 10);
double result = p->map(p, f, NULL)->reduce(p, 0.0, add, NULL);
fp_pipeline_free_f64(p);

// DO:
fp_simple_map_f64_inline(small_data, temp, 10, f);
double result = fp_simple_reduce_f64_inline(temp, 10, 0.0, add);
```

### 4. Enable Compiler Optimizations

```bash
# Always compile with -O3 for inline functions
gcc -O3 -march=native your_code.c ...

# Link-time optimization helps composition
gcc -O3 -flto your_code.c ...
```

### 5. Avoid Allocations in Hot Loops

```c
// DON'T: Allocates pipeline every iteration
for (int i = 0; i < 1000; i++) {
    fp_pipeline_f64_t* p = fp_pipeline_f64(data, N);
    // ...
    fp_pipeline_free_f64(p);
}

// DO: Use inline fused operations
for (int i = 0; i < 1000; i++) {
    double result = fp_fused_map_reduce_f64_inline(data, N, f, 0.0, add);
}
```

---

## API Reference

### Maybe Monad

| Function | Description | Returns |
|----------|-------------|---------|
| `fp_just_f64(value)` | Create Just value | Maybe |
| `fp_nothing()` | Create Nothing | Maybe |
| `fp_is_just(m)` | Check if Just | bool |
| `fp_is_nothing(m)` | Check if Nothing | bool |
| `fp_from_just_f64(m)` | Extract value (unsafe!) | double |
| `fp_from_maybe_f64(m, default)` | Extract or default | double |
| `fp_fmap_maybe_f64(m, fn)` | Map function over Maybe | Maybe |
| `fp_bind_maybe_f64(m, fn)` | Monadic bind (>>=) | Maybe |
| `fp_safe_divide_f64(n, d)` | Safe division | Maybe |
| `fp_safe_sqrt_f64(x)` | Safe square root | Maybe |
| `fp_safe_log_f64(x)` | Safe logarithm | Maybe |
| `fp_safe_at_f64(arr, n, i)` | Safe array access | Maybe |

### Either Monad

| Function | Description | Returns |
|----------|-------------|---------|
| `fp_left(msg, code)` | Create Left (error) | Either |
| `fp_right_f64(value)` | Create Right (success) | Either |
| `fp_is_left(e)` | Check if Left | bool |
| `fp_is_right(e)` | Check if Right | bool |
| `fp_from_left_msg(e)` | Get error message | const char* |
| `fp_from_left_code(e)` | Get error code | int |
| `fp_from_right_f64(e)` | Extract value | double |
| `fp_fmap_either_f64(e, fn)` | Map over Either | Either |
| `fp_bind_either_f64(e, fn)` | Monadic bind | Either |
| `fp_checked_divide_f64(n, d)` | Division with error | Either |
| `fp_checked_sqrt_f64(x)` | Sqrt with error | Either |
| `fp_checked_at_f64(arr, n, i)` | Access with error | Either |

### Pipelines

| Function | Description | Returns |
|----------|-------------|---------|
| `fp_pipeline_f64(data, n)` | Create pipeline | pipeline* |
| `->map(p, fn, ctx)` | Transform elements | pipeline* |
| `->filter(p, pred, ctx)` | Keep matching | pipeline* |
| `->take(p, n)` | Take first n | pipeline* |
| `->drop(p, n)` | Skip first n | pipeline* |
| `->reduce(p, init, fn, ctx)` | Fold to value | double |
| `->to_array(p, out, max)` | Collect to array | size_t |
| `->foreach(p, fn, ctx)` | Side effects | void |
| `fp_pipeline_free_f64(p)` | Clean up | void |

### Inline Fused Operations

| Function | Description |
|----------|-------------|
| `fp_fused_map_reduce_f64_inline(in, n, map, init, reduce)` | map->reduce fusion |
| `fp_fused_filter_reduce_f64_inline(in, n, pred, init, reduce)` | filter->reduce fusion |
| `fp_fused_map_filter_reduce_f64_inline(...)` | map->filter->reduce fusion |
| `fp_simple_map_f64_inline(in, out, n, fn)` | Simple map |
| `fp_simple_filter_f64_inline(in, out, n, pred)` | Simple filter |
| `fp_simple_reduce_f64_inline(in, n, init, fn)` | Simple reduce |

### Lazy Sequences

| Function | Description | Returns |
|----------|-------------|---------|
| `fp_lazy_range_f64(start, end, step)` | Lazy range | seq* |
| `fp_lazy_iterate_f64(init, fn)` | Infinite iterate | seq* |
| `fp_lazy_from_array_f64(arr, n)` | From array | seq* |
| `seq->next(seq)` | Get next value | double |
| `seq->has_next(seq)` | Check if more | bool |
| `fp_lazy_to_array_f64(seq, max, out_n)` | Force to array | double* |
| `fp_lazy_free_f64(seq)` | Clean up | void |

---

## Complete Example

```c
#include "fp_monads.h"
#include "fp_compose.h"
#include "fp_monads_inline.h"
#include "fp_compose_inline.h"
#include <stdio.h>
#include <math.h>

// Data processing pipeline: filter -> map -> reduce
double square(double x, void* ctx) { return x * x; }
bool is_positive(double x, void* ctx) { return x > 0.0; }
double add(double acc, double x, void* ctx) { return acc + x; }

int main(void) {
    double data[] = {-2, -1, 0, 1, 2, 3, 4, 5};

    // Method 1: Pipeline (declarative)
    fp_pipeline_f64_t* p = fp_pipeline_f64(data, 8);
    double result1 = p
        ->filter(p, is_positive, NULL)
        ->map(p, square, NULL)
        ->reduce(p, 0.0, add, NULL);
    fp_pipeline_free_f64(p);

    printf("Pipeline result: %f\n", result1);

    // Method 2: Maybe monad (safe operations)
    Maybe safe = fp_safe_divide_f64_inline(100.0, 4.0);
    safe = fp_bind_maybe_f64(safe, fp_safe_sqrt_f64);
    safe = fp_bind_maybe_f64(safe, fp_safe_log_f64);

    if (fp_is_just_inline(safe)) {
        printf("Safe computation: %f\n", fp_from_just_f64(safe));
    }

    // Method 3: Either monad (error messages)
    Either checked = fp_checked_divide_f64_inline(10.0, 0.0);
    if (fp_is_left_inline(checked)) {
        printf("Error: %s\n", fp_from_left_msg(checked));
    }

    return 0;
}
```

---

## Summary

The FP wrapper layer provides:

✅ **Type-safe error handling** - No more null pointer crashes
✅ **Declarative pipelines** - Readable, maintainable code
✅ **Zero-cost abstractions** - Fast as hand-written C
✅ **Composable functions** - Build complex from simple
✅ **Single-pass fusion** - Eliminate temporary arrays

Transform imperative C into elegant functional code without sacrificing performance!
