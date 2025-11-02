// FP-ASM Comprehensive Test Suite
// Tests all 26 functions across 6 modules for correctness and edge cases.

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include "../include/fp_core.h"

// -------------------- Test Framework --------------------
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(condition, test_name) do { \
    if (condition) { \
        printf("  [PASS] %s\n", test_name); \
        tests_passed++; \
    } else { \
        printf("  [FAIL] %s\n", test_name); \
        tests_failed++; \
    } \
} while(0)

// -------------------- Helpers --------------------
static bool arrays_equal_i64(const int64_t* a, const int64_t* b, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

static bool doubles_close(double a, double b, double tol) {
    double diff = fabs(a - b);
    double scale = fmax(1.0, fmax(fabs(a), fabs(b)));
    return diff <= tol * scale;
}

static bool arrays_close_f64(const double* a, const double* b, size_t n, double tol) {
    for (size_t i = 0; i < n; i++) {
        if (!doubles_close(a[i], b[i], tol)) return false;
    }
    return true;
}

// -------------------- C Reference Implementations --------------------

// Module 1: Reductions
static int64_t c_reduce_add_i64(const int64_t* in, size_t n) {
    int64_t sum = 0;
    for (size_t i = 0; i < n; i++) sum += in[i];
    return sum;
}

static double c_reduce_add_f64(const double* in, size_t n) {
    double sum = 0.0;
    for (size_t i = 0; i < n; i++) sum += in[i];
    return sum;
}

static int64_t c_reduce_max_i64(const int64_t* in, size_t n) {
    int64_t max = in[0];
    for (size_t i = 1; i < n; i++) {
        if (in[i] > max) max = in[i];
    }
    return max;
}

static double c_reduce_max_f64(const double* in, size_t n) {
    double max = in[0];
    for (size_t i = 1; i < n; i++) {
        if (in[i] > max) max = in[i];
    }
    return max;
}

// Module 2: Fused Folds
static int64_t c_fold_sumsq_i64(const int64_t* in, size_t n) {
    int64_t sum = 0;
    for (size_t i = 0; i < n; i++) sum += in[i] * in[i];
    return sum;
}

static int64_t c_fold_dotp_i64(const int64_t* a, const int64_t* b, size_t n) {
    int64_t sum = 0;
    for (size_t i = 0; i < n; i++) sum += a[i] * b[i];
    return sum;
}

static double c_fold_dotp_f64(const double* a, const double* b, size_t n) {
    double sum = 0.0;
    for (size_t i = 0; i < n; i++) sum += a[i] * b[i];
    return sum;
}

static int64_t c_fold_sad_i64(const int64_t* a, const int64_t* b, size_t n) {
    int64_t sum = 0;
    for (size_t i = 0; i < n; i++) {
        int64_t diff = a[i] - b[i];
        sum += (diff < 0) ? -diff : diff;
    }
    return sum;
}

// Module 3: Fused Maps
static void c_map_axpy_i64(const int64_t* x, const int64_t* y, int64_t* out, size_t n, int64_t c) {
    for (size_t i = 0; i < n; i++) out[i] = c * x[i] + y[i];
}

static void c_map_axpy_f64(const double* x, const double* y, double* out, size_t n, double c) {
    for (size_t i = 0; i < n; i++) out[i] = c * x[i] + y[i];
}

static void c_map_scale_i64(const int64_t* in, int64_t* out, size_t n, int64_t c) {
    for (size_t i = 0; i < n; i++) out[i] = c * in[i];
}

static void c_map_scale_f64(const double* in, double* out, size_t n, double c) {
    for (size_t i = 0; i < n; i++) out[i] = c * in[i];
}

static void c_map_offset_i64(const int64_t* in, int64_t* out, size_t n, int64_t c) {
    for (size_t i = 0; i < n; i++) out[i] = in[i] + c;
}

static void c_map_offset_f64(const double* in, double* out, size_t n, double c) {
    for (size_t i = 0; i < n; i++) out[i] = in[i] + c;
}

static void c_zip_add_i64(const int64_t* a, const int64_t* b, int64_t* out, size_t n) {
    for (size_t i = 0; i < n; i++) out[i] = a[i] + b[i];
}

static void c_zip_add_f64(const double* a, const double* b, double* out, size_t n) {
    for (size_t i = 0; i < n; i++) out[i] = a[i] + b[i];
}

// Module 4: Simple Maps
static void c_map_abs_i64(const int64_t* in, int64_t* out, size_t n) {
    for (size_t i = 0; i < n; i++) out[i] = (in[i] < 0) ? -in[i] : in[i];
}

static void c_map_abs_f64(const double* in, double* out, size_t n) {
    for (size_t i = 0; i < n; i++) out[i] = fabs(in[i]);
}

static void c_map_sqrt_f64(const double* in, double* out, size_t n) {
    for (size_t i = 0; i < n; i++) out[i] = sqrt(in[i]);
}

static void c_map_clamp_i64(const int64_t* in, int64_t* out, size_t n, int64_t min_val, int64_t max_val) {
    for (size_t i = 0; i < n; i++) {
        int64_t val = in[i];
        if (val < min_val) val = min_val;
        if (val > max_val) val = max_val;
        out[i] = val;
    }
}

static void c_map_clamp_f64(const double* in, double* out, size_t n, double min_val, double max_val) {
    for (size_t i = 0; i < n; i++) {
        double val = in[i];
        if (val < min_val) val = min_val;
        if (val > max_val) val = max_val;
        out[i] = val;
    }
}

// Module 5: Scans
static void c_scan_add_i64(const int64_t* in, int64_t* out, size_t n) {
    int64_t acc = 0;
    for (size_t i = 0; i < n; i++) {
        acc += in[i];
        out[i] = acc;
    }
}

static void c_scan_add_f64(const double* in, double* out, size_t n) {
    double acc = 0.0;
    for (size_t i = 0; i < n; i++) {
        acc += in[i];
        out[i] = acc;
    }
}

// Module 6: Predicates
static bool c_pred_all_eq_const_i64(const int64_t* arr, size_t n, int64_t value) {
    for (size_t i = 0; i < n; i++) {
        if (arr[i] != value) return false;
    }
    return true;
}

static bool c_pred_any_gt_const_i64(const int64_t* arr, size_t n, int64_t value) {
    for (size_t i = 0; i < n; i++) {
        if (arr[i] > value) return true;
    }
    return false;
}

static bool c_pred_all_gt_zip_i64(const int64_t* a, const int64_t* b, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (a[i] <= b[i]) return false;
    }
    return true;
}

