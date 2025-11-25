/**
 * test_nn_safe.c - Unit tests for fp_neural_network_create_safe()
 *
 * Tests the Either monad wrapper for safe error handling in neural network creation.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include "fp_neural_network.h"

#define EPSILON 1e-6
#define TEST_PASS(name) printf("[PASS] %s\n", name)
#define TEST_FAIL(name, ...) do { printf("[FAIL] %s: ", name); printf(__VA_ARGS__); printf("\n"); } while(0)

static int tests_passed = 0;
static int tests_failed = 0;

/* ============================================================================
 * Test Configuration - XOR problem (classic non-linear test)
 * ============================================================================ */

static const int test_n_inputs = 2;
static const int test_n_hidden = 4;
static const int test_n_outputs = 1;
static const uint64_t test_seed = 42;

/* ============================================================================
 * Error Case Tests
 * ============================================================================ */

void test_invalid_n_inputs(void) {
    Either result = fp_neural_network_create_safe(0, test_n_hidden, test_n_outputs, test_seed);

    if (fp_is_left(result) && fp_from_left_code(result) == 2) {
        TEST_PASS("n_inputs=0 returns Left with code 2");
        tests_passed++;
    } else {
        TEST_FAIL("n_inputs=0", "Expected Left with code 2");
        tests_failed++;
    }
}

void test_negative_n_inputs(void) {
    Either result = fp_neural_network_create_safe(-1, test_n_hidden, test_n_outputs, test_seed);

    if (fp_is_left(result) && fp_from_left_code(result) == 2) {
        TEST_PASS("n_inputs=-1 returns Left with code 2");
        tests_passed++;
    } else {
        TEST_FAIL("n_inputs=-1", "Expected Left with code 2");
        tests_failed++;
    }
}

void test_invalid_n_hidden(void) {
    Either result = fp_neural_network_create_safe(test_n_inputs, 0, test_n_outputs, test_seed);

    if (fp_is_left(result) && fp_from_left_code(result) == 2) {
        TEST_PASS("n_hidden=0 returns Left with code 2");
        tests_passed++;
    } else {
        TEST_FAIL("n_hidden=0", "Expected Left with code 2");
        tests_failed++;
    }
}

void test_negative_n_hidden(void) {
    Either result = fp_neural_network_create_safe(test_n_inputs, -1, test_n_outputs, test_seed);

    if (fp_is_left(result) && fp_from_left_code(result) == 2) {
        TEST_PASS("n_hidden=-1 returns Left with code 2");
        tests_passed++;
    } else {
        TEST_FAIL("n_hidden=-1", "Expected Left with code 2");
        tests_failed++;
    }
}

void test_invalid_n_outputs(void) {
    Either result = fp_neural_network_create_safe(test_n_inputs, test_n_hidden, 0, test_seed);

    if (fp_is_left(result) && fp_from_left_code(result) == 2) {
        TEST_PASS("n_outputs=0 returns Left with code 2");
        tests_passed++;
    } else {
        TEST_FAIL("n_outputs=0", "Expected Left with code 2");
        tests_failed++;
    }
}

void test_negative_n_outputs(void) {
    Either result = fp_neural_network_create_safe(test_n_inputs, test_n_hidden, -1, test_seed);

    if (fp_is_left(result) && fp_from_left_code(result) == 2) {
        TEST_PASS("n_outputs=-1 returns Left with code 2");
        tests_passed++;
    } else {
        TEST_FAIL("n_outputs=-1", "Expected Left with code 2");
        tests_failed++;
    }
}

void test_overflow_W1_matrix(void) {
    // Test n_hidden * n_inputs overflow (W1 weight matrix)
    // Use n_hidden=60000, n_inputs=60000: 60000 * 60000 = 3,600,000,000 > INT_MAX
    // INT_MAX / 60000 = 35,791, so 60000 > 35791 triggers overflow
    Either result = fp_neural_network_create_safe(60000, 60000, test_n_outputs, test_seed);

    if (fp_is_left(result) && fp_from_left_code(result) == 2) {
        TEST_PASS("W1 matrix overflow (60000×60000) returns Left");
        tests_passed++;
    } else {
        TEST_FAIL("W1 matrix overflow", "Expected Left with code 2");
        tests_failed++;
    }
}

