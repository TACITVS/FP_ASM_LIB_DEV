/**
 * fp_neural_network.h - Multi-Layer Perceptron (Neural Network) API
 *
 * Provides a simple feedforward neural network with both standard and
 * safe (Either monad) interfaces. Uses L0 ASM primitives for SIMD-accelerated
 * computation.
 *
 * Features:
 * - Forward propagation
 * - Backpropagation with gradient descent
 * - Sigmoid and ReLU activation functions
 * - Safe variant returning Either monad for error handling
 * - Deterministic training with seed-based initialization
 *
 * Architecture: Input → Hidden Layer → Output Layer
 *
 * Example (standard):
 *   NeuralNetwork net = fp_neural_network_create(2, 4, 1, 42);
 *   TrainingResult result = fp_neural_network_train(
 *       2, 4, 1, X_train, y_train, n_train,
 *       0.1, 1000, 1, 42);
 *   fp_neural_network_free(&net);
 *
 * Example (safe/monadic):
 *   Either result = fp_neural_network_create_safe(2, 4, 1, 42);
 *   if (fp_is_right(result)) {
 *       NeuralNetwork* net = (NeuralNetwork*)fp_from_right_ptr(result);
 *       // Use network...
 *       fp_neural_network_free(net);  // Free internal arrays
 *       free(net);                    // Free the struct itself
 *   } else {
 *       printf("Error: %s (code %d)\n",
 *              fp_from_left_msg(result), fp_from_left_code(result));
 *   }
 */

#ifndef FP_NEURAL_NETWORK_H
#define FP_NEURAL_NETWORK_H

#include <stdint.h>
#include <stddef.h>
#include "fp_monads.h"

/* ============================================================================
 * NEURAL NETWORK STRUCTURES
 * ============================================================================ */

/**
 * NeuralNetwork - Simple 3-layer neural network architecture
 */
typedef struct {
    int n_inputs;                          // Input layer size
    int n_hidden;                          // Hidden layer size
    int n_outputs;                         // Output layer size

    // Weights and biases
    double* W1;                            // Input → Hidden weights (n_hidden × n_inputs)
    double* b1;                            // Hidden biases (n_hidden)
    double* W2;                            // Hidden → Output weights (n_outputs × n_hidden)
    double* b2;                            // Output biases (n_outputs)
} NeuralNetwork;

/**
 * TrainingResult - Result of neural network training
 */
typedef struct {
    NeuralNetwork network;
    double* loss_history;                  // Loss at each epoch
    int n_epochs;                          // Number of epochs trained
    double final_loss;                     // Final training loss
    double final_accuracy;                 // Final accuracy (for classification)
} TrainingResult;

/* ============================================================================
 * STANDARD NEURAL NETWORK API
 * ============================================================================ */

/**
 * fp_neural_network_create - Create and initialize neural network
 *
 * @param n_inputs        Number of input features
 * @param n_hidden        Number of hidden layer neurons
 * @param n_outputs       Number of output neurons
 * @param seed            RNG seed for deterministic Xavier initialization
 *
 * @return NeuralNetwork with allocated and initialized weights
 *
 * NOTE: Does NOT validate inputs. For safe version with validation,
 *       use fp_neural_network_create_safe().
 */
NeuralNetwork fp_neural_network_create(
    int n_inputs,
    int n_hidden,
    int n_outputs,
    uint64_t seed
);

/**
 * fp_neural_network_train - Train neural network with backpropagation
 *
 * @param n_inputs        Number of input features
 * @param n_hidden        Number of hidden layer neurons
 * @param n_outputs       Number of output neurons
 * @param X_train         Training data (n_samples × n_inputs, row-major)
 * @param y_train         Training labels (n_samples × n_outputs, row-major)
 * @param n_samples       Number of training samples
 * @param n_epochs        Number of training epochs
 * @param learning_rate   Learning rate for gradient descent
 * @param verbose         Print progress every N epochs (0 = no output)
 * @param seed            RNG seed for weight initialization
 *
 * @return TrainingResult with trained network and training history
 *
 * NOTE: Does NOT validate inputs. For safe version with validation,
 *       use a safe wrapper (to be implemented).
 */
TrainingResult fp_neural_network_train(
    int n_inputs,
    int n_hidden,
    int n_outputs,
    const double* X_train,
    const double* y_train,
    int n_samples,
    int n_epochs,
    double learning_rate,
    int verbose,
    uint64_t seed
);

/**
 * fp_neural_network_forward - Forward propagation (prediction)
 *
 * @param net             Trained neural network
 * @param input           Input features (n_inputs elements)
 * @param hidden_out      Output parameter for hidden layer activations (caller must free)
 *
 * @return Heap-allocated output array (n_outputs elements, caller must free)
 *
 * NOTE: Does NOT validate inputs. Caller must ensure net and input are non-NULL.
 */
double* fp_neural_network_forward(
    const NeuralNetwork* net,
    const double* input,
    double** hidden_out
);

/* ============================================================================
 * MEMORY MANAGEMENT
 * ============================================================================ */

/**
 * fp_neural_network_free - Free internal arrays of NeuralNetwork
 *
 * @param net             Pointer to network to free
 */
void fp_neural_network_free(NeuralNetwork* net);

/**
 * fp_training_result_free - Free internal arrays of TrainingResult
 *
 * @param result          Pointer to training result to free
 */
void fp_training_result_free(TrainingResult* result);

/* ============================================================================
 * PRINTING / VISUALIZATION
 * ============================================================================ */

/**
 * fp_neural_network_print_summary - Print network architecture
 */
void fp_neural_network_print_summary(const NeuralNetwork* net);

/**
 * fp_training_result_print - Print training summary
 */
void fp_training_result_print(const TrainingResult* result);

/* ============================================================================
 * SAFE API (Either Monad)
 * ============================================================================ */

/**
 * fp_neural_network_create_safe - Safe network creation with validation
 *
 * Returns Either monad for explicit error handling:
 * - Right(NeuralNetwork*) on success
 * - Left(error_msg, error_code) on failure
 *
 * Error codes:
 *   1 = (reserved for NULL inputs - not applicable here)
 *   2 = Invalid parameters (n_inputs<=0, n_hidden<=0, n_outputs<=0,
 *       or overflow in n_hidden*n_inputs, n_outputs*n_hidden)
 *   3 = Memory allocation failed
 *
 * Overflow protection:
 *   - Checks n_hidden * n_inputs < INT_MAX (W1 matrix)
 *   - Checks n_outputs * n_hidden < INT_MAX (W2 matrix)
 *
 * @param n_inputs        Number of input features (must be > 0)
 * @param n_hidden        Number of hidden layer neurons (must be > 0)
 * @param n_outputs       Number of output neurons (must be > 0)
 * @param seed            RNG seed for deterministic initialization
 *
 * @return Either containing Right(NeuralNetwork*) or Left(error_msg, code)
 *
 * MEMORY: On success, caller must:
 *   1. fp_neural_network_free(net) - Free internal arrays
 *   2. free(net) - Free the heap-allocated struct
 */
Either fp_neural_network_create_safe(
    int n_inputs,
    int n_hidden,
    int n_outputs,
    uint64_t seed
);

#endif /* FP_NEURAL_NETWORK_H */
