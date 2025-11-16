# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

FP-ASM is a high-performance functional programming library for C that implements common FP patterns (map, fold, reduce, zip) using hand-optimized x64 assembly with AVX2 SIMD instructions. The library targets Windows x64 and focuses on achieving measurable performance gains over aggressively optimized C code (`gcc -O3 -march=native`).

**Core Performance Strategies:**
1. **Fused Kernels**: Combine map+reduce or map+map operations into single-pass assembly to minimize memory bandwidth (e.g., `fp_fold_dotp_i64` = map square + reduce add)
2. **Scalar Wins**: Exploit compiler blind spots where AVX2 lacks instructions (e.g., 64-bit multiply). Hand-tuned scalar assembly with loop unrolling and multiple accumulators can significantly outperform compiler's scalar fallback
3. **Guaranteed SIMD**: Provide reliable, consistent SIMD performance for operations the compiler can auto-vectorize, removing dependence on compiler heuristics

## Build Commands

### Assemble a module:
```bash
nasm -f win64 fp_core_<module>.asm -o fp_core_<module>.o
```

### Build a benchmark executable:
```bash
gcc demo_bench_<module>.c fp_core_<module>.o -o bench_<module>.exe -v
```

### Run benchmarks:
```bash
# Default: 10M elements, 10 iterations
./bench_<module>.exe

# Custom size and iterations
./bench_<module>.exe 1000000 20
```

## Module Status & Organization

### Module 1: Simple Folds (Reductions) ✅ COMPLETE
- **Files**: `fp_core_reductions.asm` (v10), `demo_bench_reductions.c`
- **Functions**: `fp_reduce_add_i64/f64`, `fp_reduce_max_i64/f64`
- **Performance**: 1.5-1.8x for f64 (GCC vectorization failure), ~1.0-1.02x for i64 (scalar optimization wins)
- **Status**: Verified, stable

### Module 2: Fused Folds (Map-Reduce) ✅ COMPLETE
- **Files**: `fp_core_fused_folds.asm` (v5), `demo_bench_fused_folds.c`
- **Functions**: `fp_fold_sumsq_i64`, `fp_fold_dotp_i64/f64`, `fp_fold_sad_i64`
- **Performance**: ~1.1x for i64 scalar wins, ~1.25x for f64 FMA, ~1.03x for sad
- **Key Learning**: Fused operations eliminate temporary arrays, keeping data in registers
- **Status**: Verified, stable

### Module 3: Fused Maps (BLAS Level 1) ✅ COMPLETE
- **Files**: `fp_core_fused_maps.asm` (v14-FINAL), `demo_bench_fused_maps.c`
- **Functions**: `fp_map_axpy_i64/f64`, `fp_map_scale_i64/f64`, `fp_map_offset_i64/f64`, `fp_zip_add_i64/f64`
- **Performance**: ~1.0-1.1x (memory-bound operations, guaranteed SIMD saturation)
- **Key Learning**: Required extensive ABI debugging (5th stack argument, register preservation)
- **Status**: Verified, stable

### Module 4: Simple Maps (Transformers) ✅ COMPLETE
- **Files**: `fp_core_simple_maps.asm` (v2-FIXED), `demo_bench_simple_maps.c`
- **Functions**: `fp_map_abs_i64/f64`, `fp_map_sqrt_f64`, `fp_map_clamp_i64/f64` (5 functions)
- **Performance**: TBD (pending full benchmarks)
- **Strategy**: Guaranteed SIMD. Bitwise trick for abs_i64, vsqrtpd for sqrt, vminpd/vmaxpd for clamp_f64
- **Key Learning**: clamp_i64 uses scalar (AVX2 lacks vpmaxsq/vpminsq)
- **Status**: Debugged (4 stack alignment bugs fixed), verified with correctness tests

### Module 5: Scans (Prefix Sums) ✅ COMPLETE
- **Files**: `fp_core_scans.asm` (v1), `demo_bench_scans.c`
- **Functions**: `fp_scan_add_i64`, `fp_scan_add_f64` (2 functions)
- **Performance**: **2.0-3.2x speedup** (exceptional for sequential operation!)
  - scan_add_i64: 1.9-2.6x
  - scan_add_f64: 2.2-3.2x (best at 100K elements)
