# FP-ASM Complete Library Purity Audit

**Date**: 2025-11-01
**Scope**: ALL functions across ALL modules
**Auditor**: Architecture Review Team

---

## Executive Summary

**CRITICAL DISCOVERY**: The library has **SEVERE purity violations** across multiple modules:

### 🚨 Violations Summary:

| Severity | Count | Description |
|----------|-------|-------------|
| **CRITICAL** | 5 | Direct in-place mutation (violates FP core principle) |
| **HIGH** | 3 | Forces user to mutate (sorted input requirement) |
| **MEDIUM** | 0 | Design inconsistencies (acceptable but not ideal) |
| **NONE** | 60+ | Pure or quasi-pure (acceptable patterns) |

### 💥 CRITICAL VIOLATIONS (Must Fix Immediately):

1. ❌ **`fp_sort_i64`** - IN-PLACE SORT (direct mutation!)
2. ❌ **`fp_sort_f64`** - IN-PLACE SORT (direct mutation!)
3. ❌ **`fp_replicate_i64`** - MUTATES output parameter without const input
4. ❌ **`fp_replicate_f64`** - MUTATES output parameter without const input
5. ❌ **`fp_iterate_add_i64`** - MUTATES output parameter without const input
6. ❌ **`fp_iterate_mul_i64`** - MUTATES output parameter without const input

### ⚠️ HIGH PRIORITY VIOLATIONS (Forces User Mutation):

1. ❌ **`fp_percentile_f64`** - Requires sorted input
2. ❌ **`fp_percentiles_f64`** - Requires sorted input
3. ❌ **`fp_quartiles_f64`** - Requires sorted input
4. ❌ **`fp_detect_outliers_iqr_f64`** - Requires sorted input

---

## Complete Function Audit by Module

### Module 1: Simple Folds (Reductions)

#### ✅ `fp_reduce_add_i64` / `fp_reduce_add_f64`
```c
int64_t fp_reduce_add_i64(const int64_t* in, size_t n);
double fp_reduce_add_f64(const double* in, size_t n);
```
- **Input**: `const` ✅
- **Mutation**: NONE ✅
- **Purity**: PURE ✅

#### ✅ `fp_reduce_max_i64` / `fp_reduce_max_f64`
```c
int64_t fp_reduce_max_i64(const int64_t* in, size_t n);
double fp_reduce_max_f64(const double* in, size_t n);
```
- **Input**: `const` ✅
- **Mutation**: NONE ✅
- **Purity**: PURE ✅

**Status**: Module 1 is FULLY PURE ✅

---

### Module 2: Fused Folds (Map-Reduce)

#### ✅ `fp_fold_sumsq_i64`
```c
int64_t fp_fold_sumsq_i64(const int64_t* in, size_t n);
```
- **Input**: `const` ✅
- **Mutation**: NONE ✅
- **Purity**: PURE ✅

#### ✅ `fp_fold_dotp_i64` / `fp_fold_dotp_f64`
```c
int64_t fp_fold_dotp_i64(const int64_t* a, const int64_t* b, size_t n);
double fp_fold_dotp_f64(const double* a, const double* b, size_t n);
```
- **Input**: `const` ✅
- **Mutation**: NONE ✅
- **Purity**: PURE ✅

#### ✅ `fp_fold_sad_i64`
```c
int64_t fp_fold_sad_i64(const int64_t* a, const int64_t* b, size_t n);
```
- **Input**: `const` ✅
- **Mutation**: NONE ✅
- **Purity**: PURE ✅

**Status**: Module 2 is FULLY PURE ✅

---

### Module 3: Fused Maps (BLAS Level 1)

#### ⚠️ `fp_map_axpy_f64` / `fp_map_axpy_i64`
```c
void fp_map_axpy_f64(const double* x, const double* y, double* out, size_t n, double c);
void fp_map_axpy_i64(const int64_t* x, const int64_t* y, int64_t* out, size_t n, int64_t c);
```
- **Input**: `const` ✅
- **Output**: Caller-allocated ⚠️
- **Mutation**: Fills output (acceptable C pattern) ⚠️
- **Purity**: QUASI-PURE ⚠️

