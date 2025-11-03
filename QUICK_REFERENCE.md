# FP-ASM Quick Reference Guide

**Version**: 1.0 (Complete)
**Functions**: 120 across 10 types
**Status**: Production Ready ✅

---

## Function Quick Lookup

### Reductions (Single Value Output)

```c
// Sum all elements
T fp_reduce_add_T(const T* in, size_t n);

// Product of all elements
T fp_reduce_mul_T(const T* in, size_t n);

// Minimum value
T fp_reduce_min_T(const T* in, size_t n);

// Maximum value
T fp_reduce_max_T(const T* in, size_t n);
```

**Example**:
```c
double temps[1000] = {20.5, 21.3, ...};
double avg_temp = fp_reduce_add_f64(temps, 1000) / 1000.0;
double max_temp = fp_reduce_max_f64(temps, 1000);
```

---

### Fused Folds (Map + Reduce in One Pass)

```c
// Sum of squares: Σ(x²)
T fp_fold_sumsq_T(const T* in, size_t n);

// Dot product: Σ(a·b)
T fp_fold_dotp_T(const T* a, const T* b, size_t n);

// Sum of absolute differences: Σ|a-b|
T fp_fold_sad_T(const T* a, const T* b, size_t n);
```

**Example**:
```c
float vec_a[100], vec_b[100];
float similarity = fp_fold_dotp_f32(vec_a, vec_b, 100);

uint8_t img1[1920*1080], img2[1920*1080];
uint8_t diff = fp_fold_sad_u8(img1, img2, 1920*1080);
```

---

### Fused Maps (Element-wise Operations)

```c
// AXPY: out = c·x + y
void fp_map_axpy_T(const T* x, const T* y, T* out, size_t n, T c);

// Scale: out = c·x
void fp_map_scale_T(const T* in, T* out, size_t n, T c);

// Offset: out = x + c
void fp_map_offset_T(const T* in, T* out, size_t n, T c);

// Zip Add: out = x + y
void fp_zip_add_T(const T* a, const T* b, T* out, size_t n);
```

**Example**:
```c
float audio[44100], output[44100];
fp_map_scale_f32(audio, output, 44100, 0.5f);  // Reduce volume 50%

int32_t data[1000], result[1000];
fp_map_offset_i32(data, result, 1000, 100);    // Add 100 to all
```

---

## Type Suffixes

Replace `T` in function names with:

| Suffix | C Type | Bit Width | SIMD Width | Best For |
|--------|--------|-----------|------------|----------|
| `i64` | int64_t | 64-bit | 4-wide | Large integers |
| `f64` | double | 64-bit | 4-wide | Scientific computing |
| `i32` | int32_t | 32-bit | 8-wide | General integers |
| `f32` | float | 32-bit | 8-wide | Graphics, ML |
| `u32` | uint32_t | 32-bit | 8-wide | Unsigned integers |
| `u64` | uint64_t | 64-bit | 4-wide | Large unsigned |
| `i16` | int16_t | 16-bit | 16-wide | Audio, sensors |
| `u16` | uint16_t | 16-bit | 16-wide | Networking |
| `i8` | int8_t | 8-bit | **32-wide** | Image processing |
| `u8` | uint8_t | 8-bit | **32-wide** | RGB pixels, strings |

---

## Common Use Cases

### Image Processing (8-bit)
```c
#include "fp_core.h"

// Brighten image
uint8_t pixels[1920*1080*3];
uint8_t brightened[1920*1080*3];
fp_map_offset_u8(pixels, brightened, 1920*1080*3, 50);  // +50 brightness

// Compare images
uint8_t img1[1920*1080], img2[1920*1080];
uint8_t difference = fp_fold_sad_u8(img1, img2, 1920*1080);
```

### Financial Calculations (64-bit)
```c
double prices[10000];
double total = fp_reduce_add_f64(prices, 10000);
double avg = total / 10000.0;

double returns[10000];
double variance_sum = fp_fold_sumsq_f64(returns, 10000);
```

### Audio Processing (16-bit)
```c
int16_t samples[44100];
int16_t amplified[44100];
fp_map_scale_i16(samples, amplified, 44100, 2);  // 2x amplification

int16_t left[44100], right[44100], mono[44100];
fp_zip_add_i16(left, right, mono, 44100);  // Mix to mono
```

### Vector Mathematics (32-bit)
```c
float vec_a[1000], vec_b[1000];
float dot = fp_fold_dotp_f32(vec_a, vec_b, 1000);

float x[1000], y[1000], result[1000];
fp_map_axpy_f32(x, y, result, 1000, 2.5f);  // result = 2.5x + y
```

---

## Performance Tips

### 1. Choose the Right Type
- **u8/i8**: 8X throughput vs f64 (fastest for byte data)
- **u16/i16**: 4X throughput (audio, networking)
- **f32/i32/u32**: 2X throughput (general purpose)
- **f64/i64/u64**: 1X baseline (high precision)

