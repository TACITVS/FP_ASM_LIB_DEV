/**
 * FP-ASM Generic Type System Implementation
 *
 * Pure functional implementations for arbitrary types.
 * All functions maintain strict immutability and determinism.
 */

#include "../../include/fp_generic.h"
#include <string.h>  /* For memcpy */
#include <stdlib.h>  /* For malloc/free */

/* ============================================================================
 * CATEGORY 12: GENERIC HIGHER-ORDER FUNCTIONS
 * ============================================================================ */

void fp_fold_left_generic(const void* input, size_t n, size_t elem_size,
                      void* acc,
                      void (*fn)(void* acc, const void* elem, void* ctx),
                      void* context) {
    if (!input || !acc || !fn || elem_size == 0) return;

    const unsigned char* ptr = (const unsigned char*)input;

    for (size_t i = 0; i < n; i++) {
        fn(acc, ptr + i * elem_size, context);
    }
}

void fp_map_apply_generic(const void* input, void* output, size_t n,
                    size_t in_size, size_t out_size,
                    void (*fn)(void* out, const void* in, void* ctx),
                    void* context) {
    if (!input || !output || !fn || in_size == 0 || out_size == 0) return;

    const unsigned char* in_ptr = (const unsigned char*)input;
    unsigned char* out_ptr = (unsigned char*)output;

    for (size_t i = 0; i < n; i++) {
        fn(out_ptr + i * out_size, in_ptr + i * in_size, context);
    }
}

size_t fp_filter_predicate_generic(const void* input, void* output, size_t n,
                         size_t elem_size,
                         bool (*predicate)(const void* elem, void* ctx),
                         void* context) {
    if (!input || !output || !predicate || elem_size == 0) return 0;

    const unsigned char* in_ptr = (const unsigned char*)input;
    unsigned char* out_ptr = (unsigned char*)output;
    size_t write_idx = 0;

    for (size_t i = 0; i < n; i++) {
        const void* elem = in_ptr + i * elem_size;
        if (predicate(elem, context)) {
            memcpy(out_ptr + write_idx * elem_size, elem, elem_size);
            write_idx++;
        }
    }

    return write_idx;
}

void fp_zip_apply_generic(const void* input_a, const void* input_b, void* output, size_t n,
                        size_t size_a, size_t size_b, size_t size_c,
                        void (*fn)(void* out, const void* a, const void* b, void* ctx),
                        void* context) {
    if (!input_a || !input_b || !output || !fn) return;
    if (size_a == 0 || size_b == 0 || size_c == 0) return;

    const unsigned char* ptr_a = (const unsigned char*)input_a;
    const unsigned char* ptr_b = (const unsigned char*)input_b;
    unsigned char* ptr_c = (unsigned char*)output;

    for (size_t i = 0; i < n; i++) {
        fn(ptr_c + i * size_c,
           ptr_a + i * size_a,
           ptr_b + i * size_b,
           context);
    }
}

/* ============================================================================
 * CATEGORY 13: GENERIC SORTING
 * ============================================================================ */

/**
 * Helper: Swap two elements of arbitrary size
 */
static void swap_elements(void* a, void* b, size_t elem_size, void* temp) {
    memcpy(temp, a, elem_size);
    memcpy(a, b, elem_size);
    memcpy(b, temp, elem_size);
}

/**
 * Quick Sort: Partition function
 *
 * Partitions array around pivot (last element).
 * Returns index of pivot in final position.
 */
static size_t partition(void* array, size_t low, size_t high,
                        size_t elem_size,
                        fp_compare_fn compare, void* context,
                        void* temp) {
    unsigned char* arr = (unsigned char*)array;
    void* pivot = arr + high * elem_size;
    size_t i = low;

    for (size_t j = low; j < high; j++) {
        void* current = arr + j * elem_size;
        if (compare(current, pivot, context) < 0) {
            swap_elements(arr + i * elem_size, current, elem_size, temp);
            i++;
        }
    }

    swap_elements(arr + i * elem_size, pivot, elem_size, temp);
    return i;
}

/**
 * Quick Sort: Recursive helper
 */
static void quicksort_recursive(void* array, size_t low, size_t high,
                                size_t elem_size,
                                fp_compare_fn compare, void* context,
                                void* temp) {
    if (low < high) {
        size_t pi = partition(array, low, high, elem_size, compare, context, temp);

        /* Sort left partition */
        if (pi > 0) {
            quicksort_recursive(array, low, pi - 1, elem_size, compare, context, temp);
        }

        /* Sort right partition */
        if (pi < high) {
            quicksort_recursive(array, pi + 1, high, elem_size, compare, context, temp);
        }
    }
}

void fp_quicksort_generic(const void* input, void* output, size_t n,
                          size_t elem_size,
                          fp_compare_fn compare,
                          void* context) {
    if (!input || !output || !compare || n == 0 || elem_size == 0) return;

    /* Copy input to output (maintain immutability of input) */
    memcpy(output, input, n * elem_size);

    if (n <= 1) return;  /* Already sorted */

    /* Allocate temp buffer for swapping */
    void* temp = malloc(elem_size);
    if (!temp) return;  /* Allocation failed */

    /* Sort in-place in output buffer */
    quicksort_recursive(output, 0, n - 1, elem_size, compare, context, temp);

    free(temp);
}

/**
 * Merge Sort: Merge two sorted halves
 */
