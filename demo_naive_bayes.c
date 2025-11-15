// demo_naive_bayes.c - Naive Bayes Classifier Demo
// Demonstrates probabilistic classification using Bayes' theorem

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Forward declarations
typedef struct {
    int n_classes;
    int n_features;
    double* class_priors;
    double* means;
    double* variances;
    int* class_counts;
} GaussianNBModel;

typedef struct {
    int n_classes;
    int n_features;
    double* class_priors;
    double* feature_log_probs;
    int* class_counts;
    double* feature_counts;
} MultinomialNBModel;

typedef struct {
    int predicted_class;
    double* probabilities;
    double confidence;
} NBPrediction;

GaussianNBModel fp_gaussian_nb_train(const double* X, const int* y, int n, int d, int n_classes);
NBPrediction fp_gaussian_nb_predict(const GaussianNBModel* model, const double* x);
double fp_nb_accuracy(const GaussianNBModel* model, const double* X_test, const int* y_test, int n_test);

MultinomialNBModel fp_multinomial_nb_train(const double* X, const int* y, int n, int d, int n_classes, double alpha);
NBPrediction fp_multinomial_nb_predict(const MultinomialNBModel* model, const double* x);

void fp_nb_generate_gaussian_data(double* X, int* y, int n, double mean_class0[2], double mean_class1[2], double std_dev, unsigned int seed);
void fp_nb_print_gaussian_model(const GaussianNBModel* model);
void fp_nb_print_prediction(const NBPrediction* pred, int n_classes);
void fp_nb_print_confusion_matrix(int** confusion, int n_classes);

void fp_nb_free_gaussian_model(GaussianNBModel* model);
void fp_nb_free_multinomial_model(MultinomialNBModel* model);
void fp_nb_free_prediction(NBPrediction* pred);

// ============================================================================
// TEST 1: Gaussian Naive Bayes (2D Classification)
// ============================================================================
void test_gaussian_nb_2d() {
    printf("================================================================\n");
    printf("TEST 1: Gaussian Naive Bayes (2D Classification)\n");
    printf("================================================================\n\n");

    int n_train = 200;
    int n_test = 100;
    int n_classes = 2;
    int n_features = 2;

    printf("Dataset: Two Gaussian clusters in 2D\n");
    printf("  Class 0: centered at (0, 0)\n");
    printf("  Class 1: centered at (3, 3)\n");
    printf("  Training samples: %d\n", n_train);
    printf("  Test samples: %d\n\n", n_test);

    // Generate training data
    double* X_train = (double*)malloc(n_train * n_features * sizeof(double));
    int* y_train = (int*)malloc(n_train * sizeof(int));

    double mean_class0[2] = {0.0, 0.0};
    double mean_class1[2] = {3.0, 3.0};

    fp_nb_generate_gaussian_data(X_train, y_train, n_train, mean_class0, mean_class1, 1.0, 42);

    // Train model
    printf("Training Gaussian Naive Bayes...\n\n");
    GaussianNBModel model = fp_gaussian_nb_train(X_train, y_train, n_train, n_features, n_classes);

    fp_nb_print_gaussian_model(&model);

    printf("Learned parameters:\n");
    printf("Class 0:\n");
    printf("  Mean: [%.2f, %.2f]\n", model.means[0], model.means[1]);
    printf("  Variance: [%.2f, %.2f]\n", model.variances[0], model.variances[1]);
    printf("Class 1:\n");
    printf("  Mean: [%.2f, %.2f]\n", model.means[2], model.means[3]);
    printf("  Variance: [%.2f, %.2f]\n\n", model.variances[2], model.variances[3]);

    // Generate test data
    double* X_test = (double*)malloc(n_test * n_features * sizeof(double));
    int* y_test = (int*)malloc(n_test * sizeof(int));

    fp_nb_generate_gaussian_data(X_test, y_test, n_test, mean_class0, mean_class1, 1.0, 123);

    // Evaluate
    double accuracy = fp_nb_accuracy(&model, X_test, y_test, n_test);

    printf("Test Accuracy: %.2f%%\n\n", accuracy * 100.0);

    // Test single prediction
    printf("Example predictions:\n");
    for (int i = 0; i < 5; i++) {
        printf("Sample %d: [%.2f, %.2f]\n", i, X_test[i*2], X_test[i*2+1]);
        printf("  True label: %d\n", y_test[i]);

        NBPrediction pred = fp_gaussian_nb_predict(&model, &X_test[i * n_features]);
        printf("  Predicted: %d (confidence: %.2f%%)\n",
               pred.predicted_class, pred.confidence * 100.0);
        printf("  Probabilities: [%.4f, %.4f]\n\n",
               pred.probabilities[0], pred.probabilities[1]);

        fp_nb_free_prediction(&pred);
    }

    printf("Status: %s\n\n", (accuracy > 0.85) ? "PASS (>85%% accuracy)" : "FAIL");

    fp_nb_free_gaussian_model(&model);
    free(X_train);
    free(y_train);
    free(X_test);
    free(y_test);
}

