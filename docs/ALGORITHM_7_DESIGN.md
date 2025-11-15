# Algorithm #7: Rolling Window Statistics - FP Composition Design

## Design Philosophy: Composition Over Reimplementation

### The Problem with Sequential Approach

**WRONG (Imperative, No Reuse):**
```c
void fp_rolling_min_f64(...) {
    // Reimplements min from scratch
    for each window {
        double min = INFINITY;
        for (i = 0; i < window; i++) {
            if (data[i] < min) min = data[i];
        }
        output[...] = min;
    }
}

void fp_rolling_max_f64(...) {
    // Reimplements max from scratch
    for each window {
        double max = -INFINITY;
        for (i = 0; i < window; i++) {
            if (data[i] > max) max = data[i];
        }
        output[...] = max;
    }
}
```

**Problems:**
- Code duplication
- No reuse of optimized assembly
- Violates DRY principle
- Not functional composition

---

### The FP Approach: Generic Composition

**RIGHT (Functional Composition):**

```c
// Generic rolling window abstraction
// Applies ANY reduction function over sliding windows
void fp_rolling_reduce_f64(
    const double* data,           // Input array
    size_t n,                     // Array length
    size_t window,                // Window size
    double (*reduce_fn)(const double*, size_t),  // Reduction function (curried!)
    double* output                // Output array
);

// Specific functions are just COMPOSITIONS:
void fp_rolling_min_f64(const double* data, size_t n, size_t window, double* output) {
    fp_rolling_reduce_f64(data, n, window, fp_reduce_min_f64, output);
    //                                      ^^^^^^^^^^^^^^^^^^
    //                                      REUSE existing optimized function!
}

void fp_rolling_max_f64(const double* data, size_t n, size_t window, double* output) {
    fp_rolling_reduce_f64(data, n, window, fp_reduce_max_f64, output);
    //                                      ^^^^^^^^^^^^^^^^^^
    //                                      REUSE existing optimized function!
}

void fp_rolling_sum_f64(const double* data, size_t n, size_t window, double* output) {
    fp_rolling_reduce_f64(data, n, window, fp_reduce_add_f64, output);
    //                                      ^^^^^^^^^^^^^^^^^^
    //                                      REUSE existing optimized function!
}
```

**Benefits:**
- ✅ **Composition**: `rolling(min) = rolling ∘ min`
- ✅ **Reuse**: All existing `fp_reduce_*` functions work immediately
- ✅ **DRY**: Window logic written once
- ✅ **Extensibility**: New reductions work automatically
- ✅ **Performance**: Leverages existing optimized assembly

---

## Existing Functions to Reuse

### Already Implemented (Module 1-2):
```c
// From fp_core_reductions.asm
double fp_reduce_add_f64(const double* in, size_t n);    // Sum
double fp_reduce_max_f64(const double* in, size_t n);    // Max

// From fp_core_reductions.asm
int64_t fp_reduce_add_i64(const int64_t* in, size_t n);  // Sum (i64)
int64_t fp_reduce_max_i64(const int64_t* in, size_t n);  // Max (i64)
```

**Note:** We're missing `fp_reduce_min_f64` - need to add it!

### Can Be Composed:
```c
// Rolling mean = rolling(sum) / window
void fp_rolling_mean_f64(...) {
    fp_rolling_reduce_f64(data, n, window, fp_reduce_add_f64, temp);
    fp_map_scale_f64(temp, output, out_size, 1.0 / window);  // Compose with map!
}

// Rolling median = rolling(percentile_50)
void fp_rolling_median_f64(...) {
    // For each window, call fp_percentile_f64(window_slice, window, 0.5)
}
```

---

## Implementation Strategy

### Phase 1: Add Missing Primitives
1. `fp_reduce_min_f64` (should have existed alongside max!)
2. `fp_reduce_product_f64` (useful for geometric mean)

### Phase 2: Generic Rolling Window
```c
// Generic windowed reduction (function pointer magic!)
void fp_rolling_reduce_f64(
    const double* data,
    size_t n,
    size_t window,
    double (*reduce_fn)(const double*, size_t),
    double* output
) {
    size_t out_size = n - window + 1;

    for (size_t i = 0; i < out_size; i++) {
        output[i] = reduce_fn(&data[i], window);  // Call curried function!
    }
}
```

### Phase 3: Thin Wrapper Functions
```c
// All rolling functions become one-liners through composition!
void fp_rolling_min_f64(const double* data, size_t n, size_t window, double* output) {
    fp_rolling_reduce_f64(data, n, window, fp_reduce_min_f64, output);
}

void fp_rolling_max_f64(const double* data, size_t n, size_t window, double* output) {
    fp_rolling_reduce_f64(data, n, window, fp_reduce_max_f64, output);
}

void fp_rolling_sum_f64(const double* data, size_t n, size_t window, double* output) {
    fp_rolling_reduce_f64(data, n, window, fp_reduce_add_f64, output);
}

void fp_rolling_mean_f64(const double* data, size_t n, size_t window, double* output) {
    fp_rolling_sum_f64(data, n, window, output);  // Compose!
    size_t out_size = n - window + 1;
    fp_map_scale_f64(output, output, out_size, 1.0 / window);  // Compose map!
}
```

---

## Function Composition Examples

