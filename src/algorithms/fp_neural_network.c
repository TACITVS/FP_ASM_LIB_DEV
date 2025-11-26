// fp_neural_network.c
//
// Simple Multi-Layer Perceptron (Neural Network)
// Demonstrates backpropagation and gradient descent
//
// This showcases:
// - Forward propagation (layer-by-layer computation)
// - Backpropagation (gradient computation via chain rule)
// - Non-linear learning (solves XOR, classification)
// - Functional composition for deep learning
//
// Architecture: Input → Hidden Layer(s) → Output
// Training: Mini-batch gradient descent with backpropagation
//
// FP Primitives Used:
// - Matrix-vector products (layer computations)
// - Element-wise operations (activations)
// - Gradient updates (weight optimization)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include "../include/fp_core.h"          // L0: Assembly primitives
#include "../include/fp_stats_v3_pure.h" // L1: Pure FP statistics
#include "../include/fp_rng.h"           // Deterministic RNG
#include "../include/fp_monads.h"        // Either monad for safe wrappers

// ============================================================================
// Pattern 1: Array Statistics - USING L1 LIBRARY (NO IMPERATIVE LOOPS!)
// ============================================================================

// Mean Squared Error: Compose L1 primitives
static inline double fp_mse_inline(const double* predicted, const double* target, size_t n) {
    if (!predicted || !target || n == 0) return 0.0;

    double* errors = (double*)malloc(n * sizeof(double));
    if (!errors) return 0.0;

    // L1: errors = actual - predicted using SIMD map
    fp_map_axpy_f64(predicted, target, errors, n, -1.0);

    // L0: MSE = dotp(errors, errors) / n using assembly
    double dotp = fp_fold_dotp_f64(errors, errors, n);
    free(errors);

    return dotp / (double)n;
}

// ============================================================================
// Pattern 4: Matrix Operations - COMPOSE L0 DOT PRODUCT
// ============================================================================
// Matrix-vector multiply: result = A * x + bias
// Each row is a dot product with x, then add bias
static inline void fp_matvec_add_bias_inline(
    const double* A,    // m × n matrix (row-major)
    const double* x,    // n × 1 vector
    const double* bias, // m × 1 bias vector
    double* result,     // m × 1 output
    size_t m,           // number of rows
    size_t n            // number of cols
) {
    // Each output element is: dot(A[row], x) + bias[row]
    for (size_t i = 0; i < m; i++) {
        // L0: Use assembly-optimized dot product for each row
        result[i] = fp_fold_dotp_f64(&A[i * n], x, n) + bias[i];
    }
}

// Neural network architecture
typedef struct {
    int n_inputs;          // Input layer size
    int n_hidden;          // Hidden layer size
    int n_outputs;         // Output layer size

    // Weights and biases
    double* W1;            // Input → Hidden weights (n_hidden × n_inputs)
    double* b1;            // Hidden biases (n_hidden)
    double* W2;            // Hidden → Output weights (n_outputs × n_hidden)
    double* b2;            // Output biases (n_outputs)
} NeuralNetwork;

// Training result
typedef struct {
    NeuralNetwork network;
    double* loss_history;  // Loss at each epoch
    int n_epochs;          // Number of epochs trained
    double final_loss;     // Final training loss
    double final_accuracy; // Final accuracy (for classification)
} TrainingResult;

// ============================================================================
// Activation Functions
// ============================================================================

// Sigmoid: σ(x) = 1 / (1 + e^(-x))
static inline double sigmoid(double x) {
    return 1.0 / (1.0 + exp(-x));
}

// Sigmoid derivative: σ'(x) = σ(x) * (1 - σ(x))
static inline double sigmoid_derivative(double sigmoid_output) {
    return sigmoid_output * (1.0 - sigmoid_output);
}

// ReLU: max(0, x)
static inline double relu(double x) {
    return x > 0.0 ? x : 0.0;
}

// ReLU derivative: 1 if x > 0, else 0
static inline double relu_derivative(double x) {
    return x > 0.0 ? 1.0 : 0.0;
}

// ============================================================================
// Network Initialization
// ============================================================================