// ============================================================================
// TEST 2: Multinomial Naive Bayes (Text Classification Simulation)
// ============================================================================
void test_multinomial_nb_text() {
    printf("================================================================\n");
    printf("TEST 2: Multinomial Naive Bayes (Text Classification)\n");
    printf("================================================================\n\n");

    // Simulate spam vs ham email classification
    printf("Scenario: Email spam detection\n");
    printf("  Classes: 0=Ham (legitimate), 1=Spam\n");
    printf("  Features: Word counts (simulated vocabulary size=10)\n\n");

    int n_train = 100;
    int n_test = 40;
    int n_classes = 2;
    int n_features = 10;  // Vocabulary size

    // Generate synthetic training data
    double* X_train = (double*)calloc(n_train * n_features, sizeof(double));
    int* y_train = (int*)malloc(n_train * sizeof(int));

    srand(42);

    for (int i = 0; i < n_train; i++) {
        int label = (i < n_train / 2) ? 0 : 1;
        y_train[i] = label;

        if (label == 0) {  // Ham emails
            // Ham uses words 0-5 more frequently
            for (int j = 0; j < 6; j++) {
                X_train[i * n_features + j] = rand() % 10 + 5;
            }
            for (int j = 6; j < 10; j++) {
                X_train[i * n_features + j] = rand() % 3;
            }
        } else {  // Spam emails
            // Spam uses words 5-9 more frequently
            for (int j = 0; j < 5; j++) {
                X_train[i * n_features + j] = rand() % 3;
            }
            for (int j = 5; j < 10; j++) {
                X_train[i * n_features + j] = rand() % 10 + 5;
            }
        }
    }

    // Train Multinomial Naive Bayes
    printf("Training Multinomial Naive Bayes (Laplace smoothing alpha=1.0)...\n\n");
    MultinomialNBModel model = fp_multinomial_nb_train(X_train, y_train, n_train, n_features, n_classes, 1.0);

    printf("Model Summary:\n");
    printf("  Classes: %d\n", model.n_classes);
    printf("  Vocabulary size: %d\n", model.n_features);
    printf("  Class priors:\n");
    printf("    Ham (class 0): %.4f\n", model.class_priors[0]);
    printf("    Spam (class 1): %.4f\n\n", model.class_priors[1]);

    // Generate test data
    double* X_test = (double*)calloc(n_test * n_features, sizeof(double));
    int* y_test = (int*)malloc(n_test * sizeof(int));

    for (int i = 0; i < n_test; i++) {
        int label = (i < n_test / 2) ? 0 : 1;
        y_test[i] = label;

        if (label == 0) {
            for (int j = 0; j < 6; j++) {
                X_test[i * n_features + j] = rand() % 10 + 5;
            }
            for (int j = 6; j < 10; j++) {
                X_test[i * n_features + j] = rand() % 3;
            }
        } else {
            for (int j = 0; j < 5; j++) {
                X_test[i * n_features + j] = rand() % 3;
            }
            for (int j = 5; j < 10; j++) {
                X_test[i * n_features + j] = rand() % 10 + 5;
            }
        }
    }

    // Evaluate
    int correct = 0;
    for (int i = 0; i < n_test; i++) {
        NBPrediction pred = fp_multinomial_nb_predict(&model, &X_test[i * n_features]);
        if (pred.predicted_class == y_test[i]) {
            correct++;
        }
        fp_nb_free_prediction(&pred);
    }

    double accuracy = (double)correct / n_test;

    printf("Test Accuracy: %.2f%%\n\n", accuracy * 100.0);

    // Example predictions
    printf("Example email classifications:\n");
    for (int i = 0; i < 3; i++) {
        printf("Email %d:\n", i);
        printf("  True label: %s\n", y_test[i] == 0 ? "Ham" : "Spam");

        NBPrediction pred = fp_multinomial_nb_predict(&model, &X_test[i * n_features]);
        printf("  Predicted: %s (confidence: %.2f%%)\n",
               pred.predicted_class == 0 ? "Ham" : "Spam",
               pred.confidence * 100.0);
        printf("  P(Ham): %.4f, P(Spam): %.4f\n\n",
               pred.probabilities[0], pred.probabilities[1]);

        fp_nb_free_prediction(&pred);
    }

    printf("Status: %s\n\n", (accuracy > 0.75) ? "PASS (>75%% accuracy)" : "FAIL");

    fp_nb_free_multinomial_model(&model);
    free(X_train);
    free(y_train);
    free(X_test);
    free(y_test);
}