### Example 1: Rolling Range
```c
// range = max - min
// Functional composition: range = subtract(max, min)
void fp_rolling_range_f64(const double* data, size_t n, size_t window, double* output) {
    size_t out_size = n - window + 1;
    double* temp_max = malloc(out_size * sizeof(double));
    double* temp_min = malloc(out_size * sizeof(double));

    fp_rolling_max_f64(data, n, window, temp_max);  // Compose!
    fp_rolling_min_f64(data, n, window, temp_min);  // Compose!

    // output = max - min (compose subtract)
    for (size_t i = 0; i < out_size; i++) {
        output[i] = temp_max[i] - temp_min[i];
    }

    free(temp_max);
    free(temp_min);
}
```

### Example 2: Rolling Std Deviation
```c
// std = sqrt(variance)
// variance = E[X^2] - (E[X])^2
// Functional composition: std = sqrt ∘ variance
void fp_rolling_std_f64(const double* data, size_t n, size_t window, double* output) {
    // Step 1: Compute rolling mean
    size_t out_size = n - window + 1;
    double* mean = malloc(out_size * sizeof(double));
    fp_rolling_mean_f64(data, n, window, mean);

    // Step 2: For each window, compute variance using existing stats
    for (size_t i = 0; i < out_size; i++) {
        DescriptiveStats stats;
        fp_descriptive_stats_f64(&data[i], window, &stats);  // Reuse!
        output[i] = stats.std_dev;  // Already computed!
    }

    free(mean);
}
```

---

## Currying in C (Function Pointers)

While C doesn't have native currying like Haskell, we can simulate it:

```c
// Haskell-style currying (conceptual):
// rolling :: (Array -> Double) -> Int -> Array -> Array
// rolling_min = rolling min

// C approximation with function pointers:
typedef double (*ReductionFn)(const double*, size_t);

ReductionFn get_min_reducer() { return fp_reduce_min_f64; }
ReductionFn get_max_reducer() { return fp_reduce_max_f64; }

// Now we can "curry" by partial application:
void apply_rolling(ReductionFn fn, const double* data, size_t n, size_t window, double* output) {
    fp_rolling_reduce_f64(data, n, window, fn, output);
}

// Usage:
apply_rolling(get_min_reducer(), data, n, window, output);  // Curried!
```

---

## Performance Considerations

### Why Composition Doesn't Hurt Performance:

1. **Inlining**: Modern compilers inline function pointers when possible
2. **Code Reuse**: Calling optimized assembly is faster than reimplementing
3. **Cache**: Window iterations access sequential memory (cache-friendly)

### Optimization Strategy:

```c
// Generic version (good for extensibility)
void fp_rolling_reduce_f64_generic(
    const double* data, size_t n, size_t window,
    double (*reduce_fn)(const double*, size_t),
    double* output
);

// Specialized version (good for performance)
void fp_rolling_sum_f64_optimized(
    const double* data, size_t n, size_t window,
    double* output
) {
    // Use sliding window trick: sum[i+1] = sum[i] - data[i] + data[i+window]
    // Like fp_map_sma_f64 from Algorithm #6!
}
```

**Best of both worlds:**
- Provide generic composition for extensibility
- Provide optimized specializations for hot paths
- User chooses based on needs

---

## Final API Design

```c
// ============================================================================
// Algorithm #7: Rolling Window Statistics (Composition Pattern)
// ============================================================================

// Generic windowed reduction (accepts any reduction function)
void fp_rolling_reduce_f64(const double* data, size_t n, size_t window,
                            double (*reduce_fn)(const double*, size_t),
                            double* output);

// Specific reductions (thin wrappers using composition)
void fp_rolling_min_f64(const double* data, size_t n, size_t window, double* output);
void fp_rolling_max_f64(const double* data, size_t n, size_t window, double* output);
void fp_rolling_sum_f64(const double* data, size_t n, size_t window, double* output);
void fp_rolling_mean_f64(const double* data, size_t n, size_t window, double* output);

// Composed operations
void fp_rolling_range_f64(const double* data, size_t n, size_t window, double* output);
void fp_rolling_std_f64(const double* data, size_t n, size_t window, double* output);
void fp_rolling_variance_f64(const double* data, size_t n, size_t window, double* output);

// Optimized specializations (when composition overhead matters)
void fp_rolling_sum_f64_optimized(const double* data, size_t n, size_t window, double* output);
```

---

## Implementation Checklist

### Missing Primitives to Add:
- [ ] `fp_reduce_min_f64` (should exist alongside max!)
- [ ] `fp_reduce_min_i64` (for completeness)

### Core Composition Functions:
- [ ] `fp_rolling_reduce_f64` (generic windowed reduction)
- [ ] `fp_rolling_reduce_i64` (integer version)

### Thin Wrappers (Pure Composition):
- [ ] `fp_rolling_min_f64` = rolling(min)
- [ ] `fp_rolling_max_f64` = rolling(max)
- [ ] `fp_rolling_sum_f64` = rolling(sum)
- [ ] `fp_rolling_mean_f64` = scale(rolling(sum), 1/window)

### Composed Operations:
- [ ] `fp_rolling_range_f64` = subtract(rolling(max), rolling(min))
- [ ] `fp_rolling_std_f64` = sqrt(rolling(variance))
- [ ] `fp_rolling_variance_f64` = rolling(descriptive_stats.variance)

### Optimized Specializations:
- [ ] `fp_rolling_sum_f64_optimized` (sliding window, O(1) per step)
- [ ] `fp_rolling_mean_f64_optimized` (sliding window, O(1) per step)

---

## Conclusion

**Algorithm #7 demonstrates TRUE functional programming:**

✅ **Composition over Implementation**
✅ **Reuse existing optimized functions**
✅ **Higher-order functions (function pointers)**
✅ **Currying simulation in C**
✅ **DRY principle**
✅ **Extensibility through abstraction**

This is the **FP way**! 🎯
