# FP-ASM Testing and Benchmarking Guidelines

**Purpose**: Ensure accurate, fair, and reproducible performance measurements for assembly-optimized functions.

**Critical Principle**: Performance comparisons MUST use the same algorithm. We measure **implementation efficiency**, not **algorithm choice**.

---

## 1. The Cardinal Rule: Apples to Apples

### ❌ WRONG: Comparing Different Algorithms

```c
// Assembly: O(n) optimized sliding window
void fp_sma_f64(data, n, window, output) {
    // Compute initial sum once
    // Then slide: sum = sum - oldest + newest
}

// C Baseline: O(n*window) naive recomputation
void sma_baseline(data, n, window, output) {
    for (i = 0; i < out_size; i++) {
        sum = 0;
        for (j = 0; j < window; j++) {  // ← RECOMPUTES ENTIRE SUM!
            sum += data[i + j];
        }
        output[i] = sum / window;
    }
}

// Result: 667x "speedup" ← MISLEADING! Mostly algorithmic difference!
```

**Problem**: This measures O(n) vs O(n²), not assembly vs C.

### ✅ CORRECT: Same Algorithm, Different Implementation

```c
// Assembly: O(n) optimized sliding window
void fp_sma_f64(data, n, window, output) {
    // Compute initial sum once
    // Then slide: sum = sum - oldest + newest
}

// C Baseline: O(n) optimized sliding window
void sma_baseline(data, n, window, output) {
    // Compute initial sum once
    double sum = 0.0;
    for (i = 0; i < window; i++) sum += data[i];
    output[0] = sum / window;

    // Slide window
    for (i = 1; i < out_size; i++) {
        sum = sum - data[i-1] + data[i+window-1];
        output[i] = sum / window;
    }
}

// Result: ~1.5-2.0x speedup ← ACCURATE! Measures implementation quality.
```

**Success**: Both use O(n) sliding window, comparison is fair.

---

## 2. Mandatory Baseline Requirements

### Rule 2.1: Match the Algorithm Complexity

**Before writing any benchmark**:

1. ✅ **Identify the algorithm** used in assembly (O(n), O(n log n), O(n²), etc.)
2. ✅ **Implement the SAME algorithm** in C baseline
3. ✅ **Verify complexity matches** by testing with different input sizes

**Complexity Verification Test**:
```c
// If algorithms match, time should scale proportionally
n=1000:   asm=0.01s, C=0.015s → ratio=1.5x
n=10000:  asm=0.10s, C=0.15s  → ratio=1.5x ✅ SAME RATIO
n=100000: asm=1.0s,  C=1.5s   → ratio=1.5x ✅ CONSISTENT

// If algorithms differ, ratio changes with size
n=1000:   asm=0.01s, C=0.5s   → ratio=50x
n=10000:  asm=0.10s, C=50s    → ratio=500x ❌ RATIO CHANGES!
n=100000: asm=1.0s,  C=5000s  → ratio=5000x ❌ ALGORITHMIC!
```

### Rule 2.2: Use Reasonable Optimizations in C

**C baselines should be reasonably optimized** (but not hand-tuned):

✅ **Allowed**:
- Compiler optimizations: `-O2` or `-O3` (use project default)
- Natural C idioms (good loop structure, avoiding redundant work)
- Standard algorithmic optimizations (sliding window, memoization, etc.)

❌ **Not Allowed**:
- Deliberately bad code (unrolling loops incorrectly, redundant operations)
- Naive algorithms when better algorithms are standard
- Ignoring obvious optimizations that any programmer would use

**Example**: For sum computation:
```c
// ✅ GOOD: Natural C
double sum = 0.0;
for (size_t i = 0; i < n; i++) {
    sum += data[i];
}

// ❌ BAD: Artificially slow
double sum = 0.0;
for (size_t i = 0; i < n; i++) {
    sum = sum + data[i];  // Prevents optimization
    sum = sum + 0.0;      // Deliberate slowdown
}
```

### Rule 2.3: Provide Multiple Baselines When Appropriate

For operations with multiple valid algorithms, provide **both**:

```c
// Baseline 1: Optimized (fair comparison)
void sma_baseline_optimized(data, n, window, output) {
    // O(n) sliding window - matches assembly
}

// Baseline 2: Naive (algorithmic comparison)
void sma_baseline_naive(data, n, window, output) {
    // O(n*window) recomputation - common naive approach
}

// Report BOTH:
printf("Implementation speedup (ASM vs C optimized): %.2fx\n", ...);
printf("Algorithmic speedup (optimized vs naive):   %.2fx\n", ...);
```

