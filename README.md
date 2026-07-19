# FP-ASM — Functional primitives for C with x64 AVX2 assembly

A small, fast, **externally linkable** library of functional-programming primitives
(reductions, folds, maps, scans) and game math (vec3 / mat4 / quaternion) whose hot
paths are hand-written x64 AVX2 assembly, wrapped in a thin C API. Intended for
consumption by games and graphics libraries.

- **L0 — Assembly:** AVX2 SIMD kernels over 10 numeric types (`src/asm/`)
- **L1 — C wrappers:** higher-order functions, composition, Maybe/Either monads (`src/wrappers/`)
- **L2 — Math/algorithms:** vec3/mat4/quaternion, matrix ops, FFT, radix sort (`src/algorithms/`)

## Status

This is an extraction-and-hardening of the FP core from a larger vector-database
project (upstream: `TACITVS/FP_ASM_LIB_DEV`). Work in progress toward a clean,
cross-platform (Linux + Windows x64), tested, linkable library.

### Provenance

The initial commit is a faithful copy of the in-scope core from upstream and still
carries upstream's **Windows-only calling convention** in the assembly — it links on
Linux but does not yet return correct results there. Cross-platform ABI abstraction,
a portable build system, and a full test suite are added in subsequent commits.

## License

MIT — see [LICENSE](LICENSE).