// Create neural network with Xavier initialization
// REFACTORED: Uses fp_rng for deterministic, reproducible weight initialization
NeuralNetwork fp_neural_network_create(int n_inputs, int n_hidden, int n_outputs, uint64_t seed) {
    // CRIT-002 FIX: Validate dimensions to prevent integer overflow
    if (n_inputs > 0 && n_hidden > INT_MAX / n_inputs) {
        // Return zero-initialized network to indicate error
        NeuralNetwork empty = {0};
        return empty;
    }
    if (n_hidden > 0 && n_outputs > INT_MAX / n_hidden) {
        NeuralNetwork empty = {0};
        return empty;
    }

    // HIGH-009 FIX: Add input validation
    if (n_inputs <= 0 || n_hidden <= 0 || n_outputs <= 0) {
        NeuralNetwork error = {0};
        return error;
    }

    NeuralNetwork net;
    net.n_inputs = n_inputs;
    net.n_hidden = n_hidden;
    net.n_outputs = n_outputs;

    // Allocate weights and biases
    net.W1 = (double*)malloc(n_hidden * n_inputs * sizeof(double));
    net.b1 = (double*)calloc(n_hidden, sizeof(double));
    net.W2 = (double*)malloc(n_outputs * n_hidden * sizeof(double));
    net.b2 = (double*)calloc(n_outputs, sizeof(double));

    // HIGH-009 FIX: Add malloc null checks to prevent crashes
    if (!net.W1 || !net.b1 || !net.W2 || !net.b2) {
        free(net.W1);
        free(net.b1);
        free(net.W2);
        free(net.b2);
        NeuralNetwork empty = {0};
        return empty;
    }

    // DETERMINISTIC: Use fp_rng for Xavier initialization
    fp_rng_t rng = fp_rng_create(seed);
    double std1 = sqrt(2.0 / (n_inputs + n_hidden));
    rng = fp_rng_fill_f64_range(rng, net.W1, (size_t)(n_hidden * n_inputs), -std1, std1);

    // Xavier initialization for W2
    double std2 = sqrt(2.0 / (n_hidden + n_outputs));
    rng = fp_rng_fill_f64_range(rng, net.W2, (size_t)(n_outputs * n_hidden), -std2, std2);
    (void)rng;  // Suppress unused warning

    return net;
}

// Safe wrapper for fp_neural_network_create with Either monad overflow protection
// Returns Either monad: Left(error_msg, error_code) or Right(NeuralNetwork*)
Either fp_neural_network_create_safe(int n_inputs, int n_hidden, int n_outputs, uint64_t seed) {
    // Validate parameters
    if (n_inputs <= 0) return fp_left("Invalid n_inputs <= 0", 2);
    if (n_hidden <= 0) return fp_left("Invalid n_hidden <= 0", 2);
    if (n_outputs <= 0) return fp_left("Invalid n_outputs <= 0", 2);

    // Critical overflow checks
    // Check 1: n_hidden * n_inputs (W1 weight matrix)
    if (n_inputs > 0 && n_hidden > INT_MAX / n_inputs) {
        return fp_left("W1 matrix dimensions (n_hidden*n_inputs) would overflow INT_MAX", 2);
    }

    // Check 2: n_outputs * n_hidden (W2 weight matrix)
    if (n_hidden > 0 && n_outputs > INT_MAX / n_hidden) {
        return fp_left("W2 matrix dimensions (n_outputs*n_hidden) would overflow INT_MAX", 2);
    }

    // All validations passed - call unsafe function
    NeuralNetwork* net = (NeuralNetwork*)malloc(sizeof(NeuralNetwork));
    if (!net) return fp_left("Failed to allocate NeuralNetwork", 3);

    *net = fp_neural_network_create(n_inputs, n_hidden, n_outputs, seed);

    // Check if allocation in create succeeded (W1 and W2 should be non-NULL)
    if (!net->W1 || !net->W2) {
        free(net);
        return fp_left("Failed to allocate network weights", 3);
    }

    // Wrap result in Either Right
    return fp_right_ptr(net);
}

// ============================================================================
// Forward Propagation
// ============================================================================

// Forward pass: compute network output
// Returns output activations (caller must free)
double* fp_neural_network_forward(
    const NeuralNetwork* net,
    const double* input,       // n_inputs
    double** hidden_out        // Output: hidden activations (caller must free)
) {
    // Allocate hidden layer activations
    *hidden_out = (double*)malloc(net->n_hidden * sizeof(double));
    double* hidden = *hidden_out;

    // Layer 1: Input → Hidden
    // hidden = sigmoid(W1 * input + b1)
    for (int i = 0; i < net->n_hidden; i++) {
        double sum = net->b1[i];
        for (int j = 0; j < net->n_inputs; j++) {
            sum += net->W1[i * net->n_inputs + j] * input[j];
        }
        hidden[i] = sigmoid(sum);
    }

    // Allocate output layer
    double* output = (double*)malloc(net->n_outputs * sizeof(double));

    // Layer 2: Hidden → Output
    // output = sigmoid(W2 * hidden + b2)
    for (int i = 0; i < net->n_outputs; i++) {
        double sum = net->b2[i];
        for (int j = 0; j < net->n_hidden; j++) {
            sum += net->W2[i * net->n_hidden + j] * hidden[j];
        }
        output[i] = sigmoid(sum);
    }

    return output;
}

