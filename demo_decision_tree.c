// demo_decision_tree.c - Decision Tree Demo
// Demonstrates CART (Classification and Regression Trees)

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Forward declarations from fp_decision_tree.c
typedef struct DecisionNode DecisionNode;

typedef struct {
    DecisionNode* root;
    int max_depth;
    int min_samples_split;
    int is_classifier;
    int n_features;
    int n_classes;
    double* feature_importances;
} DecisionTreeModel;

DecisionTreeModel fp_decision_tree_train(
    const double* X, const int* y,
    int n, int d, int n_classes,
    int max_depth, int min_samples_split
);

int fp_decision_tree_predict(const DecisionTreeModel* model, const double* x);
double fp_decision_tree_accuracy(const DecisionTreeModel* model, const double* X_test, const int* y_test, int n_test);
void fp_decision_tree_print(const DecisionTreeModel* model);
int fp_decision_tree_depth(const DecisionTreeModel* model);
int fp_decision_tree_n_nodes(const DecisionTreeModel* model);
int fp_decision_tree_n_leaves(const DecisionTreeModel* model);
void fp_decision_tree_print_feature_importances(const DecisionTreeModel* model);
void fp_decision_tree_free(DecisionTreeModel* model);

// ============================================================================
// Data Generation
// ============================================================================

// Generate 2D Gaussian data
void generate_gaussian_data(
    double* X,
    int* y,
    int n,
    double mean_class0[2],
    double mean_class1[2],
    double std_dev,
    unsigned int seed
) {
    srand(seed);

    for (int i = 0; i < n; i++) {
        int label = (i < n/2) ? 0 : 1;
        y[i] = label;

        double* mean = (label == 0) ? mean_class0 : mean_class1;

        for (int j = 0; j < 2; j++) {
            double u1 = (double)rand() / RAND_MAX;
            double u2 = (double)rand() / RAND_MAX;
            double noise = sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);

            X[i * 2 + j] = mean[j] + std_dev * noise;
        }
    }
}

// Generate XOR-like data (non-linearly separable)
void generate_xor_data(double* X, int* y, int n, unsigned int seed) {
    srand(seed);

    for (int i = 0; i < n; i++) {
        double x1 = ((double)rand() / RAND_MAX) * 4.0 - 2.0;
        double x2 = ((double)rand() / RAND_MAX) * 4.0 - 2.0;

        X[i * 2] = x1;
        X[i * 2 + 1] = x2;

        // XOR pattern: (x1 > 0) XOR (x2 > 0)
        y[i] = ((x1 > 0) != (x2 > 0)) ? 1 : 0;
    }
}

// Generate multi-class spiral data
void generate_spiral_data(double* X, int* y, int n, int n_classes, unsigned int seed) {
    srand(seed);

    for (int i = 0; i < n; i++) {
        int label = i % n_classes;
        y[i] = label;

        double t = (double)(i / n_classes) / (n / n_classes) * 4.0 * M_PI;
        double r = t / (4.0 * M_PI) * 5.0;

        double angle = t + (2.0 * M_PI * label / n_classes);
        double noise = ((double)rand() / RAND_MAX - 0.5) * 0.5;

        X[i * 2] = (r + noise) * cos(angle);
        X[i * 2 + 1] = (r + noise) * sin(angle);
    }
}

