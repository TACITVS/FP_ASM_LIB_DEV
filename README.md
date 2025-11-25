# fp_asm_lib_dev: Functional Programming for C with Assembly Performance

[![Language](https://img.shields.io/badge/language-C%20%2B%20x64%20Assembly-blue.svg)](https://github.com/TACITVS/FP_ASM_LIB_DEV)
[![Platform](https://img.shields.io/badge/platform-Windows%20x64-lightgrey.svg)](https://github.com/TACITVS/FP_ASM_LIB_DEV)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)
[![Status](https://img.shields.io/badge/status-Active%20Development-yellow.svg)](https://github.com/TACITVS/FP_ASM_LIB_DEV)
[![ASM Functions](https://img.shields.io/badge/ASM%20functions-170%2B-blueviolet.svg)](docs/API_REFERENCE.md)
[![Types](https://img.shields.io/badge/types-10%20complete-success.svg)](docs/API_REFERENCE.md)
[![L1 Tests](https://img.shields.io/badge/L1%20tests-13%20passing-brightgreen.svg)](tests/test_l1_complete.c)

**A functional programming library for C with hand-optimized x64 AVX2 assembly, building FP primitives from the ground up.**

---

## Project Vision

Build a **true FP-first library** where:
- **L0 (Assembly)**: Hand-optimized AVX2 SIMD primitives (reductions, folds, maps)
- **L1 (C Wrappers)**: Higher-order functions, composition, monads
- **L2 (Algorithms)**: ML/statistics algorithms composed from L0/L1 primitives
- **L3 (Programs)**: Applications built entirely from FP compositions

**No imperative loops in hot paths. Pure functions. Immutable data.**

---

## Current Status

### L0: Assembly Layer - COMPLETE

**170+ hand-optimized AVX2 functions** across **10 numeric types**:

| Category | Types | Functions | Status |
|----------|-------|-----------|--------|
| Reductions (sum, min, max, product) | i8-u64, f32, f64 | 40+ | Complete |
| Fused Folds (dotp, sumsq, sad) | i8-u64, f32, f64 | 30+ | Complete |
| Fused Maps (axpy, scale, offset) | i8-u64, f32, f64 | 40+ | Complete |
| Zips (add, sub, mul) | i8-u64, f32, f64 | 30+ | Complete |
| Scans (prefix sum) | i64, f64 | 2 | Complete |
| 3D Math (mat4, vec3, quat) | f32 | 10+ | Complete |
| Set Ops (unique, union, intersect) | i64 | 6 | Complete |

**Performance**: 1.5-3.5x faster than `gcc -O3` on tested operations.

### L1: C Wrappers - COMPLETE

| Component | Status | Notes |
|-----------|--------|-------|
| General HOFs (map, filter, fold) | Complete | i64/f64 types |
| Function Composition | Complete | compose, pipe, flip |
| Pipeline Builder | Complete | Fluent API |
| Maybe/Either Monads | Complete | Safe error handling |
| Lazy Evaluation | Complete | Sequences, take, range |
| Transducers | Complete | Mapping, filtering, taking, composition |
| Applicative Maybe | Complete | `ap` for f64/i64 |

**13 comprehensive tests passing** - see `tests/test_l1_complete.c`

### L2: Algorithms - REFACTORED

| Algorithm | Uses ASM Primitives? | Deterministic RNG | FP Pure? |
|-----------|---------------------|-------------------|----------|
| K-Means | Yes | Yes (fp_rng) | Yes |
| Linear Regression | Yes | Yes (fp_rng) | Yes |
| Decision Tree | Yes | N/A | Yes |
| Neural Network | Yes | Yes (fp_rng) | Yes |
| PCA | Yes | Yes (fp_rng) | Yes |
| Naive Bayes | Yes | Yes (fp_rng) | Yes |
| FFT | Yes | N/A | Yes |
| Time Series | Yes | N/A | Yes |
| Radix Sort | Yes | N/A | Yes |

**All algorithms now use deterministic `fp_rng` instead of `rand()`** - enables reproducible results with seed control.

---

## Quick Start

### Installation

```bash
git clone https://github.com/tacitvs/fp_asm_lib_dev.git
cd fp_asm_lib_dev
```

### Compiler Requirements

**Tail-Call Optimization (TCO) is REQUIRED for L2 algorithms using tail recursion.**

This library uses tail recursion extensively in batch operations (e.g., Naive Bayes batch prediction). Modern compilers optimize tail recursion into iteration (O(1) stack usage), but you **must** enable optimization flags:

#### GCC / Clang / MinGW

```bash
gcc -O3 -foptimize-sibling-calls <source files> -o output
```

**Required flags:**
- `-O3` - Enable aggressive optimizations
- `-foptimize-sibling-calls` - Enable tail-call optimization

#### MSVC

```bash
cl /O2 /Ob2 <source files>
```

**Required flags:**
- `/O2` - Enable speed optimizations
- `/Ob2` - Enable inline expansion

#### Verification

To verify TCO is working, run the recursion benchmark:

```bash
gcc examples/benchmarks/bench_nb_recursion_vs_loop.c \
    src/algorithms/fp_naive_bayes.c \
    src/wrappers/fp_monads.c \
    build/obj/fp_core_*.o \
    -o bench_tco.exe \
    -I include -O3 -foptimize-sibling-calls -Wall -Wextra -lm

./bench_tco.exe
```

**Expected result:** Completes without stack overflow, showing ~100,000 iterations.

**Proven performance:** FP purist tail recursion matches imperative loops (±1-2% variance). See [`docs/benchmarks/RECURSION_VS_LOOP_RESULTS.md`](docs/benchmarks/RECURSION_VS_LOOP_RESULTS.md) for full benchmark data.

#### Debug Builds

⚠️ **Warning:** Debug builds (`-O0` or no optimization) may NOT optimize tail calls, potentially causing stack overflow on large datasets (>10K samples).

**Recommendation:** Use release builds (`-O3`) for production and large-scale processing.

### Build & Test

```bash
# Build and run all comprehensive tests
scripts/build/build_all_tests.bat

# Run a specific test
./build/bin/test_f64_comprehensive.exe
```

### Example: Assembly Primitives (Working)

```c
#include "include/fp_core.h"

// These use hand-optimized AVX2 assembly
int64_t data[] = {1, 2, 3, 4, 5};
int64_t sum = fp_reduce_add_i64(data, 5);        // SIMD reduction
double dot = fp_fold_dotp_f64(a, b, n);          // Fused dot product
fp_map_axpy_f64(x, y, out, n, alpha);            // BLAS-style axpy
```

### Example: Statistics (Working)

```c
double prices[] = {100.5, 102.3, 101.8, 103.2, 104.1};

// Descriptive statistics
DescriptiveStats stats;
fp_descriptive_stats_f64(prices, 5, &stats);

// Percentiles (copies internally, preserves immutability)
double quartiles[3];
fp_quartiles_f64(prices, 5, quartiles);
```

### Example: Transducers (L1 - Working)

```c
#include "fp_compose.h"

double data[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

// Compose: square -> filter even -> take 3
fp_transducer_t chain[3];
chain[0] = fp_mapping_f64(square);
chain[1] = fp_filtering_f64(is_even);
chain[2] = fp_taking_f64(3);

fp_transducer_t composed = fp_compose_transducers(chain, 3);
double result = fp_transduce_f64(data, 10, composed, 0.0, add);
// Result: 4 + 16 + 36 = 56

fp_transducer_free(&composed);
```

### Example: Maybe Monad (L1 - Working)

```c
#include "fp_monads.h"

// Safe computation chain
Maybe m = fp_just_f64(21.0);
Maybe doubled = fp_fmap_maybe_f64(m, double_it);  // Just(42.0)

// Applicative: apply wrapped function to wrapped value
Maybe mfn = fp_just_ptr((void*)square);
Maybe result = fp_ap_maybe_f64(mfn, doubled);     // Just(1764.0)

// Nothing propagates safely
Maybe nothing = fp_nothing();
Maybe safe = fp_fmap_maybe_f64(nothing, square);  // Nothing
```

---

## Architecture

```
L3: Programs (demos, applications)
    |
L2: Algorithms (kmeans, regression, neural nets) <- REFACTORED
    |
L1: C Wrappers (HOFs, composition, monads) <- COMPLETE
    |
L0: Assembly (AVX2 SIMD primitives) <- COMPLETE
```

### Design Principles

1. **FP Purity at L0**: All assembly functions take `const` inputs, produce new outputs
2. **Composition over loops**: Higher layers should compose L0 primitives, not use for/while
3. **No rand() in algorithms**: Use seeded/deterministic RNG passed as parameter
4. **Immutability**: Never mutate user data; copy internally if needed

---

## Performance

Verified benchmarks (10M elements, gcc -O3 baseline):

| Operation | C Baseline | ASM | Speedup |
|-----------|------------|-----|---------|
| reduce_add_f64 | 15.0ms | 11.0ms | **1.36x** |
| reduce_max_f64 | 15.1ms | 9.6ms | **1.56x** |
| scan_add_f64 | 90ms | 28ms | **3.2x** |
| fold_dotp_f64 | 120ms | 96ms | **1.25x** |

---

## Roadmap

### Phase 1: Stabilize L0 (Complete)
- [x] 170+ ASM functions across 10 types
- [x] Comprehensive tests for all types
- [x] Windows x64 ABI compliance verified

### Phase 2: Complete L1 (Complete)
- [x] Function composition (compose, pipe, flip)
- [x] Lazy evaluation (sequences, take, range)
- [x] Complete transducer support (mapping, filtering, taking, composition)
- [x] Applicative Maybe (ap for f64/i64)
- [x] 13 comprehensive tests passing

### Phase 3: Refactor L2 (Complete)
- [x] Refactor kmeans to use ASM primitives
- [x] Remove rand() - use deterministic `fp_rng`
- [x] Refactor all 9 algorithms for FP purity
- [x] Reproducible results with seed control

### Phase 4: L3 Applications (In Progress)
- [ ] Demo applications using pure FP composition
- [ ] Linux port (System V ABI)
- [ ] AVX-512 variants

---

## Documentation

- **[docs/CLAUDE.md](docs/CLAUDE.md)** - Technical architecture, ABI details
- **[docs/API_REFERENCE.md](docs/API_REFERENCE.md)** - Function reference
- **[Codex_ASSESSMENT.md](Codex_ASSESSMENT.md)** - Third-party audit of FP compliance

---

## Contributing

Areas where help is needed:

1. **L3 Applications**: Build demo applications showcasing FP composition
2. **Linux Port**: System V AMD64 ABI adaptation
3. **AVX-512**: Implement wider SIMD variants
4. **Testing**: Property-based tests, additional edge cases

See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

---

## License

MIT License - see [LICENSE](LICENSE) file.

---

## Acknowledgments

This project proves that functional programming and systems performance are not mutually exclusive. The complete stack from L0 (AVX2 assembly) through L1 (FP wrappers) to L2 (algorithms) demonstrates production-ready FP in C.

**Current status**: L0-L2 complete with 170+ ASM functions, full FP wrapper layer, and 9 refactored algorithms using deterministic RNG.

---

<div align="center">

**Built with a commitment to both performance AND honesty about project status**

[Report Bug](https://github.com/TACITVS/FP_ASM_LIB_DEV/issues) | [Request Feature](https://github.com/TACITVS/FP_ASM_LIB_DEV/issues)

</div>