// ============================================================================
// Loss Functions
// ============================================================================

// Mean Squared Error loss
// REFACTORED: Now uses Pattern 1 fp_mse_inline()
static double mse_loss(const double* predicted, const double* target, int n) {
    return fp_mse_inline(predicted, target, n);
}

// ============================================================================
// Backpropagation
// ============================================================================

// Train network on single sample using backpropagation
static void backpropagate_single(
    NeuralNetwork* net,
    const double* input,
    const double* target,
    double learning_rate
) {
    // Forward pass
    double* hidden = NULL;
    double* output = fp_neural_network_forward(net, input, &hidden);

    // Allocate gradient storage
    double* output_grad = (double*)malloc(net->n_outputs * sizeof(double));
    double* hidden_grad = (double*)malloc(net->n_hidden * sizeof(double));

    // Backward pass - Output layer
    // output_grad = (output - target) * sigmoid'(output)
    for (int i = 0; i < net->n_outputs; i++) {
        output_grad[i] = (output[i] - target[i]) * sigmoid_derivative(output[i]);
    }

    // Backward pass - Hidden layer
    // hidden_grad = (W2^T * output_grad) * sigmoid'(hidden)
    for (int i = 0; i < net->n_hidden; i++) {
        double sum = 0.0;
        for (int j = 0; j < net->n_outputs; j++) {
            sum += net->W2[j * net->n_hidden + i] * output_grad[j];
        }
        hidden_grad[i] = sum * sigmoid_derivative(hidden[i]);
    }

    // Update weights and biases - Output layer
    // W2 -= learning_rate * output_grad * hidden^T
    // b2 -= learning_rate * output_grad
    // REFACTORED: Use L0 fp_map_axpy_f64 for each row (SIMD AVX2)
    for (int i = 0; i < net->n_outputs; i++) {
        // W2[i,:] -= (learning_rate * output_grad[i]) * hidden[:]
        double scale = -learning_rate * output_grad[i];
        fp_map_axpy_f64(hidden, &net->W2[i * net->n_hidden], &net->W2[i * net->n_hidden],
                        net->n_hidden, scale);
        net->b2[i] -= learning_rate * output_grad[i];
    }

    // Update weights and biases - Hidden layer
    // W1 -= learning_rate * hidden_grad * input^T
    // b1 -= learning_rate * hidden_grad
    // REFACTORED: Use L0 fp_map_axpy_f64 for each row (SIMD AVX2)
    for (int i = 0; i < net->n_hidden; i++) {
        // W1[i,:] -= (learning_rate * hidden_grad[i]) * input[:]
        double scale = -learning_rate * hidden_grad[i];
        fp_map_axpy_f64(input, &net->W1[i * net->n_inputs], &net->W1[i * net->n_inputs],
                        net->n_inputs, scale);
        net->b1[i] -= learning_rate * hidden_grad[i];
    }

    // Cleanup
    free(output);
    free(hidden);
    free(output_grad);
    free(hidden_grad);
}

// ============================================================================
// Training
// ============================================================================

