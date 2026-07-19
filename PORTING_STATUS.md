# Cross-platform porting status

The library builds its **complete** API on both Windows (x64) and Linux/SysV.
On Windows every function behaves as it did upstream. On Linux, correctness
depends on whether a module has been migrated to the cross-platform ABI layer
(`src/asm/abi.inc`) and validated against a scalar reference.

**Windows x64:** every module below works (the ABI shims are no-ops there).

**Linux / System V:** only the ✅ modules are verified correct. The ⏳ modules
still use the Windows-only calling convention and will return incorrect results
if called on Linux — do not use them on Linux yet.

## ✅ Ported and test-verified on Linux

| Module(s) | Functions |
|---|---|
| `fp_core_reductions*` (10 widths) | `fp_reduce_{add,min,max,mul,product}_*`, `fp_reduce_add_f64_where` |
| `fp_core_fused_folds*` (10 widths) | `fp_fold_{dotp,sumsq,sad}_*` |
| `fp_core_fused_maps*` (all widths) | `fp_map_{scale,offset,axpy}_*`, `fp_zip_add_*` (i8/i16/i32/i64, u8/u16/u32/u64, f32/f64) |
| `fp_core_predicates` | `fp_pred_all_eq_const_i64`, `fp_pred_any_gt_const_i64`, `fp_pred_all_gt_zip_i64` |
| `fp_core_tier3` | `fp_count_i64`, `fp_range_i64`, `fp_iterate_{add,mul}_i64`, `fp_group_i64`, `fp_run_length_encode_i64`, `fp_zip_with_index_i64`, `fp_reduce_{and,or}_bool`, `fp_replicate_f64` |
| `3d_math_kernels` | `fp_map_transform_vec3_f32`, `fp_zipWith_vec3_add_f32`, `fp_map_quat_rotate_vec3_f32`, `fp_reduce_vec3_add_f32`, `fp_fold_vec3_dot_f32`, `fp_quat_normalize_asm`, `fp_quat_to_mat4` |
| `fp_core_matrix` | `fp_mat4_identity`, `fp_mat4_mul`, `fp_mat4_mul_vec3`, `fp_mat4_transpose`, `fp_mat4_mul_vec3_batch` |
| `fp_core_essentials` | `fp_contains_i64`, `fp_find_index_i64`, `fp_take_n_i64`, `fp_drop_n_i64`, `fp_slice_i64`, `fp_reverse_i64`, `fp_concat_i64`, `fp_reduce_product_{i64,f64}`, `fp_replicate_i64` |
| `fp_core_simple_maps` | `fp_map_abs_{i64,f64}`, `fp_map_sqrt_f64`, `fp_map_clamp_{i64,f64}` |
| `fp_core_scans` | `fp_scan_add_{i64,f64}` (prefix sum) |
| `fp_core_descriptive_stats` | `fp_moments_f64`, `fp_descriptive_stats_f64` |

Covered by `tests/test_reductions.c`, `test_int_families.c`, `test_maps.c`,
`test_game_math.c`, `test_essentials.c` (run `make test`).

## ⏳ Not yet ported to Linux (Windows-only for now)

| Module | Notes |
|---|---|
| `fp_core_percentiles` | 5th arg on stack |
| `fp_core_compaction` | 5-arg (stack) functions |
| `fp_core_tier2` | 5-arg (stack) functions |
| `fp_blake3_avx2` | BLAKE3 hashing (candidate for removal — not core FP/game math) |

## Bugs fixed so far

- **AVX-512 instruction in an AVX2 library** — `vpbroadcastq ymm, r64` (4 sites)
  raised `SIGILL` on AVX2-only CPUs. Replaced with `vmovq`+`vpbroadcastq`.
- **`fp_reduce_vec3_add_f32`** returned `(0,0,0)` for arrays shorter than 8
  (tail accumulator was zeroed before use).
- **`fp_mat4_transpose`** produced a wrong permutation (not a valid transpose);
  replaced with a correct 4×4 transpose.
- **`fp_reverse_i64`** used an in-place pair-swap loop while writing to a
  separate output buffer, interleaving the two ends instead of reversing.
  Rewritten as `out[i] = in[n-1-i]`.
- **Narrow-width `fp_core_fused_maps_*`** clobbered callee-saved `r12/r13/r14`
  (+ `r15` in some axpy, `rbx` in `axpy_u64`, and `xmm6/7/15` on Windows)
  without preserving them — corrupting the caller on every platform (segfaults
  on Linux under `-O2`). Every narrow map function now uses a uniform frame that
  preserves `rbx`/`r12`–`r15` (and, on Win64 only, `xmm6/xmm7/xmm15`).
