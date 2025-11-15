# FP-ASM Library: Functional Programming Philosophy

## Core Mission

**fp_asm_lib_dev is a functional-first systems programming library.** Every design decision, every line of code, every API is guided by functional programming principles. Performance is critical, but **FP principles come first** - we only deviate when measurement proves it necessary.

---

## The Four Foundational Tenets (Non-Negotiable)

### 1. Immutability

**All input data is immutable.**

```c
// ✅ CORRECT: Input marked const, never modified
int64_t fp_reduce_add_i64(const int64_t* input, size_t n);

// ❌ FORBIDDEN: Modifying input
void bad_function(int64_t* data, size_t n) {
    data[0] = 0;  // MUTATION - violates immutability!
}
```

**Enforcement:**
- All input pointers declared `const`
- Compiler enforces immutability
- Zero tolerance for violations

**Benefits:**
- Thread-safe by default
- Referentially transparent (same input → same output)
- Composable without fear of side effects

---

### 2. Composition

**Complex operations built from simple, composable primitives.**

```c
// ✅ CORRECT: Composed from primitives
void fp_linear_regression_f64(const double* x, const double* y, size_t n,
                              LinearRegression* out) {
    // Each operation is a pure, composable function
    double mean_x = fp_reduce_add_f64(x, n) / n;
    double mean_y = fp_reduce_add_f64(y, n) / n;

    double* x_centered = malloc(n * sizeof(double));
    double* y_centered = malloc(n * sizeof(double));
    fp_map_offset_f64(x, x_centered, n, -mean_x);
    fp_map_offset_f64(y, y_centered, n, -mean_y);

    double covariance = fp_fold_dotp_f64(x_centered, y_centered, n);
    double variance = fp_fold_sumsq_f64(x_centered, n);

    out->slope = covariance / variance;
    out->intercept = mean_y - out->slope * mean_x;
    // ... more composition ...

    free(x_centered);
    free(y_centered);
}

// ❌ FORBIDDEN: Reimplementing from scratch
void bad_linear_regression(const double* x, const double* y, size_t n,
                           LinearRegression* out) {
    // 200+ lines of imperative loops reimplementing everything
    // - Cannot reuse existing optimized primitives
    // - Duplicates effort
    // - Error-prone
}
```

**Real Result:** Linear regression: **272 imperative lines → 25 composed lines (90.8% reduction)**

**Benefits:**
- Massive code reduction (87% across statistics module)
- Reuse of battle-tested, optimized primitives
- Easier to understand and maintain
- Automatic performance inheritance (optimize primitive → all compositions benefit)

---

### 3. Modularity

**Each function does one thing, perfectly.**

```c
// ✅ CORRECT: Single responsibility
float fp_fold_dotp_f32(const float* a, const float* b, size_t n);  // JUST dot product
float fp_reduce_add_f32(const float* a, size_t n);                 // JUST summation
void  fp_map_scale_f32(const float* in, float* out, size_t n, float c);  // JUST scaling

// ❌ FORBIDDEN: God functions that do multiple things
void bad_compute_everything(float* data, size_t n, Results* out) {
    // Computes mean, variance, skewness, kurtosis, regression, ALL IN ONE
    // - Impossible to reuse parts
    // - Hard to test
    // - Violates modularity
}
```

**Benefits:**
- Each function is independently testable
- Easy to reason about (minimal cognitive load)
- Maximum reusability
- Clear contracts (type signatures tell the whole story)

---

### 4. Declarative Style

**Express WHAT to compute, not HOW.**

```c
// ✅ CORRECT: Declarative (WHAT)
float mean = fp_reduce_add_f32(data, n) / n;
float dot_product = fp_fold_dotp_f32(vec_a, vec_b, n);

// ❌ FORBIDDEN: Imperative (HOW)
float sum = 0.0f;
for (size_t i = 0; i < n; i++) {  // HOW to loop
    sum += data[i];               // HOW to accumulate
}
float mean = sum / n;
```

**Graphics Example:**
```c
// ✅ CORRECT: Declarative lighting computation
float diffuse = fp_fold_dotp_f32(normal, light_dir, 3);  // WHAT: dot product
diffuse = fmaxf(0.0f, diffuse);                          // WHAT: clamp

// ❌ FORBIDDEN: Imperative lighting
float diffuse = 0.0f;
for (int i = 0; i < 3; i++) {     // HOW to iterate
    diffuse += normal[i] * light_dir[i];  // HOW to multiply and accumulate
}
if (diffuse < 0.0f) diffuse = 0.0f;  // HOW to clamp
```

**Benefits:**
- Intent is crystal clear
- Less code = fewer bugs
- Compiler/library handles HOW (can optimize without changing code)
- Easier to parallelize/vectorize

---

## Performance Testing Mandate

**We NEVER guess about performance - we MEASURE.**

### Mandatory Testing Process

Every new piece of code follows this workflow:

#### Step 1: Implement FP-First (Declarative)
```c
// Always start with FP approach
float fp_mean(const float* data, size_t n) {
    return fp_reduce_add_f32(data, n) / n;
}
```

#### Step 2: Implement Baseline (Imperative)
```c
// Create equivalent imperative version for comparison
float imperative_mean(const float* data, size_t n) {
    float sum = 0.0f;
    for (size_t i = 0; i < n; i++) {
        sum += data[i];
    }
    return sum / n;
}
```

