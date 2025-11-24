# Known Issues and Limitations

This document tracks known issues, limitations, and technical debt in the FP_ASM_LIB library.

## Platform Limitations

### Windows x64 Only (PR #12)

**Status:** By Design
**Severity:** Critical for cross-platform
**Affected:** All NASM assembly files in `src/asm/`

The NASM assembly implementations use the **Windows x64 ABI** calling convention:
- Parameters in: RCX, RDX, R8, R9 (then stack)
- Return value in: RAX
- Shadow space required

This is **incompatible** with the System V AMD64 ABI used on Linux/macOS:
- Parameters in: RDI, RSI, RDX, RCX, R8, R9
- Different stack alignment and red zone rules

**Current Mitigation:**
- CMake blocks non-Windows builds
- `#error` directive in headers for non-Windows platforms

**Future Work:**
- Create System V ABI variants of assembly functions
- Use conditional assembly to select correct ABI at build time

---

## Memory Management

### Silent Failures in compute_gradients (PR #44)

**Status:** Documented
**Severity:** Low
**Affected:** `src/algorithms/fp_linear_regression.c`

If memory allocation fails during gradient computation, the function silently sets gradients to zero rather than propagating an error. This causes gradient descent to stall (no weight updates) rather than explicitly failing.

**Rationale:** This is semi-intentional behavior - stalled optimization is detectable via lack of convergence, and propagating errors would require significant API changes.

---

### Memory Leak Risk in fp_compose (PR #33)

**Status:** Known
**Severity:** Medium
**Affected:** `src/wrappers/fp_compose.c`

The `fp_compose_f64()` and `fp_compose_i64()` functions allocate a context struct but do not expose a cleanup function. If the composed function is discarded without execution, the context leaks.

**Workaround:** Always execute composed functions or track the returned function pointer for manual cleanup.

---

### Monte Carlo Silent Malloc Failure (PR #35)

**Status:** Known
**Severity:** Low
**Affected:** `src/algorithms/fp_monte_carlo.c`

`fp_monte_carlo_option_price()` returns 0.0 on malloc failure, which is indistinguishable from a valid (but unlikely) option price of zero.

---

## Numerical Stability

### Variance Computation (PR #36)

**Status:** Documented
**Severity:** Low
**Affected:** Statistical functions using `E[X^2] - E[X]^2` formula

The single-pass variance formula `Var(X) = E[X^2] - E[X]^2` can suffer from catastrophic cancellation when `E[X]^2` is close to `E[X^2]`. This is acceptable for typical data ranges but may produce incorrect results for data with very large mean and small variance.

**Alternative:** Use two-pass Welford's algorithm for numerically sensitive applications.

---

## Test Coverage

### L1 Test Memory Leaks (PR #31)

**Status:** Known
**Severity:** Low (test code only)
**Affected:** `tests/unit/test_L1_wrappers.c`

Several memory leaks exist in test code:
- Non-composed transducers not freed
- Lazy sequence demo allocations
- Missing NULL checks after malloc

These do not affect production code but should be cleaned up.

---

## Version History

| Date | PR | Issue Added |
|------|-----|-------------|
| 2025-11 | #12 | Windows x64 ABI limitation |
| 2025-11 | #31 | L1 test memory leaks |
| 2025-11 | #33 | fp_compose memory leak risk |
| 2025-11 | #35 | Monte Carlo silent failure |
| 2025-11 | #36 | Variance numerical stability |
| 2025-11 | #44 | compute_gradients silent failure |

---

*Last updated: 2025-11-24 by Claude Code*