void test_overflow_W2_matrix(void) {
    // Test n_outputs * n_hidden overflow (W2 weight matrix)
    // Use n_outputs=70000, n_hidden=70000: 70000 * 70000 = 4,900,000,000 > INT_MAX
    // INT_MAX / 70000 = 30,678, so 70000 > 30678 triggers overflow
    Either result = fp_neural_network_create_safe(test_n_inputs, 70000, 70000, test_seed);

    if (fp_is_left(result) && fp_from_left_code(result) == 2) {
        TEST_PASS("W2 matrix overflow (70000×70000) returns Left");
        tests_passed++;
    } else {
        TEST_FAIL("W2 matrix overflow", "Expected Left with code 2");
        tests_failed++;
    }
}

void test_overflow_combined(void) {
    // Test with both matrices at overflow boundary
    // Use n_inputs=50000, n_hidden=50000, n_outputs=50000
    // Both W1 and W2 would be 50000*50000 = 2,500,000,000 > INT_MAX
    Either result = fp_neural_network_create_safe(50000, 50000, 50000, test_seed);

    if (fp_is_left(result) && fp_from_left_code(result) == 2) {
        TEST_PASS("Combined overflow (both matrices) returns Left");
        tests_passed++;
    } else {
        TEST_FAIL("Combined overflow", "Expected Left with code 2");
        tests_failed++;
    }
}

void test_safe_dimensions(void) {
    // Test dimensions that are safe (no overflow)
    // Use test configuration: 2×4×1 - all products << INT_MAX
    // This verifies the overflow check doesn't false-positive on safe values
    Either result = fp_neural_network_create_safe(
        test_n_inputs, test_n_hidden, test_n_outputs, test_seed);

    // This should succeed since dimensions are valid
    if (fp_is_left(result)) {
        int code = fp_from_left_code(result);
        if (code != 2) {
            TEST_PASS("Safe dimensions (2-4-1) do not trigger overflow error");
            tests_passed++;
        } else {
            TEST_FAIL("Safe dimensions", "Got overflow error (code 2), but 2*4 and 4*1 are well below INT_MAX");
            tests_failed++;
        }
    } else {
        TEST_PASS("Safe dimensions (2-4-1) do not trigger overflow error");
        tests_passed++;
        // Cleanup
        NeuralNetwork* net = (NeuralNetwork*)fp_from_right_ptr(result);
        fp_neural_network_free(net);
        free(net);
    }
}

/* ============================================================================
 * Success Case Tests
 * ============================================================================ */

void test_valid_input(void) {
    Either result = fp_neural_network_create_safe(
        test_n_inputs, test_n_hidden, test_n_outputs, test_seed);

    if (fp_is_right(result)) {
        NeuralNetwork* net = (NeuralNetwork*)fp_from_right_ptr(result);

        // Check network was created (weights allocated)
        if (net->W1 != NULL && net->b1 != NULL &&
            net->W2 != NULL && net->b2 != NULL) {
            TEST_PASS("Valid input returns Right with allocated network");
            tests_passed++;

            // Check dimensions match
            if (net->n_inputs == test_n_inputs &&
                net->n_hidden == test_n_hidden &&
                net->n_outputs == test_n_outputs) {
                TEST_PASS("Network dimensions match input");
                tests_passed++;
            } else {
                TEST_FAIL("Network dimensions", "Expected (%d,%d,%d), got (%d,%d,%d)",
                         test_n_inputs, test_n_hidden, test_n_outputs,
                         net->n_inputs, net->n_hidden, net->n_outputs);
                tests_failed++;
            }

            // Check weights are non-zero (Xavier initialization should set them)
            int has_nonzero_weights = 0;
            for (int i = 0; i < net->n_hidden * net->n_inputs; i++) {
                if (fabs(net->W1[i]) > EPSILON) {
                    has_nonzero_weights = 1;
                    break;
                }
            }
            if (has_nonzero_weights) {
                TEST_PASS("Weights are initialized (Xavier)");
                tests_passed++;
            } else {
                TEST_FAIL("Weights initialization", "All W1 weights are zero");
                tests_failed++;
            }

            // Check biases are initialized to zero
            int biases_are_zero = 1;
            for (int i = 0; i < net->n_hidden; i++) {
                if (fabs(net->b1[i]) > EPSILON) {
                    biases_are_zero = 0;
                    break;
                }
            }
            if (biases_are_zero) {
                TEST_PASS("Biases are initialized to zero");
                tests_passed++;
            } else {
                TEST_FAIL("Biases initialization", "b1 biases are not zero");
                tests_failed++;
            }
        } else {
            TEST_FAIL("Valid input", "One or more weight matrices is NULL");
            tests_failed++;
        }

        // Cleanup
        fp_neural_network_free(net);
        free(net);
    } else {
        TEST_FAIL("Valid input", "Expected Right, got Left: %s", fp_from_left_msg(result));
        tests_failed++;
    }
}