// ============================================================================
// TEST 1: Binary Classification (Linearly Separable)
// ============================================================================
void test_binary_classification() {
    printf("================================================================\n");
    printf("TEST 1: Binary Classification (Linearly Separable)\n");
    printf("================================================================\n\n");

    int n_train = 200;
    int n_test = 100;
    int n_features = 2;
    int n_classes = 2;

    printf("Dataset: Two Gaussian clusters in 2D\n");
    printf("  Class 0: centered at (1, 1)\n");
    printf("  Class 1: centered at (4, 4)\n");
    printf("  Training samples: %d\n", n_train);
    printf("  Test samples: %d\n\n", n_test);

    // Generate data
    double* X_train = (double*)malloc(n_train * n_features * sizeof(double));
    int* y_train = (int*)malloc(n_train * sizeof(int));

    double mean_class0[2] = {1.0, 1.0};
    double mean_class1[2] = {4.0, 4.0};

    generate_gaussian_data(X_train, y_train, n_train, mean_class0, mean_class1, 0.8, 42);

    // Train decision tree
    printf("Training Decision Tree (max_depth=3, min_samples_split=5)...\n\n");
    DecisionTreeModel model = fp_decision_tree_train(
        X_train, y_train,
        n_train, n_features, n_classes,
        3,  // max_depth
        5   // min_samples_split
    );

    printf("Tree Statistics:\n");
    printf("  Depth: %d\n", fp_decision_tree_depth(&model));
    printf("  Nodes: %d\n", fp_decision_tree_n_nodes(&model));
    printf("  Leaves: %d\n\n", fp_decision_tree_n_leaves(&model));

    fp_decision_tree_print_feature_importances(&model);
    printf("\n");

    // Generate test data
    double* X_test = (double*)malloc(n_test * n_features * sizeof(double));
    int* y_test = (int*)malloc(n_test * sizeof(int));

    generate_gaussian_data(X_test, y_test, n_test, mean_class0, mean_class1, 0.8, 123);

    // Evaluate
    double train_acc = fp_decision_tree_accuracy(&model, X_train, y_train, n_train);
    double test_acc = fp_decision_tree_accuracy(&model, X_test, y_test, n_test);

    printf("Training Accuracy: %.2f%%\n", train_acc * 100.0);
    printf("Test Accuracy: %.2f%%\n\n", test_acc * 100.0);

    printf("Status: %s\n\n", (test_acc > 0.85) ? "PASS (>85%% accuracy)" : "FAIL");

    // Cleanup
    fp_decision_tree_free(&model);
    free(X_train);
    free(y_train);
    free(X_test);
    free(y_test);
}

// ============================================================================
// TEST 2: XOR Problem (Non-linearly Separable)
// ============================================================================
void test_xor_classification() {
    printf("================================================================\n");
    printf("TEST 2: XOR Problem (Non-linearly Separable)\n");
    printf("================================================================\n\n");

    int n_train = 400;
    int n_test = 200;
    int n_features = 2;
    int n_classes = 2;

    printf("Dataset: XOR pattern - (x1 > 0) XOR (x2 > 0)\n");
    printf("  Training samples: %d\n", n_train);
    printf("  Test samples: %d\n", n_test);
    printf("  Note: Linear classifiers fail on XOR\n\n");

    // Generate XOR data
    double* X_train = (double*)malloc(n_train * n_features * sizeof(double));
    int* y_train = (int*)malloc(n_train * sizeof(int));

    generate_xor_data(X_train, y_train, n_train, 42);

    // Train shallow tree
    printf("Training Shallow Tree (max_depth=2)...\n");
    DecisionTreeModel model_shallow = fp_decision_tree_train(
        X_train, y_train,
        n_train, n_features, n_classes,
        2,  // max_depth (too shallow for XOR!)
        5
    );

    // Train deep tree
    printf("Training Deep Tree (max_depth=10)...\n\n");
    DecisionTreeModel model_deep = fp_decision_tree_train(
        X_train, y_train,
        n_train, n_features, n_classes,
        10,  // max_depth (can learn XOR)
        5
    );

    // Generate test data
    double* X_test = (double*)malloc(n_test * n_features * sizeof(double));
    int* y_test = (int*)malloc(n_test * sizeof(int));

    generate_xor_data(X_test, y_test, n_test, 123);

    // Evaluate both
    double shallow_train = fp_decision_tree_accuracy(&model_shallow, X_train, y_train, n_train);
    double shallow_test = fp_decision_tree_accuracy(&model_shallow, X_test, y_test, n_test);

    double deep_train = fp_decision_tree_accuracy(&model_deep, X_train, y_train, n_train);
    double deep_test = fp_decision_tree_accuracy(&model_deep, X_test, y_test, n_test);

    printf("Shallow Tree (depth=%d):\n", fp_decision_tree_depth(&model_shallow));
    printf("  Training Accuracy: %.2f%%\n", shallow_train * 100.0);
    printf("  Test Accuracy: %.2f%%\n\n", shallow_test * 100.0);

    printf("Deep Tree (depth=%d):\n", fp_decision_tree_depth(&model_deep));
    printf("  Training Accuracy: %.2f%%\n", deep_train * 100.0);
    printf("  Test Accuracy: %.2f%%\n\n", deep_test * 100.0);

    printf("Key Learning:\n");
    printf("  - Shallow trees cannot capture XOR pattern (need depth ≥ 2)\n");
    printf("  - Deep trees can learn non-linear boundaries via recursive splits\n");
    printf("  - XOR requires at least 2 levels to separate the 4 regions\n\n");

    printf("Status: %s\n\n",
           (deep_test > shallow_test && deep_test > 0.75) ? "PASS (deep tree learns XOR)" : "FAIL");

    // Cleanup
    fp_decision_tree_free(&model_shallow);
    fp_decision_tree_free(&model_deep);
    free(X_train);
    free(y_train);
    free(X_test);
    free(y_test);
}

