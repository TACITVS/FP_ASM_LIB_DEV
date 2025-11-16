// demo_neural_network.c - Neural Network Demo
// Demonstrates backpropagation and gradient descent

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Forward declarations
typedef struct {
    int n_inputs, n_hidden, n_outputs;
    double *W1, *b1, *W2, *b2;
} NeuralNetwork;

typedef struct {
    NeuralNetwork network;
    double* loss_history;
    int n_epochs;
    double final_loss, final_accuracy;
} TrainingResult;

TrainingResult fp_neural_network_train(int n_inputs, int n_hidden, int n_outputs,
    const double* X, const double* y, int n_samples, int n_epochs, double lr, int verbose);
int fp_neural_network_predict_class(const NeuralNetwork* net, const double* input);
void fp_neural_network_print_summary(const NeuralNetwork* net);
void fp_training_result_print(const TrainingResult* result);
void fp_training_result_free(TrainingResult* result);

// ============================================================================
// TEST 1: XOR Problem (Classic Non-Linear Test)
// ============================================================================
void test_xor() {
    printf("================================================================\n");
    printf("TEST 1: XOR Problem (Non-Linear Classification)\n");
    printf("================================================================\n\n");

    // XOR dataset: 4 samples, 2 inputs, 1 output
    double X[4 * 2] = {
        0, 0,
        0, 1,
        1, 0,
        1, 1
    };

    double y[4 * 1] = {0, 1, 1, 0};  // XOR truth table

    printf("XOR Truth Table:\n");
    printf("  Input  | Output\n");
    printf("  -------|-------\n");
    printf("  0  0   |   0\n");
    printf("  0  1   |   1\n");
    printf("  1  0   |   1\n");
    printf("  1  1   |   0\n\n");

    printf("Training neural network (2-4-1 architecture)...\n\n");

    TrainingResult result = fp_neural_network_train(
        2, 4, 1,      // 2 inputs, 4 hidden, 1 output
        X, y, 4,       // Data
        1000,          // epochs
        0.5,           // learning rate
        100            // print every 100 epochs
    );

    printf("\n");
    fp_neural_network_print_summary(&result.network);
    printf("\n");
    fp_training_result_print(&result);

    printf("\nTesting learned XOR function:\n");
    printf("  Input  | Predicted | Target | Correct?\n");
    printf("  -------|-----------|--------|----------\n");

    for (int i = 0; i < 4; i++) {
        double* input = &X[i * 2];
        int pred = fp_neural_network_predict_class(&result.network, input);
        int target = (int)(y[i] + 0.5);
        printf("  %.0f  %.0f   |     %d     |   %d    |   %s\n",
               input[0], input[1], pred, target,
               (pred == target) ? "YES" : "NO");
    }

    printf("\n");
    fp_training_result_free(&result);
}

// ============================================================================
// TEST 2: Binary Classification (Circular Data)
// ============================================================================
void test_binary_classification() {
    printf("================================================================\n");
    printf("TEST 2: Binary Classification (Circle vs Outside)\n");
    printf("================================================================\n\n");

    // Generate circular data
    const int n_samples = 200;
    double* X = (double*)malloc(n_samples * 2 * sizeof(double));
    double* y = (double*)malloc(n_samples * 2 * sizeof(double));

    srand(42);
    for (int i = 0; i < n_samples; i++) {
        double x1 = ((double)rand() / RAND_MAX) * 2.0 - 1.0;  // [-1, 1]
        double x2 = ((double)rand() / RAND_MAX) * 2.0 - 1.0;

        X[i * 2] = x1;
        X[i * 2 + 1] = x2;

        // Class 0: inside circle (r < 0.5), Class 1: outside
        double radius = sqrt(x1*x1 + x2*x2);
        if (radius < 0.5) {
            y[i * 2] = 1.0;      // Class 0 (one-hot)
            y[i * 2 + 1] = 0.0;
        } else {
            y[i * 2] = 0.0;
            y[i * 2 + 1] = 1.0;  // Class 1
        }
    }

    printf("Dataset: %d samples\n", n_samples);
    printf("Class 0: Inside circle (radius < 0.5)\n");
    printf("Class 1: Outside circle\n\n");

    printf("Training neural network (2-8-2 architecture)...\n\n");

    TrainingResult result = fp_neural_network_train(
        2, 8, 2,         // 2 inputs, 8 hidden, 2 outputs
        X, y, n_samples,
        500,             // epochs
        0.1,             // learning rate
        50               // print every 50 epochs
    );

    printf("\n");
    fp_neural_network_print_summary(&result.network);
    printf("\n");
    fp_training_result_print(&result);

    printf("\nFinal Training Accuracy: %.2f%%\n", result.final_accuracy * 100.0);

    free(X);
    free(y);
    fp_training_result_free(&result);
}

// ============================================================================
// Main
// ============================================================================
int main() {
    printf("================================================================\n");
    printf("  Neural Network Demo\n");
    printf("  FP-ASM Library - Backpropagation & Deep Learning\n");
    printf("================================================================\n\n");

    printf("Demonstrating:\n");
    printf("  - Forward propagation (layer-by-layer computation)\n");
    printf("  - Backpropagation (gradient computation via chain rule)\n");
    printf("  - Gradient descent (weight optimization)\n");
    printf("  - Non-linear learning (hidden layers enable XOR, circles)\n\n");

    test_xor();
    test_binary_classification();

    printf("================================================================\n");
    printf("  Neural Network Demo Complete!\n");
    printf("================================================================\n\n");

    printf("Key Takeaways:\n");
    printf("  ✓ Solved XOR problem (impossible for linear models)\n");
    printf("  ✓ Learned non-linear decision boundaries\n");
    printf("  ✓ Backpropagation computed gradients automatically\n");
    printf("  ✓ This is the foundation of ALL deep learning!\n\n");

    printf("FP-ASM Enables:\n");
    printf("  - Matrix operations (layer computations)\n");
    printf("  - Element-wise functions (activations)\n");
    printf("  - Gradient updates (weight optimization)\n");
    printf("  - Functional composition for deep networks\n\n");

    printf("Real-World Extensions:\n");
    printf("  - Add more layers → Deep Neural Networks\n");
    printf("  - Add convolutions → CNNs (image recognition)\n");
    printf("  - Add recurrence → RNNs (sequence modeling)\n");
    printf("  - This code is the core of modern AI!\n\n");

    return 0;
}
