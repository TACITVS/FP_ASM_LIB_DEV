// fp_naive_bayes.c
//
// Naive Bayes Classifier
// Demonstrates probabilistic classification using Bayes' theorem
//
// This showcases:
// - Gaussian Naive Bayes (continuous features)
// - Multinomial Naive Bayes (count/text features)
// - Bayes' theorem application
// - Maximum a posteriori (MAP) inference
// - Probability estimation from data
//
// FP Primitives Used:
// - Statistical computations (mean, variance, counts)
// - Probability calculations (log probabilities to avoid underflow)
// - Reductions (counting occurrences)
//
// Applications:
// - Spam detection (email classification)
// - Sentiment analysis (positive/negative reviews)
// - Document categorization
// - Medical diagnosis
// - Weather prediction

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================================
// Data Structures
// ============================================================================

// Gaussian Naive Bayes Model (for continuous features)
typedef struct {
    int n_classes;          // Number of classes
    int n_features;         // Number of features

    double* class_priors;   // P(class) - prior probabilities (n_classes)
    double* means;          // Feature means per class (n_classes × n_features)
    double* variances;      // Feature variances per class (n_classes × n_features)

    int* class_counts;      // Number of samples per class (n_classes)
} GaussianNBModel;

// Multinomial Naive Bayes Model (for count/text features)
typedef struct {
    int n_classes;          // Number of classes
    int n_features;         // Number of features (vocabulary size)

    double* class_priors;   // P(class) - prior probabilities (n_classes)
    double* feature_log_probs;  // log P(feature|class) (n_classes × n_features)

    int* class_counts;      // Number of samples per class (n_classes)
    double* feature_counts; // Feature counts per class (n_classes × n_features)
} MultinomialNBModel;

// Classification result with probabilities
typedef struct {
    int predicted_class;    // Most likely class
    double* probabilities;  // Probability for each class (n_classes)
    double confidence;      // Max probability
} NBPrediction;

// ============================================================================
// Gaussian Naive Bayes (Continuous Features)
// ============================================================================

// Train Gaussian Naive Bayes
// X: n × d feature matrix, y: n class labels
GaussianNBModel fp_gaussian_nb_train(
    const double* X,
    const int* y,
    int n,
    int d,
    int n_classes
) {
    GaussianNBModel model;
    model.n_classes = n_classes;
    model.n_features = d;

    // Allocate memory
    model.class_priors = (double*)calloc(n_classes, sizeof(double));
    model.means = (double*)calloc(n_classes * d, sizeof(double));
    model.variances = (double*)calloc(n_classes * d, sizeof(double));
    model.class_counts = (int*)calloc(n_classes, sizeof(int));

    // Count samples per class
    for (int i = 0; i < n; i++) {
        model.class_counts[y[i]]++;
    }

    // Compute class priors: P(class) = count(class) / n
    for (int c = 0; c < n_classes; c++) {
        model.class_priors[c] = (double)model.class_counts[c] / n;
    }

    // Compute means per class and feature
    for (int i = 0; i < n; i++) {
        int c = y[i];
        for (int j = 0; j < d; j++) {
            model.means[c * d + j] += X[i * d + j];
        }
    }

    for (int c = 0; c < n_classes; c++) {
        if (model.class_counts[c] > 0) {
            for (int j = 0; j < d; j++) {
                model.means[c * d + j] /= model.class_counts[c];
            }
        }
    }

    // Compute variances per class and feature
    for (int i = 0; i < n; i++) {
        int c = y[i];
        for (int j = 0; j < d; j++) {
            double diff = X[i * d + j] - model.means[c * d + j];
            model.variances[c * d + j] += diff * diff;
        }
    }

    for (int c = 0; c < n_classes; c++) {
        if (model.class_counts[c] > 1) {
            for (int j = 0; j < d; j++) {
                model.variances[c * d + j] /= model.class_counts[c];
                // Add small constant to avoid zero variance
                if (model.variances[c * d + j] < 1e-9) {
                    model.variances[c * d + j] = 1e-9;
                }
            }
        }
    }

    return model;
}

// Gaussian probability density function
static double gaussian_pdf(double x, double mean, double variance) {
    double exponent = -0.5 * ((x - mean) * (x - mean)) / variance;
    double normalization = 1.0 / sqrt(2.0 * M_PI * variance);
    return normalization * exp(exponent);
}