// -------------------- Test Functions --------------------

void test_module1_reductions(void) {
    printf("\n=== MODULE 1: REDUCTIONS ===\n");

    // Test data
    int64_t data_i64[] = {1, 2, 3, 4, 5, -10, 20};
    double data_f64[] = {1.5, 2.5, 3.5, 4.5, 5.5};
    size_t n = 7;

    // reduce_add_i64
    int64_t sum_c_i64 = c_reduce_add_i64(data_i64, n);
    int64_t sum_asm_i64 = fp_reduce_add_i64(data_i64, n);
    TEST_ASSERT(sum_c_i64 == sum_asm_i64, "reduce_add_i64");

    // reduce_add_f64
    double sum_c_f64 = c_reduce_add_f64(data_f64, 5);
    double sum_asm_f64 = fp_reduce_add_f64(data_f64, 5);
    TEST_ASSERT(doubles_close(sum_c_f64, sum_asm_f64, 1e-12), "reduce_add_f64");

    // reduce_max_i64
    int64_t max_c_i64 = c_reduce_max_i64(data_i64, n);
    int64_t max_asm_i64 = fp_reduce_max_i64(data_i64, n);
    TEST_ASSERT(max_c_i64 == max_asm_i64, "reduce_max_i64");

    // reduce_max_f64
    double max_c_f64 = c_reduce_max_f64(data_f64, 5);
    double max_asm_f64 = fp_reduce_max_f64(data_f64, 5);
    TEST_ASSERT(doubles_close(max_c_f64, max_asm_f64, 1e-12), "reduce_max_f64");
}

