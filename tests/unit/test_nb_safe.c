/**
 * test_nb_safe.c - Unit tests for fp_gaussian_nb_train_safe() and fp_multinomial_nb_train_safe()
 *
 * Tests the Either monad wrappers for safe error handling in Naive Bayes classifiers.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include "fp_naive_bayes.h"

#define EPSILON 1e-6
#define TEST_PASS(name) printf("[PASS] %s\n", name)
#define TEST_FAIL(name, ...) do { printf("[FAIL] %s: ", name); printf(__VA_ARGS__); printf("\n"); } while(0)

static int tests_passed = 0;
static int tests_failed = 0;

/* ============================================================================
 * Test Data - Simple binary classification problem
 * ============================================================================ */

// Class 0: Low values, Class 1: High values
static double test_X[] = {
    1.0, 1.5,  // Sample 0, Class 0
    1.2, 1.8,  // Sample 1, Class 0
    5.0, 5.5,  // Sample 2, Class 1
    5.2, 5.8   // Sample 3, Class 1
};
static int test_y[] = {0, 0, 1, 1};
static const int test_n = 4;
static const int test_d = 2;
static const int test_n_classes = 2;

/* ============================================================================
 * Gaussian Naive Bayes - Error Case Tests
 * ============================================================================ */

void test_gaussian_null_X(void) {
    Either result = fp_gaussian_nb_train_safe(NULL, test_y, test_n, test_d, test_n_classes);

    if (fp_is_left(result) && fp_from_left_code(result) == 1) {
        TEST_PASS("Gaussian: NULL X returns Left with code 1");
        tests_passed++;
    } else {
        TEST_FAIL("Gaussian NULL X", "Expected Left with code 1");
        tests_failed++;
    }
}

void test_gaussian_null_y(void) {
    Either result = fp_gaussian_nb_train_safe(test_X, NULL, test_n, test_d, test_n_classes);

    if (fp_is_left(result) && fp_from_left_code(result) == 1) {
        TEST_PASS("Gaussian: NULL y returns Left with code 1");
        tests_passed++;
    } else {
        TEST_FAIL("Gaussian NULL y", "Expected Left with code 1");
        tests_failed++;
    }
}

void test_gaussian_invalid_n(void) {
    Either result = fp_gaussian_nb_train_safe(test_X, test_y, 0, test_d, test_n_classes);

    if (fp_is_left(result) && fp_from_left_code(result) == 2) {
        TEST_PASS("Gaussian: n=0 returns Left with code 2");
        tests_passed++;
    } else {
        TEST_FAIL("Gaussian n=0", "Expected Left with code 2");
        tests_failed++;
    }
}

void test_gaussian_negative_n(void) {
    Either result = fp_gaussian_nb_train_safe(test_X, test_y, -1, test_d, test_n_classes);

    if (fp_is_left(result) && fp_from_left_code(result) == 2) {
        TEST_PASS("Gaussian: n=-1 returns Left with code 2");
        tests_passed++;
    } else {
        TEST_FAIL("Gaussian n=-1", "Expected Left with code 2");
        tests_failed++;
    }
}

void test_gaussian_invalid_d(void) {
    Either result = fp_gaussian_nb_train_safe(test_X, test_y, test_n, 0, test_n_classes);

    if (fp_is_left(result) && fp_from_left_code(result) == 2) {
        TEST_PASS("Gaussian: d=0 returns Left with code 2");
        tests_passed++;
    } else {
        TEST_FAIL("Gaussian d=0", "Expected Left with code 2");
        tests_failed++;
    }
}

void test_gaussian_invalid_n_classes(void) {
    Either result = fp_gaussian_nb_train_safe(test_X, test_y, test_n, test_d, 0);

    if (fp_is_left(result) && fp_from_left_code(result) == 2) {
        TEST_PASS("Gaussian: n_classes=0 returns Left with code 2");
        tests_passed++;
    } else {
        TEST_FAIL("Gaussian n_classes=0", "Expected Left with code 2");
        tests_failed++;
    }
}