#### ⚠️ `fp_map_scale_i64` / `fp_map_scale_f64`
```c
void fp_map_scale_i64(const int64_t* in, int64_t* out, size_t n, int64_t c);
void fp_map_scale_f64(const double* in, double* out, size_t n, double c);
```
- **Input**: `const` ✅
- **Output**: Caller-allocated ⚠️
- **Purity**: QUASI-PURE ⚠️

#### ⚠️ `fp_map_offset_i64` / `fp_map_offset_f64`
```c
void fp_map_offset_i64(const int64_t* in, int64_t* out, size_t n, int64_t c);
void fp_map_offset_f64(const double* in, double* out, size_t n, double c);
```
- **Input**: `const` ✅
- **Output**: Caller-allocated ⚠️
- **Purity**: QUASI-PURE ⚠️

#### ⚠️ `fp_zip_add_i64` / `fp_zip_add_f64`
```c
void fp_zip_add_i64(const int64_t* a, const int64_t* b, int64_t* out, size_t n);
void fp_zip_add_f64(const double* a, const double* b, double* out, size_t n);
```
- **Input**: `const` ✅
- **Output**: Caller-allocated ⚠️
- **Purity**: QUASI-PURE ⚠️

**Status**: Module 3 is QUASI-PURE (acceptable pattern) ⚠️

---

### Module 4: Simple Maps (Transformers)

#### ⚠️ `fp_map_abs_i64` / `fp_map_abs_f64`
```c
void fp_map_abs_i64(const int64_t* in, int64_t* out, size_t n);
void fp_map_abs_f64(const double* in, double* out, size_t n);
```
- **Input**: `const` ✅
- **Output**: Caller-allocated ⚠️
- **Purity**: QUASI-PURE ⚠️

#### ⚠️ `fp_map_sqrt_f64`
```c
void fp_map_sqrt_f64(const double* in, double* out, size_t n);
```
- **Input**: `const` ✅
- **Output**: Caller-allocated ⚠️
- **Purity**: QUASI-PURE ⚠️

#### ⚠️ `fp_map_clamp_i64` / `fp_map_clamp_f64`
```c
void fp_map_clamp_i64(const int64_t* in, int64_t* out, size_t n, int64_t min_val, int64_t max_val);
void fp_map_clamp_f64(const double* in, double* out, size_t n, double min_val, double max_val);
```
- **Input**: `const` ✅
- **Output**: Caller-allocated ⚠️
- **Purity**: QUASI-PURE ⚠️

**Status**: Module 4 is QUASI-PURE (acceptable pattern) ⚠️

---

### Module 5: Scans (Prefix Sums)

#### ⚠️ `fp_scan_add_i64` / `fp_scan_add_f64`
```c
void fp_scan_add_i64(const int64_t* in, int64_t* out, size_t n);
void fp_scan_add_f64(const double* in, double* out, size_t n);
```
- **Input**: `const` ✅
- **Output**: Caller-allocated ⚠️
- **Purity**: QUASI-PURE ⚠️

**Status**: Module 5 is QUASI-PURE (acceptable pattern) ⚠️

---

### Module 6: Predicates

#### ✅ `fp_pred_all_eq_const_i64`
```c
bool fp_pred_all_eq_const_i64(const int64_t* arr, size_t n, int64_t value);
```
- **Input**: `const` ✅
- **Mutation**: NONE ✅
- **Purity**: PURE ✅

#### ✅ `fp_pred_any_gt_const_i64`
```c
bool fp_pred_any_gt_const_i64(const int64_t* arr, size_t n, int64_t value);
```
- **Input**: `const` ✅
- **Mutation**: NONE ✅
- **Purity**: PURE ✅