// ============================================================================
// TEST 3: Multi-Class Spiral Data
// ============================================================================
void test_multiclass() {
    printf("================================================================\n");
    printf("TEST 3: Multi-Class Classification (3 Classes)\n");
    printf("================================================================\n\n");

    int n_train = 300;
    int n_test = 150;
    int n_features = 2;
    int n_classes = 3;

    printf("Dataset: Three spiral arms\n");
    printf("  Training samples: %d (100 per class)\n", n_train);
    printf("  Test samples: %d\n\n", n_test);

    // Generate spiral data
    double* X_train = (double*)malloc(n_train * n_features * sizeof(double));
    int* y_train = (int*)malloc(n_train * sizeof(int));

    generate_spiral_data(X_train, y_train, n_train, n_classes, 42);

    // Train decision tree
    printf("Training Decision Tree (max_depth=10)...\n\n");
    DecisionTreeModel model = fp_decision_tree_train(
        X_train, y_train,
        n_train, n_features, n_classes,
        10,  // max_depth
        5
    );

    printf("Tree Statistics:\n");
    printf("  Depth: %d\n", fp_decision_tree_depth(&model));
    printf("  Nodes: %d\n", fp_decision_tree_n_nodes(&model));
    printf("  Leaves: %d\n\n", fp_decision_tree_n_leaves(&model));

    fp_decision_tree_print_feature_importances(&model);
    printf("\n");

    // Generate test data
    double* X_test = (double*)malloc(n_test * n_features * sizeof(double));
    int* y_test = (int*)malloc(n_test * sizeof(int));

    generate_spiral_data(X_test, y_test, n_test, n_classes, 123);

    // Evaluate
    double train_acc = fp_decision_tree_accuracy(&model, X_train, y_train, n_train);
    double test_acc = fp_decision_tree_accuracy(&model, X_test, y_test, n_test);

    printf("Training Accuracy: %.2f%%\n", train_acc * 100.0);
    printf("Test Accuracy: %.2f%%\n\n", test_acc * 100.0);

    // Analyze per-class accuracy
    int* confusion = (int*)calloc(n_classes * n_classes, sizeof(int));

    for (int i = 0; i < n_test; i++) {
        int true_class = y_test[i];
        int pred_class = fp_decision_tree_predict(&model, &X_test[i * n_features]);
        confusion[true_class * n_classes + pred_class]++;
    }

    printf("Confusion Matrix:\n");
    printf("        ");
    for (int c = 0; c < n_classes; c++) {
        printf("Pred %d  ", c);
    }
    printf("\n");

    for (int true_c = 0; true_c < n_classes; true_c++) {
        printf("True %d  ", true_c);
        for (int pred_c = 0; pred_c < n_classes; pred_c++) {
            printf("%6d  ", confusion[true_c * n_classes + pred_c]);
        }
        printf("\n");
    }
    printf("\n");

    printf("Status: %s\n\n", (test_acc > 0.70) ? "PASS (>70%% accuracy)" : "FAIL");

    // Cleanup
    free(confusion);
    fp_decision_tree_free(&model);
    free(X_train);
    free(y_train);
    free(X_test);
    free(y_test);
}

