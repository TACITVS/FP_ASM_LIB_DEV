/**
 * test_pca_safe.c - Unit tests for fp_pca_fit_safe()
 *
 * Tests the Either monad wrapper for safe error handling in PCA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include "fp_pca.h"

#define EPSILON 1e-6
#define TEST_PASS(name) printf("[PASS] %s\n", name)
#define TEST_FAIL(name, ...) do { printf("[FAIL] %s: ", name); printf(__VA_ARGS__); printf("\n"); } while(0)

static int tests_passed = 0;
static int tests_failed = 0;

/* ============================================================================
 * Test Data - Simple 2D data that should produce clear principal components
 * ============================================================================ */

// 5 samples, 2 features: points along y=x line plus noise
static double test_X[] = {
    1.0, 1.1,  // Sample 1
    2.0, 2.2,  // Sample 2
    3.0, 2.9,  // Sample 3
    4.0, 4.1,  // Sample 4
    5.0, 4.8   // Sample 5
};
static const int test_n = 5;
static const int test_d = 2;
static const int test_k = 1;  // Extract 1 principal component

/* ============================================================================
 * Error Case Tests
 * ============================================================================ */

void test_null_X(void) {
    Either result = fp_pca_fit_safe(NULL, test_n, test_d, test_k, 100, 1e-6, 42);

    if (fp_is_left(result) && fp_from_left_code(result) == 1) {
        TEST_PASS("NULL X returns Left with code 1");
        tests_passed++;
    } else {
        TEST_FAIL("NULL X", "Expected Left with code 1");
        tests_failed++;
    }
}

void test_invalid_n(void) {
    Either result = fp_pca_fit_safe(test_X, 0, test_d, test_k, 100, 1e-6, 42);

    if (fp_is_left(result) && fp_from_left_code(result) == 2) {
        TEST_PASS("n=0 returns Left with code 2");
        tests_passed++;
    } else {
        TEST_FAIL("n=0", "Expected Left with code 2");
        tests_failed++;
    }
}

void test_negative_n(void) {
    Either result = fp_pca_fit_safe(test_X, -1, test_d, test_k, 100, 1e-6, 42);

    if (fp_is_left(result) && fp_from_left_code(result) == 2) {
        TEST_PASS("n=-1 returns Left with code 2");
        tests_passed++;
    } else {
        TEST_FAIL("n=-1", "Expected Left with code 2");
        tests_failed++;
    }
}

void test_invalid_d(void) {
    Either result = fp_pca_fit_safe(test_X, test_n, 0, test_k, 100, 1e-6, 42);

    if (fp_is_left(result) && fp_from_left_code(result) == 2) {
        TEST_PASS("d=0 returns Left with code 2");
        tests_passed++;
    } else {
        TEST_FAIL("d=0", "Expected Left with code 2");
        tests_failed++;
    }
}

void test_negative_d(void) {
    Either result = fp_pca_fit_safe(test_X, test_n, -1, test_k, 100, 1e-6, 42);

    if (fp_is_left(result) && fp_from_left_code(result) == 2) {
        TEST_PASS("d=-1 returns Left with code 2");
        tests_passed++;
    } else {
        TEST_FAIL("d=-1", "Expected Left with code 2");
        tests_failed++;
    }
}

void test_invalid_n_components(void) {
    Either result = fp_pca_fit_safe(test_X, test_n, test_d, 0, 100, 1e-6, 42);

    if (fp_is_left(result) && fp_from_left_code(result) == 2) {
        TEST_PASS("n_components=0 returns Left with code 2");
        tests_passed++;
    } else {
        TEST_FAIL("n_components=0", "Expected Left with code 2");
        tests_failed++;
    }
}

void test_n_components_exceeds_d(void) {
    Either result = fp_pca_fit_safe(test_X, test_n, test_d, 3, 100, 1e-6, 42);

    if (fp_is_left(result) && fp_from_left_code(result) == 2) {
        TEST_PASS("n_components > d returns Left with code 2");
        tests_passed++;
    } else {
        TEST_FAIL("n_components > d", "Expected Left with code 2");
        tests_failed++;
    }
}

