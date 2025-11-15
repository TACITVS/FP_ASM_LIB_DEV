# Assembly & Wrapper Review – November 2025

This note records the issues discovered during the latest pass over the low‑level
code, ranked from easiest to hardest to fix. Keeping this file under version
control should help future work avoid regressions.

---

## 1. `fp_rolling_mean_f64` underflow (C wrapper – quick fix)

- **Files:** `src/wrappers/fp_rolling_window.c:121‑133` and `:215‑224`
- **Problem:** Both mean helpers call `fp_rolling_sum_*` and then immediately
  compute `size_t out_size = n - window + 1`. If `window > n`, `fp_rolling_sum`
  already returns without writing anything, but the mean helpers continue and
  the subtraction underflows, causing them to stampede through the output
  buffer. `window == 0` has a similar effect.
- **Action:** Mirror the guard used in the other rolling helpers:
  `if (window == 0 || n < window) return;` before computing `out_size`.

---

## 2. `fp_map_clamp_f64` corrupts non‑volatile registers

- **File:** `src/asm/fp_core_simple_maps.asm:309‑356`
- **Problem:** The updated clamp kernel pushes `r12/r13` and then realigns `rsp`
  without saving the pre‑alignment pointer. On return it pops from the aligned
  frame, not from where the registers were stored, so `r12/r13` are restored
  with garbage, violating the Windows x64 ABI.
- **Action:** Save the original stack pointer (e.g., `mov r11, rsp`) before the
  `and rsp, …` and restore it before popping. Alternatively, adopt the same
  prologue macro used by the other map kernels so the save/restore pattern can’t
  drift again.

---

## 3. Matrix kernels clobber XMM8–XMM15

- **File:** `src/asm/fp_core_matrix.asm` (`fp_mat4_mul`, `fp_mat4_mul_vec3`,
  `fp_mat4_transpose`, `fp_mat4_mul_vec3_batch`)
- **Problem:** All of the matrix routines use XMM8+ but never save or restore
  them. Under the Windows x64 ABI, XMM6–XMM15 are callee‑saved registers, so any
  caller that keeps data in those registers will see silent corruption after
  calling into the matrix library.
- **Action:** Add the standard AVX2 save/restore scaffold (reserve 128 bytes
  after aligning `rsp`, `vmovaps` the callee‑saved XMM/YMM registers to the
  stack, and restore them before returning). Consider introducing macros (e.g.,
  `FP_SAVE_XMM_REGS`/`FP_RESTORE_XMM_REGS`) so every SIMD kernel follows the
  same discipline.

---

Please keep this file up to date whenever the low‑level layer is audited again.
# Matrix Register Preservation
- Introduced `SAVE_YMM_REGS/RESTORE_YMM_REGS` macros and wrapped all matrix kernels so they preserve XMM6–XMM15 as required by the Windows x64 ABI. Code is cleaner and fully compliant now.

