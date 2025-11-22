Timestamp: 2025-11-22 12:54:17 -05:00

# FP_ASM_LIB_DEV Functional-Purity Assessment

## Overview
- Reviewed stated FP-first/ASM vision (L0 hand-tuned AVX2, L1 HOFs, L2 algorithms, L3 programs) against code/docs/tests in this working copy dated Nov 2025.
- Focused on alignment to functional purity (immutability/composability/declarative), implementation completeness, and evidence in tests/benchmarks.

## Strengths
- Substantial AVX2 assembly coverage for numeric kernels: reductions, fused folds, fused maps, scans, compaction, percentiles, stats across several types (see e.g. src/asm/fp_core_reductions.asm:1, src/asm/fp_core_fused_maps_u32.asm:1).
- Type breadth is better than minimal: reductions/folds/maps exist for i8/u8/i16/u16/i32/u32/i64/u64/f32/f64 with targeted tests (tests/unit/test_u8_comprehensive.c, tests/unit/test_i32_comprehensive.c).
- Purity-aware wrappers for percentile/outlier functions copy and sort internally to avoid mutating user data (src/wrappers/fp_percentile_wrappers.c:45-75), and there is an explicit purity test harness (tests/test_purity.c:31-143).
- Documentation volume is high with usage guides and refactoring notes that explain intent and ABI considerations (docs/CLAUDE.md:1-140).

## Gaps and risks vs FP vision
- Claims and reality diverge: README advertises “120 hand-optimized AVX2 functions” and “100% type coverage” (README.md:10-14) while docs still mark modules/types as planned or incomplete (docs/CLAUDE.md:75; docs/reports/TYPE_IMPLEMENTATION_STATUS.md:65-175); many advanced operations exist only for i64 or are placeholders.
- Functional layer is unfinished: general HOFs are only for i64/f64 (src/wrappers/fp_general_hof.c:46-118) and the composition/pipeline API contains placeholders/TODOs for flip, lazy map/filter/take, and transducers (src/wrappers/fp_compose.c:70-71,945-956,987-999), so “100% FP language equivalence” is not met.
- Purity/composability break in algorithms: ML kernels rely on imperative loops, mutation, and randomness instead of assembling primitives (src/algorithms/fp_linear_regression.c:78-114 computes dot products manually; src/algorithms/fp_kmeans.c:73,97 uses rand() and mutable state), undermining the declarative FP goal.
- Layer usage is inconsistent: higher-tier assembly (set/group/range) is i64-only (src/asm/fp_core_tier2.asm:20-166) and simple map kernels exist only for i64/f64 (src/asm/fp_core_simple_maps.asm:18-22); many “FP language” behaviors (lazy sequences, monads) lack tests, so cross-type purity is uneven.
- Evidence gaps: tests target a subset of kernels and purity checks focus on percentiles only (tests/test_purity.c:31-143), while no automated CI/benchmark artifacts confirm the speedup claims; prebuilt .exe files lack provenance.

## Layer snapshot
- L0 (ASM): Solid coverage for arithmetic kernels across 8–10 types; higher-order list/set/group operations are i64-only and not clearly integrated with later layers.
- L1 (wrappers): General HOFs for two types; composition/monad headers exist but runtime implementations are partial and unverified.
- L2 (algorithms): Linear regression, k-means, Monte Carlo, etc. are imperative C with manual allocation/loops and scant reuse of ASM primitives.
- L3 (demos/tests): Numerous batch scripts and demo sources, but verification relies on manual execution; no reproducible test matrix or benchmark logs shipped.

## Recommendations
1) Reconcile documentation with implementation: enumerate the real function surface per type, mark planned vs delivered, and drop or qualify “100%/120 functions” until proven.
2) Finish or trim the FP layer: implement the TODOs in src/wrappers/fp_compose.c, extend general HOFs beyond i64/f64, or narrow the advertised scope.
3) Refactor algorithms to compose primitives (folds/maps) instead of bespoke loops; add property-based or golden tests that assert purity/immutability for these paths.
4) Add automated coverage: unit tests for compose/monad/lazy APIs, CI scripts, and benchmark logs to substantiate performance claims on AVX2.
5) Clarify layer boundaries: document which L0 kernels are considered stable per type, and which L2/L3 components depend on them, so future work can prioritize AVX2 gaps (e.g., predicates and tier2/3 for non-i64 types).

## Collaboration and Test Plan (Codex proposals)
- Shared truth: auto-generate a capability matrix (types × ops × layers) into `Codex_CAPABILITIES.md`; keep README claims in sync via script.
- Task queue: maintain `Codex_TASK_QUEUE.md` with small, atomic tasks (owner/stamp/status/inputs/expected output); models claim tasks before editing.
- Guardrails: `Codex_GUIDELINES.md` capturing ABI rules, purity contract, SIMD limits, coding standards, and test commands; required reading before changes.
- Change log: `Codex_CHANGELOG.md` entry per task with rationale and tests run; pair with a standard `git diff --stat` + lint + test check.
- Purity and ABI tests: extend purity harness to all const-taking APIs; add ABI canary harness to catch register/stack corruption and alignment/tail bugs.
- Property-based and cross-type checks: associative/commutative laws where applicable; cross-type agreement (i16/u16/i32 vs i64) on overlapping domains; deterministic seeds for randomized algorithms.
- FP layer verification: tests for compose/flip/pipeline/lazy/monad laws and call counts (no extra invocations, correct ordering, proper cleanup for lazy sequences).
- Performance baselines: microbench references with CPU/flags recorded; threshold alerts for regressions; ensure SIMD paths are exercised for vectorizable lengths.
- Algorithm goldens: small seeded datasets for k-means, linreg, PCA, etc., compared to reference outputs; require seeded RNG wrappers for determinism.
- Sequencing: (1) generate capability matrix + guidelines + task queue; (2) expand purity/ABI tests; (3) finish or trim compose/lazy/HOF scope; (4) add property/cross-type tests; (5) add microbench baselines; (6) regenerate docs/README from the capability matrix.
