# FP-ASM Library: Complete API Reference

**Version**: 1.0.0 (COMPLETE)
**Date**: October 28, 2025
**Operations**: 36 functions across 10 modules
**Target**: Windows x64, AVX2-capable CPUs

---

## Table of Contents

1. [Module 1: Simple Folds (Reductions)](#module-1-simple-folds-reductions)
2. [Module 2: Fused Folds (Map-Reduce)](#module-2-fused-folds-map-reduce)
3. [Module 3: Fused Maps (BLAS Level 1)](#module-3-fused-maps-blas-level-1)
4. [Module 4: Simple Maps (Transformers)](#module-4-simple-maps-transformers)
5. [Module 5: Scans (Prefix Operations)](#module-5-scans-prefix-operations)
6. [Module 6: Predicates (Boolean Tests)](#module-6-predicates-boolean-tests)
7. [Module 7: Compaction (Filter/Partition)](#module-7-compaction-filterpartition)
8. [Module 8: Essentials (List Operations)](#module-8-essentials-list-operations)
9. [Module 9: Sorting & Sets (TIER 2)](#module-9-sorting--sets-tier-2)
10. [Module 10: Advanced Operations (TIER 3)](#module-10-advanced-operations-tier-3)

---

# Module 1: Simple Folds (Reductions)

Basic reduction operations that collapse arrays into single values.

---

## fp_reduce_add_i64

**Signature**:
```c
int64_t fp_reduce_add_i64(const int64_t* array, size_t n);
```

**Description**:
Computes the sum of all elements in an integer array. Implements the fold/reduce pattern with addition as the combining function.

**Parameters**:
- `array` *(const int64_t\*)* - Input array of signed 64-bit integers
- `n` *(size_t)* - Number of elements in the array

**Returns**:
*(int64_t)* - Sum of all array elements. Returns 0 for empty arrays.

**Algorithm**:
Uses scalar implementation with multiple accumulators and loop unrolling to exploit instruction-level parallelism. Processes 4 elements per iteration with independent accumulators to avoid false dependencies.

**Complexity**:
- Time: O(n)
- Space: O(1)

**Performance**:
~1.0-1.02x vs GCC -O3 (scalar optimization wins due to lack of AVX2 64-bit integer multiply)

**Haskell Equivalent**:
```haskell
foldl (+) 0 list
-- or
sum list
```

**Example**:
```c
int64_t data[] = {1, 2, 3, 4, 5};
int64_t result = fp_reduce_add_i64(data, 5);
// result = 15
```

**See Also**:
- `fp_reduce_add_f64` - Double-precision version
- `fp_fold_sumsq_i64` - Sum of squares

---

## fp_reduce_add_f64

**Signature**:
```c
double fp_reduce_add_f64(const double* array, size_t n);
```

**Description**:
Computes the sum of all elements in a double-precision floating-point array using AVX2 SIMD acceleration.

**Parameters**:
- `array` *(const double\*)* - Input array of double-precision floats
- `n` *(size_t)* - Number of elements in the array

**Returns**:
*(double)* - Sum of all array elements. Returns 0.0 for empty arrays.

**Algorithm**:
SIMD vectorized reduction processing 4 doubles per iteration using YMM registers. Uses horizontal addition for final reduction. Handles tail elements with scalar loop.

**Complexity**:
- Time: O(n)
- Space: O(1)

**Performance**:
~1.5-1.8x vs GCC -O3 (GCC fails to vectorize consistently)

**Floating-Point Notes**:
- Summation order differs from scalar (parallel reduction), may cause small differences (~1e-12 relative error)
- Not compensated summation (no Kahan algorithm)
- Handles NaN/Inf according to IEEE 754

**Example**:
```c
double data[] = {1.5, 2.7, 3.2, 4.1, 5.9};
double result = fp_reduce_add_f64(data, 5);
// result ≈ 17.4
```

**See Also**:
- `fp_reduce_add_i64` - Integer version
- `fp_fold_dotp_f64` - Dot product (sum of products)

---

## fp_reduce_max_i64

**Signature**:
```c
int64_t fp_reduce_max_i64(const int64_t* array, size_t n);
```

**Description**:
Finds the maximum value in an integer array.

**Parameters**:
- `array` *(const int64_t\*)* - Input array of signed 64-bit integers
- `n` *(size_t)* - Number of elements in the array

**Returns**:
*(int64_t)* - Maximum value found. Returns INT64_MIN for empty arrays.

**Algorithm**:
Scalar implementation with multiple accumulators. Processes 4 elements per iteration. AVX2 lacks 64-bit integer min/max instructions (requires AVX-512).

**Complexity**:
- Time: O(n)
- Space: O(1)

**Performance**:
~1.0x vs GCC -O3 (both use scalar implementation)

**Example**:
```c
int64_t data[] = {3, 7, 2, 9, 4};
int64_t result = fp_reduce_max_i64(data, 5);
// result = 9
```

**See Also**:
- `fp_reduce_max_f64` - Double version with SIMD

---

## fp_reduce_max_f64

**Signature**:
```c
double fp_reduce_max_f64(const double* array, size_t n);
```

**Description**:
Finds the maximum value in a double-precision array using SIMD acceleration.

**Parameters**:
- `array` *(const double\*)* - Input array of double-precision floats
- `n` *(size_t)* - Number of elements in the array

**Returns**:
*(double)* - Maximum value found. Returns -INFINITY for empty arrays.

**Algorithm**:
SIMD vectorized using `vmaxpd` instruction. Processes 4 doubles per iteration. Horizontal maximum extraction for final result.

**Complexity**:
- Time: O(n)
- Space: O(1)

**Performance**:
~1.5x vs GCC -O3

**Floating-Point Notes**:
- NaN propagation follows IEEE 754
- Handles +/-INFINITY correctly
- -0.0 and +0.0 are treated as equal

**Example**:
```c
double data[] = {3.5, 7.2, 2.1, 9.8, 4.3};
double result = fp_reduce_max_f64(data, 5);
// result = 9.8
```

**See Also**:
- `fp_reduce_max_i64` - Integer version

---

# Module 2: Fused Folds (Map-Reduce)

Combined map-then-reduce operations that eliminate intermediate arrays.

---

## fp_fold_sumsq_i64

**Signature**:
```c
int64_t fp_fold_sumsq_i64(const int64_t* array, size_t n);
```

**Description**:
Computes the sum of squares: Σ(xᵢ²). Fuses the map (square) and reduce (sum) operations into a single pass.

**Parameters**:
- `array` *(const int64_t\*)* - Input array of signed 64-bit integers
- `n` *(size_t)* - Number of elements in the array

**Returns**:
*(int64_t)* - Sum of squared elements. Returns 0 for empty arrays.

**Algorithm**:
Scalar implementation with multiple accumulators. Uses `imul` for squaring. Processes 4 elements per iteration with independent accumulators.

**Complexity**:
- Time: O(n)
- Space: O(1) - no intermediate array

**Performance**:
~1.1x vs separate map + reduce (eliminates memory traffic)

**Overflow Warning**:
Can overflow for large values. Consider using `fp_fold_sumsq_f64` for large datasets.

**Haskell Equivalent**:
```haskell
foldl (\acc x -> acc + x*x) 0 list
-- or
sum $ map (\x -> x*x) list
```

**Example**:
```c
int64_t data[] = {1, 2, 3, 4, 5};
int64_t result = fp_fold_sumsq_i64(data, 5);
// result = 1 + 4 + 9 + 16 + 25 = 55
```

**Applications**:
- Variance calculation: variance = (sumsq / n) - mean²
- Euclidean norm: ||v|| = √(sumsq)
- Least squares fitting

**See Also**:
- `fp_fold_dotp_i64` - Dot product (sum of element products)

---

## fp_fold_dotp_i64

**Signature**:
```c
int64_t fp_fold_dotp_i64(const int64_t* array_a, const int64_t* array_b, size_t n);
```

**Description**:
Computes the dot product (inner product) of two integer vectors: Σ(aᵢ × bᵢ). Fuses element-wise multiplication with summation.

**Parameters**:
- `array_a` *(const int64_t\*)* - First input array
- `array_b` *(const int64_t\*)* - Second input array
- `n` *(size_t)* - Number of elements (must be same for both arrays)

**Returns**:
*(int64_t)* - Dot product. Returns 0 for empty arrays.

**Algorithm**:
Scalar with multiple accumulators. Processes 4 pairs per iteration. No AVX2 64-bit multiply instruction available.

**Complexity**:
- Time: O(n)
- Space: O(1)

**Performance**:
~1.1x vs separate operations

**Haskell Equivalent**:
```haskell
sum $ zipWith (*) list_a list_b
```

**Example**:
```c
int64_t a[] = {1, 2, 3, 4};
int64_t b[] = {5, 6, 7, 8};
int64_t result = fp_fold_dotp_i64(a, b, 4);
// result = 1×5 + 2×6 + 3×7 + 4×8 = 5 + 12 + 21 + 32 = 70
```

**Applications**:
- Vector similarity (cosine similarity)
- Linear algebra operations
- Machine learning (feature dot product)
- Signal correlation

**See Also**:
- `fp_fold_dotp_f64` - Double version with FMA

---

## fp_fold_dotp_f64

**Signature**:
```c
double fp_fold_dotp_f64(const double* array_a, const double* array_b, size_t n);
```

**Description**:
Computes the dot product of two double-precision vectors using AVX2 SIMD with FMA (Fused Multiply-Add) instructions.

**Parameters**:
- `array_a` *(const double\*)* - First input array
- `array_b` *(const double\*)* - Second input array
- `n` *(size_t)* - Number of elements in each array

**Returns**:
*(double)* - Dot product. Returns 0.0 for empty arrays.

**Algorithm**:
SIMD vectorized using `vfmadd231pd` (multiply-add). Processes 4 pairs per iteration. Horizontal addition for final reduction.

**Complexity**:
- Time: O(n)
- Space: O(1)

**Performance**:
~1.25x vs GCC -O3 (FMA provides 2× throughput)

**Floating-Point Notes**:
- FMA provides better accuracy than separate multiply-add
- Rounding only occurs once per FMA operation
- Order differs from scalar summation

**Example**:
```c
double a[] = {1.0, 2.0, 3.0, 4.0};
double b[] = {0.5, 1.5, 2.5, 3.5};
double result = fp_fold_dotp_f64(a, b, 4);
// result = 1.0×0.5 + 2.0×1.5 + 3.0×2.5 + 4.0×3.5 = 25.0
```

**Applications**:
- BLAS Level 1 operation (ddot)
- Neural network forward pass
- Physics simulations (work = F·d)

**See Also**:
- `fp_fold_dotp_i64` - Integer version
- `fp_map_axpy_f64` - Related BLAS operation (y = αx + y)

---

## fp_fold_sad_i64

**Signature**:
```c
int64_t fp_fold_sad_i64(const int64_t* array_a, const int64_t* array_b, size_t n);
```

**Description**:
Computes Sum of Absolute Differences: Σ|aᵢ - bᵢ|. Fuses subtraction, absolute value, and summation.

**Parameters**:
- `array_a` *(const int64_t\*)* - First input array
- `array_b` *(const int64_t\*)* - Second input array
- `n` *(size_t)* - Number of elements in each array

**Returns**:
*(int64_t)* - Sum of absolute differences. Returns 0 for empty arrays.

**Algorithm**:
Scalar with multiple accumulators. Uses conditional move (`cmov`) for efficient absolute value. Processes 4 pairs per iteration.

**Complexity**:
- Time: O(n)
- Space: O(1)

**Performance**:
~1.03x vs GCC -O3

**Haskell Equivalent**:
```haskell
sum $ zipWith (\a b -> abs (a - b)) list_a list_b
```

**Example**:
```c
int64_t a[] = {10, 20, 30, 40};
int64_t b[] = {15, 18, 35, 38};
int64_t result = fp_fold_sad_i64(a, b, 4);
// result = |10-15| + |20-18| + |30-35| + |40-38|
//        = 5 + 2 + 5 + 2 = 14
```

**Applications**:
- Image/video compression (motion estimation)
- Template matching
- Similarity metrics (L1 distance)
- Anomaly detection

**See Also**:
- `fp_map_abs_i64` - Absolute value operation

---

# Module 3: Fused Maps (BLAS Level 1)

Element-wise operations combining scalar multiplication, addition, and array operations.

---

## fp_map_axpy_i64

**Signature**:
```c
void fp_map_axpy_i64(int64_t* y, const int64_t* x, int64_t alpha, size_t n);
```

**Description**:
BLAS AXPY operation: **y = α·x + y**. Scales vector `x` by scalar `alpha` and adds to vector `y` in-place.

**Parameters**:
- `y` *(int64_t\*)* - Input/output array (modified in-place)
- `x` *(const int64_t\*)* - Input array (read-only)
- `alpha` *(int64_t)* - Scalar multiplier
- `n` *(size_t)* - Number of elements in each array

**Returns**:
*(void)* - Result stored in `y` array

**Algorithm**:
Scalar with multiple accumulators and loop unrolling. Processes 4 elements per iteration.

**Complexity**:
- Time: O(n)
- Space: O(1)

**Performance**:
~1.0-1.1x vs GCC -O3 (memory-bound operation)

**In-Place Modification**:
The `y` array is modified directly. Pass a copy if original must be preserved.

**Haskell Equivalent**:
```haskell
zipWith (\yi xi -> alpha * xi + yi) y x
```

**Example**:
```c
int64_t y[] = {1, 2, 3, 4};
int64_t x[] = {10, 20, 30, 40};
fp_map_axpy_i64(y, x, 2, 4);
// y = [1 + 2×10, 2 + 2×20, 3 + 2×30, 4 + 2×40]
//   = [21, 42, 63, 84]
```

**Applications**:
- Linear algebra (vector addition with scaling)
- Gradient descent updates: weights += learning_rate × gradient
- Physics simulations: position += velocity × dt

**See Also**:
- `fp_map_axpy_f64` - Double version with FMA
- `fp_map_scale_i64` - Scalar multiplication only

---

## fp_map_axpy_f64

**Signature**:
```c
void fp_map_axpy_f64(double* y, const double* x, double alpha, size_t n);
```

**Description**:
BLAS AXPY for double-precision: **y = α·x + y**. Uses AVX2 SIMD with FMA instructions.

**Parameters**:
- `y` *(double\*)* - Input/output array (modified in-place)
- `x` *(const double\*)* - Input array (read-only)
- `alpha` *(double)* - Scalar multiplier
- `n` *(size_t)* - Number of elements in each array

**Returns**:
*(void)* - Result stored in `y` array

**Algorithm**:
SIMD vectorized using `vfmadd231pd`. Broadcasts `alpha` to YMM register. Processes 4 doubles per iteration.

**Complexity**:
- Time: O(n)
- Space: O(1)

**Performance**:
~1.0-1.1x vs GCC -O3 (memory bandwidth saturated)

**Floating-Point Notes**:
- Uses FMA for better accuracy
- Results may differ slightly from scalar due to operation order

**Example**:
```c
double y[] = {1.0, 2.0, 3.0, 4.0};
double x[] = {0.5, 1.0, 1.5, 2.0};
fp_map_axpy_f64(y, x, 2.5, 4);
// y = [1.0 + 2.5×0.5, 2.0 + 2.5×1.0, 3.0 + 2.5×1.5, 4.0 + 2.5×2.0]
//   = [2.25, 4.5, 6.75, 9.0]
```

**Applications**:
- BLAS Level 1 operation
- Neural network weight updates
- ODE solvers (Runge-Kutta methods)

**See Also**:
- `fp_map_axpy_i64` - Integer version
- `fp_fold_dotp_f64` - Dot product

---

## fp_map_scale_i64

**Signature**:
```c
void fp_map_scale_i64(int64_t* output, const int64_t* input, int64_t scalar, size_t n);
```

**Description**:
Scalar multiplication: **output[i] = input[i] × scalar**. Scales each element by a constant.

**Parameters**:
- `output` *(int64_t\*)* - Output array (can be same as input for in-place)
- `input` *(const int64_t\*)* - Input array
- `scalar` *(int64_t)* - Multiplier
- `n` *(size_t)* - Number of elements

**Returns**:
*(void)* - Result stored in `output` array

**Algorithm**:
Scalar implementation with loop unrolling. Processes 4 elements per iteration.

**Complexity**:
- Time: O(n)
- Space: O(1)

**Performance**:
~1.0-1.1x vs GCC -O3

**In-Place Allowed**:
Can pass same pointer for `input` and `output`.

**Haskell Equivalent**:
```haskell
map (* scalar) list
```

**Example**:
```c
int64_t input[] = {1, 2, 3, 4, 5};
int64_t output[5];
fp_map_scale_i64(output, input, 10, 5);
// output = [10, 20, 30, 40, 50]
```

**Applications**:
- Unit conversion (e.g., meters to millimeters)
- Scaling in graphics/physics
- Amplification in signal processing

**See Also**:
- `fp_map_scale_f64` - Double version
- `fp_map_offset_i64` - Scalar addition

---

## fp_map_scale_f64

**Signature**:
```c
void fp_map_scale_f64(double* output, const double* input, double scalar, size_t n);
```

**Description**:
Scalar multiplication for doubles using SIMD: **output[i] = input[i] × scalar**.

**Parameters**:
- `output` *(double\*)* - Output array (can be same as input)
- `input` *(const double\*)* - Input array
- `scalar` *(double)* - Multiplier
- `n` *(size_t)* - Number of elements

**Returns**:
*(void)* - Result stored in `output` array

**Algorithm**:
SIMD with `vmulpd`. Broadcasts scalar to YMM register. Processes 4 doubles per iteration.

**Complexity**:
- Time: O(n)
- Space: O(1)

**Performance**:
~1.0-1.1x vs GCC -O3 (memory-bound)

**Example**:
```c
double input[] = {1.5, 2.5, 3.5, 4.5};
double output[4];
fp_map_scale_f64(output, input, 0.5, 4);
// output = [0.75, 1.25, 1.75, 2.25]
```

**Applications**:
- Normalization (divide by max: multiply by 1/max)
- Percentage calculation
- BLAS SCAL operation

**See Also**:
- `fp_map_scale_i64` - Integer version

---

## fp_map_offset_i64

**Signature**:
```c
void fp_map_offset_i64(int64_t* output, const int64_t* input, int64_t offset, size_t n);
```

**Description**:
Scalar addition: **output[i] = input[i] + offset**. Adds constant to each element.

**Parameters**:
- `output` *(int64_t\*)* - Output array (can be same as input)
- `input` *(const int64_t\*)* - Input array
- `offset` *(int64_t)* - Value to add to each element
- `n` *(size_t)* - Number of elements

**Returns**:
*(void)* - Result stored in `output` array

**Algorithm**:
Scalar with loop unrolling. Processes 4 elements per iteration.

**Complexity**:
- Time: O(n)
- Space: O(1)

**Performance**:
~1.0-1.1x vs GCC -O3

**Haskell Equivalent**:
```haskell
map (+ offset) list
```

**Example**:
```c
int64_t input[] = {10, 20, 30, 40};
int64_t output[4];
fp_map_offset_i64(output, input, 5, 4);
// output = [15, 25, 35, 45]
```

**Applications**:
- Bias addition in neural networks
- Temperature conversion (Celsius to Kelvin: +273.15)
- Time shifting

**See Also**:
- `fp_map_offset_f64` - Double version

---

## fp_map_offset_f64

**Signature**:
```c
void fp_map_offset_f64(double* output, const double* input, double offset, size_t n);
```

**Description**:
Scalar addition for doubles using SIMD: **output[i] = input[i] + offset**.

**Parameters**:
- `output` *(double\*)* - Output array (can be same as input)
- `input` *(const double\*)* - Input array
- `offset` *(double)* - Value to add
- `n` *(size_t)* - Number of elements

**Returns**:
*(void)* - Result stored in `output` array

**Algorithm**:
SIMD with `vaddpd`. Broadcasts offset to YMM register. Processes 4 doubles per iteration.

**Complexity**:
- Time: O(n)
- Space: O(1)

**Performance**:
~1.0-1.1x vs GCC -O3

**Example**:
```c
double input[] = {1.0, 2.0, 3.0, 4.0};
double output[4];
fp_map_offset_f64(output, input, 0.5, 4);
// output = [1.5, 2.5, 3.5, 4.5]
```

**Applications**:
- Mean centering: x - mean
- DC offset removal
- Coordinate transformation

**See Also**:
- `fp_map_offset_i64` - Integer version

---

## fp_zip_add_i64

**Signature**:
```c
void fp_zip_add_i64(int64_t* output, const int64_t* array_a, const int64_t* array_b, size_t n);
```

**Description**:
Element-wise addition: **output[i] = a[i] + b[i]**. Adds corresponding elements from two arrays.

**Parameters**:
- `output` *(int64_t\*)* - Output array (can alias one input for in-place)
- `array_a` *(const int64_t\*)* - First input array
- `array_b` *(const int64_t\*)* - Second input array
- `n` *(size_t)* - Number of elements in each array

**Returns**:
*(void)* - Result stored in `output` array

**Algorithm**:
Scalar with loop unrolling. Processes 4 pairs per iteration.

**Complexity**:
- Time: O(n)
- Space: O(1)

**Performance**:
~1.0-1.1x vs GCC -O3

**Haskell Equivalent**:
```haskell
zipWith (+) list_a list_b
```

**Example**:
```c
int64_t a[] = {1, 2, 3, 4};
int64_t b[] = {10, 20, 30, 40};
int64_t output[4];
fp_zip_add_i64(output, a, b, 4);
// output = [11, 22, 33, 44]
```

**Applications**:
- Vector addition
- Combining signals
- Accumulating increments

**See Also**:
- `fp_zip_add_f64` - Double version

---

## fp_zip_add_f64

**Signature**:
```c
void fp_zip_add_f64(double* output, const double* array_a, const double* array_b, size_t n);
```

**Description**:
Element-wise addition for doubles using SIMD: **output[i] = a[i] + b[i]**.

**Parameters**:
- `output` *(double\*)* - Output array
- `array_a` *(const double\*)* - First input array
- `array_b` *(const double\*)* - Second input array
- `n` *(size_t)* - Number of elements

**Returns**:
*(void)* - Result stored in `output` array

**Algorithm**:
SIMD with `vaddpd`. Processes 4 doubles per iteration.

**Complexity**:
- Time: O(n)
- Space: O(1)

**Performance**:
~1.0-1.1x vs GCC -O3 (memory-bound)

**Example**:
```c
double a[] = {1.5, 2.5, 3.5, 4.5};
double b[] = {0.5, 1.0, 1.5, 2.0};
double output[4];
fp_zip_add_f64(output, a, b, 4);
// output = [2.0, 3.5, 5.0, 6.5]
```

**Applications**:
- Vector addition in physics
- Combining feature vectors
- Financial calculations (total = revenue + adjustment)

**See Also**:
- `fp_zip_add_i64` - Integer version

---

# Module 4: Simple Maps (Transformers)

Element-wise transformation operations.

---

## fp_map_abs_i64

**Signature**:
```c
void fp_map_abs_i64(int64_t* output, const int64_t* input, size_t n);
```

**Description**:
Absolute value: **output[i] = |input[i]|**. Computes absolute value of each element.

**Parameters**:
- `output` *(int64_t\*)* - Output array (can be same as input)
- `input` *(const int64_t\*)* - Input array
- `n` *(size_t)* - Number of elements

**Returns**:
*(void)* - Result stored in `output` array

**Algorithm**:
Scalar using conditional move (`cmov`) for branchless absolute value. Processes 4 elements per iteration. AVX2 lacks `vpabsq` (requires AVX-512).

**Complexity**:
- Time: O(n)
- Space: O(1)

**Performance**:
~1.0-1.1x vs GCC -O3

**Special Case**:
INT64_MIN has no positive representation, result is INT64_MIN (undefined behavior in signed overflow).

**Haskell Equivalent**:
```haskell
map abs list
```

**Example**:
```c
int64_t input[] = {-5, 3, -8, 0, 7};
int64_t output[5];
fp_map_abs_i64(output, input, 5);
// output = [5, 3, 8, 0, 7]
```

**Applications**:
- Distance calculations (unsigned magnitude)
- Error metrics
- Rectification in signal processing

**See Also**:
- `fp_map_abs_f64` - Double version with SIMD
- `fp_fold_sad_i64` - Sum of absolute differences

---

## fp_map_abs_f64

**Signature**:
```c
void fp_map_abs_f64(double* output, const double* input, size_t n);
```

**Description**:
Absolute value for doubles using SIMD: **output[i] = |input[i]|**.

**Parameters**:
- `output` *(double\*)* - Output array
- `input` *(const double\*)* - Input array
- `n` *(size_t)* - Number of elements

**Returns**:
*(void)* - Result stored in `output` array

**Algorithm**:
SIMD using bitwise AND with sign-bit mask (0x7FFFFFFFFFFFFFFF). Processes 4 doubles per iteration.

**Complexity**:
- Time: O(n)
- Space: O(1)

**Performance**:
~1.2x vs GCC -O3

**Floating-Point Notes**:
- Works for NaN (returns positive NaN)
- abs(-0.0) = +0.0
- abs(±INFINITY) = +INFINITY

**Example**:
```c
double input[] = {-3.5, 2.1, -0.0, -7.8};
double output[4];
fp_map_abs_f64(output, input, 4);
// output = [3.5, 2.1, 0.0, 7.8]
```

**Applications**:
- Magnitude computation
- Rectified Linear Unit (ReLU) variant
- Error calculation

**See Also**:
- `fp_map_abs_i64` - Integer version

---

## fp_map_sqrt_f64

**Signature**:
```c
void fp_map_sqrt_f64(double* output, const double* input, size_t n);
```

**Description**:
Square root: **output[i] = √input[i]**. Computes square root of each element using SIMD.

**Parameters**:
- `output` *(double\*)* - Output array
- `input` *(const double\*)* - Input array (must be non-negative)
- `n` *(size_t)* - Number of elements

**Returns**:
*(void)* - Result stored in `output` array

**Algorithm**:
SIMD using `vsqrtpd` instruction. Processes 4 doubles per iteration. Hardware-accurate square root.

**Complexity**:
- Time: O(n)
- Space: O(1)

**Performance**:
~1.5x vs GCC -O3

**Floating-Point Notes**:
- sqrt(-x) = NaN for x > 0
- sqrt(+INFINITY) = +INFINITY
- sqrt(+0.0) = +0.0
- sqrt(-0.0) = -0.0

**Example**:
```c
double input[] = {4.0, 9.0, 16.0, 25.0};
double output[4];
fp_map_sqrt_f64(output, input, 4);
// output = [2.0, 3.0, 4.0, 5.0]
```

**Applications**:
- Euclidean distance: sqrt(sum of squares)
- Standard deviation calculation
- Physics: velocity = sqrt(2 * energy / mass)

**See Also**:
- `fp_fold_sumsq_i64` - Sum of squares (for norm calculation)

---

## fp_map_clamp_i64

**Signature**:
```c
void fp_map_clamp_i64(int64_t* output, const int64_t* input, int64_t min_val, int64_t max_val, size_t n);
```

**Description**:
Clamp values to range: **output[i] = max(min_val, min(input[i], max_val))**. Constrains each element to [min_val, max_val].

**Parameters**:
- `output` *(int64_t\*)* - Output array (can be same as input)
- `input` *(const int64_t\*)* - Input array
- `min_val` *(int64_t)* - Minimum allowed value
- `max_val` *(int64_t)* - Maximum allowed value
- `n` *(size_t)* - Number of elements

**Returns**:
*(void)* - Result stored in `output` array

**Algorithm**:
Scalar using conditional moves (`cmov`). Processes 4 elements per iteration. AVX2 lacks 64-bit min/max instructions.

**Complexity**:
- Time: O(n)
- Space: O(1)

**Performance**:
~1.0x vs GCC -O3

**Precondition**:
Requires `min_val <= max_val`. Behavior undefined if violated.

**Haskell Equivalent**:
```haskell
map (\x -> max min_val (min x max_val)) list
```

**Example**:
```c
int64_t input[] = {-5, 3, 15, 8, 20};
int64_t output[5];
fp_map_clamp_i64(output, input, 0, 10, 5);
// output = [0, 3, 10, 8, 10]
//           ^clipped  ^clipped
```

**Applications**:
- Saturation arithmetic
- Range limiting (e.g., percentage 0-100)
- Pixel value clipping (0-255)
- Outlier suppression

**See Also**:
- `fp_map_clamp_f64` - Double version with SIMD

---

## fp_map_clamp_f64

**Signature**:
```c
void fp_map_clamp_f64(double* output, const double* input, double min_val, double max_val, size_t n);
```

**Description**:
Clamp doubles to range using SIMD: **output[i] = max(min_val, min(input[i], max_val))**.

**Parameters**:
- `output` *(double\*)* - Output array
- `input` *(const double\*)* - Input array
- `min_val` *(double)* - Minimum allowed value
- `max_val` *(double)* - Maximum allowed value
- `n` *(size_t)* - Number of elements

**Returns**:
*(void)* - Result stored in `output` array

**Algorithm**:
SIMD using `vminpd` and `vmaxpd`. Processes 4 doubles per iteration.

**Complexity**:
- Time: O(n)
- Space: O(1)

**Performance**:
~1.5x vs GCC -O3

**Floating-Point Notes**:
- NaN propagates through min/max
- Works correctly with ±0.0
- Can clamp to ±INFINITY

**Example**:
```c
double input[] = {-1.5, 0.5, 1.5, 2.5};
double output[4];
fp_map_clamp_f64(output, input, 0.0, 1.0, 4);
// output = [0.0, 0.5, 1.0, 1.0]
```

**Applications**:
- Activation function clipping
- Normalization to [0,1]
- Color value saturation

**See Also**:
- `fp_map_clamp_i64` - Integer version

---

# Module 5: Scans (Prefix Operations)

Cumulative operations that produce arrays of partial results.

---

## fp_scan_add_i64

**Signature**:
```c
void fp_scan_add_i64(int64_t* output, const int64_t* input, size_t n);
```

**Description**:
Prefix sum (cumulative sum): **output[i] = Σ(input[0..i])**. Each output element is the sum of all inputs up to and including that position.

**Parameters**:
- `output` *(int64_t\*)* - Output array (must be different from input)
- `input` *(const int64_t\*)* - Input array
- `n` *(size_t)* - Number of elements

**Returns**:
*(void)* - Result stored in `output` array

**Algorithm**:
Sequential scan (inherently serial). Uses running accumulator. Simple scalar loop.

**Complexity**:
- Time: O(n) - cannot be parallelized trivially
- Space: O(1) auxiliary

**Performance**:
~1.0x vs GCC -O3 (memory-bound, serial dependency)

**No In-Place**:
Output must be separate from input due to dependency chain.

**Haskell Equivalent**:
```haskell
scanl1 (+) list
-- or
tail $ scanl (+) 0 list
```

**Example**:
```c
int64_t input[] = {1, 2, 3, 4, 5};
int64_t output[5];
fp_scan_add_i64(output, input, 5);
// output = [1, 3, 6, 10, 15]
//           1, 1+2, 1+2+3, 1+2+3+4, 1+2+3+4+5
```

**Applications**:
- Running totals
- Cumulative probability
- Index calculation for ragged arrays
- Parallel prefix sum (building block for parallel algorithms)
- Range sum queries (with preprocessing)

**See Also**:
- `fp_scan_add_f64` - Double version
- `fp_reduce_add_i64` - Final sum only

---

## fp_scan_add_f64

**Signature**:
```c
void fp_scan_add_f64(double* output, const double* input, size_t n);
```

**Description**:
Prefix sum for doubles: **output[i] = Σ(input[0..i])**.

**Parameters**:
- `output` *(double\*)* - Output array (must be different from input)
- `input` *(const double\*)* - Input array
- `n` *(size_t)* - Number of elements

**Returns**:
*(void)* - Result stored in `output` array

**Algorithm**:
Sequential scalar scan with running accumulator.

**Complexity**:
- Time: O(n)
- Space: O(1)

**Performance**:
~1.0x vs GCC -O3 (serial operation)

**Floating-Point Notes**:
- Accumulates left-to-right
- Rounding errors accumulate
- Not compensated (no Kahan summation)
- For high accuracy over large arrays, consider compensated algorithms

**Example**:
```c
double input[] = {1.0, 2.5, 3.2, 4.1};
double output[4];
fp_scan_add_f64(output, input, 4);
// output = [1.0, 3.5, 6.7, 10.8]
```

**Applications**:
- Cumulative distribution functions
- Running averages (scan / n)
- Integration approximation (Riemann sum)

**See Also**:
- `fp_scan_add_i64` - Integer version

---

# Module 6: Predicates (Boolean Tests)

Boolean test operations that return true/false based on array contents.

---

## fp_pred_all_eq_i64

**Signature**:
```c
bool fp_pred_all_eq_i64(const int64_t* array, size_t n, int64_t value);
```

**Description**:
Tests if all elements equal a given value: **∀i: array[i] == value**.

**Parameters**:
- `array` *(const int64_t\*)* - Input array
- `n` *(size_t)* - Number of elements
- `value` *(int64_t)* - Value to compare against

**Returns**:
*(bool)* - `true` if all elements equal `value`, `false` otherwise. Returns `true` for empty arrays (vacuous truth).

**Algorithm**:
Scalar with early exit. Uses comparison and conditional jump. Stops at first mismatch.

**Complexity**:
- Time: O(n) worst case, O(1) best case (early exit)
- Space: O(1)

**Performance**:
~1.0x vs GCC -O3 (branch prediction critical)

**Haskell Equivalent**:
```haskell
all (== value) list
```

**Example**:
```c
int64_t data1[] = {5, 5, 5, 5};
int64_t data2[] = {5, 5, 7, 5};

bool r1 = fp_pred_all_eq_i64(data1, 4, 5);  // true
bool r2 = fp_pred_all_eq_i64(data2, 4, 5);  // false
```

**Applications**:
- Constant array verification
- Sentinel checking
- Validation (all flags set)

**See Also**:
- `fp_pred_any_gt_i64` - Existential test
- `fp_reduce_and_bool` - General boolean AND

---

## fp_pred_any_gt_i64

**Signature**:
```c
bool fp_pred_any_gt_i64(const int64_t* array, size_t n, int64_t threshold);
```

**Description**:
Tests if any element exceeds threshold: **∃i: array[i] > threshold**.

**Parameters**:
- `array` *(const int64_t\*)* - Input array
- `n` *(size_t)* - Number of elements
- `threshold` *(int64_t)* - Comparison threshold

**Returns**:
*(bool)* - `true` if at least one element > threshold, `false` otherwise. Returns `false` for empty arrays.

**Algorithm**:
Scalar with early exit. Stops at first element > threshold.

**Complexity**:
- Time: O(n) worst case, O(1) best case
- Space: O(1)

**Performance**:
~1.0x vs GCC -O3

**Haskell Equivalent**:
```haskell
any (> threshold) list
```

**Example**:
```c
int64_t data[] = {3, 7, 2, 9, 4};

bool r1 = fp_pred_any_gt_i64(data, 5, 10);  // false (none > 10)
bool r2 = fp_pred_any_gt_i64(data, 5, 5);   // true (7 and 9 > 5)
```

**Applications**:
- Outlier detection
- Threshold exceedance
- Anomaly flagging

**See Also**:
- `fp_pred_all_eq_i64` - Universal test
- `fp_reduce_or_bool` - General boolean OR

---

## fp_pred_all_gt_zip_i64

**Signature**:
```c
bool fp_pred_all_gt_zip_i64(const int64_t* array_a, const int64_t* array_b, size_t n);
```

**Description**:
Tests if all elements of `a` are greater than corresponding elements of `b`: **∀i: a[i] > b[i]**.

**Parameters**:
- `array_a` *(const int64_t\*)* - First array
- `array_b` *(const int64_t\*)* - Second array
- `n` *(size_t)* - Number of elements in each array

**Returns**:
*(bool)* - `true` if all a[i] > b[i], `false` otherwise. Returns `true` for empty arrays.

**Algorithm**:
Scalar pairwise comparison with early exit.

**Complexity**:
- Time: O(n) worst case, O(1) best case
- Space: O(1)

**Performance**:
~1.0x vs GCC -O3

**Haskell Equivalent**:
```haskell
and $ zipWith (>) list_a list_b
```

**Example**:
```c
int64_t a[] = {10, 20, 30, 40};
int64_t b[] = {5, 15, 25, 35};
int64_t c[] = {5, 25, 20, 35};  // b[1]=25 > a[1]=20

bool r1 = fp_pred_all_gt_zip_i64(a, b, 4);  // true
bool r2 = fp_pred_all_gt_zip_i64(a, c, 4);  // false
```

**Applications**:
- Monotonicity checking (array_a = values, array_b = shifted values)
- Dominance testing
- Quality comparison (all metrics better)

**See Also**:
- `fp_pred_any_gt_i64` - Single array threshold test

---

# Module 7: Compaction (Filter/Partition)

Operations that select or split elements based on predicates.

---

## fp_filter_i64

**Signature**:
```c
size_t fp_filter_i64(const int64_t* input, int64_t* output, size_t n,
                     bool (*predicate)(int64_t));
```

**Description**:
Selects elements that satisfy a predicate: **output = [x | x ← input, predicate(x)]**. Copies only matching elements to output.

**Parameters**:
- `input` *(const int64_t\*)* - Input array
- `output` *(int64_t\*)* - Output array (must have space for n elements)
- `n` *(size_t)* - Number of input elements
- `predicate` *(bool (\*)(int64_t))* - Function pointer: returns true to keep element

**Returns**:
*(size_t)* - Number of elements in output (0 ≤ result ≤ n)

**Algorithm**:
Sequential scan with conditional copy. Output size depends on predicate selectivity.

**Complexity**:
- Time: O(n) + n×O(predicate)
- Space: O(1) auxiliary (output provided by caller)

**Performance**:
~1.0x vs GCC -O3 (predicate call dominates)

**Memory Requirement**:
Output must have space for n elements (worst case: all match).

**Haskell Equivalent**:
```haskell
filter predicate list
```

**Example**:
```c
bool is_positive(int64_t x) { return x > 0; }

int64_t input[] = {-3, 5, -1, 8, 0, 2};
int64_t output[6];
size_t count = fp_filter_i64(input, output, 6, is_positive);
// output = [5, 8, 2], count = 3
```

**Applications**:
- Data cleaning (remove invalid values)
- Selection (e.g., find positive numbers)
- Conditional sampling

**See Also**:
- `fp_filter_f64` - Double version
- `fp_partition_i64` - Split into matching/non-matching

---

## fp_filter_f64

**Signature**:
```c
size_t fp_filter_f64(const double* input, double* output, size_t n,
                     bool (*predicate)(double));
```

**Description**:
Filters double array based on predicate.

**Parameters**:
- `input` *(const double\*)* - Input array
- `output` *(double\*)* - Output array (space for n elements)
- `n` *(size_t)* - Number of input elements
- `predicate` *(bool (\*)(double))* - Selection function

**Returns**:
*(size_t)* - Number of elements in output

**Algorithm**:
Sequential scan with conditional copy.

**Complexity**:
- Time: O(n) + n×O(predicate)
- Space: O(1)

**Example**:
```c
bool is_large(double x) { return x > 5.0; }

double input[] = {3.5, 7.2, 2.1, 9.8, 4.3};
double output[5];
size_t count = fp_filter_f64(input, output, 5, is_large);
// output = [7.2, 9.8], count = 2
```

**Applications**:
- Outlier removal (filter abs(x - mean) > 3*sigma)
- Threshold filtering
- Data subsetting

**See Also**:
- `fp_filter_i64` - Integer version

---

## fp_partition_i64

**Signature**:
```c
size_t fp_partition_i64(const int64_t* input, int64_t* true_output,
                        int64_t* false_output, size_t n,
                        bool (*predicate)(int64_t));
```

**Description**:
Splits array into two groups: elements that satisfy predicate and those that don't. **One-pass filter into two outputs simultaneously.**

**Parameters**:
- `input` *(const int64_t\*)* - Input array
- `true_output` *(int64_t\*)* - Output for matching elements (space for n)
- `false_output` *(int64_t\*)* - Output for non-matching elements (space for n)
- `n` *(size_t)* - Number of input elements
- `predicate` *(bool (\*)(int64_t))* - Classification function

**Returns**:
*(size_t)* - Number of elements in `true_output`. Number in `false_output` = n - result.

**Algorithm**:
Single pass with dual outputs. Each element goes to exactly one output.

**Complexity**:
- Time: O(n) + n×O(predicate)
- Space: O(1)

**Performance**:
~1.0x vs separate filter calls (but only one pass over input)

**Memory Requirement**:
Both outputs must have space for n elements (worst case: all go to one side).

**Haskell Equivalent**:
```haskell
partition predicate list
```

**Example**:
```c
bool is_even(int64_t x) { return (x % 2) == 0; }

int64_t input[] = {1, 2, 3, 4, 5, 6};
int64_t evens[6], odds[6];
size_t n_evens = fp_partition_i64(input, evens, odds, 6, is_even);
// evens = [2, 4, 6], n_evens = 3
// odds = [1, 3, 5], n_odds = 6 - 3 = 3
```

**Applications**:
- Quicksort partitioning
- Binary classification
- Separating valid/invalid data
- Two-way split for parallel processing

**See Also**:
- `fp_filter_i64` - Single output filter

---

## fp_partition_f64

**Signature**:
```c
size_t fp_partition_f64(const double* input, double* true_output,
                        double* false_output, size_t n,
                        bool (*predicate)(double));
```

**Description**:
Partitions double array into matching and non-matching groups.

**Parameters**:
- `input` *(const double\*)* - Input array
- `true_output` *(double\*)* - Matching elements (space for n)
- `false_output` *(double\*)* - Non-matching elements (space for n)
- `n` *(size_t)* - Number of input elements
- `predicate` *(bool (\*)(double))* - Classification function

**Returns**:
*(size_t)* - Number in `true_output`

**Example**:
```c
bool is_positive(double x) { return x > 0.0; }

double input[] = {-2.5, 3.7, -1.2, 5.8, 0.0};
double pos[5], neg[5];
size_t n_pos = fp_partition_f64(input, pos, neg, 5, is_positive);
// pos = [3.7, 5.8], n_pos = 2
// neg = [-2.5, -1.2, 0.0], n_neg = 3
```

**Applications**:
- Signal separation (positive/negative)
- Data splitting for cross-validation
- Separating inliers/outliers

**See Also**:
- `fp_partition_i64` - Integer version

---

# Module 8: Essentials (List Operations)

Fundamental list manipulation operations from functional programming.

---

## fp_reverse_i64

**Signature**:
```c
void fp_reverse_i64(int64_t* output, const int64_t* input, size_t n);
```

**Description**:
Reverses array order: **output[i] = input[n-1-i]**. Copies elements in reverse order.

**Parameters**:
- `output` *(int64_t\*)* - Output array (can be same as input for in-place)
- `input` *(const int64_t\*)* - Input array
- `n` *(size_t)* - Number of elements

**Returns**:
*(void)* - Result stored in `output`

**Algorithm**:
Simple index reversal. In-place uses swap for first half.

**Complexity**:
- Time: O(n)
- Space: O(1)

**Performance**:
~1.0x vs GCC -O3

**In-Place Allowed**:
Can pass same pointer for input and output.

**Haskell Equivalent**:
```haskell
reverse list
```

**Example**:
```c
int64_t input[] = {1, 2, 3, 4, 5};
int64_t output[5];
fp_reverse_i64(output, input, 5);
// output = [5, 4, 3, 2, 1]
```

**Applications**:
- Time series reversal
- Stack operations
- Palindrome checking

**See Also**:
- `fp_reverse_f64` - Double version

---

## fp_reverse_f64

**Signature**:
```c
void fp_reverse_f64(double* output, const double* input, size_t n);
```

**Description**:
Reverses double array: **output[i] = input[n-1-i]**.

**Parameters**:
- `output` *(double\*)* - Output array (can be same as input)
- `input` *(const double\*)* - Input array
- `n` *(size_t)* - Number of elements

**Returns**:
*(void)* - Result stored in `output`

**Example**:
```c
double input[] = {1.0, 2.0, 3.0, 4.0};
double output[4];
fp_reverse_f64(output, input, 4);
// output = [4.0, 3.0, 2.0, 1.0]
```

**See Also**:
- `fp_reverse_i64` - Integer version

---

## fp_replicate_i64

**Signature**:
```c
void fp_replicate_i64(int64_t* output, size_t n, int64_t value);
```

**Description**:
Fills array with single value: **output[i] = value** for all i. Creates array of n copies.

**Parameters**:
- `output` *(int64_t\*)* - Output array
- `n` *(size_t)* - Number of elements to create
- `value` *(int64_t)* - Value to replicate

**Returns**:
*(void)* - Result stored in `output`

**Algorithm**:
Simple loop. Loop unrolling with 4 copies per iteration.

**Complexity**:
- Time: O(n)
- Space: O(1)

**Performance**:
~1.0-1.1x vs GCC -O3 (memory-bound)

**Haskell Equivalent**:
```haskell
replicate n value
```

**Example**:
```c
int64_t output[5];
fp_replicate_i64(output, 5, 42);
// output = [42, 42, 42, 42, 42]
```

**Applications**:
- Array initialization
- Constant vector creation
- Padding/fill operations

**See Also**:
- `fp_replicate_f64` - Double version (SIMD optimized)

---

## fp_concat_i64

**Signature**:
```c
void fp_concat_i64(int64_t* output, const int64_t* array_a, size_t len_a,
                   const int64_t* array_b, size_t len_b);
```

**Description**:
Concatenates two arrays: **output = array_a ++ array_b**. Copies first array, then second array.

**Parameters**:
- `output` *(int64_t\*)* - Output array (must have space for len_a + len_b)
- `array_a` *(const int64_t\*)* - First array
- `len_a` *(size_t)* - Length of first array
- `array_b` *(const int64_t\*)* - Second array
- `len_b` *(size_t)* - Length of second array

**Returns**:
*(void)* - Result stored in `output`

**Algorithm**:
Two sequential memcpy-style operations.

**Complexity**:
- Time: O(len_a + len_b)
- Space: O(1)

**Performance**:
~1.0x vs GCC -O3

**Memory Requirement**:
Output must have space for len_a + len_b elements.

**Haskell Equivalent**:
```haskell
list_a ++ list_b
```

**Example**:
```c
int64_t a[] = {1, 2, 3};
int64_t b[] = {4, 5};
int64_t output[5];
fp_concat_i64(output, a, 3, b, 2);
// output = [1, 2, 3, 4, 5]
```

**Applications**:
- Combining datasets
- Building result from parts
- Joining time series segments

**See Also**:
- `fp_concat_f64` - Double version

---

## fp_concat_f64

**Signature**:
```c
void fp_concat_f64(double* output, const double* array_a, size_t len_a,
                   const double* array_b, size_t len_b);
```

**Description**:
Concatenates two double arrays.

**Parameters**:
- `output` *(double\*)* - Output array (space for len_a + len_b)
- `array_a` *(const double\*)* - First array
- `len_a` *(size_t)* - Length of first array
- `array_b` *(const double\*)* - Second array
- `len_b` *(size_t)* - Length of second array

**Returns**:
*(void)* - Result stored in `output`

**Example**:
```c
double a[] = {1.0, 2.0};
double b[] = {3.0, 4.0, 5.0};
double output[5];
fp_concat_f64(output, a, 2, b, 3);
// output = [1.0, 2.0, 3.0, 4.0, 5.0]
```

**See Also**:
- `fp_concat_i64` - Integer version

---

## fp_take_i64

**Signature**:
```c
void fp_take_i64(int64_t* output, const int64_t* input, size_t n);
```

**Description**:
Takes first n elements: **output = input[0..n-1]**. Copies the initial portion of array.

**Parameters**:
- `output` *(int64_t\*)* - Output array
- `input` *(const int64_t\*)* - Input array
- `n` *(size_t)* - Number of elements to take

**Returns**:
*(void)* - Result stored in `output`

**Algorithm**:
Simple forward copy.

**Complexity**:
- Time: O(n)
- Space: O(1)

**Haskell Equivalent**:
```haskell
take n list
```

**Example**:
```c
int64_t input[] = {1, 2, 3, 4, 5};
int64_t output[3];
fp_take_i64(output, input, 3);
// output = [1, 2, 3]
```

**Applications**:
- Head extraction
- Truncation
- Sampling first n items

**See Also**:
- `fp_drop_i64` - Skip first n
- `fp_slice_i64` - Extract arbitrary range
- `fp_take_while_i64` - Conditional take

---

## fp_take_f64

**Signature**:
```c
void fp_take_f64(double* output, const double* input, size_t n);
```

**Description**:
Takes first n doubles.

**Parameters**:
- `output` *(double\*)* - Output array
- `input` *(const double\*)* - Input array
- `n` *(size_t)* - Number to take

**Returns**:
*(void)* - Result stored in `output`

**Example**:
```c
double input[] = {1.0, 2.0, 3.0, 4.0};
double output[2];
fp_take_f64(output, input, 2);
// output = [1.0, 2.0]
```

**See Also**:
- `fp_take_i64` - Integer version

---

## fp_drop_i64

**Signature**:
```c
void fp_drop_i64(int64_t* output, const int64_t* input, size_t n, size_t total);
```

**Description**:
Drops first n elements, copies rest: **output = input[n..total-1]**. Skips initial portion.

**Parameters**:
- `output` *(int64_t\*)* - Output array
- `input` *(const int64_t\*)* - Input array
- `n` *(size_t)* - Number of elements to skip
- `total` *(size_t)* - Total number of elements in input

**Returns**:
*(void)* - Result stored in `output` (total - n elements)

**Algorithm**:
Copy from offset n to end.

**Complexity**:
- Time: O(total - n)
- Space: O(1)

**Precondition**:
n ≤ total

**Haskell Equivalent**:
```haskell
drop n list
```

**Example**:
```c
int64_t input[] = {1, 2, 3, 4, 5};
int64_t output[3];
fp_drop_i64(output, input, 2, 5);
// output = [3, 4, 5]  (skipped first 2)
```

**Applications**:
- Header removal
- Tail extraction
- Skipping initial samples

**See Also**:
- `fp_take_i64` - Take first n
- `fp_drop_while_i64` - Conditional drop

---

## fp_drop_f64

**Signature**:
```c
void fp_drop_f64(double* output, const double* input, size_t n, size_t total);
```

**Description**:
Drops first n doubles, copies rest.

**Parameters**:
- `output` *(double\*)* - Output array
- `input` *(const double\*)* - Input array
- `n` *(size_t)* - Number to skip
- `total` *(size_t)* - Total input elements

**Returns**:
*(void)* - Result stored in `output`

**Example**:
```c
double input[] = {1.0, 2.0, 3.0, 4.0};
double output[2];
fp_drop_f64(output, input, 2, 4);
// output = [3.0, 4.0]
```

**See Also**:
- `fp_drop_i64` - Integer version

---

## fp_slice_i64

**Signature**:
```c
void fp_slice_i64(int64_t* output, const int64_t* input, size_t start, size_t end);
```

**Description**:
Extracts subarray: **output = input[start..end-1]**. Copies elements from start (inclusive) to end (exclusive).

**Parameters**:
- `output` *(int64_t\*)* - Output array
- `input` *(const int64_t\*)* - Input array
- `start` *(size_t)* - Start index (inclusive)
- `end` *(size_t)* - End index (exclusive)

**Returns**:
*(void)* - Result stored in `output` (end - start elements)

**Algorithm**:
Copy from input[start] to input[end-1].

**Complexity**:
- Time: O(end - start)
- Space: O(1)

**Precondition**:
start ≤ end

**Haskell Equivalent**:
```haskell
take (end - start) $ drop start list
```

**Example**:
```c
int64_t input[] = {10, 20, 30, 40, 50};
int64_t output[3];
fp_slice_i64(output, input, 1, 4);
// output = [20, 30, 40]  (indices 1, 2, 3)
```

**Applications**:
- Windowing
- Extracting subranges
- Array indexing operations

**See Also**:
- `fp_take_i64` - From start only
- `fp_drop_i64` - To end only

---

## fp_slice_f64

**Signature**:
```c
void fp_slice_f64(double* output, const double* input, size_t start, size_t end);
```

**Description**:
Extracts subarray of doubles.

**Parameters**:
- `output` *(double\*)* - Output array
- `input` *(const double\*)* - Input array
- `start` *(size_t)* - Start index (inclusive)
- `end` *(size_t)* - End index (exclusive)

**Returns**:
*(void)* - Result stored in `output`

**Example**:
```c
double input[] = {1.0, 2.0, 3.0, 4.0, 5.0};
double output[2];
fp_slice_f64(output, input, 2, 4);
// output = [3.0, 4.0]
```

**See Also**:
- `fp_slice_i64` - Integer version

---

## fp_take_while_i64

**Signature**:
```c
size_t fp_take_while_i64(int64_t* output, const int64_t* input, size_t n,
                         bool (*predicate)(int64_t));
```

**Description**:
Takes elements while predicate holds: **output = [x | x ← input, stop at first false]**. Copies elements until predicate returns false.

**Parameters**:
- `output` *(int64_t\*)* - Output array (space for n elements)
- `input` *(const int64_t\*)* - Input array
- `n` *(size_t)* - Input length
- `predicate` *(bool (\*)(int64_t))* - Continuation test

**Returns**:
*(size_t)* - Number of elements copied (0 ≤ result ≤ n)

**Algorithm**:
Sequential scan, stop at first predicate failure.

**Complexity**:
- Time: O(result) + result×O(predicate) - early termination
- Space: O(1)

**Haskell Equivalent**:
```haskell
takeWhile predicate list
```

**Example**:
```c
bool is_small(int64_t x) { return x < 10; }

int64_t input[] = {1, 3, 5, 12, 7, 9};
int64_t output[6];
size_t count = fp_take_while_i64(output, input, 6, is_small);
// output = [1, 3, 5], count = 3
// Stopped at 12 (first element ≥ 10)
```

**Applications**:
- Take until condition
- Extracting monotonic prefix
- Consuming valid header

**See Also**:
- `fp_take_i64` - Fixed count
- `fp_drop_while_i64` - Complement operation

---

## fp_drop_while_i64

**Signature**:
```c
size_t fp_drop_while_i64(int64_t* output, const int64_t* input, size_t n,
                         bool (*predicate)(int64_t));
```

**Description**:
Drops elements while predicate holds, copies rest: **output = drop leading prefix where predicate true**. Skips elements until predicate fails.

**Parameters**:
- `output` *(int64_t\*)* - Output array (space for n elements)
- `input` *(const int64_t\*)* - Input array
- `n` *(size_t)* - Input length
- `predicate` *(bool (\*)(int64_t))* - Skip condition

**Returns**:
*(size_t)* - Number of elements in output

**Algorithm**:
Find first element where predicate fails, copy from there to end.

**Complexity**:
- Time: O(n) + k×O(predicate) where k = drop count
- Space: O(1)

**Haskell Equivalent**:
```haskell
dropWhile predicate list
```

**Example**:
```c
bool is_small(int64_t x) { return x < 10; }

int64_t input[] = {1, 3, 5, 12, 7, 9};
int64_t output[6];
size_t count = fp_drop_while_i64(output, input, 6, is_small);
// output = [12, 7, 9], count = 3
// Dropped [1, 3, 5], started at 12
```

**Applications**:
- Skip header/preamble
- Find first valid element and continue
- Remove leading zeros/whitespace analog

**See Also**:
- `fp_take_while_i64` - Complement
- `fp_drop_i64` - Fixed count

---

## fp_find_index_i64

**Signature**:
```c
int64_t fp_find_index_i64(const int64_t* array, size_t n, int64_t target);
```

**Description**:
Finds first occurrence of value: **returns index i where array[i] == target**. Linear search.

**Parameters**:
- `array` *(const int64_t\*)* - Input array
- `n` *(size_t)* - Array length
- `target` *(int64_t)* - Value to find

**Returns**:
*(int64_t)* - Index of first occurrence (0-based), or -1 if not found.

**Algorithm**:
Sequential search with early exit.

**Complexity**:
- Time: O(n) worst case, O(1) best case
- Space: O(1)

**Performance**:
~1.0x vs GCC -O3

**Haskell Equivalent**:
```haskell
elemIndex target list
-- or
findIndex (== target) list
```

**Example**:
```c
int64_t data[] = {10, 20, 30, 40, 50};

int64_t idx1 = fp_find_index_i64(data, 5, 30);  // 2
int64_t idx2 = fp_find_index_i64(data, 5, 99);  // -1 (not found)
```

**Applications**:
- Finding element position
- Lookup in unsorted array
- First occurrence search

**See Also**:
- `fp_find_index_f64` - Double version
- `fp_contains_i64` - Boolean membership test

---

## fp_find_index_f64

**Signature**:
```c
int64_t fp_find_index_f64(const double* array, size_t n, double target);
```

**Description**:
Finds first occurrence in double array.

**Parameters**:
- `array` *(const double\*)* - Input array
- `n` *(size_t)* - Array length
- `target` *(double)* - Value to find

**Returns**:
*(int64_t)* - Index or -1 if not found

**Floating-Point Notes**:
- Uses exact equality (==), problematic for floating-point
- Consider using tolerance-based search for robust FP comparison

**Example**:
```c
double data[] = {1.5, 2.7, 3.2, 4.1};
int64_t idx = fp_find_index_f64(data, 4, 3.2);  // 2
```

**See Also**:
- `fp_find_index_i64` - Integer version

---

## fp_contains_i64

**Signature**:
```c
bool fp_contains_i64(const int64_t* array, size_t n, int64_t value);
```

**Description**:
Membership test: **returns true if value ∈ array**. Checks if value exists anywhere in array.

**Parameters**:
- `array` *(const int64_t\*)* - Input array
- `n` *(size_t)* - Array length
- `value` *(int64_t)* - Value to search for

**Returns**:
*(bool)* - `true` if found, `false` otherwise

**Algorithm**:
Linear search with early exit.

**Complexity**:
- Time: O(n) worst case, O(1) best case
- Space: O(1)

**Haskell Equivalent**:
```haskell
elem value list
```

**Example**:
```c
int64_t data[] = {10, 20, 30, 40, 50};

bool found1 = fp_contains_i64(data, 5, 30);  // true
bool found2 = fp_contains_i64(data, 5, 99);  // false
```

**Applications**:
- Membership checking
- Validation (is value in whitelist?)
- Existence testing

**See Also**:
- `fp_find_index_i64` - Returns position instead of boolean

---

# Module 9: Sorting & Sets (TIER 2)

Sorting and set-theoretic operations on arrays.

---

## fp_sort_i64

**Signature**:
```c
void fp_sort_i64(int64_t* array, size_t n);
```

**Description**:
Sorts array in-place in ascending order using optimized quicksort. **Modifies input array directly.**

**Parameters**:
- `array` *(int64_t\*)* - Array to sort (modified in-place)
- `n` *(size_t)* - Number of elements

**Returns**:
*(void)* - Array sorted in-place

**Algorithm**:
Hybrid quicksort with optimizations:
- **Median-of-three pivot selection** - Reduces worst-case probability
- **Insertion sort for small subarrays** (n < 16) - Better for small data
- **Tail recursion optimization** - Stack depth O(log n) instead of O(n)
- **In-place partitioning** - No extra memory

**Complexity**:
- Time: O(n log n) average, O(n²) worst case (rare with median-of-3)
- Space: O(log n) stack (tail recursion)

**Performance**:
~1.0-1.2x vs C stdlib `qsort` (fewer function call overheads)

**In-Place Modification**:
Original array is modified. Pass a copy if needed.

**Stability**:
Not stable (equal elements may be reordered).

**Haskell Equivalent**:
```haskell
sort list  -- from Data.List
```

**Example**:
```c
int64_t data[] = {5, 2, 8, 1, 9, 3, 7};
fp_sort_i64(data, 7);
// data = [1, 2, 3, 5, 7, 8, 9]
```

**Applications**:
- General-purpose sorting
- Preprocessing for median/percentiles
- Preprocessing for set operations
- Binary search preparation

**See Also**:
- `fp_sort_f64` - Double version
- `fp_unique_i64` - Remove duplicates (requires sorted input)

---

## fp_sort_f64

**Signature**:
```c
void fp_sort_f64(double* array, size_t n);
```

**Description**:
Sorts double array in-place using optimized quicksort.

**Parameters**:
- `array` *(double\*)* - Array to sort (modified in-place)
- `n` *(size_t)* - Number of elements

**Returns**:
*(void)* - Array sorted in-place

**Algorithm**:
Same hybrid quicksort as i64 version. Uses SSE scalar comparisons (`vcomisd`) for correct floating-point semantics.

**Complexity**:
- Time: O(n log n) average
- Space: O(log n) stack

**Performance**:
~1.0-1.2x vs C qsort

**Floating-Point Notes**:
- NaN values sort to end (unspecified relative order among NaNs)
- -0.0 and +0.0 compare equal
- Handles ±INFINITY correctly

**Example**:
```c
double data[] = {3.5, 1.2, 7.8, 2.1, 9.3};
fp_sort_f64(data, 5);
// data = [1.2, 2.1, 3.5, 7.8, 9.3]
```

**Applications**:
- Statistical analysis (median, quartiles)
- Percentile calculation
- Outlier detection (after sorting)

**See Also**:
- `fp_sort_i64` - Integer version

---

## fp_unique_i64

**Signature**:
```c
size_t fp_unique_i64(const int64_t* input, int64_t* output, size_t n);
```

**Description**:
Removes consecutive duplicates: **output = nub(input)**. Requires sorted input for full deduplication.

**Parameters**:
- `input` *(const int64_t\*)* - Input array (should be sorted)
- `output` *(int64_t\*)* - Output array (space for n elements)
- `n` *(size_t)* - Input length

**Returns**:
*(size_t)* - Number of unique elements (1 ≤ result ≤ n for n > 0)

**Algorithm**:
Single pass, compare consecutive elements. Copy when different.

**Complexity**:
- Time: O(n)
- Space: O(1) auxiliary

**Performance**:
~2.0-3.0x vs C++ `std::unique`

**Precondition for Full Dedup**:
Input must be sorted. Otherwise only removes consecutive duplicates.

**Haskell Equivalent**:
```haskell
nub list  -- from Data.List (on sorted list)
```

**Example**:
```c
int64_t input[] = {1, 1, 2, 2, 2, 3, 4, 4, 5};
int64_t output[9];
size_t count = fp_unique_i64(input, output, 9);
// output = [1, 2, 3, 4, 5], count = 5
```

**Typical Usage**:
```c
// Full deduplication
fp_sort_i64(data, n);  // Sort first
size_t unique_count = fp_unique_i64(data, result, n);
```

**Applications**:
- DISTINCT in SQL
- Finding unique elements
- Deduplication
- Counting distinct values

**See Also**:
- `fp_sort_i64` - Required preprocessing
- `fp_group_i64` - Group consecutive equals with counts

---

## fp_union_i64

**Signature**:
```c
size_t fp_union_i64(const int64_t* array_a, const int64_t* array_b,
                    int64_t* output, size_t len_a, size_t len_b);
```

**Description**:
Set union: **output = A ∪ B** (with deduplication). Merges two sorted arrays, removing duplicates. **Requires both inputs to be sorted.**

**Parameters**:
- `array_a` *(const int64_t\*)* - First sorted array
- `array_b` *(const int64_t\*)* - Second sorted array
- `output` *(int64_t\*)* - Output array (space for len_a + len_b)
- `len_a` *(size_t)* - Length of array_a
- `len_b` *(size_t)* - Length of array_b

**Returns**:
*(size_t)* - Number of elements in output (≤ len_a + len_b)

**Algorithm**:
Two-pointer merge with deduplication:
- Take smaller element from A or B
- If equal, take once and advance both
- Linear time complexity

**Complexity**:
- Time: O(len_a + len_b)
- Space: O(1) auxiliary

**Performance**:
~1.5-2.0x vs C++ `std::set_union`

**Precondition**:
Both inputs MUST be sorted in ascending order.

**Haskell Equivalent**:
```haskell
union list_a list_b  -- from Data.List
```

**Example**:
```c
int64_t a[] = {1, 3, 5, 7, 9};
int64_t b[] = {2, 3, 5, 8};
int64_t output[9];

size_t count = fp_union_i64(a, b, output, 5, 4);
// output = [1, 2, 3, 5, 7, 8, 9], count = 7
// Note: 3 and 5 appear only once (deduplication)
```

**Typical Usage**:
```c
// Ensure inputs are sorted
fp_sort_i64(a, len_a);
fp_sort_i64(b, len_b);
size_t union_size = fp_union_i64(a, b, result, len_a, len_b);
```

**Applications**:
- Set union in databases (SQL UNION)
- Combining datasets without duplicates
- Merging sorted lists
- Finding all unique elements from two sources

**See Also**:
- `fp_intersect_i64` - Set intersection
- `fp_unique_i64` - Remove duplicates from single array

---

## fp_intersect_i64

**Signature**:
```c
size_t fp_intersect_i64(const int64_t* array_a, const int64_t* array_b,
                        int64_t* output, size_t len_a, size_t len_b);
```

**Description**:
Set intersection: **output = A ∩ B**. Finds common elements from two sorted arrays. **Requires both inputs to be sorted.**

**Parameters**:
- `array_a` *(const int64_t\*)* - First sorted array
- `array_b` *(const int64_t\*)* - Second sorted array
- `output` *(int64_t\*)* - Output array (space for min(len_a, len_b))
- `len_a` *(size_t)* - Length of array_a
- `len_b` *(size_t)* - Length of array_b

**Returns**:
*(size_t)* - Number of common elements (0 ≤ result ≤ min(len_a, len_b))

**Algorithm**:
Two-pointer merge:
- Advance smaller value's pointer
- When equal, add to output and advance both
- Linear time

**Complexity**:
- Time: O(len_a + len_b)
- Space: O(1) auxiliary

**Performance**:
~1.5-2.0x vs C++ `std::set_intersection`

**Precondition**:
Both inputs MUST be sorted.

**Haskell Equivalent**:
```haskell
intersect list_a list_b  -- from Data.List
```

**Example**:
```c
int64_t a[] = {1, 3, 5, 7, 9};
int64_t b[] = {2, 3, 5, 8, 9};
int64_t output[5];

size_t count = fp_intersect_i64(a, b, output, 5, 5);
// output = [3, 5, 9], count = 3
// Common elements only
```

**Typical Usage**:
```c
// Ensure sorted
fp_sort_i64(a, len_a);
fp_sort_i64(b, len_b);
size_t common = fp_intersect_i64(a, b, result, len_a, len_b);
```

**Applications**:
- Database JOIN (INNER JOIN on IDs)
- Finding common elements
- Set intersection in data analysis
- Collaborative filtering (common items)

**See Also**:
- `fp_union_i64` - Set union
- `fp_sort_i64` - Required preprocessing

---

# Module 10: Advanced Operations (TIER 3)

Advanced functional programming operations: grouping, unfold, boolean reductions, and utilities.

---

## fp_group_i64

**Signature**:
```c
size_t fp_group_i64(const int64_t* input, int64_t* groups_out,
                    int64_t* counts_out, size_t n);
```

**Description**:
Groups consecutive equal elements. **Haskell's `group` function.** Returns representatives and counts in parallel arrays.

**Parameters**:
- `input` *(const int64_t\*)* - Input array
- `groups_out` *(int64_t\*)* - Output array of group representative values (space for n)
- `counts_out` *(int64_t\*)* - Output array of group sizes (space for n)
- `n` *(size_t)* - Input length

**Returns**:
*(size_t)* - Number of groups (1 ≤ result ≤ n for n > 0)

**Algorithm**:
Single pass comparing consecutive elements. Start new group on value change.

**Complexity**:
- Time: O(n)
- Space: O(1) auxiliary

**Why Parallel Arrays**:
C cannot return list-of-lists like Haskell. Parallel arrays provide same information.

**Haskell Equivalent**:
```haskell
group [1,1,2,2,2,3] → [[1,1],[2,2,2],[3]]
-- FP-ASM returns: groups=[1,2,3], counts=[2,3,1]
```

**Example**:
```c
int64_t input[] = {1, 1, 2, 2, 2, 3, 4, 4};
int64_t groups[8], counts[8];

size_t n_groups = fp_group_i64(input, groups, counts, 8);
// n_groups = 4
// groups = [1, 2, 3, 4]
// counts = [2, 3, 1, 2]
```

**Applications**:
- Run-length encoding
- Frequency counting (after sorting)
- Finding mode (most frequent value)
- Histogram computation

**Typical Usage for Mode**:
```c
fp_sort_i64(data, n);  // Sort first
int64_t groups[n], counts[n];
size_t ng = fp_group_i64(data, groups, counts, n);

// Find index of maximum count
size_t max_idx = fp_find_index_i64(counts, ng,
    fp_reduce_max_i64(counts, ng));
int64_t mode = groups[max_idx];
```

**See Also**:
- `fp_run_length_encode_i64` - Alternative interleaved format
- `fp_sort_i64` - Preprocessing for frequency analysis

---

## fp_run_length_encode_i64

**Signature**:
```c
size_t fp_run_length_encode_i64(const int64_t* input, int64_t* output, size_t n);
```

**Description**:
Run-length encoding: compresses consecutive runs. **Alternative to group with interleaved output format.**

**Parameters**:
- `input` *(const int64_t\*)* - Input array
- `output` *(int64_t\*)* - Output array (space for 2×n)
- `n` *(size_t)* - Input length

**Returns**:
*(size_t)* - Number of output elements = 2 × (number of runs)

**Output Format**:
Interleaved: `[value1, count1, value2, count2, ...]`

**Algorithm**:
Single pass, emit (value, count) pairs.

**Complexity**:
- Time: O(n)
- Space: O(1) auxiliary

**Performance**:
More cache-friendly than parallel arrays for some applications.

**Example**:
```c
int64_t input[] = {5, 5, 5, 2, 2, 7, 7, 7, 7};
int64_t output[18];  // Worst case: 2×9

size_t output_len = fp_run_length_encode_i64(input, output, 9);
// output_len = 6
// output = [5, 3,  2, 2,  7, 4]
//          value count value count value count
```

**Applications**:
- Data compression (especially for images/video)
- Sparse data encoding
- Protocol encoding
- Efficient storage of repetitive data

**Compression Ratio**:
```c
size_t runs = output_len / 2;
double compression_ratio = (double)n / runs;
printf("Compressed to %.1f%% of original\n",
       (runs * 2.0 / n) * 100);
```

**See Also**:
- `fp_group_i64` - Parallel array format

---

## fp_iterate_add_i64

**Signature**:
```c
void fp_iterate_add_i64(int64_t* output, size_t n, int64_t start, int64_t step);
```

**Description**:
Generates arithmetic sequence: **output[i] = start + i × step**. Unfold operation with addition.

**Parameters**:
- `output` *(int64_t\*)* - Output array
- `n` *(size_t)* - Number of elements to generate
- `start` *(int64_t)* - Initial value
- `step` *(int64_t)* - Increment per element

**Returns**:
*(void)* - Result stored in `output`

**Algorithm**:
Simple loop with accumulator.

**Complexity**:
- Time: O(n)
- Space: O(1)

**Haskell Equivalent**:
```haskell
take n $ iterate (+ step) start
```

**Example**:
```c
int64_t output[10];
fp_iterate_add_i64(output, 10, 5, 3);
// output = [5, 8, 11, 14, 17, 20, 23, 26, 29, 32]
//          start, start+step, start+2*step, ...
```

**Special Cases**:
```c
// Counting sequence [0, 1, 2, ...]
fp_iterate_add_i64(output, n, 0, 1);

// Countdown
fp_iterate_add_i64(output, n, 100, -1);
// [100, 99, 98, ...]

// Even numbers
fp_iterate_add_i64(output, n, 0, 2);
// [0, 2, 4, 6, ...]
```

**Applications**:
- Test data generation
- Index arrays
- Time series (linear trend)
- Coordinate generation

**See Also**:
- `fp_iterate_mul_i64` - Geometric sequence
- `fp_range_i64` - Range [start..end)

---

## fp_iterate_mul_i64

**Signature**:
```c
void fp_iterate_mul_i64(int64_t* output, size_t n, int64_t start, int64_t factor);
```

**Description**:
Generates geometric sequence: **output[i] = start × factor^i**. Unfold with multiplication.

**Parameters**:
- `output` *(int64_t\*)* - Output array
- `n` *(size_t)* - Number of elements to generate
- `start` *(int64_t)* - Initial value
- `factor` *(int64_t)* - Multiplier per step

**Returns**:
*(void)* - Result stored in `output`

**Algorithm**:
Loop with multiplication accumulator.

**Complexity**:
- Time: O(n)
- Space: O(1)

**Overflow Warning**:
Can overflow quickly for large factors or many iterations.

**Haskell Equivalent**:
```haskell
take n $ iterate (* factor) start
```

**Example**:
```c
int64_t output[6];
fp_iterate_mul_i64(output, 6, 2, 3);
// output = [2, 6, 18, 54, 162, 486]
//          2, 2×3, 2×3², 2×3³, ...
```

**Special Cases**:
```c
// Powers of 2
fp_iterate_mul_i64(output, 10, 1, 2);
// [1, 2, 4, 8, 16, 32, 64, 128, 256, 512]

// Powers of 10
fp_iterate_mul_i64(output, 5, 1, 10);
// [1, 10, 100, 1000, 10000]

// Repeated doubling
fp_iterate_mul_i64(output, 8, 1, 2);
```

**Applications**:
- Exponential growth modeling
- Powers generation
- Bit manipulation patterns (powers of 2)
- Geometric progressions

**See Also**:
- `fp_iterate_add_i64` - Arithmetic sequence

---

## fp_range_i64

**Signature**:
```c
size_t fp_range_i64(int64_t* output, int64_t start, int64_t end);
```

**Description**:
Generates integer range **[start, end)**: start inclusive, end exclusive. **Haskell's [start..end-1]**.

**Parameters**:
- `output` *(int64_t\*)* - Output array (space for at least end - start)
- `start` *(int64_t)* - Start value (inclusive)
- `end` *(int64_t)* - End value (exclusive)

**Returns**:
*(size_t)* - Number of elements = max(0, end - start)

**Algorithm**:
Simple counting loop.

**Complexity**:
- Time: O(end - start)
- Space: O(1)

**Empty Range**:
Returns 0 if start ≥ end.

**Haskell Equivalent**:
```haskell
[start..end-1]
-- or
[start..(end-1)]
```

**Example**:
```c
int64_t output[20];

size_t n = fp_range_i64(output, 5, 15);
// n = 10
// output = [5, 6, 7, 8, 9, 10, 11, 12, 13, 14]

// Empty range
n = fp_range_i64(output, 10, 5);
// n = 0 (start > end)
```

**Common Uses**:
```c
// Index array [0, 1, 2, ..., n-1]
fp_range_i64(indices, 0, n);

// Days of month [1..31]
fp_range_i64(days, 1, 32);

// Countdown (use iterate_add instead)
// For descending, use fp_iterate_add_i64(out, n, high, -1)
```

**Applications**:
- Index generation
- Enumeration
- Loop-free iteration patterns
- Test data

**See Also**:
- `fp_iterate_add_i64` - More flexible (supports step)

---

## fp_reduce_and_bool

**Signature**:
```c
bool fp_reduce_and_bool(const int64_t* input, size_t n);
```

**Description**:
Logical AND of all values (non-zero = true): **returns true iff all elements non-zero**. SIMD-accelerated with early exit.

**Parameters**:
- `input` *(const int64_t\*)* - Input array (0 = false, non-zero = true)
- `n` *(size_t)* - Number of elements

**Returns**:
*(bool)* - `true` if all non-zero, `false` if any zero. Returns `true` for empty arrays (vacuous truth).

**Algorithm**:
SIMD processes 4 elements per iteration. Uses `vpcmpeqq` to check for zeros. Early exit on first zero found.

**Complexity**:
- Time: O(n) worst case, O(1) best case (early exit)
- Space: O(1)

**Performance**:
~2.0-4.0x vs scalar (SIMD + early exit)

**Haskell Equivalent**:
```haskell
and $ map (/= 0) list
-- or
all (/= 0) list
```

**Example**:
```c
int64_t all_true[] = {1, 5, -3, 100};
int64_t has_false[] = {1, 5, 0, 100};

bool r1 = fp_reduce_and_bool(all_true, 4);   // true
bool r2 = fp_reduce_and_bool(has_false, 4);  // false (0 at index 2)

// Empty array
bool r3 = fp_reduce_and_bool(NULL, 0);  // true (vacuous)
```

**Applications**:
- Validation (all checks passed?)
- Logical conjunction
- "All conditions met" test
- Bit flags (all set?)

**Typical Validation Pattern**:
```c
// Check if all values are positive
int64_t checks[n];
for (size_t i = 0; i < n; i++) {
    checks[i] = (data[i] > 0) ? 1 : 0;
}
bool all_positive = fp_reduce_and_bool(checks, n);
```

**See Also**:
- `fp_reduce_or_bool` - Logical OR
- `fp_pred_all_eq_i64` - Universal equality test

---

## fp_reduce_or_bool

**Signature**:
```c
bool fp_reduce_or_bool(const int64_t* input, size_t n);
```

**Description**:
Logical OR of all values: **returns true if any element non-zero**. SIMD-accelerated with early exit.

**Parameters**:
- `input` *(const int64_t\*)* - Input array (0 = false, non-zero = true)
- `n` *(size_t)* - Number of elements

**Returns**:
*(bool)* - `true` if any non-zero, `false` if all zero. Returns `false` for empty arrays.

**Algorithm**:
SIMD processes 4 elements per iteration. Early exit on first non-zero.

**Complexity**:
- Time: O(n) worst case, O(1) best case
- Space: O(1)

**Performance**:
~2.0-4.0x vs scalar

**Haskell Equivalent**:
```haskell
or $ map (/= 0) list
-- or
any (/= 0) list
```

**Example**:
```c
int64_t all_false[] = {0, 0, 0, 0};
int64_t has_true[] = {0, 0, 5, 0};

bool r1 = fp_reduce_or_bool(all_false, 4);  // false
bool r2 = fp_reduce_or_bool(has_true, 4);   // true (5 at index 2)

// Empty array
bool r3 = fp_reduce_or_bool(NULL, 0);  // false
```

**Applications**:
- Existence check (any value satisfies?)
- Logical disjunction
- "At least one" test
- Error detection (any errors occurred?)

**Typical Pattern**:
```c
// Check if any values exceed threshold
int64_t flags[n];
for (size_t i = 0; i < n; i++) {
    flags[i] = (data[i] > THRESHOLD) ? 1 : 0;
}
bool has_anomaly = fp_reduce_or_bool(flags, n);
```

**See Also**:
- `fp_reduce_and_bool` - Logical AND
- `fp_pred_any_gt_i64` - Direct threshold test

---

## fp_zip_with_index_i64

**Signature**:
```c
size_t fp_zip_with_index_i64(const int64_t* input, int64_t* output, size_t n);
```

**Description**:
Pairs each element with its index: **output = [(0, input[0]), (1, input[1]), ...]**. Interleaved format.

**Parameters**:
- `input` *(const int64_t\*)* - Input array
- `output` *(int64_t\*)* - Output array (space for 2×n)
- `n` *(size_t)* - Number of input elements

**Returns**:
*(size_t)* - Output length = 2×n

**Output Format**:
Interleaved: `[idx0, val0, idx1, val1, idx2, val2, ...]`

**Algorithm**:
Simple loop emitting index-value pairs.

**Complexity**:
- Time: O(n)
- Space: O(1)

**Haskell Equivalent**:
```haskell
zip [0..] list
```

**Example**:
```c
int64_t input[] = {100, 200, 300, 400};
int64_t output[8];

size_t out_len = fp_zip_with_index_i64(input, output, 4);
// out_len = 8
// output = [0, 100,  1, 200,  2, 300,  3, 400]
//          idx val  idx val  idx val  idx val
```

**Applications**:
- Enumeration (track original positions)
- Creating (index, value) pairs for sorting
- Maintaining correspondence after transformations
- Preparing for grouped operations

**Example - Sort by value, keep indices**:
```c
int64_t data[] = {50, 20, 80, 10};
int64_t pairs[8];

fp_zip_with_index_i64(data, pairs, 4);
// pairs = [0,50, 1,20, 2,80, 3,10]

// Sort pairs by value (custom comparator on odd indices)
// After sort: [3,10, 1,20, 0,50, 2,80]
// Can retrieve original indices
```

**See Also**:
- `fp_range_i64` - Generate index array only

---

## fp_replicate_f64

**Signature**:
```c
void fp_replicate_f64(double* output, size_t n, double value);
```

**Description**:
Fills array with single double value using SIMD: **output[i] = value** for all i. **Optimized broadcast.**

**Parameters**:
- `output` *(double\*)* - Output array
- `n` *(size_t)* - Number of elements to fill
- `value` *(double)* - Value to replicate

**Returns**:
*(void)* - Result stored in `output`

**Algorithm**:
SIMD with `vbroadcastsd` and `vmovupd`. Processes 4 doubles per iteration.

**Complexity**:
- Time: O(n)
- Space: O(1)

**Performance**:
~1.5x vs scalar (SIMD broadcast advantage)

**Haskell Equivalent**:
```haskell
replicate n value
```

**Example**:
```c
double output[5];
fp_replicate_f64(output, 5, 3.14);
// output = [3.14, 3.14, 3.14, 3.14, 3.14]
```

**Applications**:
- Array initialization
- Constant vector creation
- Bias initialization in neural networks
- Fill operations

**See Also**:
- `fp_replicate_i64` - Integer version (Module 8)

---

## fp_count_i64

**Signature**:
```c
size_t fp_count_i64(const int64_t* input, size_t n, int64_t target);
```

**Description**:
Counts occurrences of value: **returns |{i : input[i] == target}|**. SIMD-optimized with POPCNT.

**Parameters**:
- `input` *(const int64_t\*)* - Input array
- `n` *(size_t)* - Number of elements
- `target` *(int64_t)* - Value to count

**Returns**:
*(size_t)* - Number of occurrences (0 ≤ result ≤ n)

**Algorithm**:
SIMD with `vpcmpeqq` (compare 4 values), `vmovmskpd` (extract mask), `popcnt` (count set bits).

**Complexity**:
- Time: O(n)
- Space: O(1)

**Performance**:
~1.5-2.0x vs scalar (SIMD + POPCNT)

**Haskell Equivalent**:
```haskell
length $ filter (== target) list
```

**Example**:
```c
int64_t data[] = {1, 5, 3, 5, 7, 5, 9};
size_t count = fp_count_i64(data, 7, 5);
// count = 3  (three occurrences of 5)
```

**Applications**:
- Frequency counting
- Membership counting
- Histogram bin counting
- Error/event counting

**See Also**:
- `fp_contains_i64` - Boolean existence check
- `fp_group_i64` - Count all unique values (after sorting)

---

## Quick Reference Tables

### Performance Summary

| Operation Type | Expected Speedup | Key Technique |
|----------------|------------------|---------------|
| Simple Reductions (f64) | 1.5-1.8x | SIMD, FMA |
| Simple Reductions (i64) | 1.0-1.02x | Scalar optimization |
| Fused Folds | 1.1-1.25x | Eliminate intermediate arrays |
| BLAS Level 1 | 1.0-1.1x | Memory bandwidth saturated |
| Simple Maps | 1.0-1.5x | SIMD where available |
| Predicates | 1.0x | Early exit, branch prediction |
| Sorting | 1.0-1.2x | Optimized quicksort |
| Set Operations | 1.5-2.0x | Linear merge algorithms |
| Boolean Reductions | 2.0-4.0x | SIMD + early exit |
| Unfold/Utilities | 1.0-2.0x | Tight loops, SIMD broadcasts |

### Data Type Availability

| Category | i64 | f64 | Notes |
|----------|-----|-----|-------|
| **Reductions** | ✅ | ✅ | |
| **Fused Folds** | ✅ | ✅ | f64 uses FMA |
| **BLAS Ops** | ✅ | ✅ | |
| **Maps** | ✅ | ✅ | |
| **Scans** | ✅ | ✅ | |
| **Predicates** | ✅ | ❌ | i64 only |
| **Filter/Partition** | ✅ | ✅ | |
| **List Ops** | ✅ | ✅ | |
| **Sorting** | ✅ | ✅ | |
| **Set Ops** | ✅ | ❌ | i64 only |
| **Advanced (TIER 3)** | ✅ | partial | See individual functions |

### Complexity Reference

| Pattern | Time | Space | Notes |
|---------|------|-------|-------|
| **Map** | O(n) | O(1) | Element-wise |
| **Fold** | O(n) | O(1) | Reduction |
| **Scan** | O(n) | O(1) | Sequential dependency |
| **Filter** | O(n) | O(1) | Output size variable |
| **Sort** | O(n log n) avg | O(log n) | Quicksort |
| **Set Ops** | O(n+m) | O(1) | On sorted inputs |
| **Search** | O(n) | O(1) | Early exit possible |
| **Group** | O(n) | O(1) | Single pass |

---

## Integration Examples

### Example 1: Complete Statistical Analysis

```c
#include "fp_core.h"
#include <stdio.h>
#include <math.h>

void analyze_dataset(double* data, size_t n) {
    // Make a copy for sorting (preserve original)
    double sorted[n];
    memcpy(sorted, data, n * sizeof(double));

    // 1. Basic statistics
    double sum = fp_reduce_add_f64(data, n);
    double mean = sum / n;
    printf("Mean: %.2f\n", mean);

    // 2. Sum of squares for variance
    double sumsq = fp_fold_sumsq_f64(data, n);  // Note: Use f64 version
    double variance = (sumsq / n) - (mean * mean);
    printf("Variance: %.2f\n", variance);
    printf("Std Dev: %.2f\n", sqrt(variance));

    // 3. Percentiles (requires sorting)
    fp_sort_f64(sorted, n);
    printf("Min: %.2f\n", sorted[0]);
    printf("Q1 (25%%): %.2f\n", sorted[n/4]);
    printf("Median: %.2f\n", sorted[n/2]);
    printf("Q3 (75%%): %.2f\n", sorted[3*n/4]);
    printf("Max: %.2f\n", sorted[n-1]);

    // 4. Outlier detection (IQR method)
    double q1 = sorted[n/4];
    double q3 = sorted[3*n/4];
    double iqr = q3 - q1;
    double lower = q1 - 1.5 * iqr;
    double upper = q3 + 1.5 * iqr;

    size_t outliers = 0;
    for (size_t i = 0; i < n; i++) {
        if (data[i] < lower || data[i] > upper) outliers++;
    }
    printf("Outliers: %zu (%.1f%%)\n", outliers, (outliers * 100.0) / n);
}
```

### Example 2: Set Operations Pipeline

```c
void find_common_customers(int64_t* store_a, size_t na,
                          int64_t* store_b, size_t nb) {
    // Sort both lists
    fp_sort_i64(store_a, na);
    fp_sort_i64(store_b, nb);

    // Remove duplicates
    int64_t unique_a[na], unique_b[nb];
    size_t n_a = fp_unique_i64(store_a, unique_a, na);
    size_t n_b = fp_unique_i64(store_b, unique_b, nb);

    // Find intersection (common customers)
    int64_t common[n_a];
    size_t n_common = fp_intersect_i64(unique_a, unique_b, common, n_a, n_b);

    // Find union (all customers)
    int64_t all[n_a + n_b];
    size_t n_all = fp_union_i64(unique_a, unique_b, all, n_a, n_b);

    printf("Store A: %zu unique customers\n", n_a);
    printf("Store B: %zu unique customers\n", n_b);
    printf("Common: %zu customers\n", n_common);
    printf("Total: %zu unique customers\n", n_all);
}
```

### Example 3: Data Validation Pipeline

```c
bool validate_sensor_data(double* readings, size_t n) {
    // 1. Check all values are positive
    int64_t positive_checks[n];
    for (size_t i = 0; i < n; i++) {
        positive_checks[i] = (readings[i] > 0.0) ? 1 : 0;
    }
    bool all_positive = fp_reduce_and_bool(positive_checks, n);

    if (!all_positive) {
        printf("ERROR: Found negative readings\n");
        return false;
    }

    // 2. Check if any exceed maximum threshold
    int64_t exceed_checks[n];
    for (size_t i = 0; i < n; i++) {
        exceed_checks[i] = (readings[i] > MAX_READING) ? 1 : 0;
    }
    bool has_exceed = fp_reduce_or_bool(exceed_checks, n);

    if (has_exceed) {
        size_t count = fp_count_i64(exceed_checks, n, 1);
        printf("WARNING: %zu readings exceed maximum\n", count);
    }

    // 3. Validate range
    double min = fp_reduce_min_f64(readings, n);  // Note: Use min variant
    double max = fp_reduce_max_f64(readings, n);
    printf("Range: [%.2f, %.2f]\n", min, max);

    return true;
}
```

---

## Conclusion

This API reference documents all 36 functions in the FP-ASM library, providing complete information for production use. Each function includes:

- Complete signature
- Detailed parameter descriptions
- Return value semantics
- Algorithm description
- Complexity analysis
- Performance expectations
- Haskell equivalents
- Working examples
- Real-world applications
- Cross-references

**The FP-ASM library provides a complete, high-performance functional programming toolkit for C, enabling expressive code with measurable performance advantages.**

For questions or issues, refer to:
- `COMPLETE_LIBRARY_REPORT.md` - Overall library documentation
- `TIER2_COMPLETENESS_REPORT.md` - TIER 2 operations details
- Individual module assembly files for implementation details

---

*API Reference Version: 1.0.0*
*Last Updated: October 28, 2025*
*Library Completeness: 100%*