void test_gaussian_overflow_model(void) {
    // Test n_classes * d overflow (means/variances matrices)
    // Use n_classes=60000, d=60000: 60000 * 60000 = 3,600,000,000 > INT_MAX
    // INT_MAX / 60000 = 35,791, so 60000 > 35791 triggers overflow
    Either result = fp_gaussian_nb_train_safe(test_X, test_y, test_n, 60000, 60000);

    if (fp_is_left(result) && fp_from_left_code(result) == 2) {
        TEST_PASS("Gaussian: Model dimensions overflow (60000×60000) returns Left");
        tests_passed++;
    } else {
        TEST_FAIL("Gaussian overflow", "Expected Left with code 2");
        tests_failed++;
    }
}

void test_gaussian_safe_dimensions(void) {
    // Test dimensions that are safe (no overflow)
    // Use test data dimensions: 4 samples, 2 features, 2 classes
    // n_classes * d = 2 * 2 = 4 << INT_MAX
    Either result = fp_gaussian_nb_train_safe(test_X, test_y, test_n, test_d, test_n_classes);

    if (fp_is_left(result)) {
        int code = fp_from_left_code(result);
        if (code != 2) {
            TEST_PASS("Gaussian: Safe dimensions (2 classes × 2 features) do not trigger overflow");
            tests_passed++;
        } else {
            TEST_FAIL("Gaussian safe dimensions", "Got overflow error (code 2), but 2*2 << INT_MAX");
            tests_failed++;
        }
    } else {
        TEST_PASS("Gaussian: Safe dimensions (2 classes × 2 features) do not trigger overflow");
        tests_passed++;
        GaussianNBModel* model = (GaussianNBModel*)fp_from_right_ptr(result);
        fp_nb_free_gaussian_model(model);
        free(model);
    }
}

/* ============================================================================
 * Gaussian Naive Bayes - Success Case Tests
 * ============================================================================ */

void test_gaussian_valid_input(void) {
    Either result = fp_gaussian_nb_train_safe(test_X, test_y, test_n, test_d, test_n_classes);

    if (fp_is_right(result)) {
        GaussianNBModel* model = (GaussianNBModel*)fp_from_right_ptr(result);

        // Check model was trained (parameters allocated)
        if (model->means != NULL && model->variances != NULL &&
            model->class_priors != NULL && model->class_counts != NULL) {
            TEST_PASS("Gaussian: Valid input returns Right with allocated model");
            tests_passed++;

            // Check dimensions match
            if (model->n_classes == test_n_classes && model->n_features == test_d) {
                TEST_PASS("Gaussian: Model dimensions match input");
                tests_passed++;
            } else {
                TEST_FAIL("Gaussian dimensions", "Expected (%d,%d), got (%d,%d)",
                         test_n_classes, test_d, model->n_classes, model->n_features);
                tests_failed++;
            }

            // Check class priors sum to 1.0
            double prior_sum = 0.0;
            for (int c = 0; c < model->n_classes; c++) {
                prior_sum += model->class_priors[c];
            }
            if (fabs(prior_sum - 1.0) < EPSILON) {
                TEST_PASS("Gaussian: Class priors sum to 1.0");
                tests_passed++;
            } else {
                TEST_FAIL("Gaussian priors", "Priors sum to %f, expected 1.0", prior_sum);
                tests_failed++;
            }
        } else {
            TEST_FAIL("Gaussian valid input", "One or more model parameters is NULL");
            tests_failed++;
        }

        // Cleanup
        fp_nb_free_gaussian_model(model);
        free(model);
    } else {
        TEST_FAIL("Gaussian valid input", "Expected Right, got Left: %s", fp_from_left_msg(result));
        tests_failed++;
    }
}

