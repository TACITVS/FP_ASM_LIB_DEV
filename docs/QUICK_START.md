# FP-ASM Quick Start Guide

**Learn the FP-ASM library through progressive examples - from basics to advanced patterns.**

---

## Table of Contents

1. [Installation](#installation)
2. [Your First Program](#your-first-program)
3. [Basic Operations](#basic-operations)
4. [List Operations](#list-operations)
5. [Sorting & Sets](#sorting--sets)
6. [Advanced Patterns](#advanced-patterns)
7. [Real-World Applications](#real-world-applications)
8. [Performance Tips](#performance-tips)

---

## Installation

### Step 1: Get the Library

All object files (`*.o`) are pre-compiled and ready to use. No assembly required!

### Step 2: Include the Header

```c
#include "fp_core.h"
```

### Step 3: Link and Compile

```bash
gcc your_program.c fp_core_*.o -o your_program.exe
```

That's it! You're ready to go.

---

## Your First Program

Let's start with the simplest possible program:

```c
#include <stdio.h>
#include "fp_core.h"

int main() {
    // Create an array
    int64_t numbers[] = {1, 2, 3, 4, 5};

    // Sum it (1.5-1.8x faster than C loop)
    int64_t sum = fp_reduce_add_i64(numbers, 5);

    printf("Sum: %lld\n", sum);  // Output: Sum: 15
    return 0;
}
```

**Compile and run**:
```bash
gcc hello_fp.c fp_core_reductions.o -o hello_fp.exe
./hello_fp.exe
```

**What just happened?**
- You called `fp_reduce_add_i64` to sum an int64_t array
- The function used AVX2 SIMD instructions internally
- Result: 1.5-1.8x faster than equivalent C code!

---

## Basic Operations

### Lesson 1: Reductions (Fold Operations)

Reductions collapse an array into a single value.

```c
#include <stdio.h>
#include "fp_core.h"

int main() {
    double temperatures[] = {23.5, 19.2, 25.8, 21.3, 27.1};
    size_t n = 5;

    // 1. Sum all temperatures
    double total = fp_reduce_add_f64(temperatures, n);
    printf("Total: %.1f°C\n", total);

    // 2. Find maximum
    double max_temp = fp_reduce_max_f64(temperatures, n);
    printf("Max temperature: %.1f°C\n", max_temp);

    // 3. Calculate mean
    double mean = total / n;
    printf("Average: %.1f°C\n", mean);

    return 0;
}
```

**Key Takeaway**: Use `*_i64` for integers, `*_f64` for doubles.

---

### Lesson 2: Fused Operations (Map + Reduce)

Fused operations combine multiple steps into one pass for better performance.

```c
#include <stdio.h>
#include <math.h>
#include "fp_core.h"

int main() {
    double values[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    size_t n = 5;

    // Calculate variance in one pass!
    double sum = fp_reduce_add_f64(values, n);
    double mean = sum / n;

    // Sum of squares (fused operation)
    double sumsq = fp_fold_sumsq_f64(values, n);

    // Variance formula
    double variance = (sumsq / n) - (mean * mean);
    double std_dev = sqrt(variance);

    printf("Mean: %.2f\n", mean);
    printf("Variance: %.2f\n", variance);
    printf("Std Dev: %.2f\n", std_dev);

    return 0;
}
```

**Why it's fast**: `fp_fold_sumsq_f64` computes Σ(x²) in a single pass, keeping all data in registers!

---

### Lesson 3: Transformations (Map Operations)

Transform every element of an array.

```c
#include <stdio.h>
#include "fp_core.h"

int main() {
    double readings[] = {-2.5, 3.7, -1.2, 5.8, -0.3};
    double absolute[5];
    size_t n = 5;

    // Get absolute values
    fp_map_abs_f64(readings, absolute, n);

    printf("Original: ");
    for (size_t i = 0; i < n; i++) printf("%.1f ", readings[i]);

    printf("\nAbsolute: ");
    for (size_t i = 0; i < n; i++) printf("%.1f ", absolute[i]);
    printf("\n");

    return 0;
}
```

**Output**:
```
Original: -2.5 3.7 -1.2 5.8 -0.3
Absolute: 2.5 3.7 1.2 5.8 0.3
```

---

### Lesson 4: Scans (Prefix Sums)

Scans compute cumulative results - perfect for running totals.

```c
#include <stdio.h>
#include "fp_core.h"

int main() {
    int64_t monthly_sales[] = {100, 150, 120, 180, 200};
    int64_t cumulative[5];
    size_t n = 5;

    // Cumulative sum (2.0-3.2x faster!)
    fp_scan_add_i64(monthly_sales, cumulative, n);

    printf("Month | Sales | Year-to-Date\n");
    printf("------+-------+-------------\n");
    for (size_t i = 0; i < n; i++) {
        printf("  %zu   | %5lld | %5lld\n",
               i+1, monthly_sales[i], cumulative[i]);
    }

    return 0;
}
```

**Output**:
```
Month | Sales | Year-to-Date
------+-------+-------------
  1   |   100 |   100
  2   |   150 |   250
  3   |   120 |   370
  4   |   180 |   550
  5   |   200 |   750
```

---

## List Operations

### Lesson 5: Filtering

Select elements that match a condition.

```c
#include <stdio.h>
#include <stdbool.h>
#include "fp_core.h"

// Predicate function
bool is_positive(int64_t x) {
    return x > 0;
}

int main() {
    int64_t data[] = {-3, 5, -1, 8, 0, 2, -7};
    int64_t positive[7];
    size_t n = 7;

    // Filter for positive numbers
    size_t count = fp_filter_i64(data, positive, n, is_positive);

    printf("Original: ");
    for (size_t i = 0; i < n; i++) printf("%lld ", data[i]);

    printf("\nPositive only (%zu values): ", count);
    for (size_t i = 0; i < count; i++) printf("%lld ", positive[i]);
    printf("\n");

    return 0;
}
```

**Output**:
```
Original: -3 5 -1 8 0 2 -7
Positive only (3 values): 5 8 2
```

---

### Lesson 6: Slicing and Taking

Extract portions of arrays.

```c
#include <stdio.h>
#include "fp_core.h"

void print_array(const char* name, int64_t* arr, size_t n) {
    printf("%s: ", name);
    for (size_t i = 0; i < n; i++) printf("%lld ", arr[i]);
    printf("\n");
}

int main() {
    int64_t data[] = {10, 20, 30, 40, 50, 60, 70, 80};
    int64_t output[8];

    // Take first 3
    fp_take_i64(output, data, 3);
    print_array("First 3", output, 3);

    // Skip first 3, take rest
    fp_drop_i64(output, data, 3, 8);
    print_array("After drop 3", output, 5);

    // Slice [2..5)
    fp_slice_i64(output, data, 2, 5);
    print_array("Slice [2..5)", output, 3);

    return 0;
}
```

**Output**:
```
First 3: 10 20 30
After drop 3: 40 50 60 70 80
Slice [2..5): 30 40 50
```

---

## Sorting & Sets

### Lesson 7: Sorting and Statistics

```c
#include <stdio.h>
#include <string.h>
#include "fp_core.h"

int main() {
    double data[] = {25.5, 19.2, 31.8, 22.3, 28.7};
    size_t n = 5;

    // Make a copy (sorting is in-place)
    double sorted[5];
    memcpy(sorted, data, n * sizeof(double));

    // Sort (1.0-1.2x faster than qsort)
    fp_sort_f64(sorted, n);

    printf("Original: ");
    for (size_t i = 0; i < n; i++) printf("%.1f ", data[i]);

    printf("\nSorted: ");
    for (size_t i = 0; i < n; i++) printf("%.1f ", sorted[i]);

    printf("\n\nStatistics:\n");
    printf("Min: %.1f\n", sorted[0]);
    printf("Q1:  %.1f\n", sorted[n/4]);
    printf("Median: %.1f\n", sorted[n/2]);
    printf("Q3:  %.1f\n", sorted[3*n/4]);
    printf("Max: %.1f\n", sorted[n-1]);

    return 0;
}
```

---

### Lesson 8: Set Operations

Work with sets using sorted arrays.

```c
#include <stdio.h>
#include "fp_core.h"

void print_set(const char* name, int64_t* arr, size_t n) {
    printf("%s: {", name);
    for (size_t i = 0; i < n; i++) {
        printf("%lld", arr[i]);
        if (i < n-1) printf(", ");
    }
    printf("}\n");
}

int main() {
    int64_t a[] = {1, 3, 5, 7, 9};
    int64_t b[] = {2, 3, 5, 8, 9};
    size_t na = 5, nb = 5;

    // Both arrays must be sorted!
    fp_sort_i64(a, na);
    fp_sort_i64(b, nb);

    // Union (all unique elements)
    int64_t union_result[10];
    size_t n_union = fp_union_i64(a, b, union_result, na, nb);
    print_set("A ∪ B", union_result, n_union);

    // Intersection (common elements)
    int64_t intersect_result[5];
    size_t n_intersect = fp_intersect_i64(a, b, intersect_result, na, nb);
    print_set("A ∩ B", intersect_result, n_intersect);

    return 0;
}
```

**Output**:
```
A ∪ B: {1, 2, 3, 5, 7, 8, 9}
A ∩ B: {3, 5, 9}
```

---

## Advanced Patterns

### Lesson 9: Grouping and Run-Length Encoding

```c
#include <stdio.h>
#include "fp_core.h"

int main() {
    // Simulate image scanline with repeated pixels
    int64_t pixels[] = {255,255,255,255, 0,0,0, 128,128, 255,255};
    size_t n = 11;

    printf("Original pixels (%zu): ", n);
    for (size_t i = 0; i < n; i++) printf("%lld ", pixels[i]);
    printf("\n");

    // Method 1: Group (returns parallel arrays)
    int64_t groups[11], counts[11];
    size_t ng = fp_group_i64(pixels, groups, counts, n);

    printf("\nGrouped:\n");
    for (size_t i = 0; i < ng; i++) {
        printf("  Value %lld appears %lld times\n", groups[i], counts[i]);
    }

    // Method 2: Run-length encoding (interleaved)
    int64_t compressed[22];
    size_t comp_len = fp_run_length_encode_i64(pixels, compressed, n);

    printf("\nRLE compressed (%zu elements):\n  ", comp_len);
    for (size_t i = 0; i < comp_len; i += 2) {
        printf("[%lld×%lld] ", compressed[i+1], compressed[i]);
    }
    printf("\n");

    printf("\nCompression: %.0f%% of original size\n",
           (comp_len * 100.0) / n);

    return 0;
}
```

---

### Lesson 10: Sequence Generation

```c
#include <stdio.h>
#include "fp_core.h"

void print_seq(const char* name, int64_t* arr, size_t n) {
    printf("%s: ", name);
    for (size_t i = 0; i < n; i++) printf("%lld ", arr[i]);
    printf("\n");
}

int main() {
    int64_t seq[10];

    // Arithmetic sequence (start=5, step=3)
    fp_iterate_add_i64(seq, 10, 5, 3);
    print_seq("Arithmetic [5, +3]", seq, 10);

    // Geometric sequence (start=2, factor=3)
    fp_iterate_mul_i64(seq, 6, 2, 3);
    print_seq("Geometric [2, ×3] ", seq, 6);

    // Range [10..20)
    size_t n = fp_range_i64(seq, 10, 20);
    print_seq("Range [10..20)    ", seq, n);

    // Powers of 2
    fp_iterate_mul_i64(seq, 10, 1, 2);
    print_seq("Powers of 2       ", seq, 10);

    return 0;
}
```

**Output**:
```
Arithmetic [5, +3]: 5 8 11 14 17 20 23 26 29 32
Geometric [2, ×3]:  2 6 18 54 162 486
Range [10..20):     10 11 12 13 14 15 16 17 18 19
Powers of 2:        1 2 4 8 16 32 64 128 256 512
```

---

### Lesson 11: Boolean Operations

```c
#include <stdio.h>
#include <stdbool.h>
#include "fp_core.h"

int main() {
    // Data validation: check sensor readings
    int64_t sensors[] = {1, 1, 1, 1, 1};  // 1 = OK, 0 = Error

    // Check if ALL sensors are OK (AND)
    bool all_ok = fp_reduce_and_bool(sensors, 5);
    printf("All sensors OK: %s\n", all_ok ? "YES" : "NO");

    // Simulate one failure
    sensors[2] = 0;

    all_ok = fp_reduce_and_bool(sensors, 5);
    printf("After failure, all OK: %s\n", all_ok ? "YES" : "NO");

    // Check if ANY sensor failed (OR on error flags)
    int64_t errors[] = {0, 0, 1, 0, 0};  // 1 = Error
    bool has_error = fp_reduce_or_bool(errors, 5);
    printf("Has errors: %s\n", has_error ? "YES" : "NO");

    // Count total errors
    size_t error_count = fp_count_i64(sensors, 5, 0);
    printf("Total failures: %zu\n", error_count);

    return 0;
}
```

---

## Real-World Applications

### Application 1: Finding the Mode

The most frequent value in a dataset.

```c
#include <stdio.h>
#include "fp_core.h"

int64_t find_mode(int64_t* data, size_t n) {
    // Sort data first
    fp_sort_i64(data, n);

    // Group consecutive equal values
    int64_t groups[n], counts[n];
    size_t ng = fp_group_i64(data, groups, counts, n);

    // Find maximum count
    int64_t max_count = fp_reduce_max_i64(counts, ng);

    // Find which group has that count
    int64_t max_idx = fp_find_index_i64(counts, ng, max_count);

    // Return the value (mode)
    return groups[max_idx];
}

int main() {
    int64_t votes[] = {1, 2, 1, 3, 1, 2, 1, 4};
    size_t n = 8;

    int64_t mode = find_mode(votes, n);

    printf("Votes: ");
    for (size_t i = 0; i < n; i++) printf("%lld ", votes[i]);
    printf("\nMode (most common): %lld\n", mode);

    return 0;
}
```

---

### Application 2: Outlier Detection

Remove values outside acceptable range.

```c
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "fp_core.h"

void detect_outliers(double* data, size_t n) {
    // Make a sorted copy
    double sorted[n];
    memcpy(sorted, data, n * sizeof(double));
    fp_sort_f64(sorted, n);

    // Calculate IQR (Interquartile Range)
    double q1 = sorted[n/4];
    double q3 = sorted[3*n/4];
    double iqr = q3 - q1;

    // Outlier boundaries: Q1 - 1.5×IQR, Q3 + 1.5×IQR
    double lower = q1 - 1.5 * iqr;
    double upper = q3 + 1.5 * iqr;

    printf("Q1: %.2f, Q3: %.2f, IQR: %.2f\n", q1, q3, iqr);
    printf("Outlier range: [%.2f, %.2f]\n\n", lower, upper);

    printf("Value | Status\n");
    printf("------+--------\n");
    for (size_t i = 0; i < n; i++) {
        if (data[i] < lower || data[i] > upper) {
            printf("%5.1f | OUTLIER\n", data[i]);
        } else {
            printf("%5.1f | OK\n", data[i]);
        }
    }
}

int main() {
    double data[] = {10.2, 11.5, 12.1, 11.8, 50.0, 12.3, 11.9};
    detect_outliers(data, 7);
    return 0;
}
```

---

## Performance Tips

### Tip 1: Fuse Operations When Possible

**Slow** (two passes):
```c
// Pass 1: Square each element
for (int i = 0; i < n; i++) temp[i] = data[i] * data[i];
// Pass 2: Sum
for (int i = 0; i < n; i++) sum += temp[i];
```

**Fast** (one pass):
```c
// Single fused operation
int64_t sumsq = fp_fold_sumsq_i64(data, n);
```

---

### Tip 2: Sort Once, Use Many Times

```c
// Sort once
fp_sort_f64(data, n);

// Now you can cheaply:
double median = data[n/2];
double min = data[0];
double max = data[n-1];
double q1 = data[n/4];
double q3 = data[3*n/4];

// And use set operations
size_t unique_count = fp_unique_i64(data, unique, n);
```

---

### Tip 3: Choose the Right Data Type

- Use `int64_t` for exact calculations (counts, IDs, indices)
- Use `double` for measurements and floating-point math
- Performance is similar, but semantics matter!

---

### Tip 4: Pre-allocate Output Buffers

```c
// Worst case: all elements match
int64_t filtered[n];
size_t count = fp_filter_i64(input, filtered, n, predicate);
// count ≤ n
```

Always allocate enough space for worst case!

---

## Next Steps

1. **Read the [API_REFERENCE.md](API_REFERENCE.md)** for complete function documentation
2. **Check [EXAMPLES.md](EXAMPLES.md)** for more complete programs
3. **See [COMPLETE_LIBRARY_REPORT.md](COMPLETE_LIBRARY_REPORT.md)** for performance details

---

## Quick Reference Card

| Task | Function | Example |
|------|----------|---------|
| **Sum** | `fp_reduce_add_*` | `sum = fp_reduce_add_i64(arr, n)` |
| **Max** | `fp_reduce_max_*` | `max = fp_reduce_max_f64(arr, n)` |
| **Dot product** | `fp_fold_dotp_*` | `dot = fp_fold_dotp_f64(a, b, n)` |
| **Prefix sum** | `fp_scan_add_*` | `fp_scan_add_i64(in, out, n)` |
| **Sort** | `fp_sort_*` | `fp_sort_f64(arr, n)` |
| **Filter** | `fp_filter_*` | `n2 = fp_filter_i64(in, out, n, pred)` |
| **Group** | `fp_group_*` | `ng = fp_group_i64(in, g, c, n)` |
| **Range** | `fp_range_*` | `n = fp_range_i64(out, 0, 100)` |
| **All true** | `fp_reduce_and_bool` | `ok = fp_reduce_and_bool(arr, n)` |
| **Count** | `fp_count_*` | `n = fp_count_i64(arr, n, val)` |

---

**Congratulations! You now know how to use the FP-ASM library!** 🎉

Start building high-performance functional programs in C today.