void test_determinism(void) {
    Either r1 = fp_neural_network_create_safe(
        test_n_inputs, test_n_hidden, test_n_outputs, test_seed);
    Either r2 = fp_neural_network_create_safe(
        test_n_inputs, test_n_hidden, test_n_outputs, test_seed);

    if (fp_is_right(r1) && fp_is_right(r2)) {
        NeuralNetwork* net1 = (NeuralNetwork*)fp_from_right_ptr(r1);
        NeuralNetwork* net2 = (NeuralNetwork*)fp_from_right_ptr(r2);

        // Same seed should produce identical weights
        int weights_match = 1;
        for (int i = 0; i < net1->n_hidden * net1->n_inputs; i++) {
            if (fabs(net1->W1[i] - net2->W1[i]) > EPSILON) {
                weights_match = 0;
                break;
            }
        }

        if (weights_match) {
            for (int i = 0; i < net1->n_outputs * net1->n_hidden; i++) {
                if (fabs(net1->W2[i] - net2->W2[i]) > EPSILON) {
                    weights_match = 0;
                    break;
                }
            }
        }

        if (weights_match) {
            TEST_PASS("Same seed produces identical weight initialization");
            tests_passed++;
        } else {
            TEST_FAIL("Determinism", "Same seed produced different weights");
            tests_failed++;
        }

        fp_neural_network_free(net1);
        free(net1);
        fp_neural_network_free(net2);
        free(net2);
    } else {
        // Clean up any successful allocation before failing
        if (fp_is_right(r1)) {
            NeuralNetwork* net1 = (NeuralNetwork*)fp_from_right_ptr(r1);
            fp_neural_network_free(net1);
            free(net1);
        }
        if (fp_is_right(r2)) {
            NeuralNetwork* net2 = (NeuralNetwork*)fp_from_right_ptr(r2);
            fp_neural_network_free(net2);
            free(net2);
        }
        TEST_FAIL("Determinism", "One or both calls returned Left");
        tests_failed++;
    }
}

void test_large_but_safe_network(void) {
    // Test a large network that's still safe (no overflow)
    // Use 1000 inputs, 1000 hidden, 10 outputs
    // W1: 1000 * 1000 = 1,000,000 << INT_MAX (safe)
    // W2: 10 * 1000 = 10,000 << INT_MAX (safe)
    Either result = fp_neural_network_create_safe(1000, 1000, 10, test_seed);

    if (fp_is_right(result)) {
        TEST_PASS("Large but safe network (1000-1000-10) succeeds");
        tests_passed++;
        NeuralNetwork* net = (NeuralNetwork*)fp_from_right_ptr(result);
        fp_neural_network_free(net);
        free(net);
    } else {
        int code = fp_from_left_code(result);
        if (code == 2) {
            TEST_FAIL("Large but safe network", "Got overflow error, but 1000*1000 < INT_MAX");
            tests_failed++;
        } else if (code == 3) {
            // Memory allocation failure is acceptable for such a large network
            TEST_PASS("Large network allocation failure (code 3) is acceptable");
            tests_passed++;
        } else {
            TEST_FAIL("Large but safe network", "Unexpected error code: %d", code);
            tests_failed++;
        }
    }
}

/* ============================================================================
 * Main
 * ============================================================================ */

int main(void) {
    printf("========================================\n");
    printf("Neural Network Safe API Tests\n");
    printf("========================================\n\n");

    printf("--- Error Case Tests ---\n");
    test_invalid_n_inputs();
    test_negative_n_inputs();
    test_invalid_n_hidden();
    test_negative_n_hidden();
    test_invalid_n_outputs();
    test_negative_n_outputs();

    printf("\n--- Overflow Protection Tests ---\n");
    test_overflow_W1_matrix();
    test_overflow_W2_matrix();
    test_overflow_combined();
    test_safe_dimensions();

    printf("\n--- Success Case Tests ---\n");
    test_valid_input();
    test_determinism();
    test_large_but_safe_network();

    printf("\n========================================\n");
    printf("Results: %d passed, %d failed\n", tests_passed, tests_failed);
    printf("========================================\n");

    return tests_failed > 0 ? 1 : 0;
}
