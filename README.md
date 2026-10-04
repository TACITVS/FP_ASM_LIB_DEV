# FP-ASM — functional primitives for C, at assembly speed

A small, **externally linkable** library of functional-programming primitives
(reductions, folds, maps, scans) and game math (vec3 / mat4 / quaternion) whose
hot paths are hand-written **x64 AVX2 assembly**, wrapped in a thin C API.
Cross-platform (Linux + Windows x64), tested, and dependency-free — built to be
dropped into a game or graphics engine.

**→ [Landing page & interactive benchmarks](https://tacitvs.github.io/FP_ASM_LIB_DEV/)**

```c
#include "fp_types.h"
extern void  fp_mat4_mul_vec3_batch(Vec3f* out, const Mat4* m, const Vec3f* in, int n);
extern float fp_fold_dotp_f32(const float* a, const float* b, size_t n);

fp_mat4_mul_vec3_batch(world, mvp, local, n);   // transform n vertices, one AVX2 pass
float e = fp_fold_dotp_f32(a, b, n);            // ~5x a scalar dot product
```

## Why

The kernels pull ahead of the compiler exactly where the compiler can't help
you: **reductions and dot products**, where a scalar float accumulator is a
serial dependency chain that `-O3` cannot reorder (float addition isn't
associative). The assembly keeps several independent SIMD accumulators in flight
instead. Memory-bound element-wise ops sit at parity with autovectorized scalar
code — as they should.

### Benchmarks

Each kernel timed against a scalar C reference at the **same** `-O3 -march=native`
(so the baseline is autovectorized too). 100k f32 elements, best of 200 runs,
Intel Core i7-4600M (Haswell, AVX2), gcc 13. Reproduce with `make bench`.

| Operation | Scalar `-O3` | FP-ASM | Speedup |
|---|--:|--:|--:|
| reduce sum (f32) | 85.0 µs | 9.9 µs | **8.6×** |
| dot product (f32) | 96.0 µs | 18.9 µs | **5.1×** |
| mat4 × vec3 (batch transform) | 189 µs | 122 µs | **1.5×** |
| axpy `o = 2.5·x + y` (f32) | 34.4 µs | 34.2 µs | ≈ parity |
| scale `o = 3·x` (f32) | 25.8 µs | 25.7 µs | ≈ parity |

*Your numbers will vary with CPU and workload size. The honest takeaway: big
wins on reductions/dot/batched math, parity on the memory-bound kernels.*

## Architecture

| Layer | What | Where |
|---|---|---|
| **L0 — assembly** | AVX2 SIMD kernels: reductions, fused folds (dot/sumsq/sad), maps (scale/offset/axpy), prefix scans, set ops, and vec3/mat4/quaternion math over 10 numeric types | `src/asm/` |
| **L1 — C wrappers** | the functional operation set — map/filter/foldl/foldr, scans, zipWith, takeWhile/dropWhile/span, partition, all/any/find, concatMap, iterate/unfoldr, composition & pipelines, `Maybe`/`Either` monads, lazy sequences, transducers | `src/wrappers/` |
| **L2 — math/algorithms** | batched vertex transforms, matrix ops, FFT, radix sort, statistics — composed from L0 | `src/algorithms/` |

No imperative loops in the inner kernels; no runtime allocation in the kernels;
non-executable stack on ELF.

**FP coverage.** The aim is full coverage of the standard functional *operation*
set (Haskell `Prelude`/`Data.List`, Lisp, ML) — not re-implementing those
languages, which a linked C library can't do (no lazy-by-default evaluation,
type inference, pattern matching, or enforced purity). See
[FP_COVERAGE.md](FP_COVERAGE.md) for exactly what's covered today and what's
still planned.

## Build

Requirements: **NASM** (≥ 2.13) and a C11 compiler (gcc/clang). The kernels
require a CPU with **AVX2 + FMA**; faster AVX-VNNI / AVX-512 variants are
selected at runtime on CPUs that have them.

```bash
make            # static + shared libraries into build/
make test       # build and run all test suites
make bench      # run the benchmarks
make install    # install headers + libs under PREFIX (default /usr/local)
```

### Build targets: old and new CPUs

The assembly is the same for every target. `ISA=` only chooses what the
**C code** may use (`-march`), and runtime dispatch picks the fastest kernel
variant the CPU actually has.

| `ISA=` | C code needs | Runs on | Use it for |
|---|---|---|---|
| `x86-64-v3` | AVX2, FMA, BMI2 | Haswell / Zen 1 and newer | a library you ship to other machines |
| `haswell` | same, tuned for Haswell | Haswell and newer | the original target (i7-4600M) |
| `alderlake` / `raptorlake` | + AVX-VNNI, GFNI, VAES… | 12th–14th gen Core, Meteor/Arrow Lake | your 13th-gen laptop |
| `x86-64-v4` (`avx512`) | + AVX-512 F/BW/CD/DQ/VL | Ice Lake, Sapphire Rapids, Zen 4/5 | AVX-512 servers/desktops |
| `native` (default) | whatever the build machine has | the build machine | local development |