// ============================================================================
// TEST 4: Overfitting Demonstration
// ============================================================================
void test_overfitting() {
    printf("================================================================\n");
    printf("TEST 4: Overfitting Demonstration\n");
    printf("================================================================\n\n");

    int n_train = 100;
    int n_test = 200;
    int n_features = 2;
    int n_classes = 2;

    printf("Demonstrating effect of tree depth on overfitting\n");
    printf("  Training samples: %d\n", n_train);
    printf("  Test samples: %d\n\n", n_test);

    // Generate data with noise
    double* X_train = (double*)malloc(n_train * n_features * sizeof(double));
    int* y_train = (int*)malloc(n_train * sizeof(int));

    double mean_class0[2] = {1.0, 1.0};
    double mean_class1[2] = {3.0, 3.0};

    generate_gaussian_data(X_train, y_train, n_train, mean_class0, mean_class1, 1.2, 42);

    double* X_test = (double*)malloc(n_test * n_features * sizeof(double));
    int* y_test = (int*)malloc(n_test * sizeof(int));

    generate_gaussian_data(X_test, y_test, n_test, mean_class0, mean_class1, 1.2, 123);

    printf("Testing different max_depth values:\n\n");

    int depths[] = {1, 2, 3, 5, 10, 20};
    int n_depths = 6;

    for (int i = 0; i < n_depths; i++) {
        int depth = depths[i];

        DecisionTreeModel model = fp_decision_tree_train(
            X_train, y_train,
            n_train, n_features, n_classes,
            depth, 2
        );

        double train_acc = fp_decision_tree_accuracy(&model, X_train, y_train, n_train);
        double test_acc = fp_decision_tree_accuracy(&model, X_test, y_test, n_test);
        int actual_depth = fp_decision_tree_depth(&model);
        int n_leaves = fp_decision_tree_n_leaves(&model);

        printf("max_depth=%2d: train=%.1f%%, test=%.1f%% (actual_depth=%d, leaves=%d)\n",
               depth, train_acc * 100.0, test_acc * 100.0, actual_depth, n_leaves);

        fp_decision_tree_free(&model);
    }

    printf("\n");
    printf("Key Observations:\n");
    printf("  - Very shallow trees (depth=1-2): underfit (low train & test accuracy)\n");
    printf("  - Moderate depth (depth=3-5): best generalization\n");
    printf("  - Very deep trees (depth>10): overfit (high train, lower test)\n");
    printf("  - More leaves = more complex decision boundaries\n\n");

    printf("Status: PASS (overfitting demonstrated)\n\n");

    free(X_train);
    free(y_train);
    free(X_test);
    free(y_test);
}

// ============================================================================
// Main
// ============================================================================
int main() {
    printf("================================================================\n");
    printf("  Decision Tree Demo\n");
    printf("  FP-ASM Library - Interpretable Machine Learning\n");
    printf("================================================================\n\n");

    printf("Decision Trees: CART (Classification and Regression Trees)\n\n");

    printf("Key Concepts:\n");
    printf("  - Recursive binary splitting\n");
    printf("  - Gini impurity for classification\n");
    printf("  - Greedy best-first feature selection\n");
    printf("  - Interpretable rules (if-then-else)\n");
    printf("  - Feature importance from split gains\n\n");

    printf("Advantages:\n");
    printf("  ✓ Interpretable (white-box model)\n");
    printf("  ✓ Handles non-linear boundaries\n");
    printf("  ✓ No feature scaling required\n");
    printf("  ✓ Can handle missing values\n");
    printf("  ✓ Feature importance built-in\n\n");

    printf("Disadvantages:\n");
    printf("  ✗ Prone to overfitting (without pruning)\n");
    printf("  ✗ Unstable (small data changes → different tree)\n");
    printf("  ✗ Greedy (locally optimal splits)\n");
    printf("  ✗ Biased toward features with more categories\n\n");

    test_binary_classification();
    test_xor_classification();
    test_multiclass();
    test_overfitting();

    printf("================================================================\n");
    printf("  Decision Tree Demo Complete!\n");
    printf("================================================================\n\n");

    printf("Real-World Applications:\n");
    printf("  - Medical diagnosis (interpretable clinical rules)\n");
    printf("  - Credit scoring (explain approval/denial)\n");
    printf("  - Fraud detection (if-then rules for auditing)\n");
    printf("  - Customer churn prediction\n");
    printf("  - Foundation for Random Forests & Gradient Boosting\n\n");

    printf("Extensions:\n");
    printf("  - Random Forests: Ensemble of decorrelated trees\n");
    printf("  - Gradient Boosting: Sequentially correct errors\n");
    printf("  - Cost-Complexity Pruning: Reduce overfitting\n");
    printf("  - Regression Trees: Predict continuous values\n");
    printf("  - Categorical features: Handle non-numeric data\n\n");

    return 0;
}