void test_overflow_covariance_matrix(void) {
    // Test d*d overflow (covariance matrix) - MOST CRITICAL
    // Use d=50000: 50000 * 50000 = 2,500,000,000 > INT_MAX
    // INT_MAX / 50000 = 42,949, so 50000 > 42949 triggers overflow check
    Either result = fp_pca_fit_safe(test_X, test_n, 50000, test_k, 100, 1e-6, 42);

    if (fp_is_left(result) && fp_from_left_code(result) == 2) {
        TEST_PASS("Covariance matrix overflow (d=50000) returns Left");
        tests_passed++;
    } else {
        TEST_FAIL("Covariance matrix overflow", "Expected Left with code 2");
        tests_failed++;
    }
}

void test_overflow_data_matrix(void) {
    // Test n*d overflow (data matrix)
    // Use n=100000, d=30000: 100000 * 30000 = 3,000,000,000 > INT_MAX
    // INT_MAX / 100000 = 21,474, so 30000 > 21474 triggers overflow
    Either result = fp_pca_fit_safe(test_X, 100000, 30000, test_k, 100, 1e-6, 42);

    if (fp_is_left(result) && fp_from_left_code(result) == 2) {
        TEST_PASS("Data matrix overflow (n*d) returns Left");
        tests_passed++;
    } else {
        TEST_FAIL("Data matrix overflow", "Expected Left with code 2");
        tests_failed++;
    }
}

void test_overflow_components_matrix(void) {
    // Test n_components*d overflow
    // Use n_components=60000, d=60000: 60000 * 60000 = 3,600,000,000 > INT_MAX
    // INT_MAX / 60000 = 35,791, so 60000 > 35791 triggers overflow
    Either result = fp_pca_fit_safe(test_X, test_n, 60000, 60000, 100, 1e-6, 42);

    if (fp_is_left(result) && fp_from_left_code(result) == 2) {
        TEST_PASS("Components matrix overflow returns Left");
        tests_passed++;
    } else {
        TEST_FAIL("Components matrix overflow", "Expected Left with code 2");
        tests_failed++;
    }
}

void test_safe_dimensions(void) {
    // Test dimensions that are safe (no overflow) and match our test data
    // Use n=5, d=2, k=1 (matches test_X): all products << INT_MAX
    // This verifies the overflow check doesn't false-positive on safe values
    Either result = fp_pca_fit_safe(test_X, test_n, test_d, test_k, 100, 1e-6, 42);

    // This should succeed since dimensions are valid
    if (fp_is_left(result)) {
        int code = fp_from_left_code(result);
        if (code != 2) {
            TEST_PASS("Safe dimensions (5×2) do not trigger overflow error");
            tests_passed++;
        } else {
            TEST_FAIL("Safe dimensions", "Got overflow error (code 2), but 5*2 is well below INT_MAX");
            tests_failed++;
        }
    } else {
        TEST_PASS("Safe dimensions (5×2) do not trigger overflow error");
        tests_passed++;
        // Cleanup
        PCAResult* r = (PCAResult*)fp_from_right_ptr(result);
        fp_pca_free_model(&r->model);
        free(r);
    }
}

/* ============================================================================
 * Success Case Tests
 * ============================================================================ */

void test_valid_input(void) {
    Either result = fp_pca_fit_safe(test_X, test_n, test_d, test_k, 100, 1e-6, 42);

    if (fp_is_right(result)) {
        PCAResult* r = (PCAResult*)fp_from_right_ptr(result);

        // Check model was trained (components allocated)
        if (r->model.components != NULL && r->model.mean != NULL &&
            r->model.eigenvalues != NULL) {
            TEST_PASS("Valid input returns Right with allocated model");
            tests_passed++;

            // Check basic properties
            if (r->model.n_features == test_d && r->model.n_components == test_k) {
                TEST_PASS("Model dimensions match input");
                tests_passed++;
            } else {
                TEST_FAIL("Model dimensions", "Expected (%d,%d), got (%d,%d)",
                         test_d, test_k, r->model.n_features, r->model.n_components);
                tests_failed++;
            }

            // Check eigenvalue is positive (variance must be positive)
            if (r->model.eigenvalues[0] > 0.0) {
                TEST_PASS("Eigenvalue is positive");
                tests_passed++;
            } else {
                TEST_FAIL("Eigenvalue", "Expected positive, got %f", r->model.eigenvalues[0]);
                tests_failed++;
            }

            // Check total variance is positive
            if (r->model.total_variance > 0.0) {
                TEST_PASS("Total variance is positive");
                tests_passed++;
            } else {
                TEST_FAIL("Total variance", "Expected positive, got %f", r->model.total_variance);
                tests_failed++;
            }
        } else {
            TEST_FAIL("Valid input", "Model components, mean, or eigenvalues is NULL");
            tests_failed++;
        }

        // Cleanup
        fp_pca_free_model(&r->model);
        free(r);
    } else {
        TEST_FAIL("Valid input", "Expected Right, got Left: %s", fp_from_left_msg(result));
        tests_failed++;
    }
}