/* ============================================================================
 * Multinomial Naive Bayes - Error Case Tests
 * ============================================================================ */

void test_multinomial_null_X(void) {
    Either result = fp_multinomial_nb_train_safe(NULL, test_y, test_n, test_d, test_n_classes, 1.0);

    if (fp_is_left(result) && fp_from_left_code(result) == 1) {
        TEST_PASS("Multinomial: NULL X returns Left with code 1");
        tests_passed++;
    } else {
        TEST_FAIL("Multinomial NULL X", "Expected Left with code 1");
        tests_failed++;
    }
}

void test_multinomial_null_y(void) {
    Either result = fp_multinomial_nb_train_safe(test_X, NULL, test_n, test_d, test_n_classes, 1.0);

    if (fp_is_left(result) && fp_from_left_code(result) == 1) {
        TEST_PASS("Multinomial: NULL y returns Left with code 1");
        tests_passed++;
    } else {
        TEST_FAIL("Multinomial NULL y", "Expected Left with code 1");
        tests_failed++;
    }
}

void test_multinomial_invalid_n(void) {
    Either result = fp_multinomial_nb_train_safe(test_X, test_y, 0, test_d, test_n_classes, 1.0);

    if (fp_is_left(result) && fp_from_left_code(result) == 2) {
        TEST_PASS("Multinomial: n=0 returns Left with code 2");
        tests_passed++;
    } else {
        TEST_FAIL("Multinomial n=0", "Expected Left with code 2");
        tests_failed++;
    }
}

void test_multinomial_invalid_alpha(void) {
    Either result = fp_multinomial_nb_train_safe(test_X, test_y, test_n, test_d, test_n_classes, -1.0);

    if (fp_is_left(result) && fp_from_left_code(result) == 2) {
        TEST_PASS("Multinomial: alpha<0 returns Left with code 2");
        tests_passed++;
    } else {
        TEST_FAIL("Multinomial alpha<0", "Expected Left with code 2");
        tests_failed++;
    }
}

void test_multinomial_overflow_model(void) {
    // Test n_classes * d overflow (feature matrices)
    // Use n_classes=70000, d=70000: 70000 * 70000 = 4,900,000,000 > INT_MAX
    // INT_MAX / 70000 = 30,678, so 70000 > 30678 triggers overflow
    Either result = fp_multinomial_nb_train_safe(test_X, test_y, test_n, 70000, 70000, 1.0);

    if (fp_is_left(result) && fp_from_left_code(result) == 2) {
        TEST_PASS("Multinomial: Model dimensions overflow (70000×70000) returns Left");
        tests_passed++;
    } else {
        TEST_FAIL("Multinomial overflow", "Expected Left with code 2");
        tests_failed++;
    }
}

void test_multinomial_safe_dimensions(void) {
    // Test dimensions that are safe (no overflow)
    // Use test data dimensions: 4 samples, 2 features, 2 classes
    // n_classes * d = 2 * 2 = 4 << INT_MAX
    Either result = fp_multinomial_nb_train_safe(test_X, test_y, test_n, test_d, test_n_classes, 1.0);

    if (fp_is_left(result)) {
        int code = fp_from_left_code(result);
        if (code != 2) {
            TEST_PASS("Multinomial: Safe dimensions (2 classes × 2 features) do not trigger overflow");
            tests_passed++;
        } else {
            TEST_FAIL("Multinomial safe dimensions", "Got overflow error (code 2), but 2*2 << INT_MAX");
            tests_failed++;
        }
    } else {
        TEST_PASS("Multinomial: Safe dimensions (2 classes × 2 features) do not trigger overflow");
        tests_passed++;
        MultinomialNBModel* model = (MultinomialNBModel*)fp_from_right_ptr(result);
        fp_nb_free_multinomial_model(model);
        free(model);
    }
}

/* ============================================================================
 * Multinomial Naive Bayes - Success Case Tests
 * ============================================================================ */