// Predict using Gaussian Naive Bayes
NBPrediction fp_gaussian_nb_predict(
    const GaussianNBModel* model,
    const double* x  // Single sample (d-dimensional)
) {
    NBPrediction result;
    result.probabilities = (double*)malloc(model->n_classes * sizeof(double));

    // Compute log probability for each class
    // log P(class|x) ∝ log P(class) + Σ log P(x_j|class)
    double max_log_prob = -INFINITY;

    for (int c = 0; c < model->n_classes; c++) {
        // Start with class prior
        double log_prob = log(model->class_priors[c]);

        // Add log likelihood for each feature (assuming independence!)
        for (int j = 0; j < model->n_features; j++) {
            double mean = model->means[c * model->n_features + j];
            double variance = model->variances[c * model->n_features + j];
            double pdf = gaussian_pdf(x[j], mean, variance);

            // Use log to avoid underflow
            log_prob += log(pdf + 1e-300);  // Add tiny constant to avoid log(0)
        }

        result.probabilities[c] = log_prob;

        if (log_prob > max_log_prob) {
            max_log_prob = log_prob;
            result.predicted_class = c;
        }
    }

    // Convert log probabilities to probabilities via softmax
    double sum_exp = 0.0;
    for (int c = 0; c < model->n_classes; c++) {
        result.probabilities[c] = exp(result.probabilities[c] - max_log_prob);
        sum_exp += result.probabilities[c];
    }

    for (int c = 0; c < model->n_classes; c++) {
        result.probabilities[c] /= sum_exp;
    }

    result.confidence = result.probabilities[result.predicted_class];

    return result;
}

// ============================================================================
// Multinomial Naive Bayes (Count/Text Features)
// ============================================================================

// Train Multinomial Naive Bayes
// X: n × d feature matrix (counts), y: n class labels
MultinomialNBModel fp_multinomial_nb_train(
    const double* X,
    const int* y,
    int n,
    int d,
    int n_classes,
    double alpha  // Laplace smoothing parameter (usually 1.0)
) {
    MultinomialNBModel model;
    model.n_classes = n_classes;
    model.n_features = d;

    // Allocate memory
    model.class_priors = (double*)calloc(n_classes, sizeof(double));
    model.feature_log_probs = (double*)calloc(n_classes * d, sizeof(double));
    model.class_counts = (int*)calloc(n_classes, sizeof(int));
    model.feature_counts = (double*)calloc(n_classes * d, sizeof(double));

    // Count samples per class
    for (int i = 0; i < n; i++) {
        model.class_counts[y[i]]++;
    }

    // Compute class priors
    for (int c = 0; c < n_classes; c++) {
        model.class_priors[c] = (double)model.class_counts[c] / n;
    }

    // Count features per class
    for (int i = 0; i < n; i++) {
        int c = y[i];
        for (int j = 0; j < d; j++) {
            model.feature_counts[c * d + j] += X[i * d + j];
        }
    }

    // Compute feature log probabilities with Laplace smoothing
    // log P(feature|class) = log((count + alpha) / (total_count + alpha*n_features))
    for (int c = 0; c < n_classes; c++) {
        double total_count = 0.0;
        for (int j = 0; j < d; j++) {
            total_count += model.feature_counts[c * d + j];
        }

        for (int j = 0; j < d; j++) {
            double smoothed_count = model.feature_counts[c * d + j] + alpha;
            double smoothed_total = total_count + alpha * d;
            model.feature_log_probs[c * d + j] = log(smoothed_count / smoothed_total);
        }
    }

    return model;
}

