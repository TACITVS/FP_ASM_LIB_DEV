# fp_asm_lib_dev: Functional Programming for C with Assembly Performance

[![Language](https://img.shields.io/badge/language-C%20%2B%20x64%20Assembly-blue.svg)](https://github.com/TACITVS/FP_ASM_LIB_DEV)
[![Platform](https://img.shields.io/badge/platform-Windows%20x64-lightgrey.svg)](https://github.com/TACITVS/FP_ASM_LIB_DEV)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)
[![Status](https://img.shields.io/badge/status-Active%20Development-yellow.svg)](https://github.com/TACITVS/FP_ASM_LIB_DEV)
[![ASM Functions](https://img.shields.io/badge/ASM%20functions-170%2B-blueviolet.svg)](docs/API_REFERENCE.md)
[![Types](https://img.shields.io/badge/types-10%20complete-success.svg)](docs/API_REFERENCE.md)

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

### L1: C Wrappers - IN PROGRESS

| Component | Status | Notes |
|-----------|--------|-------|
| General HOFs (map, filter, fold) | Partial | i64/f64 only |
| Function Composition | Partial | `fp_flip` is placeholder |
| Pipeline Builder | Working | Uses internal loops |
| Maybe/Either Monads | Complete | Safe error handling |
| Lazy Evaluation | Placeholder | TODOs in code |
| Transducers | Placeholder | Simplified stubs |

### L2: Algorithms - NEEDS REFACTORING

| Algorithm | Uses ASM Primitives? | Uses rand()? | FP Pure? |
|-----------|---------------------|--------------|----------|
| K-Means | No (0 calls) | Yes | No |
| Linear Regression | No (0 calls) | Yes | No |
| Decision Tree | Minimal (1 call) | No | No |
| Neural Network | Some (3 calls) | Yes | Partial |
| PCA | Minimal | Yes | No |

**Issue**: Algorithms use imperative loops instead of composing from L0/L1 primitives. This is the main gap between vision and implementation.

---

## Quick Start

### Installation

```bash
git clone https://github.com/tacitvs/fp_asm_lib_dev.git
cd fp_asm_lib_dev
```

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

---

## Architecture

```
L3: Programs (demos, applications)
    |
L2: Algorithms (kmeans, regression, neural nets) <- NEEDS WORK
    |
L1: C Wrappers (HOFs, composition, monads) <- IN PROGRESS
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

### Phase 2: Complete L1 (In Progress)
- [ ] Extend HOFs beyond i64/f64
- [ ] Implement lazy evaluation
- [ ] Complete transducer support
- [ ] Fix placeholder functions

### Phase 3: Refactor L2 (Planned)
- [ ] Refactor kmeans to compose from fp_reduce/fp_fold
- [ ] Remove rand() - use deterministic seeded RNG
- [ ] Refactor all algorithms to be loop-free
- [ ] Add property-based tests for purity

### Phase 4: L3 Applications (Future)
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

1. **L1 Completion**: Extend HOFs to all types, implement lazy evaluation
2. **L2 Refactoring**: Convert algorithms to use ASM primitives
3. **Linux Port**: System V AMD64 ABI adaptation
4. **Testing**: Property-based tests, purity verification

See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

---

## License

MIT License - see [LICENSE](LICENSE) file.

---

## Acknowledgments

This project aims to prove that functional programming and systems performance are not mutually exclusive. The L0 assembly layer demonstrates this is achievable; the higher layers are still evolving toward that goal.

**Honest status**: Great assembly foundation, wrapper layer incomplete, algorithms need refactoring to match FP vision.

---

<div align="center">

**Built with a commitment to both performance AND honesty about project status**

[Report Bug](https://github.com/TACITVS/FP_ASM_LIB_DEV/issues) | [Request Feature](https://github.com/TACITVS/FP_ASM_LIB_DEV/issues)

</div>
