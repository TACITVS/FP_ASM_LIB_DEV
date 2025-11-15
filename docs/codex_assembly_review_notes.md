# FP_ASM_LIB Assembly & Wrapper Review

Technical observations from the first pass over the assembly kernels, their C wrappers, and benchmark harnesses. These notes are meant to guide future optimization or cleanup work.

## 1. Assembly Kernels

### Reductions (`src/asm/fp_core_reductions.asm`)

- `fp_reduce_add_{i64,f64}` keep four YMM accumulators live and finish with scalar tails. Consider handling the tail with a masked SIMD load (`vmaskmov`) to avoid scalar loops when `n mod 16` is large.
- `fp_reduce_max_i64` stays scalar because AVX2 lacks `vpmaxsq`. There may still be room to vectorize via `vpcmpgtq` + `vblend*` to keep four elements in flight.
- Every AVX2 routine repeats the same “save YMM6–YMM9, align stack, restore” boilerplate. Adding NASM `%macro`s for the prologue/epilogue would reduce copy/paste risk when kernels evolve.
- **Done** ✔️ `fp_reduce_max_i64`/`fp_reduce_min_i64` now use AVX2 compare + blend loops with a horizontal reduction, eliminating the old scalar 4-way unroll.

### Fused folds (`src/asm/fp_core_fused_folds.asm`)

- `fp_fold_sad_i64` correctly emulates `vpabsq` with compare/xor/sub, but it reuses `ymm5` as both zero constant and an accumulator. Standardizing which registers hold masks/zeros would improve readability and make it easier to audit.
- `fp_fold_dotp_f64` uses FMA but does not prefetch like the i64 version. Adding `prefetcht0` on the input arrays may help large workloads.

### Compaction (`src/asm/fp_core_compaction.asm`)

- **Done** ✔️ Replaced the branchy LUT/scalar writes with a SIMD path that uses `vmovmskpd` + popcount and `vpermps` to pack lanes, followed by `vmaskmovpd` to write contiguous results. Popcount and store-mask tables now stay hot, and the loop no longer jumps per element.

### 3D math kernels (`src/asm/3d_math_kernels.asm`)

- Comments say “matched to row-major layout,” but `Mat4` is documented as column-major in `include/fp_core.h`. Need to confirm the actual math order; otherwise the assembly produces transposed transforms.
- **Done** ✔️ Confirmed column-major usage and updated `fp_map_transform_vec3_f32` to multiply columns (and corrected the comment) so it matches the C reference and OpenGL-style data layout.

### Cross-cutting

- Many files perform the same four-way horizontal reduction pattern (add, extract upper 128, repeat). A macro or helper comment block describing the pattern would aid future maintenance.
- **Done** ✔️ Added `src/asm/fp_common.inc` with `FP_SAVE_YMMS/FP_RESTORE_YMMS` macros and refactored `fp_core_reductions.asm` + `fp_core_fused_folds.asm` to use them, eliminating duplicated YMM save/restore boilerplate.

## 2. Wrappers & Headers

### `include/fp_core.h`

- Public API is well documented, but array parameters could benefit from `restrict` qualifiers to help the compiler vectorize wrapper logic.
- Reconcile the Mat4 layout description with the assembly note mentioned above.

### General HOFs (`src/wrappers/fp_general_hof.c`)

- Implementations are straightforward but rely entirely on the compiler to optimize loop bodies. Adding `restrict` to pointers and manually unrolling to four iterations could reduce the 20–30% callback overhead cited in docs.

### Percentile & Quartile wrappers (`src/wrappers/fp_percentile_wrappers.c`)

- **Done** ✔️ Added `*_with_buffer` overloads for percentiles, multiple percentiles, quartiles, and IQR outlier detection so callers can supply reusable scratch buffers instead of triggering per-call allocations.

### Moving averages (`src/wrappers/fp_moving_averages_wrappers.c`)

- SMA now composes from `fp_rolling_mean_f64_optimized`, which is excellent. EMA/WMA still have bespoke loops; consider expressing WMA in terms of `fp_zipWith_f64` + `fp_reduce_add_f64` with cached weight vectors so composability remains visible.

### Rolling windows (`src/wrappers/fp_rolling_window.c`)

- `fp_rolling_reduce_*` rebuild each window from scratch, even though optimized runners exist later in the file. Provide optional closures or helper structs so min/max can also piggyback on sliding sums.

## 3. Benchmarks

### `bench_refactoring.c`

- Uses `QueryPerformanceCounter` but only measures the composition implementation because the original asm has been replaced. Reintroducing a simple scalar baseline would make the metrics more meaningful.
- Seed `rand()` (currently unseeded) to ensure reproducible data generation.

### `bench_general_hof.c`

- **Done** ✔️ Switched timing to `QueryPerformanceCounter`, randomized whether the general or specialized variant runs first in each benchmark, and stored per-benchmark timings for a summary table. This removed cache bias and makes high-level reports less ambiguous.

### `bench_generic_vs_specialized.c`

- **Done** ✔️ Added a single `RNG_SEED` and seeded the benchmark before any data generation so runs are reproducible.

## 4. Follow-up Questions / Ideas

All items identified so far have been addressed (Mat4 layout, NASM prologue macros, SIMD max/min, compaction rewrite, scratch-buffer overloads, and benchmark RNG/timing fixes). No outstanding follow-ups at this time.

## Recent Updates

- [x] 2025-11-14 – `src/algorithms/fp_time_series.c`: `fp_forecast_sma` now reuses `fp_rolling_sum_f64_optimized` for both the horizon forecast and training-error computation. The function allocates a single sliding-sum buffer when memory allows and falls back to the original scalar loops if the allocation fails, so behavior stays correct on low-memory systems.