// Train neural network using gradient descent
TrainingResult fp_neural_network_train(
    int n_inputs,
    int n_hidden,
    int n_outputs,
    const double* X_train,     // n_samples × n_inputs (row-major)
    const double* y_train,     // n_samples × n_outputs (one-hot for classification)
    int n_samples,
    int n_epochs,
    double learning_rate,
    int verbose,               // Print progress every N epochs (0 = no output)
    uint64_t seed              // RNG seed for reproducible weight initialization
) {
    TrainingResult result;
    // Use user-provided seed for reproducibility (FP purity)
    result.network = fp_neural_network_create(n_inputs, n_hidden, n_outputs, seed);
    result.loss_history = (double*)malloc(n_epochs * sizeof(double));
    result.n_epochs = n_epochs;

    // Training loop
    for (int epoch = 0; epoch < n_epochs; epoch++) {
        double total_loss = 0.0;

        // Train on each sample
        for (int i = 0; i < n_samples; i++) {
            const double* input = &X_train[i * n_inputs];
            const double* target = &y_train[i * n_outputs];

            // Compute loss before update
            double* hidden = NULL;
            double* output = fp_neural_network_forward(&result.network, input, &hidden);
            total_loss += mse_loss(output, target, n_outputs);
            free(output);
            free(hidden);

            // Backpropagation
            backpropagate_single(&result.network, input, target, learning_rate);
        }

        // Record average loss
        result.loss_history[epoch] = total_loss / n_samples;

        // Print progress
        if (verbose > 0 && (epoch % verbose == 0 || epoch == n_epochs - 1)) {
            printf("Epoch %4d/%d | Loss: %.6f\n", epoch + 1, n_epochs, result.loss_history[epoch]);
        }
    }

    result.final_loss = result.loss_history[n_epochs - 1];

    // Compute final accuracy (for classification)
    int correct = 0;
    for (int i = 0; i < n_samples; i++) {
        const double* input = &X_train[i * n_inputs];
        const double* target = &y_train[i * n_outputs];

        double* hidden = NULL;
        double* output = fp_neural_network_forward(&result.network, input, &hidden);

        // Find predicted and true class
        int pred_class = 0, true_class = 0;
        for (int j = 0; j < n_outputs; j++) {
            if (output[j] > output[pred_class]) pred_class = j;
            if (target[j] > target[true_class]) true_class = j;
        }

        if (pred_class == true_class) correct++;

        free(output);
        free(hidden);
    }
    result.final_accuracy = (double)correct / n_samples;

    return result;
}

// ============================================================================
// Prediction
// ============================================================================

// Predict single sample
double* fp_neural_network_predict(
    const NeuralNetwork* net,
    const double* input
) {
    double* hidden = NULL;
    double* output = fp_neural_network_forward(net, input, &hidden);
    free(hidden);
    return output;  // Caller must free
}

// Predict class (returns index of max output)
int fp_neural_network_predict_class(
    const NeuralNetwork* net,
    const double* input
) {
    double* output = fp_neural_network_predict(net, input);

    int max_idx = 0;
    for (int i = 1; i < net->n_outputs; i++) {
        if (output[i] > output[max_idx]) {
            max_idx = i;
        }
    }

    free(output);
    return max_idx;
}

// ============================================================================
// Evaluation
// ============================================================================

// Compute accuracy on dataset
double fp_neural_network_accuracy(
    const NeuralNetwork* net,
    const double* X_test,
    const double* y_test,
    int n_samples
) {
    int correct = 0;

    for (int i = 0; i < n_samples; i++) {
        const double* input = &X_test[i * net->n_inputs];
        const double* target = &y_test[i * net->n_outputs];

        int pred_class = fp_neural_network_predict_class(net, input);

        // Find true class
        int true_class = 0;
        for (int j = 1; j < net->n_outputs; j++) {
            if (target[j] > target[true_class]) {
                true_class = j;
            }
        }

        if (pred_class == true_class) correct++;
    }

    return (double)correct / n_samples;
}

// ============================================================================
// Memory Management
// ============================================================================

void fp_neural_network_free(NeuralNetwork* net) {
    free(net->W1);
    free(net->b1);
    free(net->W2);
    free(net->b2);
}

void fp_training_result_free(TrainingResult* result) {
    fp_neural_network_free(&result->network);
    free(result->loss_history);
}

// ============================================================================
// Printing and Visualization
// ============================================================================

void fp_neural_network_print_summary(const NeuralNetwork* net) {
    printf("Neural Network Architecture:\n");
    printf("  Input layer:  %d neurons\n", net->n_inputs);
    printf("  Hidden layer: %d neurons (sigmoid activation)\n", net->n_hidden);
    printf("  Output layer: %d neurons (sigmoid activation)\n", net->n_outputs);
    printf("  Total parameters: %d\n",
           net->n_hidden * net->n_inputs + net->n_hidden +
           net->n_outputs * net->n_hidden + net->n_outputs);
}

void fp_training_result_print(const TrainingResult* result) {
    printf("Training Results:\n");
    printf("  Epochs: %d\n", result->n_epochs);
    printf("  Final Loss: %.6f\n", result->final_loss);
    printf("  Final Accuracy: %.2f%%\n", result->final_accuracy * 100.0);
    printf("  Initial Loss: %.6f\n", result->loss_history[0]);
    printf("  Loss Reduction: %.6f (%.1f%%)\n",
           result->loss_history[0] - result->final_loss,
           100.0 * (result->loss_history[0] - result->final_loss) / result->loss_history[0]);
}
