# FP-ASM Generic Type System

## Overview

The Generic Type System extends FP-ASM to work with **ANY data type**, not just `i64` and `f64`. This answers your question: "Can we write versions of this library functions that will work with other types and still stick firmly to our pure FP values?"

**Answer: YES! Absolutely!**

The generic type system enables:
- ✅ Quick sort for **any comparable type** (int, double, strings, custom structs)
- ✅ Map/filter/fold over **arbitrary data structures**
- ✅ Functional composition with **non-numeric types**
- ✅ **100% functional purity maintained** (immutability, no side effects)

---

## ⚠️ CRITICAL: Performance Trade-offs

### Generic Functions Are SLOWER Than Specialized Assembly

**Generic functions trade performance for flexibility:**

| Aspect | Generic C | Specialized Assembly (i64/f64) |
|--------|-----------|--------------------------------|
| **Performance** | ~1.0x vs gcc -O3 (maybe slower) | 1.5-3.5x faster than gcc -O3 |
| **Optimization** | Pure C, function pointers | Hand-optimized AVX2 SIMD |
| **Types** | ANY type | i64, f64 only |
| **Use case** | Flexibility, non-numeric types | Performance-critical numeric ops |

**Why generic is slower:**
- ❌ No AVX2 SIMD instructions (scalar operations only)
- ❌ Function pointer overhead (prevents inlining)
- ❌ Extra memcpy for immutability
- ❌ Type erasure overhead (void* + size_t)

**When to use generic:**
- ✅ Non-numeric types (structs, strings)
- ✅ Small arrays (< 10,000 elements)
- ✅ Non-critical paths
- ✅ Flexibility needed (custom logic, prototyping)

**When to use specialized:**
- ✅ i64/f64 numeric operations
- ✅ Large arrays (> 10,000 elements)
- ✅ Hot paths (called frequently)
- ✅ Performance-critical code

**See `docs/PERFORMANCE_TIERS.md` for detailed analysis.**

---

## Design Philosophy

### Key Principles

1. **Type Erasure with `void*`**
   - Functions accept `const void*` for inputs
   - Use `size_t elem_size` to handle arbitrary element sizes
   - User provides type-specific callbacks

2. **Zero-Heap Allocation (User-Controlled)**
   - User provides output buffers
   - No hidden malloc/free in generic HOFs
   - Sorting functions use temporary buffers (provided or allocated once)

3. **Functional Purity Guaranteed**
   - All inputs are `const void*` (immutable)
   - No global state
   - No side effects in core functions
   - Deterministic execution

4. **Callback-Based Operations**
   - User provides comparison functions for sorting
   - User provides predicate functions for filtering
   - User provides transform functions for mapping
   - Context parameter for "closures" in C

---

## API Overview

### Category 12: Generic Higher-Order Functions

```c
/* Generic fold left (reduce) */
void fp_foldl_generic(const void* input, size_t n, size_t elem_size,
                      void* acc,
                      void (*fn)(void* acc, const void* elem, void* ctx),
                      void* context);

/* Generic map (transform) */
void fp_map_generic(const void* input, void* output, size_t n,
                    size_t in_size, size_t out_size,
                    void (*fn)(void* out, const void* in, void* ctx),
                    void* context);

/* Generic filter (select) */
size_t fp_filter_generic(const void* input, void* output, size_t n,
                         size_t elem_size,
                         bool (*predicate)(const void* elem, void* ctx),
                         void* context);

/* Generic zipWith (combine two arrays) */
void fp_zipWith_generic(const void* input_a, const void* input_b, void* output, size_t n,
                        size_t size_a, size_t size_b, size_t size_c,
                        void (*fn)(void* out, const void* a, const void* b, void* ctx),
                        void* context);
```

### Category 13: Generic Sorting

```c
/* Comparison function signature */
typedef int (*fp_compare_fn)(const void* a, const void* b, void* context);

/* Generic Quick Sort (O(n log n) average, O(n²) worst) */
void fp_quicksort_generic(const void* input, void* output, size_t n,
                          size_t elem_size,
                          fp_compare_fn compare,
                          void* context);

/* Generic Merge Sort (O(n log n) guaranteed, stable) */
void fp_mergesort_generic(const void* input, void* output, size_t n,
                          size_t elem_size,
                          fp_compare_fn compare,
                          void* context,
                          void* temp);
```