// Predict using Multinomial Naive Bayes
NBPrediction fp_multinomial_nb_predict(
    const MultinomialNBModel* model,
    const double* x  // Single sample (d-dimensional count vector)
) {
    NBPrediction result;
    result.probabilities = (double*)malloc(model->n_classes * sizeof(double));

    double max_log_prob = -INFINITY;

    for (int c = 0; c < model->n_classes; c++) {
        // Start with class prior
        double log_prob = log(model->class_priors[c]);

        // Add log likelihood: Σ count[j] * log P(feature_j|class)
        for (int j = 0; j < model->n_features; j++) {
            log_prob += x[j] * model->feature_log_probs[c * model->n_features + j];
        }

        result.probabilities[c] = log_prob;

        if (log_prob > max_log_prob) {
            max_log_prob = log_prob;
            result.predicted_class = c;
        }
    }

    // Convert to probabilities via softmax
    double sum_exp = 0.0;
    for (int c = 0; c < model->n_classes; c++) {
        result.probabilities[c] = exp(result.probabilities[c] - max_log_prob);
        sum_exp += result.probabilities[c];
    }

    for (int c = 0; c < model->n_classes; c++) {
        result.probabilities[c] /= sum_exp;
    }

    result.confidence = result.probabilities[result.predicted_class];

    return result;
}

// ============================================================================
// Evaluation Metrics
// ============================================================================

// Compute accuracy on test set
double fp_nb_accuracy(
    const GaussianNBModel* model,
    const double* X_test,
    const int* y_test,
    int n_test
) {
    int correct = 0;

    for (int i = 0; i < n_test; i++) {
        NBPrediction pred = fp_gaussian_nb_predict(model, &X_test[i * model->n_features]);
        if (pred.predicted_class == y_test[i]) {
            correct++;
        }
        free(pred.probabilities);
    }

    return (double)correct / n_test;
}

// Confusion matrix
void fp_nb_confusion_matrix(
    const GaussianNBModel* model,
    const double* X_test,
    const int* y_test,
    int n_test,
    int** confusion  // Output: n_classes × n_classes matrix
) {
    for (int i = 0; i < n_test; i++) {
        NBPrediction pred = fp_gaussian_nb_predict(model, &X_test[i * model->n_features]);
        int true_class = y_test[i];
        int pred_class = pred.predicted_class;
        confusion[true_class][pred_class]++;
        free(pred.probabilities);
    }
}

// ============================================================================
// Data Generation (for testing)
// ============================================================================

// Generate 2D Gaussian data for classification
void fp_nb_generate_gaussian_data(
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
            // Box-Muller transform for Gaussian noise
            double u1 = (double)rand() / RAND_MAX;
            double u2 = (double)rand() / RAND_MAX;
            double noise = sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);

            X[i * 2 + j] = mean[j] + std_dev * noise;
        }
    }
}

// ============================================================================
// Printing Utilities
// ============================================================================

void fp_nb_print_gaussian_model(const GaussianNBModel* model) {
    printf("Gaussian Naive Bayes Model:\n");
    printf("  Classes: %d\n", model->n_classes);
    printf("  Features: %d\n", model->n_features);
    printf("\nClass priors:\n");
    for (int c = 0; c < model->n_classes; c++) {
        printf("  Class %d: %.4f (n=%d)\n", c, model->class_priors[c], model->class_counts[c]);
    }
    printf("\n");
}

void fp_nb_print_prediction(const NBPrediction* pred, int n_classes) {
    printf("Prediction: Class %d (confidence: %.2f%%)\n",
           pred->predicted_class, pred->confidence * 100.0);
    printf("Probabilities:\n");
    for (int c = 0; c < n_classes; c++) {
        printf("  Class %d: %.4f\n", c, pred->probabilities[c]);
    }
}

void fp_nb_print_confusion_matrix(int** confusion, int n_classes) {
    printf("Confusion Matrix:\n");
    printf("      ");
    for (int c = 0; c < n_classes; c++) {
        printf("Pred %d ", c);
    }
    printf("\n");

    for (int true_c = 0; true_c < n_classes; true_c++) {
        printf("True %d ", true_c);
        for (int pred_c = 0; pred_c < n_classes; pred_c++) {
            printf("%6d ", confusion[true_c][pred_c]);
        }
        printf("\n");
    }
}

// Free models
void fp_nb_free_gaussian_model(GaussianNBModel* model) {
    free(model->class_priors);
    free(model->means);
    free(model->variances);
    free(model->class_counts);
}

void fp_nb_free_multinomial_model(MultinomialNBModel* model) {
    free(model->class_priors);
    free(model->feature_log_probs);
    free(model->class_counts);
    free(model->feature_counts);
}

void fp_nb_free_prediction(NBPrediction* pred) {
    free(pred->probabilities);
}