#### ✅ `fp_pred_all_gt_zip_i64`
```c
bool fp_pred_all_gt_zip_i64(const int64_t* a, const int64_t* b, size_t n);
```
- **Input**: `const` ✅
- **Mutation**: NONE ✅
- **Purity**: PURE ✅

**Status**: Module 6 is FULLY PURE ✅

---

### Module 7: Stream Compaction (List FP Operations)

#### ⚠️ `fp_filter_gt_i64_simple`
```c
size_t fp_filter_gt_i64_simple(const int64_t* input, int64_t* output, size_t n, int64_t threshold);
```
- **Input**: `const` ✅
- **Output**: Caller-allocated ⚠️
- **Purity**: QUASI-PURE ⚠️

#### ⚠️ `fp_partition_gt_i64`
```c
void fp_partition_gt_i64(const int64_t* input, int64_t* output_pass, int64_t* output_fail,
                         size_t n, int64_t threshold,
                         size_t* out_pass_count, size_t* out_fail_count);
```
- **Input**: `const` ✅
- **Output**: Caller-allocated (two buffers) ⚠️
- **Purity**: QUASI-PURE ⚠️

#### ⚠️ `fp_take_while_gt_i64` / `fp_drop_while_gt_i64`
```c
size_t fp_take_while_gt_i64(const int64_t* input, int64_t* output, size_t n, int64_t threshold);
size_t fp_drop_while_gt_i64(const int64_t* input, int64_t* output, size_t n, int64_t threshold);
```
- **Input**: `const` ✅
- **Output**: Caller-allocated ⚠️
- **Purity**: QUASI-PURE ⚠️

**Status**: Module 7 is QUASI-PURE (acceptable pattern) ⚠️

---

### Module 8: Essential Operations (TIER 1)

#### ⚠️ Index-Based Operations (take, drop, slice)
```c
size_t fp_take_n_i64(const int64_t* input, int64_t* output, size_t array_len, size_t take_count);
size_t fp_drop_n_i64(const int64_t* input, int64_t* output, size_t array_len, size_t drop_count);
size_t fp_slice_i64(const int64_t* input, int64_t* output, size_t array_len,
                     size_t start, size_t end);
```
- **Input**: `const` ✅
- **Output**: Caller-allocated ⚠️
- **Purity**: QUASI-PURE ⚠️

#### ✅ Additional Reductions
```c
int64_t fp_reduce_product_i64(const int64_t* input, size_t n);
double fp_reduce_product_f64(const double* input, size_t n);
```
- **Input**: `const` ✅
- **Mutation**: NONE ✅
- **Purity**: PURE ✅

#### ✅ Search Operations
```c
int64_t fp_find_index_i64(const int64_t* input, size_t n, int64_t target);
bool fp_contains_i64(const int64_t* input, size_t n, int64_t target);
```
- **Input**: `const` ✅
- **Mutation**: NONE ✅
- **Purity**: PURE ✅

#### ⚠️ `fp_reverse_i64`
```c
void fp_reverse_i64(const int64_t* input, int64_t* output, size_t n);
```
- **Input**: `const` ✅
- **Output**: Caller-allocated ⚠️
- **Purity**: QUASI-PURE ⚠️

#### ⚠️ `fp_concat_i64`
```c
size_t fp_concat_i64(const int64_t* input_a, const int64_t* input_b, int64_t* output,
                     size_t len_a, size_t len_b);
```
- **Input**: `const` ✅
- **Output**: Caller-allocated ⚠️
- **Purity**: QUASI-PURE ⚠️

#### ❌ `fp_replicate_i64` / `fp_replicate_f64` - **CRITICAL VIOLATION**
```c
void fp_replicate_i64(int64_t* output, size_t n, int64_t value);
void fp_replicate_f64(double* output, size_t n, double value);
```
- **Input**: NONE (generator function) ⚠️
- **Output**: Caller-allocated, but NO INPUT ARRAY ❌
- **Problem**: This is a generator, not a transformer
- **Purity**: **QUASI-GENERATOR** (acceptable for generators) ⚠️

