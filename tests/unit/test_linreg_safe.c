/**
 * test_linreg_safe.c - Unit tests for fp_linear_regression_gradient_descent_safe()
 *
 * Tests the Either monad wrapper for safe error handling in gradient descent.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "fp_linear_regression.h"

#define EPSILON 1e-6
#define TEST_PASS(name) printf("[PASS] %s\n", name)
#define TEST_FAIL(name, ...) do { printf("[FAIL] %s: ", name); printf(__VA_ARGS__); printf("\n"); } while(0)

static int tests_passed = 0;
static int tests_failed = 0;

/* ============================================================================
 * Test Data - Simple linear relationship: y = 2*x + 1
 * ============================================================================ */

static double test_X[] = {1.0, 2.0, 3.0, 4.0, 5.0};
static double test_y[] = {3.0, 5.0, 7.0, 9.0, 11.0};  // y = 2x + 1
static const int test_n = 5;
static const int test_d = 1;

/* ============================================================================
 * Error Case Tests
 * ============================================================================ */

void test_null_X(void) {
    Either result = fp_linear_regression_gradient_descent_safe(
        NULL, test_y, test_n, test_d, 0.01, 100, 1e-6, 42);

    if (fp_is_left(result) && fp_from_left_code(result) == 1) {
        TEST_PASS("NULL X returns Left with code 1");
        tests_passed++;
    } else {
        TEST_FAIL("NULL X", "Expected Left with code 1");
        tests_failed++;
    }
}

void test_null_y(void) {
    Either result = fp_linear_regression_gradient_descent_safe(
        test_X, NULL, test_n, test_d, 0.01, 100, 1e-6, 42);

    if (fp_is_left(result) && fp_from_left_code(result) == 1) {
        TEST_PASS("NULL y returns Left with code 1");
        tests_passed++;
    } else {
        TEST_FAIL("NULL y", "Expected Left with code 1");
        tests_failed++;
    }
}

void test_invalid_n(void) {
    Either result = fp_linear_regression_gradient_descent_safe(
        test_X, test_y, 0, test_d, 0.01, 100, 1e-6, 42);

    if (fp_is_left(result) && fp_from_left_code(result) == 2) {
        TEST_PASS("n=0 returns Left with code 2");
        tests_passed++;
    } else {
        TEST_FAIL("n=0", "Expected Left with code 2");
        tests_failed++;
    }
}

void test_invalid_d(void) {
    Either result = fp_linear_regression_gradient_descent_safe(
        test_X, test_y, test_n, 0, 0.01, 100, 1e-6, 42);

    if (fp_is_left(result) && fp_from_left_code(result) == 2) {
        TEST_PASS("d=0 returns Left with code 2");
        tests_passed++;
    } else {
        TEST_FAIL("d=0", "Expected Left with code 2");
        tests_failed++;
    }
}

void test_invalid_learning_rate(void) {
    Either result = fp_linear_regression_gradient_descent_safe(
        test_X, test_y, test_n, test_d, 0.0, 100, 1e-6, 42);

    if (fp_is_left(result) && fp_from_left_code(result) == 2) {
        TEST_PASS("learning_rate=0 returns Left with code 2");
        tests_passed++;
    } else {
        TEST_FAIL("learning_rate=0", "Expected Left with code 2");
        tests_failed++;
    }
}

void test_invalid_max_iterations(void) {
    Either result = fp_linear_regression_gradient_descent_safe(
        test_X, test_y, test_n, test_d, 0.01, 0, 1e-6, 42);

    if (fp_is_left(result) && fp_from_left_code(result) == 2) {
        TEST_PASS("max_iterations=0 returns Left with code 2");
        tests_passed++;
    } else {
        TEST_FAIL("max_iterations=0", "Expected Left with code 2");
        tests_failed++;
    }
}

void test_invalid_threshold(void) {
    Either result = fp_linear_regression_gradient_descent_safe(
        test_X, test_y, test_n, test_d, 0.01, 100, -1.0, 42);

    if (fp_is_left(result) && fp_from_left_code(result) == 2) {
        TEST_PASS("convergence_threshold<0 returns Left with code 2");
        tests_passed++;
    } else {
        TEST_FAIL("convergence_threshold<0", "Expected Left with code 2");
        tests_failed++;
    }
}