### Category 14: Generic List Operations

```c
/* Partition (split by predicate) */
void fp_partition_generic(...);

/* Take first n elements */
void fp_take_generic(...);

/* Drop first n elements */
size_t fp_drop_generic(...);

/* Reverse array */
void fp_reverse_generic(...);

/* Find first matching element */
bool fp_find_generic(...);
```

---

## Usage Examples

### Example 1: Quick Sort for Integers

```c
#include "include/fp_generic.h"

int compare_ints(const void* a, const void* b, void* ctx) {
    (void)ctx;  /* Unused */
    return *(const int*)a - *(const int*)b;
}

int main() {
    int data[] = {64, 34, 25, 12, 22, 11, 90};
    int sorted[7];
    size_t n = 7;

    /* Sort using generic quick sort */
    FP_QUICKSORT(int, data, sorted, n, compare_ints, NULL);

    /* data is UNCHANGED (immutable) */
    /* sorted contains: 11, 12, 22, 25, 34, 64, 90 */
}
```

### Example 2: Quick Sort for Strings

```c
int compare_strings(const void* a, const void* b, void* ctx) {
    (void)ctx;
    const char* str_a = *(const char**)a;
    const char* str_b = *(const char**)b;
    return strcmp(str_a, str_b);
}

int main() {
    const char* languages[] = {"Haskell", "Lisp", "ML", "C", "Assembly"};
    const char* sorted[5];

    FP_QUICKSORT(const char*, languages, sorted, 5, compare_strings, NULL);

    /* sorted: "Assembly", "C", "Haskell", "Lisp", "ML" */
}
```

### Example 3: Quick Sort for Custom Struct

```c
typedef struct {
    int id;
    double score;
    const char* name;
} Student;

int compare_students_by_score(const void* a, const void* b, void* ctx) {
    (void)ctx;
    const Student* s1 = (const Student*)a;
    const Student* s2 = (const Student*)b;
    double diff = s1->score - s2->score;
    return (diff > 0) ? 1 : (diff < 0) ? -1 : 0;
}

int main() {
    Student students[] = {
        {101, 85.5, "Alice"},
        {102, 92.0, "Bob"},
        {103, 78.3, "Charlie"}
    };
    Student sorted[3];

    FP_QUICKSORT(Student, students, sorted, 3, compare_students_by_score, NULL);

    /* sorted by score: Charlie (78.3), Alice (85.5), Bob (92.0) */
}
```

### Example 4: Generic Map (Extract Field)

```c
void extract_score(void* out, const void* in, void* ctx) {
    (void)ctx;
    *(double*)out = ((const Student*)in)->score;
}

int main() {
    Student students[] = {
        {101, 85.5, "Alice"},
        {102, 92.0, "Bob"},
        {103, 78.3, "Charlie"}
    };
    double scores[3];

    /* Extract scores from students */
    FP_MAP(Student, double, students, scores, 3, extract_score, NULL);

    /* scores: [85.5, 92.0, 78.3] */
}
```

### Example 5: Generic Filter with Threshold

```c
bool high_score_predicate(const void* elem, void* ctx) {
    double threshold = *(double*)ctx;
    return ((const Student*)elem)->score >= threshold;
}

int main() {
    Student students[] = {
        {101, 85.5, "Alice"},
        {102, 92.0, "Bob"},
        {103, 78.3, "Charlie"}
    };
    Student high_scorers[3];
    double threshold = 90.0;

    /* Filter students with score >= 90.0 */
    size_t count = FP_FILTER(Student, students, high_scorers, 3,
                             high_score_predicate, &threshold);

    /* count = 1, high_scorers: [Bob (92.0)] */
}
```

### Example 6: Generic Fold (Sum Field)

```c
void sum_scores(void* acc, const void* elem, void* ctx) {
    (void)ctx;
    *(double*)acc += ((const Student*)elem)->score;
}

int main() {
    Student students[] = {
        {101, 85.5, "Alice"},
        {102, 92.0, "Bob"},
        {103, 78.3, "Charlie"}
    };
    double total = 0.0;

    fp_foldl_generic(students, 3, sizeof(Student), &total, sum_scores, NULL);

    /* total = 255.8 */
}
```

