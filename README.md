# FP-ASM — Functional primitives for C with x64 AVX2 assembly

A small, fast, **externally linkable** library of functional-programming primitives
(reductions, folds, maps, scans) and game math (vec3 / mat4 / quaternion) whose hot
paths are hand-written x64 AVX2 assembly, wrapped in a thin C API. Intended for
consumption by games and graphics libraries.

- **L0 — Assembly:** AVX2 SIMD kernels over 10 numeric types (`src/asm/`)
- **L1 — C wrappers:** higher-order functions, composition, Maybe/Either monads (`src/wrappers/`)
- **L2 — Math/algorithms:** vec3/mat4/quaternion, matrix ops, FFT, radix sort (`src/algorithms/`)

## Build

Requirements: **NASM** (≥ 2.13) and a C11 compiler (gcc/clang). The kernels need
a CPU with **AVX2 + FMA**.

### Make

```bash
make            # build static + shared libs into build/
make test       # build and run the test suite
make install    # install headers + libs under PREFIX (default /usr/local)
```

### CMake (for game / graphics projects)

```bash
cmake -S . -B build && cmake --build build
ctest --test-dir build
```

Then link from your project:

```cmake
add_subdirectory(path/to/fp_asm_c)
target_link_libraries(mygame PRIVATE fpasm::fpasm)
```

## Status

Extraction-and-hardening of the FP core from a larger vector-database project
(upstream: `TACITVS/FP_ASM_LIB_DEV`), in progress toward a clean, cross-platform
(Linux + Windows x64), tested, linkable library.

The **full API builds on both Windows and Linux**. On Windows it behaves as
upstream. On Linux, the modules migrated to the cross-platform ABI layer are
test-verified; the rest are still Windows-only. See
[PORTING_STATUS.md](PORTING_STATUS.md) for the exact per-module state and the
bugs fixed so far.

## License

MIT — see [LICENSE](LICENSE).