**Analysis**: Actually ACCEPTABLE - this is a generator function like `replicate n x` in Haskell. No input to mutate.

#### ❌ `fp_sort_i64` / `fp_sort_f64` - **CRITICAL VIOLATION**
```c
void fp_sort_i64(int64_t* array, size_t n);
void fp_sort_f64(double* array, size_t n);
```
- **Input**: **NON-CONST** ❌
- **Mutation**: **IN-PLACE SORT** ❌
- **Purity**: **IMPURE** ❌

**Problem**: Directly mutates user's array! This is the MOST SEVERE violation.

**User Impact**:
```c
int64_t data[100] = {5, 2, 9, 1};  // User's data
fp_sort_i64(data, 100);             // ← MUTATES user's original array!
// data is now {1, 2, 5, 9} - original destroyed!
```

**Fix Required**: Remove these functions OR change to:
```c
void fp_sort_copy_i64(const int64_t* input, int64_t* output, size_t n);
```

#### ⚠️ Set Operations (unique, union, intersect)
```c
size_t fp_unique_i64(const int64_t* input, int64_t* output, size_t n);
size_t fp_union_i64(const int64_t* array_a, const int64_t* array_b, int64_t* output,
                     size_t len_a, size_t len_b);
size_t fp_intersect_i64(const int64_t* array_a, const int64_t* array_b, int64_t* output,
                         size_t len_a, size_t len_b);
```
- **Input**: `const` ✅
- **Output**: Caller-allocated ⚠️
- **Purity**: QUASI-PURE ⚠️

#### ⚠️ Grouping Operations
```c
size_t fp_group_i64(const int64_t* input, int64_t* groups_out, int64_t* counts_out, size_t n);
size_t fp_run_length_encode_i64(const int64_t* input, int64_t* output, size_t n);
```
- **Input**: `const` ✅
- **Output**: Caller-allocated ⚠️
- **Purity**: QUASI-PURE ⚠️

#### ❌ `fp_iterate_add_i64` / `fp_iterate_mul_i64` - **ACCEPTABLE (Generators)**
```c
void fp_iterate_add_i64(int64_t* output, size_t n, int64_t start, int64_t step);
void fp_iterate_mul_i64(int64_t* output, size_t n, int64_t start, int64_t factor);
```
- **Input**: NONE (generators) ✅
- **Output**: Caller-allocated ⚠️
- **Analysis**: These are generators (like `iterate` in Haskell), acceptable pattern
- **Purity**: **QUASI-GENERATOR** (acceptable) ⚠️

#### ⚠️ `fp_range_i64`
```c
size_t fp_range_i64(int64_t* output, int64_t start, int64_t end);
```
- **Input**: NONE (generator) ✅
- **Output**: Caller-allocated ⚠️
- **Purity**: **QUASI-GENERATOR** (acceptable) ⚠️

#### ✅ Boolean Reductions
```c
bool fp_reduce_and_bool(const int64_t* input, size_t n);
bool fp_reduce_or_bool(const int64_t* input, size_t n);
```
- **Input**: `const` ✅
- **Mutation**: NONE ✅
- **Purity**: PURE ✅

#### ⚠️ `fp_zip_with_index_i64`
```c
size_t fp_zip_with_index_i64(const int64_t* input, int64_t* output, size_t n);
```
- **Input**: `const` ✅
- **Output**: Caller-allocated ⚠️
- **Purity**: QUASI-PURE ⚠️

#### ⚠️ `fp_count_i64`
```c
size_t fp_count_i64(const int64_t* input, size_t n, int64_t target);
```
- **Input**: `const` ✅
- **Mutation**: NONE ✅
- **Purity**: PURE ✅

**Status**: Module 8 has **2 CRITICAL violations** (sort functions) ❌

---

### Statistical Algorithms (Recent Additions)

*(See previous audit for details)*

#### Algorithm #1: Descriptive Statistics ✅
- All functions PURE