- **Strategy**: Scalar with 4-way loop unrolling, optimal register allocation
- **Key Learning**: Scalar optimization still matters - achieved 3.2x despite being inherently sequential
- **Status**: Implemented, tested, verified production-ready

### Module 6: Predicates 📋 PLANNED
- **Functions**: `fp_pred_all_eq_const_i64`, `fp_pred_any_gt_const_i64`, `fp_pred_all_gt_zip_i64`
- **Strategy**: Use AVX2 compare instructions (vpcmpeqq, vpcmpgtq), mask extraction, bit testing

## File Naming Conventions

- **Core API**: `fp_core.h` - Public function declarations
- **Assembly modules**: `fp_core_<module>.asm` - Hand-optimized kernels
- **Object files**: `fp_core_<module>.o` - Assembled binaries (checked into repo)
- **Benchmarks**: `demo_bench_<module>.c` - Correctness + performance tests
- **Executables**: `bench_<module>.exe` - Compiled benchmarks

## Windows x64 ABI Requirements (CRITICAL)

**This is the #1 source of bugs. Strict adherence is non-negotiable:**

- **Arguments**: RCX, RDX, R8, R9, then stack at [RBP+48], [RBP+56], etc.
- **Volatile registers**: RAX, RCX, RDX, R8-R11, XMM0-XMM5 (can be clobbered)
- **Non-volatile registers**: RBX, RBP, RDI, RSI, R12-R15, XMM6-XMM15 (MUST preserve)
- **Shadow space**: Caller allocates 32 bytes; functions can use without saving
- **Stack alignment**: 16-byte aligned before CALL (RSP % 16 == 8 inside function after return address push)
- **YMM preservation**: Must save/restore full YMM state for non-volatile XMM6-15

## Assembly Implementation Patterns

### Standard Function Prologue:
```nasm
push rbp
mov rbp, rsp
sub rsp, 32                  ; Shadow space
and rsp, 0xFFFFFFFFFFFFFFE0  ; Align to 32 bytes
sub rsp, 128                 ; Space for YMM6-9
vmovdqa [rsp], ymm6
vmovdqa [rsp+32], ymm7
vmovdqa [rsp+64], ymm8
vmovdqa [rsp+96], ymm9
```

### Standard Function Epilogue:
```nasm
vzeroupper                   ; Clear upper YMM state
vmovdqa ymm6, [rsp]
vmovdqa ymm7, [rsp+32]
vmovdqa ymm8, [rsp+64]
vmovdqa ymm9, [rsp+96]
mov rsp, rbp
pop rbp
ret
```

### Main Loop Pattern (16 elements = 4 YMM registers):
```nasm
.loop16:
    cmp rcx, 16
    jb .tail
    ; Process 4 YMM registers (64 bytes)
    vmovupd ymm0, [r12]
    vmovupd ymm1, [r12+32]
    vmovupd ymm2, [r12+64]
    vmovupd ymm3, [r12+96]
    ; ... operations ...
    add r12, 128
    sub rcx, 16
    jmp .loop16
```

### Scalar Tail Loop:
```nasm
.tail:
    test rcx, rcx
    jz .cleanup
.tail_loop:
    vmovsd xmm0, [r12]
    ; ... operations ...
    add r12, 8
    dec rcx
    jnz .tail_loop
```

## Critical Lessons Learned

### Performance:
- **Avoid false dependencies**: In scalar unrolled loops, use distinct registers per iteration to enable CPU-level parallelism
- **Summation order matters**: Parallel SIMD reduction uses different order than scalar, causing small FP differences (acceptable with tolerance checks)
- **FMA instructions**: Use vfmadd213pd for `(a*b)+c` patterns (3 operands, result in 1st operand)

### Correctness:
- **Floating-point comparisons**: Always use relative tolerance (1e-12) for correctness checks
- **Instruction availability**: vpsraq (arithmetic right shift for i64) requires AVX-512, not AVX2. Use scalar sar for AVX2
- **Register reuse bugs**: In Module 3, reusing R12-R14 without preservation corrupted caller state

### Testing:
- **Benchmark structure**: Each demo_bench_<module>.c runs correctness checks FIRST, halts on failure, then runs performance tests
- **Volatile sinks**: Use `volatile` variables to prevent dead code elimination in benchmarks
- **Iterative verification**: Test each module individually before proceeding to next