#### Step 3: Benchmark Both
```c
// Test with realistic data sizes
#define TEST_SIZE 1000000
#define ITERATIONS 100

float test_data[TEST_SIZE];
// ... initialize test_data ...

// Benchmark FP version
clock_t start = clock();
for (int i = 0; i < ITERATIONS; i++) {
    volatile float result = fp_mean(test_data, TEST_SIZE);
}
clock_t fp_time = clock() - start;

// Benchmark imperative version
start = clock();
for (int i = 0; i < ITERATIONS; i++) {
    volatile float result = imperative_mean(test_data, TEST_SIZE);
}
clock_t imp_time = clock() - start;

printf("FP:         %ld ms\n", fp_time);
printf("Imperative: %ld ms\n", imp_time);
printf("Speedup:    %.2fx\n", (double)imp_time / fp_time);
```

#### Step 4: Decision Matrix

| FP Performance | Decision | Action |
|----------------|----------|--------|
| **Faster** | ✅ **Keep FP version** | Ship it! |
| **Same speed (±10%)** | ✅ **Keep FP version** | FP wins ties (better code quality) |
| **10-50% slower** | ⚠️ **Review** | Is this a hot path? If no, keep FP. If yes, investigate. |
| **50-100% slower** | ⚠️ **Review** | Profile to find bottleneck. Can we fuse operations? |
| **>2x slower** | 🔴 **Optimize or deviate** | Only then consider imperative alternative |

#### Step 5: Document Decision
```c
/**
 * fp_complex_operation_f32
 *
 * PERFORMANCE NOTE: Benchmarked against imperative baseline (demo_bench_*.c)
 * - FP version:         45 ms (1M elements, 100 iterations)
 * - Imperative version: 82 ms
 * - Speedup:           1.82x
 * - Decision:          ✅ FP version kept (faster + cleaner)
 */
```

---

## Real-World Results

### Success Stories (FP Wins)

| Operation | FP Time | Imperative Time | Speedup | Reason |
|-----------|---------|-----------------|---------|--------|
| Sum (f64) | 55ms | 100ms | **1.8x** | SIMD vs scalar loop |
| Dot Product (f64) | 96ms | 120ms | **1.25x** | FMA instructions |
| Scan Add (f64) | 28ms | 90ms | **3.2x** | Loop unrolling + registers |
| Linear Regression | 25 lines | 272 lines | **90.8% less code** | Composition |

### Lessons Learned

**FP version often FASTER because:**
1. Hand-optimized assembly primitives (AVX2 SIMD)
2. Fused operations eliminate memory traffic
3. Compiler can't always auto-vectorize imperative loops
4. Better register allocation in assembly

**When FP needs help:**
1. Fuse map+reduce into single-pass (eliminate temporary arrays)
2. Hand-write assembly for operations compiler struggles with
3. Use scalar optimizations (multiple accumulators, loop unrolling)

---

## Graphics Engine: FP-First 3D Rendering

### Principle: Every Graphics Operation via FP Library

```c
// ✅ CORRECT: Matrix-vector multiply using fp_fold_dotp
void transform_vertex_fp(const float matrix[16], const float vertex[4],
                         float result[4]) {
    result[0] = fp_fold_dotp_f32(&matrix[0], vertex, 4);   // Row 0
    result[1] = fp_fold_dotp_f32(&matrix[4], vertex, 4);   // Row 1
    result[2] = fp_fold_dotp_f32(&matrix[8], vertex, 4);   // Row 2
    result[3] = fp_fold_dotp_f32(&matrix[12], vertex, 4);  // Row 3
}

// ❌ FORBIDDEN: Imperative loops
void transform_vertex_bad(const float matrix[16], const float vertex[4],
                          float result[4]) {
    for (int i = 0; i < 4; i++) {
        result[i] = 0.0f;
        for (int j = 0; j < 4; j++) {
            result[i] += matrix[i*4 + j] * vertex[j];
        }
    }
}
```

### Lighting Calculations (FP Library)

```c
// Diffuse lighting = clamp(normal · light_dir, 0, 1)
float compute_diffuse(const float normal[3], const float light_dir[3]) {
    float ndotl = fp_fold_dotp_f32(normal, light_dir, 3);
    return fmaxf(0.0f, ndotl);
}

// Specular lighting = (reflect · view)^shininess
float compute_specular(const float reflect[3], const float view[3],
                       float shininess) {
    float rdotv = fp_fold_dotp_f32(reflect, view, 3);
    return powf(fmaxf(0.0f, rdotv), shininess);
}
```

### SSAO (Screen-Space Ambient Occlusion) - FP Example

```c
// Occlusion calculation using fp_reduce_add
float compute_occlusion(const float* occlusion_samples, size_t count) {
    // Declarative: sum all occlusion flags
    float total_occlusion = fp_reduce_add_f32(occlusion_samples, count);
    return 1.0f - (total_occlusion / (float)count);
}
```

---

## Code Review Checklist

Before merging any code, verify:

- [ ] **Immutability**: All inputs declared `const`
- [ ] **Composition**: Uses existing FP primitives where possible
- [ ] **Modularity**: Function does one thing, has clear contract
- [ ] **Declarative**: Expresses WHAT, not HOW
- [ ] **Performance Tested**: Benchmarked against imperative baseline
- [ ] **Documented**: Performance notes in comments if non-obvious

---

## Summary

**FP principles are the foundation of this library, not an afterthought.**

1. **Immutability** - Enforced by `const`, no exceptions
2. **Composition** - Build complex from simple, 87% code reduction achieved
3. **Modularity** - One function, one responsibility
4. **Declarative** - WHAT to compute, not HOW

**Performance is achieved THROUGH these principles:**
- Composed primitives are hand-optimized (AVX2 SIMD)
- Declarative code is easier to optimize
- Measurement-driven optimization (not guesswork)

**This is not "FP toy code" - this is production-grade systems programming that happens to be functional.**

---

**Written**: November 2025
**Status**: Foundational Document - Read Before Contributing