void test_module2_fused_folds(void) {
    printf("\n=== MODULE 2: FUSED FOLDS ===\n");

    int64_t a_i64[] = {1, 2, 3, 4, 5};
    int64_t b_i64[] = {5, 4, 3, 2, 1};
    double a_f64[] = {1.5, 2.5, 3.5};
    double b_f64[] = {2.0, 3.0, 4.0};

    // fold_sumsq_i64
    int64_t sumsq_c = c_fold_sumsq_i64(a_i64, 5);
    int64_t sumsq_asm = fp_fold_sumsq_i64(a_i64, 5);
    TEST_ASSERT(sumsq_c == sumsq_asm, "fold_sumsq_i64");

    // fold_dotp_i64
    int64_t dotp_c_i64 = c_fold_dotp_i64(a_i64, b_i64, 5);
    int64_t dotp_asm_i64 = fp_fold_dotp_i64(a_i64, b_i64, 5);
    TEST_ASSERT(dotp_c_i64 == dotp_asm_i64, "fold_dotp_i64");

    // fold_dotp_f64
    double dotp_c_f64 = c_fold_dotp_f64(a_f64, b_f64, 3);
    double dotp_asm_f64 = fp_fold_dotp_f64(a_f64, b_f64, 3);
    TEST_ASSERT(doubles_close(dotp_c_f64, dotp_asm_f64, 1e-5), "fold_dotp_f64");

    // fold_sad_i64
    int64_t sad_c = c_fold_sad_i64(a_i64, b_i64, 5);
    int64_t sad_asm = fp_fold_sad_i64(a_i64, b_i64, 5);
    TEST_ASSERT(sad_c == sad_asm, "fold_sad_i64");
}

void test_module3_fused_maps(void) {
    printf("\n=== MODULE 3: FUSED MAPS ===\n");

    int64_t x_i64[] = {1, 2, 3, 4};
    int64_t y_i64[] = {10, 20, 30, 40};
    int64_t out_c_i64[4], out_asm_i64[4];

    double x_f64[] = {1.0, 2.0, 3.0};
    double y_f64[] = {0.5, 1.5, 2.5};
    double out_c_f64[3], out_asm_f64[3];

    // map_axpy_i64
    c_map_axpy_i64(x_i64, y_i64, out_c_i64, 4, 5);
    fp_map_axpy_i64(x_i64, y_i64, out_asm_i64, 4, 5);
    TEST_ASSERT(arrays_equal_i64(out_c_i64, out_asm_i64, 4), "map_axpy_i64");

    // map_axpy_f64
    c_map_axpy_f64(x_f64, y_f64, out_c_f64, 3, 2.5);
    fp_map_axpy_f64(x_f64, y_f64, out_asm_f64, 3, 2.5);
    TEST_ASSERT(arrays_close_f64(out_c_f64, out_asm_f64, 3, 1e-12), "map_axpy_f64");

    // map_scale_i64
    c_map_scale_i64(x_i64, out_c_i64, 4, 10);
    fp_map_scale_i64(x_i64, out_asm_i64, 4, 10);
    TEST_ASSERT(arrays_equal_i64(out_c_i64, out_asm_i64, 4), "map_scale_i64");

    // map_scale_f64
    c_map_scale_f64(x_f64, out_c_f64, 3, 0.5);
    fp_map_scale_f64(x_f64, out_asm_f64, 3, 0.5);
    TEST_ASSERT(arrays_close_f64(out_c_f64, out_asm_f64, 3, 1e-12), "map_scale_f64");

    // map_offset_i64
    c_map_offset_i64(x_i64, out_c_i64, 4, 100);
    fp_map_offset_i64(x_i64, out_asm_i64, 4, 100);
    TEST_ASSERT(arrays_equal_i64(out_c_i64, out_asm_i64, 4), "map_offset_i64");

    // map_offset_f64
    c_map_offset_f64(x_f64, out_c_f64, 3, -10.0);
    fp_map_offset_f64(x_f64, out_asm_f64, 3, -10.0);
    TEST_ASSERT(arrays_close_f64(out_c_f64, out_asm_f64, 3, 1e-12), "map_offset_f64");

    // zip_add_i64
    c_zip_add_i64(x_i64, y_i64, out_c_i64, 4);
    fp_zip_add_i64(x_i64, y_i64, out_asm_i64, 4);
    TEST_ASSERT(arrays_equal_i64(out_c_i64, out_asm_i64, 4), "zip_add_i64");

    // zip_add_f64
    c_zip_add_f64(x_f64, y_f64, out_c_f64, 3);
    fp_zip_add_f64(x_f64, y_f64, out_asm_f64, 3);
    TEST_ASSERT(arrays_close_f64(out_c_f64, out_asm_f64, 3, 1e-12), "zip_add_f64");
}

