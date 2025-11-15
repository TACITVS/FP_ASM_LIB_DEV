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
#include <time.h>

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

// Softmax: exp(x_i) / sum(exp(x_j))
static void softmax(const double* input, double* output, int n) {
    // Find max for numerical stability
    double max_val = input[0];
    for (int i = 1; i < n; i++) {
        if (input[i] > max_val) max_val = input[i];
    }

    // Compute exp(x - max) and sum
    double sum = 0.0;
    for (int i = 0; i < n; i++) {
        output[i] = exp(input[i] - max_val);
        sum += output[i];
    }

    // Normalize
    for (int i = 0; i < n; i++) {
        output[i] /= sum;
    }
}

// ============================================================================
// Network Initialization
// ============================================================================

// Create neural network with Xavier initialization
NeuralNetwork fp_neural_network_create(int n_inputs, int n_hidden, int n_outputs) {
    NeuralNetwork net;
    net.n_inputs = n_inputs;
    net.n_hidden = n_hidden;
    net.n_outputs = n_outputs;

    // Allocate weights and biases
    net.W1 = (double*)malloc(n_hidden * n_inputs * sizeof(double));
    net.b1 = (double*)calloc(n_hidden, sizeof(double));
    net.W2 = (double*)malloc(n_outputs * n_hidden * sizeof(double));
    net.b2 = (double*)calloc(n_outputs, sizeof(double));

    // Xavier initialization for W1
    srand(42);
    double std1 = sqrt(2.0 / (n_inputs + n_hidden));
    for (int i = 0; i < n_hidden * n_inputs; i++) {
        net.W1[i] = ((double)rand() / RAND_MAX - 0.5) * 2.0 * std1;
    }

    // Xavier initialization for W2
    double std2 = sqrt(2.0 / (n_hidden + n_outputs));
    for (int i = 0; i < n_outputs * n_hidden; i++) {
        net.W2[i] = ((double)rand() / RAND_MAX - 0.5) * 2.0 * std2;
    }

    return net;
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
static double mse_loss(const double* predicted, const double* target, int n) {
    double sum = 0.0;
    for (int i = 0; i < n; i++) {
        double error = predicted[i] - target[i];
        sum += error * error;
    }
    return sum / n;
}

// Cross-entropy loss (for classification)
static double cross_entropy_loss(const double* predicted, const double* target, int n) {
    double sum = 0.0;
    for (int i = 0; i < n; i++) {
        // Avoid log(0)
        double p = predicted[i];
        if (p < 1e-15) p = 1e-15;
        if (p > 1.0 - 1e-15) p = 1.0 - 1e-15;
        sum -= target[i] * log(p);
    }
    return sum;
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
    for (int i = 0; i < net->n_outputs; i++) {
        for (int j = 0; j < net->n_hidden; j++) {
            net->W2[i * net->n_hidden + j] -= learning_rate * output_grad[i] * hidden[j];
        }
        net->b2[i] -= learning_rate * output_grad[i];
    }

    // Update weights and biases - Hidden layer
    // W1 -= learning_rate * hidden_grad * input^T
    // b1 -= learning_rate * hidden_grad
    for (int i = 0; i < net->n_hidden; i++) {
        for (int j = 0; j < net->n_inputs; j++) {
            net->W1[i * net->n_inputs + j] -= learning_rate * hidden_grad[i] * input[j];
        }
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
    int verbose                // Print progress every N epochs (0 = no output)
) {
    TrainingResult result;
    result.network = fp_neural_network_create(n_inputs, n_hidden, n_outputs);
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