### Example 7: Generic ZipWith (Join Structs)

```c
typedef struct {
    int person_id;
    const char* name;
} Person;

typedef struct {
    int job_id;
    double salary;
} Job;

typedef struct {
    int id;
    const char* name;
    double salary;
} Employee;

void join_person_job(void* out, const void* a, const void* b, void* ctx) {
    (void)ctx;
    Employee* emp = (Employee*)out;
    const Person* person = (const Person*)a;
    const Job* job = (const Job*)b;

    emp->id = person->person_id;
    emp->name = person->name;
    emp->salary = job->salary;
}

int main() {
    Person persons[] = {
        {101, "Alice"},
        {102, "Bob"}
    };

    Job jobs[] = {
        {201, 75000.0},
        {202, 82000.0}
    };

    Employee employees[2];

    fp_zipWith_generic(persons, jobs, employees, 2,
                       sizeof(Person), sizeof(Job), sizeof(Employee),
                       join_person_job, NULL);

    /* employees: [
         {101, "Alice", 75000.0},
         {102, "Bob", 82000.0}
       ] */
}
```

### Example 8: Functional Composition

**Task**: Filter high scorers, extract scores, sort them

```c
int main() {
    Student students[] = {
        {101, 85.5, "Alice"},
        {102, 92.0, "Bob"},
        {103, 78.3, "Charlie"},
        {104, 95.7, "Diana"},
        {105, 88.9, "Eve"}
    };
    double threshold = 85.0;

    /* Step 1: Filter high scorers */
    Student high_scorers[5];
    size_t count = FP_FILTER(Student, students, high_scorers, 5,
                             high_score_predicate, &threshold);

    /* Step 2: Extract scores */
    double scores[5];
    FP_MAP(Student, double, high_scorers, scores, count, extract_score, NULL);

    /* Step 3: Sort scores */
    double sorted_scores[5];
    FP_QUICKSORT(double, scores, sorted_scores, count, compare_doubles, NULL);

    /* Result: sorted scores of students with score >= 85.0 */
}
```

---

## Comparison with Haskell

### Haskell Code

```haskell
data Student = Student { studentId :: Int, score :: Double, name :: String }

-- Filter high scorers, extract scores, sort
processStudents :: [Student] -> Double -> [Double]
processStudents students threshold =
    let highScorers = filter (\s -> score s >= threshold) students
        scores = map score highScorers
        sorted = sort scores
    in sorted
```

### FP-ASM Generic Code (C)

```c
typedef struct {
    int id;
    double score;
    const char* name;
} Student;

double* process_students(Student* students, size_t n, double threshold, size_t* result_count) {
    /* Filter */
    Student high_scorers[100];
    size_t count = FP_FILTER(Student, students, high_scorers, n,
                             high_score_predicate, &threshold);

    /* Map */
    double scores[100];
    FP_MAP(Student, double, high_scorers, scores, count, extract_score, NULL);

    /* Sort */
    double sorted[100];
    FP_QUICKSORT(double, scores, sorted, count, compare_doubles, NULL);

    *result_count = count;
    return sorted;  /* Note: should malloc/copy in real code */
}
```

**Result**: Near-identical structure! FP-ASM achieves Haskell-level expressiveness in C.

---

## Purity Guarantees

### Input Immutability

```c
const void* input  /* NEVER modified */
```

All input pointers are `const void*`, enforcing immutability at the type level.

### No Side Effects

- No global state access
- No I/O operations
- No hidden malloc/free (except internally in quicksort for temp buffer)
- All state passed explicitly via context parameter

### Deterministic Execution

Same inputs → Same outputs, always.

```c
/* This is GUARANTEED to produce identical results every time */
int data[] = {5, 2, 8, 1, 9};
int sorted1[5], sorted2[5];

FP_QUICKSORT(int, data, sorted1, 5, compare_ints, NULL);
FP_QUICKSORT(int, data, sorted2, 5, compare_ints, NULL);

/* sorted1 == sorted2, ALWAYS */
```

---

## Performance Characteristics