void test_module4_simple_maps(void) {
    printf("\n=== MODULE 4: SIMPLE MAPS ===\n");

    int64_t data_i64[] = {-5, 10, -3, 8, -12};
    double data_f64[] = {-2.5, 3.5, -1.5, 4.0, 9.0};
    int64_t out_c_i64[5], out_asm_i64[5];
    double out_c_f64[5], out_asm_f64[5];

    // map_abs_i64
    c_map_abs_i64(data_i64, out_c_i64, 5);
    fp_map_abs_i64(data_i64, out_asm_i64, 5);
    TEST_ASSERT(arrays_equal_i64(out_c_i64, out_asm_i64, 5), "map_abs_i64");

    // map_abs_f64
    c_map_abs_f64(data_f64, out_c_f64, 5);
    fp_map_abs_f64(data_f64, out_asm_f64, 5);
    TEST_ASSERT(arrays_close_f64(out_c_f64, out_asm_f64, 5, 1e-12), "map_abs_f64");

    // map_sqrt_f64
    double sqrt_data[] = {4.0, 9.0, 16.0, 25.0};
    c_map_sqrt_f64(sqrt_data, out_c_f64, 4);
    fp_map_sqrt_f64(sqrt_data, out_asm_f64, 4);
    TEST_ASSERT(arrays_close_f64(out_c_f64, out_asm_f64, 4, 1e-12), "map_sqrt_f64");

    // map_clamp_i64
    c_map_clamp_i64(data_i64, out_c_i64, 5, 0, 10);
    fp_map_clamp_i64(data_i64, out_asm_i64, 5, 0, 10);
    TEST_ASSERT(arrays_equal_i64(out_c_i64, out_asm_i64, 5), "map_clamp_i64");

    // map_clamp_f64
    c_map_clamp_f64(data_f64, out_c_f64, 5, 0.0, 5.0);
    fp_map_clamp_f64(data_f64, out_asm_f64, 5, 0.0, 5.0);
    TEST_ASSERT(arrays_close_f64(out_c_f64, out_asm_f64, 5, 1e-12), "map_clamp_f64");
}

void test_module5_scans(void) {
    printf("\n=== MODULE 5: SCANS ===\n");

    int64_t data_i64[] = {1, 2, 3, 4, 5};
    double data_f64[] = {1.0, 2.0, 3.0, 4.0};
    int64_t out_c_i64[5], out_asm_i64[5];
    double out_c_f64[4], out_asm_f64[4];

    // scan_add_i64
    c_scan_add_i64(data_i64, out_c_i64, 5);
    fp_scan_add_i64(data_i64, out_asm_i64, 5);
    TEST_ASSERT(arrays_equal_i64(out_c_i64, out_asm_i64, 5), "scan_add_i64");

    // scan_add_f64
    c_scan_add_f64(data_f64, out_c_f64, 4);
    fp_scan_add_f64(data_f64, out_asm_f64, 4);
    TEST_ASSERT(arrays_close_f64(out_c_f64, out_asm_f64, 4, 1e-9), "scan_add_f64");
}

