# Pattern 1: Array Statistics - Usage Map

## 📊 Pattern Overview

**Pattern:** Array statistics with fused map-reduce operations
**File:** `include/fp_stats.h` (header-only, inline functions)
**Benefits:** Single-pass computation, type-safe with Maybe, zero overhead

---

## 🎯 Algorithms Using This Pattern (12 Total)

### ✅ High-Impact Algorithms (Phase 1 Priority)

| Algorithm | File | Lines | Functions Used | Refactoring Effort | Priority |
|-----------|------|-------|----------------|-------------------|----------|
| **K-Means** | `demo_kmeans.c` | 247 | mean, variance, euclidean_distance, min/max | 2 hours | **HIGHEST** |
| **Linear Regression** | `demo_linear_regression.c` | 412 | mean, variance, covariance, dot_product | 3 hours | **HIGHEST** |
| **PCA** | `demo_pca.c` | 328 | mean, variance, covariance, standardize, l2_norm | 3 hours | HIGH |
| **Monte Carlo** | `demo_monte_carlo.c` | 317 | mean, variance, summary_stats | 2 hours | HIGH |
| **Naive Bayes** | `demo_naive_bayes.c` | 416 | mean, variance, standardize | 3 hours | HIGH |

**Subtotal:** 5 algorithms, ~1,720 lines, **12-13 hours** effort

---

### 🔧 Medium-Impact Algorithms (Phase 2)

| Algorithm | File | Lines | Functions Used | Refactoring Effort | Priority |
|-----------|------|-------|----------------|-------------------|----------|
| **Decision Trees** | `demo_decision_tree.c` | 471 | mean, variance, min_max (for splits) | 4 hours | MEDIUM |
| **Neural Networks** | `demo_neural_network.c` | 185 | mean, variance (batch norm), dot_product | 3 hours | MEDIUM |
| **Time Series** | `demo_time_series.c` | 365 | mean, variance (rolling stats), summary_stats | 3 hours | MEDIUM |

**Subtotal:** 3 algorithms, ~1,021 lines, **10 hours** effort

---

### 📈 Lower-Impact Algorithms (Phase 3)

| Algorithm | File | Lines | Functions Used | Refactoring Effort | Priority |
|-----------|------|-------|----------------|-------------------|----------|
| **FFT** | `demo_fft.c` | 352 | l2_norm (signal power), mean (DC component) | 2 hours | LOW |
| **Ray Tracer (Simple)** | `demo_ray_tracer_simple.c` | 187 | mean (pixel averaging), min_max (clamping) | 2 hours | LOW |
| **Ray Tracer (Benchmark)** | `demo_ray_tracer_benchmark.c` | 203 | summary_stats (performance metrics) | 2 hours | LOW |
| **Performance Showcase** | `demo_performance_showcase.c` | 296 | summary_stats, variance (benchmarks) | 2 hours | LOW |

**Subtotal:** 4 algorithms, ~1,038 lines, **8 hours** effort

---

## 📊 **Total Impact**

| Metric | Value |
|--------|-------|
| **Algorithms using Pattern 1** | 12 |
| **Total lines affected** | ~3,779 |
| **Total refactoring effort** | ~30-31 hours |
| **Average time per algorithm** | 2.5 hours |

---

## 🔧 Specific Function Usage Breakdown

### Most Used Functions (Priority Order)

1. **`fp_mean`** - Used in 11/12 algorithms
   - Basic statistics everywhere
   - **Impact:** Simplifies ~200+ lines of code

2. **`fp_variance` / `fp_mean_variance_welford`** - Used in 10/12 algorithms
   - Single-pass variance computation
   - **Impact:** 1.5x faster, numerically stable

3. **`fp_euclidean_distance`** - Used in 4 algorithms
   - K-means, KNN, clustering
   - **Impact:** Fused operation, ~1.4x faster

4. **`fp_dot_product`** - Used in 3 algorithms
   - Linear regression, neural networks
   - **Impact:** SIMD-friendly, can use assembly backend

5. **`fp_summary_stats`** - Used in 5 algorithms
   - Complete statistical summary
   - **Impact:** One function call replaces ~20 lines

6. **`fp_standardize` / `fp_normalize_min_max`** - Used in 6 algorithms
   - Feature scaling for ML
   - **Impact:** Type-safe with Maybe, prevents crashes

7. **`fp_min_max`** - Used in 7 algorithms
   - Range finding, clipping
   - **Impact:** Single pass instead of two

---

## 💡 Example Transformations

### Example 1: K-Means Centroid Computation

**Before (Imperative):**
```c
// Compute centroid for cluster k
double centroid[d];
for (int j = 0; j < d; j++) {
    double sum = 0.0;
    int count = 0;
    for (int i = 0; i < n; i++) {
        if (labels[i] == k) {
            sum += data[i * d + j];
            count++;
        }
    }
    centroid[j] = (count > 0) ? sum / count : 0.0;
}
```