// ============================================================================
// TEST 3: Multi-Class Classification
// ============================================================================
void test_multiclass() {
    printf("================================================================\n");
    printf("TEST 3: Multi-Class Classification (3 Classes)\n");
    printf("================================================================\n\n");

    int n_train = 300;
    int n_test = 150;
    int n_classes = 3;
    int n_features = 2;

    printf("Dataset: Three Gaussian clusters in 2D\n");
    printf("  Class 0: centered at (0, 0)\n");
    printf("  Class 1: centered at (4, 0)\n");
    printf("  Class 2: centered at (2, 3)\n");
    printf("  Training samples: %d (%d per class)\n", n_train, n_train / n_classes);
    printf("  Test samples: %d\n\n", n_test);

    // Generate training data (3 classes)
    double* X_train = (double*)malloc(n_train * n_features * sizeof(double));
    int* y_train = (int*)malloc(n_train * sizeof(int));

    srand(42);

    double centers[3][2] = {{0.0, 0.0}, {4.0, 0.0}, {2.0, 3.0}};
    double std_dev = 0.8;

    for (int i = 0; i < n_train; i++) {
        int label = i / (n_train / n_classes);
        if (label >= n_classes) label = n_classes - 1;
        y_train[i] = label;

        for (int j = 0; j < n_features; j++) {
            double u1 = (double)rand() / RAND_MAX;
            double u2 = (double)rand() / RAND_MAX;
            double noise = sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
            X_train[i * n_features + j] = centers[label][j] + std_dev * noise;
        }
    }

    // Train
    printf("Training Gaussian Naive Bayes...\n\n");
    GaussianNBModel model = fp_gaussian_nb_train(X_train, y_train, n_train, n_features, n_classes);

    fp_nb_print_gaussian_model(&model);

    // Generate test data
    double* X_test = (double*)malloc(n_test * n_features * sizeof(double));
    int* y_test = (int*)malloc(n_test * sizeof(int));

    for (int i = 0; i < n_test; i++) {
        int label = i / (n_test / n_classes);
        if (label >= n_classes) label = n_classes - 1;
        y_test[i] = label;

        for (int j = 0; j < n_features; j++) {
            double u1 = (double)rand() / RAND_MAX;
            double u2 = (double)rand() / RAND_MAX;
            double noise = sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
            X_test[i * n_features + j] = centers[label][j] + std_dev * noise;
        }
    }

    // Evaluate and build confusion matrix
    int** confusion = (int**)calloc(n_classes, sizeof(int*));
    for (int i = 0; i < n_classes; i++) {
        confusion[i] = (int*)calloc(n_classes, sizeof(int));
    }

    int correct = 0;
    for (int i = 0; i < n_test; i++) {
        NBPrediction pred = fp_gaussian_nb_predict(&model, &X_test[i * n_features]);
        confusion[y_test[i]][pred.predicted_class]++;
        if (pred.predicted_class == y_test[i]) {
            correct++;
        }
        fp_nb_free_prediction(&pred);
    }

    double accuracy = (double)correct / n_test;

    printf("Test Accuracy: %.2f%%\n\n", accuracy * 100.0);

    fp_nb_print_confusion_matrix(confusion, n_classes);
    printf("\n");

    printf("Status: %s\n\n", (accuracy > 0.80) ? "PASS (>80%% accuracy)" : "FAIL");

    // Cleanup
    for (int i = 0; i < n_classes; i++) {
        free(confusion[i]);
    }
    free(confusion);
    fp_nb_free_gaussian_model(&model);
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
    printf("  Naive Bayes Classifier Demo\n");
    printf("  FP-ASM Library - Probabilistic Classification\n");
    printf("================================================================\n\n");

    printf("Naive Bayes Variants Demonstrated:\n");
    printf("  1. Gaussian Naive Bayes - for continuous features\n");
    printf("  2. Multinomial Naive Bayes - for count/text features\n\n");

    printf("Key Concept: Bayes' Theorem\n");
    printf("  P(class|features) ∝ P(features|class) × P(class)\n");
    printf("  'Naive' assumption: features are independent given class\n\n");

    printf("Why it works despite being 'naive':\n");
    printf("  - Independence assumption is often violated\n");
    printf("  - But classification only needs correct ranking, not exact probabilities\n");
    printf("  - Fast training (just count and compute statistics)\n");
    printf("  - Works surprisingly well in practice!\n\n");

    test_gaussian_nb_2d();
    test_multinomial_nb_text();
    test_multiclass();

    printf("================================================================\n");
    printf("  Naive Bayes Demo Complete!\n");
    printf("================================================================\n\n");

    printf("Key Takeaways:\n");
    printf("  ✓ Probabilistic classification via Bayes' theorem\n");
    printf("  ✓ Fast training (just compute statistics)\n");
    printf("  ✓ Fast inference (multiply probabilities)\n");
    printf("  ✓ Returns probability distribution over classes\n");
    printf("  ✓ Works well despite 'naive' independence assumption\n\n");

    printf("Real-World Applications:\n");
    printf("  - Email spam detection (Multinomial NB)\n");
    printf("  - Sentiment analysis (Multinomial NB)\n");
    printf("  - Document categorization (Multinomial NB)\n");
    printf("  - Medical diagnosis (Gaussian NB)\n");
    printf("  - Weather prediction (Gaussian NB)\n");
    printf("  - Real-time classification (fast inference!)\n\n");

    printf("Why Naive Bayes is Popular:\n");
    printf("  - Extremely fast (training and inference)\n");
    printf("  - Works well with high-dimensional data\n");
    printf("  - Handles missing features gracefully\n");
    printf("  - Probabilistic outputs (confidence scores)\n");
    printf("  - Baseline for text classification\n");
    printf("  - Online learning capable (update statistics incrementally)\n\n");

    printf("Bayes' Theorem Foundation:\n");
    printf("  Posterior = (Likelihood × Prior) / Evidence\n");
    printf("  P(C|X) = P(X|C)·P(C) / P(X)\n");
    printf("  For classification: argmax P(C|X) = argmax P(X|C)·P(C)\n");
    printf("  'Naive' independence: P(X|C) = Π P(x_i|C)\n\n");

    return 0;
}