**13th-gen Core (Raptor Lake) has no AVX-512.** Intel disables it on all
hybrid P-core/E-core client chips, because the E-cores don't implement it.
On those CPUs the new path is **AVX-VNNI** plus `-march=raptorlake` tuning. The
AVX-512 kernels are there for CPUs that do have it.

```bash
make ISA=x86-64-v3                 # portable: old and new machines
make ISA=raptorlake                # tuned for 12th-14th gen Core
make DISPATCH=0                    # legacy layout: public symbols = AVX2 kernels, no dispatcher
make all-isas                      # build/x86-64-v3/, build/raptorlake/, build/x86-64-v4/ side by side
make BUILD=build-old ISA=haswell   # keep a separate build tree per target
```

Changing `ISA`/`DISPATCH`/`CFLAGS` triggers a full rebuild automatically, so
objects from two targets are never mixed. CMake has the same options:
`-DFPASM_ISA=raptorlake -DFPASM_DISPATCH=ON -DFPASM_BUILD_TOOLS=ON`.

**Runtime dispatch** (`include/fp_dispatch.h`). These kernels have ISA
variants. The best one is chosen on first call:

| kernel | avx2 | avxvnni | avx512 |
|---|:-:|:-:|:-:|
| `fp_reduce_add_f32/f64`, `fp_fold_sumsq_f32`, `fp_fold_dotp_f32/f64` | ✓ | | ✓ |
| `fp_fold_dotp_i8/u8/i16/u16` | ✓ | ✓ | ✓ (AVX512-VNNI) |

Every variant is also exported by name (`fp_fold_dotp_i8_avxvnni`, …) for A/B
tests. `FPASM_TIER=avx2|avxvnni|avx512` caps the tier at runtime.
`FPASM_VERBOSE=1` prints the selection. `fp_dispatch_set_max_tier()` does the
same from code.

### Diagnostics

```bash
make info        # toolchain, NASM, ISA -> -march, what -march=native resolves to, supported presets
make diag        # build/fpasm-info: CPU, OS register state, every feature vs. what this build needs, dispatch table
make check-cpu   # fpasm-info --check: fail with an explanation if this build can't run here (runs before `make test`)
make bench-isa   # fpasm-info --bench: time every kernel variant this CPU supports
make lint-asm    # static check: callee-saved GPRs / Win64 xmm6-15 used without saving
make test-win64-abi  # Linux run of the Win64 register-preservation contract
```

`fpasm-info` also has `--json` and `--tier`. It reports hybrid P/E-core
layout (and which core type the thread is on), hypervisors (WSL2, Windows
VBS), and features the CPU has but the OS hasn't enabled. If a library built
for a newer CPU runs on an older one, it prints what is missing and which
`ISA=` to rebuild with, instead of failing with *Illegal instruction*.
From code: `fp_cpu_report(stdout)`, `fp_cpu_check(stderr)`, `fp_cpu_has(FP_CPU_AVX_VNNI)`
(`include/fp_cpu.h`). The shared library checks automatically at load time.
Set `FPASM_CPU_CHECK=0|strict|report` to turn this off, make it abort, or
print the full report. With static linking, call `fp_cpu_check()` yourself.

Measured with `fpasm-info --bench` (64k elements, one core) on an AVX-512
Xeon (Emerald Rapids):

| kernel | avx2 | avxvnni | avx512 |
|---|--:|--:|--:|
| `fp_fold_dotp_i8` | 1.0× | **12.1×** | **14.5×** |
| `fp_fold_dotp_f32` | 1.0× | — | 1.41× |
| `fp_fold_sumsq_f32` | 1.0× | — | 1.33× |
| `fp_reduce_add_f32` | 1.0× | — | 1.10× (memory-bound) |
| `fp_fold_dotp_i16` | 1.0× | 1.00× | 1.20× |

On a Raptor Lake laptop, run `taskset -c 0-7 make bench-isa` (P-cores) and
again on an E-core to see the hybrid difference.

### CMake — link it into a game / graphics project

```bash
cmake -S . -B build && cmake --build build && ctest --test-dir build
```

```cmake
add_subdirectory(path/to/fp_asm)
target_link_libraries(mygame PRIVATE fpasm::fpasm)
```

Or link the built library directly: `cc game.c -lfpasm`.

## Cross-platform status

The entire in-scope library is ported to a single ABI-abstracted source tree and
**test-verified on Linux/System V**, while assembling for Windows x64. On Windows
the ABI shims compile to nothing, so the original, verified behavior is preserved
byte-for-byte. See [PORTING_STATUS.md](PORTING_STATUS.md) for the per-module
matrix and the list of bugs fixed during the port.

## About this version

A clean-room extraction and hardening of the FP core from a larger
vector-database project: cross-platform ABI, a portable build system, a real
test suite, benchmarks, and several correctness fixes (including an AVX-512
instruction that had shipped in an "AVX2" library, and multiple callee-saved
register violations). The graphics engine, OpenCL, ML classifiers, and vector-DB
query code from upstream are intentionally out of scope.

A compact SIMD-first graphics library built on this math core is on the roadmap.

## License

MIT — see [LICENSE](LICENSE).