**Benefit**: Shows both implementation quality AND value of algorithm choice.

---

## 3. Benchmark Design Patterns

### Pattern 3.1: Standard Benchmark Structure

```c
void benchmark_function(size_t n, int iterations) {
    // 1. Setup
    allocate_test_data();

    // 2. Warmup (prevent cold-start effects)
    fp_function_asm(...);
    function_baseline(...);

    // 3. Benchmark assembly
    clock_t start = clock();
    for (int i = 0; i < iterations; i++) {
        fp_function_asm(...);
    }
    clock_t end = clock();
    double time_asm = (double)(end - start) / CLOCKS_PER_SEC;

    // 4. Benchmark C baseline
    start = clock();
    for (int i = 0; i < iterations; i++) {
        function_baseline(...);
    }
    end = clock();
    double time_c = (double)(end - start) / CLOCKS_PER_SEC;

    // 5. Report results
    printf("Assembly: %.6f seconds\n", time_asm);
    printf("C:        %.6f seconds\n", time_c);
    printf("Speedup:  %.2fx\n", time_c / time_asm);

    // 6. Cleanup
    free_test_data();
}
```

### Pattern 3.2: Prevent Dead Code Elimination

**Problem**: Compilers may optimize away unused results.

```c
// ❌ WRONG: Result never used, might be optimized away
for (int i = 0; i < iterations; i++) {
    fp_function_asm(data, n, output);
    // output never read - compiler may skip the call!
}

// ✅ CORRECT: Use volatile sink
volatile double sink;
for (int i = 0; i < iterations; i++) {
    fp_function_asm(data, n, output);
    sink = output[0];  // Force computation
}

// ✅ ALSO CORRECT: Use result after loop
for (int i = 0; i < iterations; i++) {
    fp_function_asm(data, n, output);
}
printf("Checksum: %.2f\n", output[0] + output[n-1]);
```

### Pattern 3.3: Multiple Test Sizes

**Always test with multiple input sizes** to verify algorithmic complexity:

```c
// Test size scaling
benchmark_function(1000,   1000);  // Small
benchmark_function(10000,  1000);  // Medium
benchmark_function(100000, 100);   // Large

// Expected for O(n) algorithm:
// - 10x size → ~10x time (both ASM and C)
// - Speedup ratio stays constant

// Red flag if you see:
// - 10x size → ~100x time (suggests O(n²) somewhere)
// - Speedup ratio changes drastically (different algorithms)
```

---

## 4. Common Pitfalls and How to Avoid Them

### Pitfall 4.1: The "SMA Trap" (Different Algorithms)

**Mistake**: Assembly uses O(n) sliding window, C uses O(n*window) recomputation.

**Detection**:
1. Speedup increases with window size ← Red flag!
2. Speedup is suspiciously high (>10x for memory-bound operations)

**Fix**: Implement sliding window in C baseline too.

---

### Pitfall 4.2: Cold Cache Effects

**Mistake**: First run is much slower due to cache misses.

**Detection**: First iteration takes 2-10x longer than subsequent ones.

**Fix**: Always include warmup runs before timing:
```c
// Warmup
fp_function_asm(data, n, output);
function_baseline(data, n, output);

// Now time (caches are warm)
start_timing();
```

---

### Pitfall 4.3: Insufficient Iterations

**Mistake**: Timing very fast operations with too few iterations.

**Detection**: Time measurements < 0.01 seconds (clock resolution issues).

**Fix**: Scale iterations so total time > 0.1 seconds:
```c
// Target: ~0.5-1.0 seconds total
if (n < 10000) {
    iterations = 10000;  // Small data, more iterations
} else if (n < 100000) {
    iterations = 1000;
} else {
    iterations = 100;    // Large data, fewer iterations
}
```

---

### Pitfall 4.4: Compiler Over-Optimization

**Mistake**: Compiler optimizes benchmark loop in unexpected ways.

**Detection**:
- Time is suspiciously close to zero
- Assembly time > C time (should rarely happen)

**Fix**:
```c
// Mark data as volatile if needed
volatile double* data_vol = data;

// Or compile benchmark with -O2 instead of -O3
```

---

### Pitfall 4.5: Input Data Characteristics

**Mistake**: Using artificial data that favors one implementation.

**Examples**:
- All zeros (branch predictors work perfectly)
- Sorted data (cache-friendly access)
- Repeated patterns (CPU prefetcher optimizes)