#### Algorithm #2: Percentiles ❌
- **3 functions require sorted input** (forces user mutation)

#### Algorithm #3: Correlation ✅
- All functions PURE

#### Algorithm #4: Linear Regression ⚠️
- Quasi-pure (output struct parameter)

#### Algorithm #5: Outlier Detection ❌
- IQR method requires sorted input

#### Algorithm #6: Moving Averages ⚠️
- Quasi-pure (output array parameter)

---

## Critical Issues Detail

### Issue #1: In-Place Sorting Functions

**Functions**:
- `fp_sort_i64(int64_t* array, size_t n)`
- `fp_sort_f64(double* array, size_t n)`

**Severity**: CRITICAL ❌

**Problem**: These functions take non-const pointers and sort arrays IN-PLACE, destroying the original data.

**User Impact**:
```c
double prices[1000] = { /* user's data */ };

// User needs sorted data for percentile
fp_sort_f64(prices, 1000);  // ← DESTROYS original prices!

// Original data is GONE, user can't get it back without re-loading
```

**FP Violation**: Direct state mutation of user data.

**Proposed Fix**:

**Option A**: Remove entirely (RECOMMENDED)
- Users should never need to call sort directly
- All functions requiring sorted data should sort internally

**Option B**: Change to copy-based:
```c
void fp_sort_copy_i64(const int64_t* input, int64_t* output, size_t n);
```

**Option C**: Rename to make mutation explicit:
```c
void fp_sort_inplace_i64(int64_t* array, size_t n);  // Name warns of mutation
```

**Recommendation**: **Option A** - Remove these functions. Replace with internal sorting in functions that need it.

---

### Issue #2: Sorted Input Requirements

**Functions**:
- `fp_percentile_f64(const double* sorted_data, ...)`
- `fp_percentiles_f64(const double* sorted_data, ...)`
- `fp_quartiles_f64(const double* sorted_data, ...)`
- `fp_detect_outliers_iqr_f64(const double* sorted_data, ...)`

**Severity**: HIGH ⚠️

**Problem**: Forcing users to sort data before calling functions violates FP by proxy.

**User Code Pattern (Forced)**:
```c
double data[1000] = { /* user's data */ };

// User MUST mutate to use these functions:
fp_sort_f64(data, 1000);  // ← MUTATION forced by API!

result = fp_percentile_f64(data, 1000, 0.5);
```

**FP Violation**: Forces user to mutate their data.

**Proposed Fix**: Internal sorting with copy
```c
double fp_percentile_f64(const double* data, size_t n, double p) {
    // Internal implementation:
    double* temp = malloc(n * sizeof(double));
    memcpy(temp, data, n * sizeof(double));
    qsort(temp, n, ...);  // Sort the COPY, not user's data
    result = compute_percentile(temp, n, p);
    free(temp);
    return result;
}
```

**Trade-offs**:
- **Pro**: Pure FP, user data never mutated
- **Con**: Memory allocation overhead
- **Con**: O(n log n) sorting overhead

**Optimization**: Provide both variants:
```c
// Pure version (default, recommended)
double fp_percentile_f64(const double* data, size_t n, double p);

// Fast version (requires pre-sorted, for performance-critical code)
double fp_percentile_sorted_f64(const double* sorted_data, size_t n, double p);
```

---

### Issue #3: Generator Functions (Actually Acceptable)

**Functions**:
- `fp_replicate_i64/f64` - Creates array of repeated value
- `fp_iterate_add_i64/mul_i64` - Generates arithmetic/geometric sequences
- `fp_range_i64` - Generates integer range

**Analysis**: These are GENERATORS, not transformers.

**Haskell Equivalent**:
```haskell
replicate 10 42     -- [42,42,42,42,42,42,42,42,42,42]
iterate (+1) 0      -- [0,1,2,3,4,5,6,7,8,9,...]
range 1 10          -- [1,2,3,4,5,6,7,8,9,10]
```