void test_module6_predicates(void) {
    printf("\n=== MODULE 6: PREDICATES ===\n");

    int64_t all_42[] = {42, 42, 42, 42, 42};
    int64_t mixed[] = {1, 5, 10, 100, 3};
    int64_t arr_a[] = {10, 20, 30, 40};
    int64_t arr_b[] = {5, 15, 25, 35};

    // pred_all_eq_const (true case)
    bool c_res = c_pred_all_eq_const_i64(all_42, 5, 42);
    bool asm_res = fp_pred_all_eq_const_i64(all_42, 5, 42);
    TEST_ASSERT(c_res == asm_res && asm_res == true, "pred_all_eq_const (true)");

    // pred_all_eq_const (false case)
    c_res = c_pred_all_eq_const_i64(mixed, 5, 42);
    asm_res = fp_pred_all_eq_const_i64(mixed, 5, 42);
    TEST_ASSERT(c_res == asm_res && asm_res == false, "pred_all_eq_const (false)");

    // pred_any_gt_const (true case)
    c_res = c_pred_any_gt_const_i64(mixed, 5, 50);
    asm_res = fp_pred_any_gt_const_i64(mixed, 5, 50);
    TEST_ASSERT(c_res == asm_res && asm_res == true, "pred_any_gt_const (true)");

    // pred_any_gt_const (false case)
    c_res = c_pred_any_gt_const_i64(mixed, 5, 200);
    asm_res = fp_pred_any_gt_const_i64(mixed, 5, 200);
    TEST_ASSERT(c_res == asm_res && asm_res == false, "pred_any_gt_const (false)");

    // pred_all_gt_zip (true case)
    c_res = c_pred_all_gt_zip_i64(arr_a, arr_b, 4);
    asm_res = fp_pred_all_gt_zip_i64(arr_a, arr_b, 4);
    TEST_ASSERT(c_res == asm_res && asm_res == true, "pred_all_gt_zip (true)");

    // pred_all_gt_zip (false case)
    arr_a[2] = 20;  // Now arr_a[2] < arr_b[2]
    c_res = c_pred_all_gt_zip_i64(arr_a, arr_b, 4);
    asm_res = fp_pred_all_gt_zip_i64(arr_a, arr_b, 4);
    TEST_ASSERT(c_res == asm_res && asm_res == false, "pred_all_gt_zip (false)");
}

void test_edge_cases(void) {
    printf("\n=== EDGE CASE TESTS ===\n");

    // Test with various array sizes
    size_t sizes[] = {1, 15, 16, 17, 31, 32, 33, 100};

    for (int s = 0; s < 8; s++) {
        size_t n = sizes[s];

        // Allocate arrays
        int64_t* data_i64 = malloc(n * sizeof(int64_t));
        int64_t* out_c_i64 = malloc(n * sizeof(int64_t));
        int64_t* out_asm_i64 = malloc(n * sizeof(int64_t));

        // Initialize
        for (size_t i = 0; i < n; i++) {
            data_i64[i] = (int64_t)i;
        }

        // Test reduce_add_i64
        int64_t sum_c = c_reduce_add_i64(data_i64, n);
        int64_t sum_asm = fp_reduce_add_i64(data_i64, n);

        // Test scan_add_i64
        c_scan_add_i64(data_i64, out_c_i64, n);
        fp_scan_add_i64(data_i64, out_asm_i64, n);

        bool pass = (sum_c == sum_asm) && arrays_equal_i64(out_c_i64, out_asm_i64, n);

        char test_name[100];
        sprintf(test_name, "Edge case n=%zu", n);
        TEST_ASSERT(pass, test_name);

        free(data_i64);
        free(out_c_i64);
        free(out_asm_i64);
    }
}

// -------------------- Main --------------------
int main(void) {
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║   FP-ASM COMPREHENSIVE TEST SUITE                         ║\n");
    printf("║   Testing all 26 functions across 6 modules               ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");

    test_module1_reductions();
    test_module2_fused_folds();
    test_module3_fused_maps();
    test_module4_simple_maps();
    test_module5_scans();
    test_module6_predicates();
    test_edge_cases();

    // Summary
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║   TEST SUMMARY                                             ║\n");
    printf("╠════════════════════════════════════════════════════════════╣\n");
    printf("║   Total Tests:  %3d                                        ║\n", tests_passed + tests_failed);
    printf("║   Passed:       %3d  ✓                                     ║\n", tests_passed);
    printf("║   Failed:       %3d  %s                                     ║\n",
           tests_failed, tests_failed > 0 ? "✗" : "✓");
    printf("║   Pass Rate:    %3.0f%%                                       ║\n",
           100.0 * tests_passed / (tests_passed + tests_failed));
    printf("╚════════════════════════════════════════════════════════════╝\n");

    if (tests_failed == 0) {
        printf("\n🎉 ALL TESTS PASSED! Library is production-ready. 🎉\n\n");
        return 0;
    } else {
        printf("\n❌ SOME TESTS FAILED. Please review failures above. ❌\n\n");
        return 1;
    }
}