**Fix**: Use realistic data:
```c
// Good: Randomized data
for (size_t i = 0; i < n; i++) {
    data[i] = (double)(rand() % 1000);
}

// Good: Mix of patterns
for (size_t i = 0; i < n; i++) {
    data[i] = sin((double)i) * 1000.0 + noise();
}

// Bad: All same value
for (size_t i = 0; i < n; i++) {
    data[i] = 42.0;  // ← Unrealistic
}
```

---

## 5. Verification Checklist

Before finalizing any benchmark, verify:

### ✅ Correctness Verification
- [ ] Assembly and C produce **identical results** (within floating-point tolerance)
- [ ] Tested with edge cases (n=0, n=1, n=2, large n)
- [ ] Tested with special values (NaN, Infinity, negative values if applicable)

### ✅ Algorithm Verification
- [ ] Both implementations use the **same algorithm** (same Big-O complexity)
- [ ] Verified by testing multiple input sizes (ratio should stay constant)
- [ ] If using different algorithms, **clearly document** and report separately

### ✅ Benchmark Validity
- [ ] Warmup runs included before timing
- [ ] Sufficient iterations (total time > 0.1 seconds)
- [ ] Results reproducible (run multiple times, variance < 5%)
- [ ] Tested on realistic data (not all zeros, not artificial patterns)
- [ ] Dead code elimination prevented (results used or volatile sink)

### ✅ Reporting Standards
- [ ] Clearly state what is being measured ("implementation" vs "algorithmic")
- [ ] Report absolute times, not just speedup ratios
- [ ] Document test parameters (n, iterations, data characteristics)
- [ ] Include target speedup expectations (based on theoretical analysis)

---

## 6. Reporting Standards

### Format 6.1: Standard Performance Report

```
Benchmarking <Function Name> (n=<size>, iterations=<count>)
============================================================
Algorithm: <O(?) complexity, brief description>

Results:
  FP-ASM:     X.XXXXXX seconds
  C Baseline: X.XXXXXX seconds
  Speedup:    X.XXx

Analysis:
  - Expected speedup: X.X-X.Xx (based on <reason>)
  - Actual speedup: X.XXx
  - Status: [MEETS TARGET | EXCEEDS TARGET | BELOW TARGET]

Notes:
  - <Any relevant observations>
```

### Format 6.2: When Including Multiple Baselines

```
Benchmarking SMA (n=10000, window=1000, iterations=1000)
=========================================================
Algorithm: Sliding window (O(n))

Results:
  FP-ASM (sliding window):       0.039 seconds
  C Optimized (sliding window):  0.060 seconds
  C Naive (recompute sum):       26.045 seconds

Speedup Analysis:
  Implementation (ASM vs C optimized): 1.54x
    → Measures assembly optimization quality
  Algorithmic (optimized vs naive):   433.8x
    → Demonstrates value of sliding window algorithm

Target: 1.5-2.0x (implementation speedup)
Status: MEETS TARGET ✅
```

---

## 7. Algorithm-Specific Guidelines

### 7.1: Reductions (sum, max, min)

**Expected Speedup**: 1.0-2.0x
- C compilers auto-vectorize well
- Assembly guarantees SIMD, avoids heuristics

**C Baseline**:
```c
// Simple loop (let compiler vectorize)
double sum = 0.0;
for (size_t i = 0; i < n; i++) {
    sum += data[i];
}
```

---

### 7.2: Map Operations (scale, offset, sqrt)

**Expected Speedup**: 1.0-1.5x
- Memory-bound operations
- Limited by bandwidth, not computation

**C Baseline**:
```c
// Simple loop
for (size_t i = 0; i < n; i++) {
    output[i] = data[i] * scale + offset;
}
```

---

### 7.3: Fused Operations (map + reduce)

**Expected Speedup**: 1.2-2.5x
- Avoids temporary array writes
- Better cache utilization

**C Baseline**:
```c
// Fused in single loop (not separate passes!)
double sum = 0.0;
for (size_t i = 0; i < n; i++) {
    double val = data[i] * data[i];  // map: square
    sum += val;                      // reduce: sum
}
```

---

### 7.4: Statistical Operations (mean, stddev, correlation)

**Expected Speedup**: 2.0-10.0x
- Multiple passes or complex operations
- SIMD parallelism + loop fusion

**C Baseline**:
```c
// Use efficient algorithm (single-pass where possible)
// Match the number of passes in assembly
```

---

### 7.5: Sliding Window Operations (SMA, convolution)

**Expected Speedup**: 1.5-2.5x (with same algorithm!)

**⚠️ CRITICAL**: Must use sliding window in C!