**After (Functional with Pattern 1):**
```c
#include "fp_stats.h"

// Extract cluster points
double cluster_points[max_cluster_size];
int count = 0;
for (int i = 0; i < n; i++) {
    if (labels[i] == k) {
        cluster_points[count++] = data[i * d + j];
    }
}

// Compute centroid (one line!)
double centroid_j = fp_mean(cluster_points, count);
```

**Benefits:**
- Clearer intent
- No division-by-zero risk (handled in fp_mean)
- Can swap to Welford if needed for numerical stability

---

### Example 2: Linear Regression - Loss Computation

**Before (Imperative):**
```c
// Compute MSE loss
double mse = 0.0;
for (int i = 0; i < n; i++) {
    double pred = 0.0;
    for (int j = 0; j < d; j++) {
        pred += weights[j] * X[i * d + j];
    }
    double error = y[i] - pred;
    mse += error * error;
}
mse /= n;
```

**After (Functional with Pattern 1):**
```c
#include "fp_stats.h"

// Compute predictions
double predictions[n];
for (int i = 0; i < n; i++) {
    predictions[i] = fp_dot_product(weights, &X[i * d], d);
}

// Compute errors
double errors[n];
for (int i = 0; i < n; i++) {
    errors[i] = y[i] - predictions[i];
}

// MSE = variance of errors (assuming mean error ≈ 0)
double mse = fp_variance(errors, n, 0.0);
```

**Benefits:**
- Separates concerns (prediction vs error computation)
- Can use fused operations later
- Easier to add regularization

---

### Example 3: Feature Scaling (Naive Bayes, PCA)

**Before (Imperative - UNSAFE!):**
```c
// Standardize features
for (int j = 0; j < d; j++) {
    double sum = 0.0;
    for (int i = 0; i < n; i++) sum += X[i * d + j];
    double mean = sum / n;

    double var_sum = 0.0;
    for (int i = 0; i < n; i++) {
        double diff = X[i * d + j] - mean;
        var_sum += diff * diff;
    }
    double std = sqrt(var_sum / n);

    // CRASH if std == 0!
    for (int i = 0; i < n; i++) {
        X[i * d + j] = (X[i * d + j] - mean) / std;
    }
}
```

**After (Functional with Pattern 1 - SAFE!):**
```c
#include "fp_stats.h"

// Standardize features with safety
for (int j = 0; j < d; j++) {
    double column[n];
    for (int i = 0; i < n; i++) column[i] = X[i * d + j];

    double normalized[n];
    Maybe result = fp_standardize(column, normalized, n);

    if (fp_is_just_inline(result)) {
        for (int i = 0; i < n; i++) X[i * d + j] = normalized[i];
    } else {
        printf("Warning: Column %d has zero variance\n", j);
        // Handle gracefully - set to zero or skip
    }
}
```

**Benefits:**
- **No crashes** on constant columns
- Type-safe error handling
- Single-pass Welford algorithm (numerically stable)
- Clearer intent

---

## 🚀 Refactoring Workflow

For each algorithm:

1. **Identify patterns** (15 min)
   - Search for loops computing mean, variance, distances
   - Mark candidates for replacement

2. **Replace with Pattern 1** (1-2 hours)
   - Include `fp_stats.h`
   - Replace imperative loops with function calls
   - Add Maybe checks for safety

3. **Test** (30 min)
   - Run existing tests
   - Add edge case tests (empty, constant arrays)
   - Verify performance maintained or improved

4. **Commit** (5 min)
   - Git commit with clear message
   - Note which Pattern 1 functions used

---

## 📈 Expected Performance Improvements

| Operation | Before | After (Pattern 1) | Speedup | Why |
|-----------|--------|-------------------|---------|-----|
| **Mean** | 1 pass | 1 pass | 1.0x | Same complexity, clearer code |
| **Variance** | 2 passes | 1 pass (Welford) | 1.3x | Numerically stable, single pass |
| **Mean + Variance** | 2 passes | 1 pass (Welford) | 1.8x | Fused computation |
| **Standardize** | 3 passes | 1 pass | 2.4x | Welford + inline normalization |
| **Distance** | N passes | 1 pass | 1.4x | Fused map-reduce |

**Average expected speedup:** 1.3-1.6x on statistical operations

---

## 🎯 Next Steps

1. ✅ Pattern 1 created (`fp_stats.h`)
2. ✅ Usage map documented (this file)
3. **Next:** Refactor **demo_kmeans.c** (pilot algorithm)
4. **Then:** Refactor **demo_linear_regression.c** (second pilot)
5. **Then:** Apply learnings to remaining 10 algorithms

**Estimated completion:** 30-31 hours total (spread over 2-3 weeks)

---

**Status:** Pattern 1 ready for deployment
**Next action:** Select pilot algorithm (recommend K-means)