/* ============================================================================
 * Success Case Tests
 * ============================================================================ */

void test_valid_input(void) {
    Either result = fp_linear_regression_gradient_descent_safe(
        test_X, test_y, test_n, test_d, 0.1, 1000, 1e-6, 42);

    if (fp_is_right(result)) {
        GradientDescentResult* r = (GradientDescentResult*)fp_from_right_ptr(result);

        // Check model was trained (weights allocated)
        if (r->model.weights != NULL && r->loss_history != NULL) {
            TEST_PASS("Valid input returns Right with allocated model");
            tests_passed++;

            // Check weights are reasonable for y = 2x + 1
            // weights[0] should be ~1 (bias), weights[1] should be ~2 (slope)
            double bias = r->model.weights[0];
            double slope = r->model.weights[1];

            if (fabs(bias - 1.0) < 0.5 && fabs(slope - 2.0) < 0.5) {
                TEST_PASS("Learned weights approximate y=2x+1");
                tests_passed++;
            } else {
                TEST_FAIL("Learned weights", "Expected ~(1,2), got (%f,%f)", bias, slope);
                tests_failed++;
            }
        } else {
            TEST_FAIL("Valid input", "Model weights or history is NULL");
            tests_failed++;
        }

        // Cleanup
        fp_gradient_descent_free(r);
        free(r);
    } else {
        TEST_FAIL("Valid input", "Expected Right, got Left: %s", fp_from_left_msg(result));
        tests_failed++;
    }
}

void test_determinism(void) {
    Either r1 = fp_linear_regression_gradient_descent_safe(
        test_X, test_y, test_n, test_d, 0.1, 100, 1e-6, 42);
    Either r2 = fp_linear_regression_gradient_descent_safe(
        test_X, test_y, test_n, test_d, 0.1, 100, 1e-6, 42);

    if (fp_is_right(r1) && fp_is_right(r2)) {
        GradientDescentResult* res1 = (GradientDescentResult*)fp_from_right_ptr(r1);
        GradientDescentResult* res2 = (GradientDescentResult*)fp_from_right_ptr(r2);

        // Same seed should produce identical results
        int weights_match = 1;
        for (int i = 0; i <= res1->model.n_features; i++) {
            if (fabs(res1->model.weights[i] - res2->model.weights[i]) > EPSILON) {
                weights_match = 0;
                break;
            }
        }

        if (weights_match) {
            TEST_PASS("Same seed produces identical results");
            tests_passed++;
        } else {
            TEST_FAIL("Determinism", "Same seed produced different weights");
            tests_failed++;
        }

        fp_gradient_descent_free(res1);
        free(res1);
        fp_gradient_descent_free(res2);
        free(res2);
    } else {
        TEST_FAIL("Determinism", "One or both calls returned Left");
        tests_failed++;
    }
}

void test_zero_threshold(void) {
    // Zero threshold is valid (means no early stopping)
    Either result = fp_linear_regression_gradient_descent_safe(
        test_X, test_y, test_n, test_d, 0.1, 10, 0.0, 42);

    if (fp_is_right(result)) {
        GradientDescentResult* r = (GradientDescentResult*)fp_from_right_ptr(result);
        TEST_PASS("Zero convergence threshold is valid");
        tests_passed++;
        fp_gradient_descent_free(r);
        free(r);
    } else {
        TEST_FAIL("Zero threshold", "Expected Right, got Left");
        tests_failed++;
    }
}

/* ============================================================================
 * Main
 * ============================================================================ */

int main(void) {
    printf("========================================\n");
    printf("Linear Regression Safe API Tests\n");
    printf("========================================\n\n");

    printf("--- Error Case Tests ---\n");
    test_null_X();
    test_null_y();
    test_invalid_n();
    test_invalid_d();
    test_invalid_learning_rate();
    test_invalid_max_iterations();
    test_invalid_threshold();

    printf("\n--- Success Case Tests ---\n");
    test_valid_input();
    test_determinism();
    test_zero_threshold();

    printf("\n========================================\n");
    printf("Results: %d passed, %d failed\n", tests_passed, tests_failed);
    printf("========================================\n");

    return tests_failed > 0 ? 1 : 0;
}