```c
// ✅ CORRECT: O(n) sliding window
double sum = initial_window_sum();
for (each position) {
    sum = sum - oldest + newest;  // O(1) update
    output[i] = sum / window;
}

// ❌ WRONG: O(n*window) recomputation
for (each position) {
    sum = 0;
    for (j in window) {  // Recomputes entire sum!
        sum += data[j];
    }
    output[i] = sum / window;
}
```

---

## 8. Red Flags: When to Question Your Results

### 🚩 Speedup > 10x for Simple Operations

**Likely cause**: Different algorithms, not just different implementations.

**Action**: Review C baseline complexity.

---

### 🚩 Speedup Changes Dramatically With Input Size

**Example**:
- n=1,000: 5x speedup
- n=10,000: 50x speedup
- n=100,000: 500x speedup

**Likely cause**: O(n) vs O(n²) algorithm difference.

**Action**: Verify both use same algorithm.

---

### 🚩 Assembly Slower Than C

**Possible causes**:
1. Bug in assembly (incorrect logic)
2. Missing optimization (not using SIMD when you should)
3. ABI violation (incorrect calling convention)
4. Benchmark issue (cold cache, insufficient warmup)

**Action**: Investigate thoroughly - this should be rare.

---

### 🚩 Speedup < 1.0x for Vectorizable Operations

**Expected**: Should be at least 1.0x (assembly shouldn't be slower).

**Possible causes**:
1. Assembly not using SIMD effectively
2. Memory alignment issues
3. Benchmark measurement error

**Action**: Review assembly implementation.

---

### 🚩 Zero or Negative Time Measurements

**Cause**: Insufficient iterations or dead code elimination.

**Fix**:
1. Increase iterations
2. Use volatile sink
3. Verify operation actually executes

---

## 9. Examples: Before and After

### Example 9.1: SMA (The Original Mistake)

#### ❌ Before (Misleading)

```c
// Assembly: O(n) sliding window
void fp_sma_f64(...) { /* sliding window */ }

// C: O(n*window) naive
void sma_baseline(...) {
    for (i) {
        for (j in window) {  // Recomputes!
            sum += data[i+j];
        }
    }
}

Result: 667x speedup ← WRONG! Mostly algorithmic.
```

#### ✅ After (Correct)

```c
// Assembly: O(n) sliding window
void fp_sma_f64(...) { /* sliding window */ }

// C: O(n) sliding window
void sma_baseline(...) {
    sum = initial_sum;
    for (i) {
        sum = sum - oldest + newest;  // O(1)!
    }
}

Result: 1.54x speedup ← CORRECT! Implementation quality.
```

---

## 10. Document Template for New Algorithms

When adding a new algorithm, include this in the demo file comments:

```c
// ===========================================================================
// ALGORITHM: <Name>
// ===========================================================================
// Complexity: O(?)
// Algorithm: <Brief description>
//
// Assembly optimization strategy:
//   - <What SIMD instructions are used>
//   - <What algorithmic improvements>
//   - <What memory access patterns>
//
// C Baseline strategy:
//   - MATCHES assembly algorithm (same complexity)
//   - Uses <same algorithmic approach>
//   - Natural C idioms, no artificial slowdowns
//
// Expected speedup: X.X-X.Xx
// Rationale: <Why this speedup is expected>
//
// Measurement verified:
//   ✅ Same algorithm (checked complexity with multiple sizes)
//   ✅ Correctness (outputs match within tolerance)
//   ✅ Warmup included
//   ✅ Sufficient iterations
// ===========================================================================
```

---

## 11. Final Checklist: Before Committing Benchmarks

- [ ] **Same Algorithm**: Verified both use identical Big-O complexity
- [ ] **Complexity Test**: Ran with 3+ input sizes, ratio stays constant
- [ ] **Correctness**: Assembly and C produce identical outputs
- [ ] **Warmup**: Included warmup runs before timing
- [ ] **Iterations**: Total time > 0.1 seconds for reliable measurement
- [ ] **Realistic Data**: Not all zeros, not artificial patterns
- [ ] **Documentation**: Clearly documented algorithm and expected speedup
- [ ] **Sanity Check**: Speedup is reasonable for operation type
- [ ] **Reproducible**: Ran 3+ times, variance < 5%
- [ ] **Red Flags**: No warning signs from Section 8

---

## Remember:

> **"We measure implementation efficiency, not algorithm choice."**
>
> If your speedup is surprisingly high (>10x for simple operations), you're probably comparing different algorithms. This is valuable information, but should be reported separately as "algorithmic speedup" vs "implementation speedup".

---

**Version**: 1.0
**Created**: 2025-11-01
**Last Updated**: 2025-11-01
**Maintainer**: FP-ASM Project Team