void test_determinism(void) {
    Either r1 = fp_pca_fit_safe(test_X, test_n, test_d, test_k, 100, 1e-6, 42);
    Either r2 = fp_pca_fit_safe(test_X, test_n, test_d, test_k, 100, 1e-6, 42);

    if (fp_is_right(r1) && fp_is_right(r2)) {
        PCAResult* res1 = (PCAResult*)fp_from_right_ptr(r1);
        PCAResult* res2 = (PCAResult*)fp_from_right_ptr(r2);

        // Same seed should produce identical results
        int components_match = 1;
        for (int i = 0; i < test_k * test_d; i++) {
            if (fabs(res1->model.components[i] - res2->model.components[i]) > EPSILON) {
                components_match = 0;
                break;
            }
        }

        if (components_match) {
            TEST_PASS("Same seed produces identical principal components");
            tests_passed++;
        } else {
            TEST_FAIL("Determinism", "Same seed produced different components");
            tests_failed++;
        }

        fp_pca_free_model(&res1->model);
        free(res1);
        fp_pca_free_model(&res2->model);
        free(res2);
    } else {
        // Clean up any successful allocation before failing
        if (fp_is_right(r1)) {
            PCAResult* res1 = (PCAResult*)fp_from_right_ptr(r1);
            fp_pca_free_model(&res1->model);
            free(res1);
        }
        if (fp_is_right(r2)) {
            PCAResult* res2 = (PCAResult*)fp_from_right_ptr(r2);
            fp_pca_free_model(&res2->model);
            free(res2);
        }
        TEST_FAIL("Determinism", "One or both calls returned Left");
        tests_failed++;
    }
}

void test_full_rank_extraction(void) {
    // Extract all components (k = d = 2)
    Either result = fp_pca_fit_safe(test_X, test_n, test_d, test_d, 100, 1e-6, 42);

    if (fp_is_right(result)) {
        PCAResult* r = (PCAResult*)fp_from_right_ptr(result);

        // Check we got d components
        if (r->model.n_components == test_d) {
            TEST_PASS("Full rank extraction (k=d) succeeds");
            tests_passed++;
        } else {
            TEST_FAIL("Full rank extraction", "Expected %d components, got %d",
                     test_d, r->model.n_components);
            tests_failed++;
        }

        // Check cumulative variance ratio reaches ~1.0
        double cum_var = r->model.cumulative_variance_ratio[test_d - 1];
        if (fabs(cum_var - 1.0) < 0.01) {
            TEST_PASS("Cumulative variance ratio reaches 1.0 for full rank");
            tests_passed++;
        } else {
            TEST_FAIL("Cumulative variance", "Expected ~1.0, got %f", cum_var);
            tests_failed++;
        }

        fp_pca_free_model(&r->model);
        free(r);
    } else {
        TEST_FAIL("Full rank extraction", "Expected Right, got Left");
        tests_failed++;
    }
}

/* ============================================================================
 * Main
 * ============================================================================ */

int main(void) {
    printf("========================================\n");
    printf("PCA Safe API Tests\n");
    printf("========================================\n\n");

    printf("--- Error Case Tests ---\n");
    test_null_X();
    test_invalid_n();
    test_negative_n();
    test_invalid_d();
    test_negative_d();
    test_invalid_n_components();
    test_n_components_exceeds_d();

    printf("\n--- Overflow Protection Tests ---\n");
    test_overflow_covariance_matrix();
    test_overflow_data_matrix();
    test_overflow_components_matrix();
    test_safe_dimensions();

    printf("\n--- Success Case Tests ---\n");
    test_valid_input();
    test_determinism();
    test_full_rank_extraction();

    printf("\n========================================\n");
    printf("Results: %d passed, %d failed\n", tests_passed, tests_failed);
    printf("========================================\n");

    return tests_failed > 0 ? 1 : 0;
}