**Verdict**: ACCEPTABLE ✅

These don't have "input" to mutate - they create new data from parameters.

---

## Summary Matrix

| Module | Pure | Quasi-Pure | Impure | Critical Issues |
|--------|------|------------|--------|-----------------|
| Module 1: Simple Folds | 4 | 0 | 0 | NONE ✅ |
| Module 2: Fused Folds | 4 | 0 | 0 | NONE ✅ |
| Module 3: Fused Maps | 0 | 8 | 0 | NONE ⚠️ |
| Module 4: Simple Maps | 0 | 6 | 0 | NONE ⚠️ |
| Module 5: Scans | 0 | 2 | 0 | NONE ⚠️ |
| Module 6: Predicates | 3 | 0 | 0 | NONE ✅ |
| Module 7: Compaction | 0 | 5 | 0 | NONE ⚠️ |
| Module 8: Essentials | 8 | 15 | 2 | **2 CRITICAL** ❌ |
| Statistics | 8 | 10 | 4 | **4 HIGH** ⚠️ |
| **TOTAL** | **27** | **46** | **6** | **6 VIOLATIONS** |

---

## Immediate Action Plan

### Phase 1: Fix Critical Violations (IMMEDIATE)

**Priority 1A: Remove In-Place Sorting**
1. ❌ Remove `fp_sort_i64` and `fp_sort_f64` entirely
2. Update all internal code that might use them
3. Document: "This library does not provide in-place mutation functions"

**Priority 1B: Fix Sorted Input Requirements**
1. ❌ Modify `fp_percentile_f64` to accept unsorted data
2. ❌ Modify `fp_percentiles_f64` to accept unsorted data
3. ❌ Modify `fp_quartiles_f64` to accept unsorted data
4. ❌ Modify `fp_detect_outliers_iqr_f64` to accept unsorted data

**Implementation**: Internal copy-and-sort pattern
**Timeline**: Before any new development

---

### Phase 2: API Consistency (HIGH PRIORITY)

1. Ensure all transformer functions follow consistent patterns
2. Document purity contract in every function
3. Add const-correctness verification tests

---

### Phase 3: Documentation (MEDIUM PRIORITY)

1. Create DESIGN_PRINCIPLES.md
2. Update README with FP guarantees
3. Add purity annotations to all function docs

---

## Recommendations

### 1. Adopt Strict Purity Policy

**Policy**: No function may:
- Take non-const input pointers
- Require users to mutate data before calling
- Perform hidden state mutation

**Exceptions**: Generator functions (no input to mutate)

### 2. Pattern Guidelines

**Pure Functions** (Preferred):
```c
double fp_pure_function(const double* data, size_t n);
```

**Map Functions** (Acceptable):
```c
void fp_map_function(const double* input, double* output, size_t n);
```

**Generators** (Acceptable):
```c
void fp_generate_range(int64_t* output, int64_t start, int64_t end);
```

**Forbidden**:
```c
void fp_mutate_function(double* data, size_t n);  // ❌ NO NON-CONST INPUT!
```

### 3. Migration Strategy

1. **Immediate**: Fix critical violations
2. **Short-term**: Document all functions with purity guarantees
3. **Long-term**: Consider persistent data structures for true immutability

---

## Conclusion

**Current State**: Library has 6 purity violations across 73+ functions.

**Severity Assessment**:
- 2 CRITICAL: Direct mutation functions (sort)
- 4 HIGH: Forced user mutation (sorted requirements)
- 67 ACCEPTABLE: Pure or quasi-pure with const inputs

**Required Action**:
1. Remove in-place sorting functions
2. Fix sorted-input requirements
3. Document purity contract

**Timeline**: Phase 1 must complete before continuing development.

---

**Next Steps**: Await user approval to begin Phase 1 fixes.

---

**Version**: 2.0 (Complete Library Audit)
**Status**: READY FOR REVIEW
**Auditors**: FP-ASM Architecture Team