## AVX2 Instruction Limitations

- **No i64 min/max**: vpmaxsq/vpminsq require AVX-512. Use scalar cmov for AVX2
- **No i64 multiply**: imul is scalar only. Use multiple accumulators + unrolling
- **No i64 abs**: vpabsq requires AVX-512. Use bitwise trick: `abs(x) = (x XOR mask) - mask` where `mask = x >> 63`
- **No i64 arithmetic shift**: vpsraq requires AVX-512. Use scalar sar

## Adding New Operations

1. Add declaration to `fp_core.h` in appropriate module section
2. Implement in `fp_core_<module>.asm` following ABI and patterns above
3. Reassemble: `nasm -f win64 fp_core_<module>.asm -o fp_core_<module>.o`
4. Add C baseline implementation to `demo_bench_<module>.c`
5. Add correctness check (runs before benchmarks)
6. Add performance benchmark
7. Build and verify: `gcc demo_bench_<module>.c fp_core_<module>.o -o bench_<module>.exe`

## Functional Programming Wrapper Layer ✅ COMPLETE

### Module 7: FP Composition & Pipelines
- **Files**: `fp_compose.h`, `fp_compose.c` (1049 lines), `fp_compose_inline.h` (387 lines)
- **Features**: Function composition (f . g), pipeline builder with fluent API, transducers, partial application, lazy evaluation
- **Status**: Fully implemented and documented

### Module 8: Monadic Error Handling
- **Files**: `fp_monads.h`, `fp_monads.c` (665 lines), `fp_monads_inline.h` (283 lines)
- **Features**: Maybe monad (Just/Nothing), Either monad (Left/Right), safe arithmetic, sequence operations, error propagation
- **Status**: Fully implemented and documented

### Module 9: Examples & Documentation
- **Files**: `examples/basic/fp_wrapper_demo.c` (447 lines), `docs/guides/FP_WRAPPER_USER_GUIDE.md` (511 lines), `docs/guides/FP_WRAPPER_ENHANCEMENT_PLAN.md`
- **Coverage**: 8 comprehensive examples, complete API reference, performance tips
- **Status**: Complete with user guide

### Key FP Capabilities

**Maybe Monad (Safe Optional Values):**
```c
Maybe result = fp_safe_divide_f64(10.0, 2.0);  // Just(5.0)
result = fp_bind_maybe_f64(result, fp_safe_sqrt_f64);  // Just(√5)
double value = fp_from_maybe_f64(result, 0.0);  // Extract or default
```

**Either Monad (Error Messages):**
```c
Either result = fp_checked_divide_f64(10.0, 0.0);  // Left("Division by zero", -1)
if (fp_is_left(result)) {
    printf("Error: %s\n", fp_from_left_msg(result));
}
```

**Pipeline Builder (Fluent API):**
```c
fp_pipeline_f64_t* p = fp_pipeline_f64(data, n);
double result = p
    ->map(p, square, NULL)
    ->filter(p, is_positive, NULL)
    ->reduce(p, 0.0, add, NULL);
fp_pipeline_free_f64(p);
```

**Fused Operations (Zero-Copy):**
```c
// Single pass, no temporary array!
double sum = fp_fused_map_reduce_f64_inline(data, n, square, 0.0, add);
```

**Lazy Sequences:**
```c
fp_lazy_seq_t* seq = fp_lazy_range_f64(0.0, 100.0, 1.0);
double* array = fp_lazy_to_array_f64(seq, 50, &count);  // Force first 50
```

### Performance Characteristics

- **Inline versions**: Zero-overhead abstractions via `*_inline.h` headers
- **Fused operations**: Eliminate temporary arrays (1.5-2x memory bandwidth savings)
- **Tagged unions**: Maybe/Either compile to single comparison + conditional move
- **Pipeline single-pass**: Double-buffering for chained transformations
- **Lazy evaluation**: Process only what's needed

## Future Work

- Expand assembly modules to i32 and f32 data types
- Implement argmin/argmax, mean, variance, boolean reductions
- Add Applicative/Alternative/MonadPlus instances for FP layer
- Explore AVX-512 versions (vpmulq, vpabsq, 512-bit operations)
- Port to Linux System V AMD64 ABI
- Create Makefile/CMake build system
- Add Free monads and Arrows for advanced FP patterns