| Operation | Time Complexity | Space Complexity | Notes |
|-----------|----------------|------------------|-------|
| `fp_foldl_generic` | O(n) | O(1) | Single pass |
| `fp_map_generic` | O(n) | O(n) | Output buffer |
| `fp_filter_generic` | O(n) | O(n) | Worst case: all pass |
| `fp_zipWith_generic` | O(n) | O(n) | Output buffer |
| `fp_quicksort_generic` | O(n log n) avg, O(n²) worst | O(log n) stack | In-place after copy |
| `fp_mergesort_generic` | O(n log n) guaranteed | O(n) | Requires temp buffer |
| `fp_partition_generic` | O(n) | O(n) | Two output buffers |
| `fp_reverse_generic` | O(n) | O(n) | Output buffer |
| `fp_find_generic` | O(n) worst | O(1) | Early exit possible |

---

## Build Instructions

### Compile Generic Module

```bash
gcc -c src/wrappers/fp_generic.c -o build/obj/fp_generic.o -I include
```

### Build Test Suite

```bash
gcc test_generic.c build/obj/fp_generic.o -o test_generic.exe -I include
```

### Run Tests

```bash
./test_generic.exe
```

Expected output:
```
╔════════════════════════════════════════════════════════════════╗
║   FP-ASM GENERIC TYPE SYSTEM TEST SUITE                      ║
║   Functional Programming for ANY Type in C                   ║
╚════════════════════════════════════════════════════════════════╝

=== TEST 1: Quick Sort (Integers) ===
Original: 64 34 25 12 22 11 90 88 45 50
Sorted:   11 12 22 25 34 45 50 64 88 90
Result: PASSED ✓

=== TEST 2: Quick Sort (Doubles) ===
...

=== TEST 12: Functional Composition ===
Step 1 - Filter (score >= 85.0): 4 students
Step 2 - Extract scores: 85.5 92.0 95.7 88.9
Step 3 - Sort scores: 85.5 88.9 92.0 95.7
Result: PASSED ✓

╔════════════════════════════════════════════════════════════════╗
║   ALL TESTS PASSED ✓✓✓                                       ║
║   TRUE functional programming in C for ALL types!            ║
╚════════════════════════════════════════════════════════════════╝
```

---

## Helper Macros for Type Safety

```c
/* Type-safe wrappers */
#define FP_QUICKSORT(TYPE, input, output, n, cmp, ctx) \
    fp_quicksort_generic((input), (output), (n), sizeof(TYPE), (cmp), (ctx))

#define FP_MAP(IN_TYPE, OUT_TYPE, input, output, n, fn, ctx) \
    fp_map_generic((input), (output), (n), sizeof(IN_TYPE), sizeof(OUT_TYPE), (fn), (ctx))

#define FP_FILTER(TYPE, input, output, n, pred, ctx) \
    fp_filter_generic((input), (output), (n), sizeof(TYPE), (pred), (ctx))

#define FP_REVERSE(TYPE, input, output, n) \
    fp_reverse_generic((input), (output), (n), sizeof(TYPE))
```

Usage:
```c
/* Instead of: */
fp_quicksort_generic(data, sorted, 100, sizeof(Student), cmp, NULL);

/* Use: */
FP_QUICKSORT(Student, data, sorted, 100, cmp, NULL);
```

---

## Comparison with Standard C `qsort`

### Standard C `qsort`

```c
void qsort(void *base, size_t nmemb, size_t size,
           int (*compar)(const void *, const void *));
```

**Problems:**
- ❌ **Mutates input in-place** (breaks immutability)
- ❌ No context parameter (hard to pass threshold, etc.)
- ❌ Not composable with other FP operations

### FP-ASM `fp_quicksort_generic`

```c
void fp_quicksort_generic(const void* input, void* output, size_t n,
                          size_t elem_size,
                          fp_compare_fn compare,
                          void* context);
```

**Advantages:**
- ✅ **Input immutable** (`const void* input`)
- ✅ Context parameter for closures
- ✅ Composable with map/filter/fold
- ✅ Pure functional (same input → same output)

---

## Advanced Use Cases

### 1. Sorting by Multiple Criteria

