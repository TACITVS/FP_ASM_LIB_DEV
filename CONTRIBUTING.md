# Contributing to FP-ASM

Thank you for your interest in contributing to FP-ASM! This document provides guidelines and information for contributors.

## 🎯 Project Vision

FP-ASM aims to provide:
1. **100% FP language equivalence** with Haskell, Lisp, and ML
2. **Assembly-level performance** through hand-optimized SIMD
3. **Functional purity** with guaranteed immutability
4. **Practical usability** for systems programming

## 🤝 How to Contribute

### Reporting Bugs

**Before submitting a bug report:**
- Check existing [GitHub Issues](https://github.com/TACITVS/FP_ASM_LIB_DEV/issues)
- Verify the bug exists in the latest version
- Test with minimal reproduction case

**Bug report should include:**
- Clear description of the issue
- Steps to reproduce
- Expected vs actual behavior
- System information (OS, GCC version, NASM version)
- Code sample demonstrating the bug

### Suggesting Enhancements

**Enhancement suggestions should include:**
- Clear use case and motivation
- Proposed API (function signature)
- Performance expectations
- Haskell/Lisp/ML equivalent (if applicable)

### Pull Requests

**Before submitting a PR:**
1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Follow coding standards (see below)
4. Write/update tests
5. Update documentation
6. Ensure all tests pass
7. Submit PR with clear description

## 📝 Coding Standards

### C Code Style

```c
// Function naming: snake_case with fp_ prefix
void fp_my_function(const int64_t* input, size_t n);

// Constants: UPPER_SNAKE_CASE
#define MAX_ARRAY_SIZE 1000000

// Types: PascalCase for structs
typedef struct {
    double mean;
    double variance;
} DescriptiveStats;

// Comments: Explain WHY, not WHAT
// Use relative tolerance for FP comparisons (avoids rounding errors)
if (fabs(result - expected) / expected < 1e-12) { ... }
```

### Assembly Code Style

```nasm
; Function naming: Match C API exactly
global fp_reduce_add_i64

; Comments: Document register usage and algorithm
; RCX = input array pointer
; RDX = array length
; Returns: RAX = sum of all elements

; Labels: descriptive, prefixed with function name
.loop:
    ; ... instructions ...
    jmp .loop

.cleanup:
    ; ... cleanup code ...
    ret
```

### Commit Messages

Follow [Conventional Commits](https://www.conventionalcommits.org/):

```
feat: add fp_foldr_i64 (right fold) function
fix: correct stack alignment in fp_map_abs_i64
docs: update README with AVX-512 requirements
perf: optimize fp_filter by 15% using prefetching
test: add edge cases for fp_quartiles_f64
```

## 🧪 Testing Requirements

### All contributions must include tests:

1. **Correctness tests** - Verify output matches expected results
2. **Edge case tests** - Empty arrays, single elements, zero values
3. **Purity tests** - Verify input immutability
4. **Performance tests** (for optimized functions) - Document speedup vs naive C

### Running tests:

```bash
# General HOF tests
build_test_general_hof.bat

# Specific module tests
build_test_outliers.bat
build/scripts/test_correlation_refactoring.bat

# Performance benchmarks
build_bench_general_hof.bat
```

## 🏗️ Project Structure

```
fp-asm/
├── include/
│   └── fp_core.h           # Public API declarations
├── src/
│   ├── asm/                # Hand-optimized assembly modules
│   │   ├── fp_core_reductions.asm
│   │   ├── fp_core_fused_folds.asm
│   │   └── ... (15 total modules)
│   └── wrappers/           # C wrapper functions
│       ├── fp_general_hof.c
│       ├── fp_percentile_wrappers.c
│       └── ... (composition-based implementations)
├── build/
│   ├── obj/                # Compiled object files
│   └── scripts/            # Build and test scripts
├── docs/                   # Comprehensive documentation
├── examples/               # Usage examples
└── tests/                  # Test suites
```

## 🎯 Priority Areas for Contribution

### High Priority
1. **Linux port** - System V AMD64 ABI conversion
2. **AVX-512 implementations** - Utilizing 512-bit vectors
3. **i32/f32 variants** - 32-bit integer and float support
4. **Additional statistical functions** - Median, mode, percentile interpolation methods
5. **Performance benchmarks** - Testing on different CPU architectures

### Medium Priority
6. **foldr** (right fold) - Complete fold functionality
7. **zip/unzip** - Tuple creation/destruction
8. **find** with arbitrary predicate - General search
9. **Additional moving averages** - MACD, Bollinger Bands
10. **Matrix operations** - Basic linear algebra

### Documentation
11. **Tutorial series** - Beginner to advanced
12. **Porting guide** - Haskell → FP-ASM translation
13. **Performance analysis** - Detailed profiling reports
14. **Video tutorials** - YouTube walkthroughs

## 🏛️ Architecture Guidelines

### When to use Assembly vs C

**Use Assembly for:**
- ✅ Hot path operations (called millions of times)
- ✅ SIMD-friendly algorithms (map, reduce, scan)
- ✅ Operations with known optimal instruction sequences
- ✅ Specialized functions for common use cases

**Use C for:**
- ✅ General higher-order functions (function pointer indirection)
- ✅ Complex control flow
- ✅ Wrapper functions (composition-based implementations)
- ✅ Memory management and edge case handling

### Windows x64 ABI Compliance

**CRITICAL**: All assembly must follow Windows x64 calling convention:
- Arguments: RCX, RDX, R8, R9, then stack
- Preserve: RBX, RBP, RDI, RSI, R12-R15, XMM6-XMM15
- Volatile: RAX, RCX, RDX, R8-R11, XMM0-XMM5
- Shadow space: 32 bytes allocated by caller
- Stack alignment: 16-byte aligned

See [`docs/CLAUDE.md`](docs/CLAUDE.md) for detailed ABI requirements.

### Functional Purity Contract

**Every function MUST guarantee:**
1. **Input immutability** - All inputs marked `const`, never modified
2. **No side effects** - No global state, no I/O
3. **Deterministic** - Same inputs → same outputs, always
4. **Const-correctness** - Compiler-enforced immutability

## 🔍 Code Review Process

### Pull requests will be reviewed for:

1. **Correctness** - Does it work as intended?
2. **Performance** - Benchmark results vs existing implementations
3. **Purity** - Maintains functional guarantees?
4. **ABI compliance** - Follows Windows x64 calling convention?
5. **Test coverage** - Includes comprehensive tests?
6. **Documentation** - API docs, comments, examples?
7. **Code style** - Follows project conventions?

### Review timeline:
- Initial feedback: 1-3 days
- Full review: 1 week
- Merge decision: 2 weeks maximum

## 📚 Resources

### Learning Materials
- [Windows x64 ABI Documentation](https://docs.microsoft.com/en-us/cpp/build/x64-calling-convention)
- [Intel Intrinsics Guide](https://www.intel.com/content/www/us/en/docs/intrinsics-guide/index.html)
- [NASM Documentation](https://www.nasm.us/xdoc/2.15.05/html/nasmdoc0.html)
- [Haskell Prelude](https://hackage.haskell.org/package/base-4.16.0.0/docs/Prelude.html)

### Project Documentation
- [CLAUDE.md](docs/CLAUDE.md) - Technical architecture
- [FP_LANGUAGE_EQUIVALENCE.md](docs/FP_LANGUAGE_EQUIVALENCE.md) - Haskell/Lisp/ML comparison
- [REFACTORING_SUMMARY.md](docs/REFACTORING_SUMMARY.md) - Composition patterns

## 💬 Communication

- **GitHub Issues** - Bug reports and feature requests
- **GitHub Discussions** - General questions and ideas
- **Pull Requests** - Code contributions
- **Email** - For sensitive security issues only

## 🙏 Recognition

Contributors will be:
- Listed in README.md acknowledgments
- Credited in release notes
- Mentioned in commit history
- Thanked profusely!

## 📜 License

By contributing, you agree that your contributions will be licensed under the MIT License.

---

<div align="center">

**Thank you for contributing to FP-ASM!**

Together we're proving that functional programming and systems performance are not mutually exclusive.

</div>
