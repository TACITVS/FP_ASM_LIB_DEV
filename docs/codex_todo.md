# FP_ASM_LIB Functional Composition TODO

Tracked tasks come directly from the latest audit of `src/algorithms`. Each entry must be checked off only after the corresponding module is refactored to compose from FP_ASM primitives (and verified via benchmarks/output capture).

## Task Queue (easiest → hardest)

1. [ ] `src/algorithms/3d_math_wrapper.c` – Replace scalar kernels (`kernel_vec3_add`, `kernel_transform_vec3`, etc.) with compositions of the vector map/zip primitives, eliminating bespoke loops.
2. [ ] `src/algorithms/fp_quaternion_ops.c` – Stop duplicating quaternion math; call the existing `fp_quat_mul`/`fp_map_quat_rotate_vec3` kernels directly.
3. [ ] `src/algorithms/fp_linear_regression.c` – Rework both closed-form and gradient-descent paths to compose from `fp_linear_regression_f64`, `fp_fold_dotp_f64`, `fp_map_axpy_f64`, etc., instead of inline loops.
4. [ ] `src/algorithms/fp_time_series.c` – Extend the SMA refactor to every forecast routine: use rolling helpers for trends/errors and compute MAE via absolute-error reductions.
5. [ ] `src/algorithms/fp_kmeans.c` – Express distance and centroid updates via existing fold/zip primitives (e.g., `fp_fold_dotp_f64`, `fp_map_zip_sub_f64`, `fp_rolling_mean_f64_optimized`).
6. [ ] `src/algorithms/fp_lighting.c` – Ensure shading helpers compose from SIMD vector operators (no raw `vec3_*` loops once `fp_vector_ops` is updated).
7. [ ] `src/algorithms/fp_monte_carlo.c` – Replace manual accumulation in PI/integration/option payoffs with folds/scans (`fp_reduce_add_*`, `fp_scan_inclusive_*`).
8. [ ] `src/algorithms/fp_naive_bayes.c` – Delegate statistics to `fp_descriptive_stats_f64`, `fp_reduce_add_f64`, and exponent/log map combinators instead of bespoke loops.
9. [ ] `src/algorithms/fp_pca.c` – Use `fp_covariance_f64`, dot-product folds, and matrix combinators for covariance/projection/reconstruction steps.
10. [ ] `src/algorithms/fp_fft.c` – Integrate FP primitives into butterfly/convolution flows (dot products, axpy, map/filter) to eliminate redundant scalar math.
11. [ ] `src/algorithms/fp_radix_sort.c` – Implement counting/prefix phases with existing scan/rolling-sum primitives rather than ad-hoc loops.
12. [ ] `src/algorithms/fp_decision_tree.c` – Rebuild splitting and impurity evaluation atop FP predicates, partitions, and reductions.
13. [ ] `src/algorithms/fp_neural_network.c` – Rewrite forward/backprop as compositions of dot, map, and axpy primitives; no manual neuron loops.
14. [ ] `src/algorithms/fp_ray_tracer.c` – Route shading/intersection math through FP vector primitives; eliminate scalar math once `fp_vector_ops` is upgraded.
15. [ ] `src/algorithms/fp_vector_ops.c` – Core dependency: convert every Vec3 helper to use the AVX map/zip/reduce kernels so higher layers automatically become FP-compliant.

## Notes

- Keep this list synchronized with `docs/codex_assembly_review_notes.md` after each completed item.
- Each completed task must include: code diff, demo/test run with Notepad output, and benchmark comparison when applicable.