void test_multinomial_valid_input(void) {
    Either result = fp_multinomial_nb_train_safe(test_X, test_y, test_n, test_d, test_n_classes, 1.0);

    if (fp_is_right(result)) {
        MultinomialNBModel* model = (MultinomialNBModel*)fp_from_right_ptr(result);

        // Check model was trained (parameters allocated)
        if (model->feature_log_probs != NULL && model->class_priors != NULL &&
            model->class_counts != NULL && model->feature_counts != NULL) {
            TEST_PASS("Multinomial: Valid input returns Right with allocated model");
            tests_passed++;

            // Check dimensions match
            if (model->n_classes == test_n_classes && model->n_features == test_d) {
                TEST_PASS("Multinomial: Model dimensions match input");
                tests_passed++;
            } else {
                TEST_FAIL("Multinomial dimensions", "Expected (%d,%d), got (%d,%d)",
                         test_n_classes, test_d, model->n_classes, model->n_features);
                tests_failed++;
            }

            // Check class priors sum to 1.0
            double prior_sum = 0.0;
            for (int c = 0; c < model->n_classes; c++) {
                prior_sum += model->class_priors[c];
            }
            if (fabs(prior_sum - 1.0) < EPSILON) {
                TEST_PASS("Multinomial: Class priors sum to 1.0");
                tests_passed++;
            } else {
                TEST_FAIL("Multinomial priors", "Priors sum to %f, expected 1.0", prior_sum);
                tests_failed++;
            }
        } else {
            TEST_FAIL("Multinomial valid input", "One or more model parameters is NULL");
            tests_failed++;
        }

        // Cleanup
        fp_nb_free_multinomial_model(model);
        free(model);
    } else {
        TEST_FAIL("Multinomial valid input", "Expected Right, got Left: %s", fp_from_left_msg(result));
        tests_failed++;
    }
}

void test_multinomial_zero_alpha(void) {
    // alpha=0 is valid (no smoothing)
    Either result = fp_multinomial_nb_train_safe(test_X, test_y, test_n, test_d, test_n_classes, 0.0);

    if (fp_is_right(result)) {
        TEST_PASS("Multinomial: alpha=0 (no smoothing) is valid");
        tests_passed++;
        MultinomialNBModel* model = (MultinomialNBModel*)fp_from_right_ptr(result);
        fp_nb_free_multinomial_model(model);
        free(model);
    } else {
        TEST_FAIL("Multinomial zero alpha", "Expected Right, got Left");
        tests_failed++;
    }
}

/* ============================================================================
 * Main
 * ============================================================================ */

int main(void) {
    printf("========================================\n");
    printf("Naive Bayes Safe API Tests\n");
    printf("========================================\n\n");

    printf("--- Gaussian Naive Bayes: Error Cases ---\n");
    test_gaussian_null_X();
    test_gaussian_null_y();
    test_gaussian_invalid_n();
    test_gaussian_negative_n();
    test_gaussian_invalid_d();
    test_gaussian_invalid_n_classes();
    test_gaussian_overflow_model();
    test_gaussian_safe_dimensions();

    printf("\n--- Gaussian Naive Bayes: Success Cases ---\n");
    test_gaussian_valid_input();

    printf("\n--- Multinomial Naive Bayes: Error Cases ---\n");
    test_multinomial_null_X();
    test_multinomial_null_y();
    test_multinomial_invalid_n();
    test_multinomial_invalid_alpha();
    test_multinomial_overflow_model();
    test_multinomial_safe_dimensions();

    printf("\n--- Multinomial Naive Bayes: Success Cases ---\n");
    test_multinomial_valid_input();
    test_multinomial_zero_alpha();

    printf("\n========================================\n");
    printf("Results: %d passed, %d failed\n", tests_passed, tests_failed);
    printf("========================================\n");

    return tests_failed > 0 ? 1 : 0;
}
