# FP-ASM Function Purity Audit

**Date**: 2025-11-01
**Purpose**: Systematic analysis of state mutation in all library functions
**Principle**: A functional programming library must NEVER mutate input state

---

## Executive Summary

**Critical Finding**: The library has **inconsistent mutation semantics** that violate FP principles:

1. ✅ **Scalar functions** are pure (return values, don't mutate)
2. ⚠️ **Array/struct functions** use output parameters (acceptable in C)
3. ❌ **Several functions require pre-sorted input** (forces user to mutate!)
4. ❌ **Inconsistent API patterns** across similar operations

**Recommendation**: Adopt strict immutability policy with clear architectural patterns.

---

## 1. Functional Programming Principles in C

### 1.1 Core FP Requirement: Input Immutability

```c
// ✅ PURE: Never modifies input
double pure_function(const double* input, size_t n) {
    // Read from input, return new value
    // Input remains unchanged
}

// ❌ IMPURE: Modifies input in place
void impure_function(double* input, size_t n) {
    input[0] = 42.0;  // MUTATION! Violates FP!
}

// ⚠️ ACCEPTABLE IN C: Output parameter (not input mutation)
void acceptable_function(const double* input, size_t n, double* output) {
    // Input: const (never modified)
    // Output: caller-allocated buffer (not "mutation" of input state)
    output[0] = compute(input[0]);
}
```

**Key Distinction**:
- **Input mutation**: FORBIDDEN (violates FP)
- **Output filling**: ACCEPTABLE (idiomatic C, input stays immutable)

### 1.2 The Sorted Data Problem

```c
// ❌ FORCES USER MUTATION: Requires pre-sorted input
double percentile(const double* sorted_data, size_t n, double p) {
    // "sorted_data" requirement forces user to sort their data
    // Sorting = IN-PLACE MUTATION of user's original array!
}

// User code:
double data[] = {5, 2, 9, 1};  // Original data
qsort(data, 4, ...);            // ← MUTATION! Original destroyed!
result = percentile(data, 4, 0.5);  // Now data is mutated

// ✅ FP-FRIENDLY: Accepts unsorted input
double percentile_unsorted(const double* data, size_t n, double p) {
    // Internal: Copy data, sort the copy
    // User's original data remains untouched
}
```

---

## 2. Complete Function Audit

### Algorithm #1: Descriptive Statistics

#### ✅ `fp_mean_f64`
```c
double fp_mean_f64(const double* data, size_t n);
```
- **Input**: `const` (immutable) ✅
- **Output**: Scalar return ✅
- **Mutation**: NONE ✅
- **Purity**: PURE ✅

#### ✅ `fp_variance_f64`
```c
double fp_variance_f64(const double* data, size_t n);
```
- **Input**: `const` (immutable) ✅
- **Output**: Scalar return ✅
- **Mutation**: NONE ✅
- **Purity**: PURE ✅

#### ✅ `fp_stddev_f64`
```c
double fp_stddev_f64(const double* data, size_t n);
```
- **Input**: `const` (immutable) ✅
- **Output**: Scalar return ✅
- **Mutation**: NONE ✅
- **Purity**: PURE ✅

#### ✅ `fp_min_f64` / `fp_max_f64`
```c
double fp_min_f64(const double* data, size_t n);
double fp_max_f64(const double* data, size_t n);
```
- **Input**: `const` (immutable) ✅
- **Output**: Scalar return ✅
- **Mutation**: NONE ✅
- **Purity**: PURE ✅

**Status**: Algorithm #1 is FULLY PURE ✅

---

### Algorithm #2: Percentile Calculations

#### ❌ `fp_percentile_f64` - **CRITICAL ISSUE**
```c
double fp_percentile_f64(const double* sorted_data, size_t n, double p);
```
- **Input**: `const` but **REQUIRES PRE-SORTED DATA** ❌
- **Output**: Scalar return ✅
- **Mutation**: None directly, but **FORCES USER TO MUTATE** ❌
- **Purity**: **IMPURE BY PROXY** ❌

**Problem**: User must sort their data before calling:
```c
double data[100];  // User's data
qsort(data, 100, ...);  // ← USER FORCED TO MUTATE!
result = fp_percentile_f64(data, 100, 0.5);
```

**Impact**: User's original data is destroyed by sorting requirement.

**Fix Required**: Provide `fp_percentile_unsorted_f64` variant.

#### ❌ `fp_median_f64` - **CRITICAL ISSUE**
```c
double fp_median_f64(const double* sorted_data, size_t n);
```
- **Same issue as percentile** ❌
- **Requires pre-sorted input** ❌
- **Forces user mutation** ❌

**Status**: Algorithm #2 VIOLATES FP PRINCIPLES ❌

---

### Algorithm #3: Correlation & Covariance

#### ✅ `fp_covariance_f64`
```c
double fp_covariance_f64(const double* x, const double* y, size_t n);
```
- **Input**: `const` (immutable) ✅
- **Output**: Scalar return ✅
- **Mutation**: NONE ✅
- **Purity**: PURE ✅

#### ✅ `fp_correlation_f64`
```c
double fp_correlation_f64(const double* x, const double* y, size_t n);
```
- **Input**: `const` (immutable) ✅
- **Output**: Scalar return ✅
- **Mutation**: NONE ✅
- **Purity**: PURE ✅

**Status**: Algorithm #3 is FULLY PURE ✅

---

### Algorithm #4: Linear Regression

#### ⚠️ `fp_linear_regression_f64` - **DESIGN ISSUE**
```c
typedef struct {
    double slope;
    double intercept;
    double r_squared;
    double std_error;
} LinearRegression;

void fp_linear_regression_f64(const double* x, const double* y,
                               size_t n, LinearRegression* result);
```
- **Input**: `const` (immutable) ✅
- **Output**: Struct pointer (caller-allocated) ⚠️
- **Mutation**: Fills output struct (acceptable in C) ⚠️
- **Purity**: **QUASI-PURE** (input immutable, output is parameter) ⚠️

**Analysis**:
- Inputs are `const` ✅
- Output is caller-allocated buffer ⚠️
- NOT technically mutating user's input state ✅
- Pattern is acceptable in C but not "pure" FP ⚠️

**Alternative Pure Design**:
```c
// Return struct by value (C99+)
LinearRegression fp_linear_regression_f64(const double* x,
                                          const double* y, size_t n);
```

#### ✅ `fp_predict_f64`
```c
double fp_predict_f64(double x_value, const LinearRegression* model);
```
- **Input**: `const` (immutable) ✅
- **Output**: Scalar return ✅
- **Mutation**: NONE ✅
- **Purity**: PURE ✅

**Status**: Algorithm #4 is QUASI-PURE (acceptable but not ideal) ⚠️

---

### Algorithm #5: Outlier Detection

#### ⚠️ `fp_detect_outliers_zscore_f64` - **DESIGN ISSUE**
```c
size_t fp_detect_outliers_zscore_f64(const double* data, size_t n,
                                      double threshold, uint8_t* is_outlier);
```
- **Input**: `const` (immutable) ✅
- **Output**: Array pointer (caller-allocated) ⚠️
- **Mutation**: Fills output array (acceptable in C) ⚠️
- **Purity**: **QUASI-PURE** ⚠️

**Analysis**: Same as linear regression - acceptable pattern but not pure FP.

#### ❌ `fp_detect_outliers_iqr_f64` - **CRITICAL ISSUE**
```c
size_t fp_detect_outliers_iqr_f64(const double* sorted_data, size_t n,
                                   double k_factor, uint8_t* is_outlier);
```
- **Input**: **REQUIRES PRE-SORTED DATA** ❌
- **Output**: Array pointer ⚠️
- **Mutation**: Forces user to sort (mutate) input ❌
- **Purity**: **IMPURE BY PROXY** ❌

**Status**: Algorithm #5 PARTIALLY VIOLATES FP PRINCIPLES ❌

---

### Algorithm #6: Moving Averages

#### ⚠️ `fp_map_sma_f64` - **DESIGN ISSUE**
```c
void fp_map_sma_f64(const double* data, size_t n, size_t window, double* output);
```
- **Input**: `const` (immutable) ✅
- **Output**: Array pointer (caller-allocated) ⚠️
- **Mutation**: Fills output array (acceptable in C) ⚠️
- **Purity**: **QUASI-PURE** ⚠️

#### ⚠️ `fp_map_ema_f64` - **DESIGN ISSUE**
```c
void fp_map_ema_f64(const double* data, size_t n, size_t window, double* output);
```
- **Input**: `const` (immutable) ✅
- **Output**: Array pointer (caller-allocated) ⚠️
- **Mutation**: Fills output array (acceptable in C) ⚠️
- **Purity**: **QUASI-PURE** ⚠️

#### ⚠️ `fp_map_wma_f64` - **DESIGN ISSUE**
```c
void fp_map_wma_f64(const double* data, size_t n, size_t window, double* output);
```
- **Input**: `const` (immutable) ✅
- **Output**: Array pointer (caller-allocated) ⚠️
- **Mutation**: Fills output array (acceptable in C) ⚠️
- **Purity**: **QUASI-PURE** ⚠️

**Status**: Algorithm #6 is QUASI-PURE (acceptable but not ideal) ⚠️

---

## 3. Summary of Issues

### 3.1 Critical Violations (Must Fix)

| Function | Issue | Impact | Priority |
|----------|-------|--------|----------|
| `fp_percentile_f64` | Requires sorted input | Forces user to mutate | **CRITICAL** |
| `fp_median_f64` | Requires sorted input | Forces user to mutate | **CRITICAL** |
| `fp_detect_outliers_iqr_f64` | Requires sorted input | Forces user to mutate | **CRITICAL** |

### 3.2 Design Inconsistencies (Should Fix)

| Pattern | Functions | Issue | Priority |
|---------|-----------|-------|----------|
| Output struct pointer | `fp_linear_regression_f64` | Not pure FP | **HIGH** |
| Output array pointer | All map/transform functions | Not pure FP | **MEDIUM** |

### 3.3 Pure Functions (No Issues)

✅ All scalar-returning functions (mean, variance, stddev, min, max, correlation, etc.)

---

## 4. Proposed Solutions

### 4.1 Critical Fix: Eliminate Sorted Input Requirement

**Current (Impure)**:
```c
// ❌ Forces user to mutate their data
double fp_percentile_f64(const double* sorted_data, size_t n, double p);

// User code:
double data[100];
qsort(data, 100, ...);  // ← MUTATION!
result = fp_percentile_f64(data, 100, 0.5);
```

**Proposed (Pure)**:
```c
// ✅ Accepts unsorted data, handles sorting internally
double fp_percentile_f64(const double* data, size_t n, double p);

// User code:
double data[100];  // Original data
result = fp_percentile_f64(data, 100, 0.5);  // data unchanged!
```

**Implementation Strategy**:
1. Allocate temporary buffer inside function
2. Copy input data to temporary buffer
3. Sort the temporary buffer (not user's data!)
4. Compute percentile from sorted copy
5. Free temporary buffer
6. Return result

**Trade-offs**:
- **Pro**: Pure FP, user data never mutated ✅
- **Pro**: Simpler API (no pre-sorting requirement) ✅
- **Con**: Memory allocation overhead (one array copy)
- **Con**: Performance overhead (O(n log n) sorting inside function)

**Optimization**: Provide BOTH variants:
```c
// Pure version: Accepts unsorted (default, recommended)
double fp_percentile_f64(const double* data, size_t n, double p);

// Fast version: Requires sorted (for performance-critical code)
double fp_percentile_sorted_f64(const double* sorted_data, size_t n, double p);
```

### 4.2 Design Improvement: Return Structs by Value

**Current (Quasi-Pure)**:
```c
void fp_linear_regression_f64(const double* x, const double* y,
                               size_t n, LinearRegression* result);

// User code:
LinearRegression result;  // Must allocate
fp_linear_regression_f64(x, y, n, &result);  // Fill pointer
```

**Proposed (Pure)**:
```c
LinearRegression fp_linear_regression_f64(const double* x,
                                          const double* y, size_t n);

// User code:
LinearRegression result = fp_linear_regression_f64(x, y, n);  // Direct return
```

**Benefits**:
- More functional style ✅
- Cleaner API ✅
- No pointer passing needed ✅
- Compiler can optimize (RVO) ✅

**Compatibility**: C99+ supports struct return by value efficiently.

### 4.3 Array Output Pattern: Accept as Necessary

**Current Pattern (Acceptable)**:
```c
void fp_map_sma_f64(const double* data, size_t n, size_t window, double* output);
```

**Analysis**:
- Input is `const` (never mutated) ✅
- Output is caller-allocated (clear ownership) ✅
- No dynamic allocation (performance-friendly) ✅
- Idiomatic C pattern ✅

**Recommendation**: **KEEP THIS PATTERN** for array outputs.

**Rationale**:
1. Returning arrays in C requires allocation (memory management burden)
2. Caller-allocated output is standard BLAS/LAPACK pattern
3. Input immutability is preserved (key FP requirement)
4. Performance-optimal (no hidden allocations)

**Document clearly**:
```c
/**
 * fp_map_sma_f64 - Simple Moving Average
 *
 * @param data   Input array (NEVER modified, guaranteed immutable)
 * @param n      Number of input elements
 * @param window Window size
 * @param output Output array (caller-allocated, size = n - window + 1)
 *
 * PURITY: Input data is never modified. Output parameter is caller-owned.
 * THREAD-SAFETY: Safe if output buffers don't overlap between threads.
 */
void fp_map_sma_f64(const double* data, size_t n, size_t window, double* output);
```

---

## 5. Architectural Design Principles

### 5.1 The FP-ASM Purity Contract

**Principle 1: Input Immutability (MANDATORY)**
```
ALL input data MUST be declared `const` and NEVER modified.
Violation: Compilation error or undefined behavior.
```

**Principle 2: No Hidden Mutations (MANDATORY)**
```
Functions MUST NOT require users to mutate their data before calling.
Example: No "sorted_data" parameters without unsorted alternatives.
```

**Principle 3: Predictable Output Patterns (RECOMMENDED)**
```
- Scalars → Return by value
- Small structs (≤32 bytes) → Return by value
- Arrays → Output parameter (caller-allocated)
- Large structs (>32 bytes) → Output parameter OR return by value (C99+)
```

**Principle 4: Clear Documentation (MANDATORY)**
```
Every function MUST document:
- Which parameters are inputs (const, never modified)
- Which parameters are outputs (caller-allocated)
- Memory ownership and lifetime
- Thread-safety guarantees
```

### 5.2 Function Signature Patterns

#### Pattern A: Pure Scalar Function ✅
```c
/**
 * Pure function: Returns scalar, never mutates input.
 * Thread-safe: Yes (no shared state).
 */
double fp_mean_f64(const double* data, size_t n);
```

#### Pattern B: Pure Struct Function ✅
```c
/**
 * Pure function: Returns struct, never mutates input.
 * Thread-safe: Yes (no shared state).
 */
LinearRegression fp_linear_regression_f64(const double* x,
                                          const double* y, size_t n);
```

#### Pattern C: Map Function (Acceptable) ⚠️
```c
/**
 * Map function: Transforms input to output.
 * Input: Immutable (const, never modified).
 * Output: Caller-allocated buffer.
 * Thread-safe: Yes (if output buffers are distinct).
 */
void fp_map_sma_f64(const double* data, size_t n,
                size_t window, double* output);
```

#### Pattern D: Compound Result (Acceptable) ⚠️
```c
/**
 * Returns multiple values via output parameters.
 * Input: Immutable (const, never modified).
 * Outputs: Caller-allocated.
 * Thread-safe: Yes (if output buffers are distinct).
 */
void fp_stats_f64(const double* data, size_t n,
                  double* mean, double* variance, double* stddev);
```

---

## 6. Migration Plan

### Phase 1: Fix Critical Violations (IMMEDIATE)

**Targets**:
1. `fp_percentile_f64` - Remove sorted requirement
2. `fp_median_f64` - Remove sorted requirement
3. `fp_detect_outliers_iqr_f64` - Remove sorted requirement

**Action**:
- Implement internal sorting (copy-then-sort pattern)
- Keep `_sorted` variants for performance-critical users
- Update all documentation and examples

**Timeline**: Before implementing any new algorithms

### Phase 2: Improve Struct Returns (HIGH PRIORITY)

**Targets**:
1. `fp_linear_regression_f64` - Return struct by value

**Action**:
- Change signature to return `LinearRegression` by value
- Update assembly to use struct return ABI
- Test on multiple compilers

**Timeline**: Before Algorithm #10

### Phase 3: Document Purity Contract (MEDIUM PRIORITY)

**Actions**:
1. Add purity annotations to all function docs
2. Create DESIGN_PRINCIPLES.md
3. Update README with FP guarantees
4. Add compile-time const-correctness checks

**Timeline**: Before public release

### Phase 4: Consider Advanced Patterns (LOW PRIORITY)

**Explorations**:
1. Persistent data structures for true immutability?
2. Copy-on-write optimization for large arrays?
3. Arena allocators for temporary buffers?

**Timeline**: Future research (post-v1.0)

---

## 7. Testing the Purity Contract

### 7.1 Const-Correctness Test

```c
// This should compile
const double data[100] = { /* ... */ };
double result = fp_mean_f64(data, 100);

// This should NOT compile (if we accidentally remove const)
void bad_function(double* data, size_t n);  // Missing const!
bad_function(data, 100);  // ← Compilation error! Good!
```

### 7.2 Input Preservation Test

```c
// Verify input is never modified
double original[100];
double copy[100];
memcpy(copy, original, sizeof(original));

fp_percentile_f64(original, 100, 0.5);

// Verify: original should be unchanged
assert(memcmp(original, copy, sizeof(original)) == 0);
```

### 7.3 Thread-Safety Test

```c
// Multiple threads should be able to read same input
const double shared_data[1000] = { /* ... */ };

#pragma omp parallel for
for (int i = 0; i < 100; i++) {
    double result = fp_mean_f64(shared_data, 1000);
    // Should work without data races
}
```

---

## 8. Documentation Template

For every function, include:

```c
/**
 * fp_function_name - Brief description
 *
 * Detailed description of what the function does.
 *
 * @param input   Input data (IMMUTABLE - never modified) [const]
 * @param n       Input size
 * @param output  Output buffer (caller-allocated, size must be X)
 *
 * @return Description of return value
 *
 * PURITY:
 *   - Input immutability: GUARANTEED (input never modified)
 *   - Side effects: NONE (pure computation)
 *   - Output: Caller-owned buffer (no internal allocation)
 *
 * THREAD-SAFETY:
 *   - Safe: YES (if output buffers are distinct per thread)
 *   - Re-entrant: YES
 *   - Lock-free: YES
 *
 * COMPLEXITY:
 *   - Time: O(?)
 *   - Space: O(?)
 *
 * EXAMPLE:
 *   double data[100] = { ... };  // Your data
 *   double output[100];
 *   fp_function_name(data, 100, output);
 *   // data is UNCHANGED after call
 */
void fp_function_name(const double* input, size_t n, double* output);
```

---

## 9. Final Recommendations

### 9.1 Immediate Actions (Before Algorithm #7)

1. ✅ **Fix percentile/median**: Remove sorted requirement
2. ✅ **Fix IQR outlier detection**: Remove sorted requirement
3. ✅ **Audit all existing functions**: Verify const correctness
4. ✅ **Add purity tests**: Verify inputs never change
5. ✅ **Update documentation**: Add purity guarantees

### 9.2 Design Principles Going Forward

1. **NEVER require pre-sorted input** (provide fast variants if needed)
2. **ALWAYS use const for inputs** (compiler-enforced immutability)
3. **Return scalars/small structs by value** (pure FP style)
4. **Use output parameters for arrays** (performance + clarity)
5. **Document purity contract** (guarantee to users)

### 9.3 Quality Gate

Before ANY function is considered complete:

- [ ] All inputs declared `const`
- [ ] No pre-processing requirements (sorting, etc.)
- [ ] Purity documented
- [ ] Thread-safety documented
- [ ] Input preservation test passes
- [ ] Const-correctness test compiles

---

## Conclusion

**Current State**: Library has 3 CRITICAL purity violations and several design inconsistencies.

**Required Action**: Immediate fix for sorted-input requirements before continuing to Algorithm #7.

**Long-term Vision**: FP-ASM should be a **reference implementation** of functional programming principles in high-performance C. Every function should have a clear purity contract, and users should trust that their input data is NEVER mutated.

**Next Steps**:
1. Fix percentile/median implementations (internal sorting)
2. Fix IQR outlier detection (internal sorting)
3. Consider struct-by-value returns for linear regression
4. Continue to Algorithm #7 with strict purity guidelines

---

**Version**: 1.0
**Status**: DRAFT - Requires user approval and prioritization
**Author**: FP-ASM Architecture Team