```c
typedef struct {
    int priority;  /* Primary sort key */
    double value;  /* Secondary sort key */
} Record;

typedef struct {
    bool reverse_priority;
    bool reverse_value;
} SortContext;

int compare_records(const void* a, const void* b, void* ctx) {
    const Record* r1 = (const Record*)a;
    const Record* r2 = (const Record*)b;
    SortContext* options = (SortContext*)ctx;

    /* Primary: priority */
    int diff = r1->priority - r2->priority;
    if (options->reverse_priority) diff = -diff;
    if (diff != 0) return diff;

    /* Secondary: value */
    double vdiff = r1->value - r2->value;
    if (options->reverse_value) vdiff = -vdiff;
    return (vdiff > 0) ? 1 : (vdiff < 0) ? -1 : 0;
}

int main() {
    Record data[100];
    Record sorted[100];
    SortContext ctx = {.reverse_priority = false, .reverse_value = true};

    FP_QUICKSORT(Record, data, sorted, 100, compare_records, &ctx);
}
```

### 2. Case-Insensitive String Sorting

```c
int compare_strings_case_insensitive(const void* a, const void* b, void* ctx) {
    (void)ctx;
    const char* str_a = *(const char**)a;
    const char* str_b = *(const char**)b;
    return strcasecmp(str_a, str_b);  /* POSIX function */
}
```

### 3. Filtering with Complex Logic

```c
typedef struct {
    double min_score;
    double max_score;
    bool require_even_id;
} FilterContext;

bool complex_predicate(const void* elem, void* ctx) {
    const Student* s = (const Student*)elem;
    FilterContext* filter = (FilterContext*)ctx;

    if (s->score < filter->min_score || s->score > filter->max_score)
        return false;

    if (filter->require_even_id && (s->id % 2 != 0))
        return false;

    return true;
}

int main() {
    Student students[100];
    Student filtered[100];
    FilterContext ctx = {
        .min_score = 80.0,
        .max_score = 95.0,
        .require_even_id = true
    };

    size_t count = FP_FILTER(Student, students, filtered, 100,
                             complex_predicate, &ctx);
}
```

---

## Limitations and Future Work

### Current Limitations

1. **No Generic AVX2 SIMD**
   - Generic functions use scalar C code
   - Performance is good but not assembly-optimized
   - Type-specific versions (i64/f64) still faster for numeric operations

2. **Heap Allocation in Quicksort**
   - `fp_quicksort_generic` uses `malloc` for temp buffer
   - Single allocation (size = one element), but breaks "zero heap" ideal
   - Alternative: user provides temp buffer (like mergesort)

3. **No Type Safety at Compile Time**
   - Using `void*` loses type checking
   - Macros help but aren't foolproof
   - User must ensure size parameters match types

### Future Enhancements

1. **Stack-Only Quicksort**
   - Use fixed-size buffer (e.g., 1KB) for temp
   - Fallback to mergesort for larger elements

2. **SIMD for Sorting Primitives**
   - AVX2 for comparison arrays (e.g., int32, float)
   - Specialized implementations for common types

3. **C++ Template Wrappers**
   - Provide type-safe C++ interface
   - Use templates for automatic size deduction

4. **More Generic Operations**
   - `foldr` (right fold)
   - `scanl` (prefix sum, generic)
   - `groupBy` (group consecutive equal elements)

---

## Conclusion

**YES, we can write generic versions of FP-ASM functions that work with ANY type while maintaining pure FP values!**

### What We Achieved

✅ **Complete Type Genericity**
- Works with int, double, strings, structs, **any type**
- No limitations on data size or structure

✅ **100% Functional Purity**
- Input immutability guaranteed (`const`)
- No side effects
- Deterministic execution

✅ **Haskell-Level Expressiveness**
- Quick sort, merge sort
- Map, filter, fold, zipWith
- Partition, reverse, find
- Full functional composition

✅ **Practical and Usable**
- Type-safe macros
- Context parameters for closures
- Clear, documented API

### The Bottom Line

**FP-ASM now provides TRUE functional programming for C across ALL types, not just numerics.**

You can now build:
- **Quick sort** for any comparable type
- **Map/filter pipelines** for data transformations
- **Complex data processing** with pure functions
- **Type-generic algorithms** while maintaining FP principles

**This is Haskell-level FP in C, for ALL types. Mission accomplished!** 🎉

---

## Files

- **`include/fp_generic.h`** - Generic type system API declarations
- **`src/wrappers/fp_generic.c`** - Generic implementations
- **`test_generic.c`** - Comprehensive test suite (12 test cases)
- **`build_test_generic.bat`** - Build script

Try it yourself:
```bash
build_test_generic.bat
```