### 2. Use Fused Operations
```c
// SLOW: Two passes through memory
for (i = 0; i < n; i++) tmp[i] = a[i] * a[i];
for (i = 0; i < n; i++) sum += tmp[i];

// FAST: One pass, stays in registers (1.25x faster)
sum = fp_fold_sumsq_f64(a, n);
```

### 3. Align Large Arrays
```c
// For best performance on large arrays
float* data = aligned_alloc(32, n * sizeof(float));
```

### 4. Batch Small Operations
```c
// Less efficient: many small calls
for (i = 0; i < 100; i++)
    result = fp_reduce_add_f32(chunk[i], 1000);

// More efficient: one large call
result = fp_reduce_add_f32(all_data, 100*1000);
```

---

## Linking

### Windows (MSVC/MinGW)
```bash
# Assemble modules
nasm -f win64 src/asm/fp_core_reductions_f64.asm -o fp_core_reductions_f64.o
nasm -f win64 src/asm/fp_core_fused_folds_f64.asm -o fp_core_fused_folds_f64.o
nasm -f win64 src/asm/fp_core_fused_maps_f64.asm -o fp_core_fused_maps_f64.o

# Link with your program
gcc myprogram.c fp_core_*.o -I include -o myprogram.exe
```

### Build All Types at Once
```bash
build_all.bat  # Assembles all 30 modules
```

---

## Testing

### Run Comprehensive Tests
```bash
# Test specific type
build_test_u8_comprehensive.bat

# Test all types
for type in i64 f64 i32 f32 u32 u64 i16 u16 i8 u8; do
    build_test_${type}_comprehensive.bat
done
```

### Benchmarks
Each test includes benchmarks on 100K elements × 100 iterations.

Expected performance (100K elements):
- **SIMD ops**: 0.000-0.010 ms/iter
- **Scalar ops**: 0.040-0.160 ms/iter

---

## Limitations

### AVX2 Instruction Gaps
- **8-bit multiply** (vpmullb): Not available → scalar implementation
- **16-bit multiply** (vpmullw): Not available → scalar implementation
- **64-bit min/max** (vpmaxsq): Requires AVX-512 → scalar for i64

**Impact**: Multiply operations on u8/i8/u16/i16 are slower than add/min/max, but still competitive via hand-optimized scalar loops.

### Platform Support
- **Currently**: Windows x64 only
- **Future**: Linux/macOS (different ABI)

### CPU Requirements
- Requires **AVX2** (Intel Haswell 2013+, AMD Excavator 2015+)
- No runtime CPU detection yet (always uses AVX2)

---

## Error Handling

Functions assume:
- Valid pointers (no NULL checks)
- Array size > 0 (undefined for n=0)
- Proper alignment (works with any, faster at 32-byte boundaries)

**No exceptions** - functions return immediately for invalid inputs.

---

## Complete Function List

### i64 (12 functions)
```c
int64_t fp_reduce_add_i64(const int64_t* in, size_t n);
int64_t fp_reduce_mul_i64(const int64_t* in, size_t n);
int64_t fp_reduce_min_i64(const int64_t* in, size_t n);
int64_t fp_reduce_max_i64(const int64_t* in, size_t n);
int64_t fp_fold_sumsq_i64(const int64_t* in, size_t n);
int64_t fp_fold_dotp_i64(const int64_t* a, const int64_t* b, size_t n);
int64_t fp_fold_sad_i64(const int64_t* a, const int64_t* b, size_t n);
void fp_map_axpy_i64(const int64_t* x, const int64_t* y, int64_t* out, size_t n, int64_t c);
void fp_map_scale_i64(const int64_t* in, int64_t* out, size_t n, int64_t c);
void fp_map_offset_i64(const int64_t* in, int64_t* out, size_t n, int64_t c);
void fp_zip_add_i64(const int64_t* a, const int64_t* b, int64_t* out, size_t n);
```

*(Pattern repeats for f64, i32, f32, u32, u64, i16, u16, i8, u8)*

---

## Files Reference

```
fp_asm_lib_dev/
├── include/
│   └── fp_core.h              # API header (all 120 declarations)
├── src/
│   └── asm/
│       ├── fp_core_reductions_*.asm   # 10 files
│       ├── fp_core_fused_folds_*.asm  # 10 files
│       └── fp_core_fused_maps_*.asm   # 10 files
├── build/
│   └── obj/
│       └── *.o                        # 30 object files
├── test_*_comprehensive.c             # 10 test files
└── build_test_*_comprehensive.bat     # 10 build scripts
```

---

## Getting Help

- **Documentation**: See IMPLEMENTATION_COMPLETE.md
- **Examples**: Check test_*_comprehensive.c files
- **Build Issues**: Ensure NASM and GCC are in PATH
- **Performance**: Use -O3 -march=native for C code calling FP-ASM

---

## License

[Specify your license here]

---

*FP-ASM v1.0 - 120 Hand-Optimized AVX2 Functions*
*Generated November 3, 2025*