static void merge(void* array, size_t left, size_t mid, size_t right,
                  size_t elem_size,
                  fp_compare_fn compare, void* context,
                  void* temp) {
    unsigned char* arr = (unsigned char*)array;
    unsigned char* tmp = (unsigned char*)temp;

    size_t left_size = mid - left + 1;
    size_t right_size = right - mid;

    /* Copy left and right halves to temp buffer */
    memcpy(tmp, arr + left * elem_size, left_size * elem_size);
    memcpy(tmp + left_size * elem_size, arr + (mid + 1) * elem_size, right_size * elem_size);

    size_t i = 0, j = 0, k = left;

    /* Merge back into array */
    while (i < left_size && j < right_size) {
        void* left_elem = tmp + i * elem_size;
        void* right_elem = tmp + (left_size + j) * elem_size;

        if (compare(left_elem, right_elem, context) <= 0) {
            memcpy(arr + k * elem_size, left_elem, elem_size);
            i++;
        } else {
            memcpy(arr + k * elem_size, right_elem, elem_size);
            j++;
        }
        k++;
    }

    /* Copy remaining elements */
    while (i < left_size) {
        memcpy(arr + k * elem_size, tmp + i * elem_size, elem_size);
        i++;
        k++;
    }

    while (j < right_size) {
        memcpy(arr + k * elem_size, tmp + (left_size + j) * elem_size, elem_size);
        j++;
        k++;
    }
}

/**
 * Merge Sort: Recursive helper
 */
static void mergesort_recursive(void* array, size_t left, size_t right,
                                size_t elem_size,
                                fp_compare_fn compare, void* context,
                                void* temp) {
    if (left < right) {
        size_t mid = left + (right - left) / 2;

        mergesort_recursive(array, left, mid, elem_size, compare, context, temp);
        mergesort_recursive(array, mid + 1, right, elem_size, compare, context, temp);

        merge(array, left, mid, right, elem_size, compare, context, temp);
    }
}

void fp_mergesort_generic(const void* input, void* output, size_t n,
                          size_t elem_size,
                          fp_compare_fn compare,
                          void* context,
                          void* temp) {
    if (!input || !output || !compare || !temp || n == 0 || elem_size == 0) return;

    /* Copy input to output (maintain immutability) */
    memcpy(output, input, n * elem_size);

    if (n <= 1) return;

    /* Sort in-place in output buffer */
    mergesort_recursive(output, 0, n - 1, elem_size, compare, context, temp);
}

/* ============================================================================
 * CATEGORY 14: GENERIC LIST OPERATIONS
 * ============================================================================ */

void fp_partition_generic(const void* input,
                          void* output_true, void* output_false, size_t n,
                          size_t elem_size,
                          bool (*predicate)(const void* elem, void* ctx),
                          void* context,
                          size_t* count_true, size_t* count_false) {
    if (!input || !output_true || !output_false || !predicate || elem_size == 0) {
        if (count_true) *count_true = 0;
        if (count_false) *count_false = 0;
        return;
    }

    const unsigned char* in_ptr = (const unsigned char*)input;
    unsigned char* true_ptr = (unsigned char*)output_true;
    unsigned char* false_ptr = (unsigned char*)output_false;
    size_t true_idx = 0, false_idx = 0;

    for (size_t i = 0; i < n; i++) {
        const void* elem = in_ptr + i * elem_size;
        if (predicate(elem, context)) {
            memcpy(true_ptr + true_idx * elem_size, elem, elem_size);
            true_idx++;
        } else {
            memcpy(false_ptr + false_idx * elem_size, elem, elem_size);
            false_idx++;
        }
    }

    if (count_true) *count_true = true_idx;
    if (count_false) *count_false = false_idx;
}

void fp_take_generic(const void* input, void* output, size_t n, size_t count,
                     size_t elem_size) {
    if (!input || !output || elem_size == 0) return;

    size_t take_count = (count < n) ? count : n;
    memcpy(output, input, take_count * elem_size);
}

size_t fp_drop_generic(const void* input, void* output, size_t n, size_t count,
                       size_t elem_size) {
    if (!input || !output || elem_size == 0 || count >= n) return 0;

    size_t remaining = n - count;
    const unsigned char* in_ptr = (const unsigned char*)input;
    memcpy(output, in_ptr + count * elem_size, remaining * elem_size);
    return remaining;
}

void fp_reverse_generic(const void* input, void* output, size_t n,
                        size_t elem_size) {
    if (!input || !output || elem_size == 0) return;

    const unsigned char* in_ptr = (const unsigned char*)input;
    unsigned char* out_ptr = (unsigned char*)output;

    for (size_t i = 0; i < n; i++) {
        memcpy(out_ptr + i * elem_size,
               in_ptr + (n - 1 - i) * elem_size,
               elem_size);
    }
}

bool fp_find_generic(const void* input, size_t n, size_t elem_size,
                     bool (*predicate)(const void* elem, void* ctx),
                     void* context,
                     void* result) {
    if (!input || !predicate || elem_size == 0) return false;

    const unsigned char* ptr = (const unsigned char*)input;

    for (size_t i = 0; i < n; i++) {
        const void* elem = ptr + i * elem_size;
        if (predicate(elem, context)) {
            if (result) {
                memcpy(result, elem, elem_size);
            }
            return true;
        }
    }

    return false;
}
