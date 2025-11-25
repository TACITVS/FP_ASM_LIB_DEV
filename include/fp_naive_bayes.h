/**
 * fp_naive_bayes.h - Naive Bayes Classifier API
 *
 * Provides both Gaussian and Multinomial Naive Bayes classifiers with
 * standard and safe (Either monad) interfaces. Uses L0 ASM primitives
 * for SIMD-accelerated computation.
 *
 * Features:
 * - Gaussian Naive Bayes (continuous features)
 * - Multinomial Naive Bayes (count/frequency features)
 * - Probabilistic classification using Bayes' theorem
 * - Safe variants returning Either monad for error handling
 * - Log probabilities for numerical stability
 *
 * Example (Gaussian - standard):
 *   GaussianNBModel model = fp_gaussian_nb_train(X, y, n, d, n_classes);
 *   int prediction = fp_gaussian_nb_predict(&model, x_test);
 *   fp_nb_free_gaussian_model(&model);
 *
 * Example (Gaussian - safe/monadic):
 *   Either result = fp_gaussian_nb_train_safe(X, y, n, d, n_classes);
 *   if (fp_is_right(result)) {
 *       GaussianNBModel* model = (GaussianNBModel*)fp_from_right_ptr(result);
 *       // Use model...
 *       fp_nb_free_gaussian_model(model);  // Free internal arrays
 *       free(model);                       // Free the struct itself
 *   } else {
 *       printf("Error: %s (code %d)\n",
 *              fp_from_left_msg(result), fp_from_left_code(result));
 *   }
 *
 * Example (Multinomial - standard):
 *   MultinomialNBModel model = fp_multinomial_nb_train(
 *       X, y, n, d, n_classes, 1.0);  // alpha=1.0 for Laplace smoothing
 *   int prediction = fp_multinomial_nb_predict(&model, x_test);
 *   fp_nb_free_multinomial_model(&model);
 */

#ifndef FP_NAIVE_BAYES_H
#define FP_NAIVE_BAYES_H

#include <stdint.h>
#include <stddef.h>
#include "fp_monads.h"

/* ============================================================================
 * NAIVE BAYES STRUCTURES
 * ============================================================================ */

/**
 * GaussianNBModel - Gaussian Naive Bayes for continuous features
 */
typedef struct {
    int n_classes;                         // Number of classes
    int n_features;                        // Number of features

    double* class_priors;                  // P(class) - prior probabilities (n_classes)
    double* means;                         // Feature means per class (n_classes × n_features)
    double* variances;                     // Feature variances per class (n_classes × n_features)

    int* class_counts;                     // Number of samples per class (n_classes)
} GaussianNBModel;

/**
 * MultinomialNBModel - Multinomial Naive Bayes for count/frequency features
 */
typedef struct {
    int n_classes;                         // Number of classes
    int n_features;                        // Number of features (vocabulary size)

    double* class_priors;                  // P(class) - prior probabilities (n_classes)
    double* feature_log_probs;             // log P(feature|class) (n_classes × n_features)

    int* class_counts;                     // Number of samples per class (n_classes)
    double* feature_counts;                // Feature counts per class (n_classes × n_features)
} MultinomialNBModel;

/**
 * NBPrediction - Classification result with probabilities
 */
typedef struct {
    int predicted_class;                   // Most likely class
    double* probabilities;                 // Probability for each class (n_classes)
    double confidence;                     // Max probability
} NBPrediction;

/* ============================================================================
 * GAUSSIAN NAIVE BAYES API (Continuous Features)
 * ============================================================================ */

/**
 * fp_gaussian_nb_train - Train Gaussian Naive Bayes classifier
 *
 * @param X             n x d feature matrix (row-major)
 * @param y             Class labels (n elements, values 0 to n_classes-1)
 * @param n             Number of training samples
 * @param d             Number of features
 * @param n_classes     Number of classes
 *
 * @return GaussianNBModel with trained parameters
 *
 * NOTE: Does NOT validate inputs. For safe version with validation,
 *       use fp_gaussian_nb_train_safe().
 */
GaussianNBModel fp_gaussian_nb_train(
    const double* X,
    const int* y,
    int n,
    int d,
    int n_classes
);

/**
 * fp_gaussian_nb_predict - Predict class for a single sample
 *
 * @param model         Trained Gaussian NB model
 * @param x             Feature vector (d elements)
 *
 * @return NBPrediction with predicted_class, probabilities, and confidence
 *         Caller must free NBPrediction.probabilities after use!
 */
NBPrediction fp_gaussian_nb_predict(
    const GaussianNBModel* model,
    const double* x
);

/**
 * fp_gaussian_nb_predict_batch - Predict classes for multiple samples
 *
 * @param model         Trained Gaussian NB model
 * @param X             n x d feature matrix (row-major)
 * @param n             Number of samples
 * @param predictions   Output predictions (n elements, pre-allocated)
 *
 * @note Implementation uses FP purist tail recursion (ZERO for-loops).
 * @note Requires compiler TCO: GCC/Clang with -O3 -foptimize-sibling-calls
 * @note Stack usage: O(1) with optimization, O(n) without.
 * @note Tested: 100,000+ samples on Intel i7-4600M without stack overflow.
 * @note Performance: Identical to imperative loops (±1-2%) with TCO enabled.
 *
 * @see docs/benchmarks/RECURSION_VS_LOOP_RESULTS.md for performance data
 */
void fp_gaussian_nb_predict_batch(
    const GaussianNBModel* model,
    const double* X,
    int n,
    int* predictions
);

/* ============================================================================
 * MULTINOMIAL NAIVE BAYES API (Count/Frequency Features)
 * ============================================================================ */

/**
 * fp_multinomial_nb_train - Train Multinomial Naive Bayes classifier
 *
 * @param X             n x d feature matrix (row-major, non-negative counts)
 * @param y             Class labels (n elements, values 0 to n_classes-1)
 * @param n             Number of training samples
 * @param d             Number of features (vocabulary size)
 * @param n_classes     Number of classes
 * @param alpha         Smoothing parameter (1.0 = Laplace smoothing, 0.0 = no smoothing)
 *
 * @return MultinomialNBModel with trained parameters
 *
 * NOTE: Does NOT validate inputs. For safe version with validation,
 *       use fp_multinomial_nb_train_safe().
 */
MultinomialNBModel fp_multinomial_nb_train(
    const double* X,
    const int* y,
    int n,
    int d,
    int n_classes,
    double alpha
);

/**
 * fp_multinomial_nb_predict - Predict class for a single sample
 *
 * @param model         Trained Multinomial NB model
 * @param x             Feature vector (d elements, non-negative counts)
 *
 * @return NBPrediction with predicted_class, probabilities, and confidence
 *         Caller must free NBPrediction.probabilities after use!
 */
NBPrediction fp_multinomial_nb_predict(
    const MultinomialNBModel* model,
    const double* x
);

/**
 * fp_multinomial_nb_predict_batch - Predict classes for multiple samples
 *
 * @param model         Trained Multinomial NB model
 * @param X             n x d feature matrix (row-major, non-negative counts)
 * @param n             Number of samples
 * @param predictions   Output predictions (n elements, pre-allocated)
 *
 * @note Implementation uses FP purist tail recursion (ZERO for-loops).
 * @note Requires compiler TCO: GCC/Clang with -O3 -foptimize-sibling-calls
 * @note Stack usage: O(1) with optimization, O(n) without.
 * @note Tested: 100,000+ samples on Intel i7-4600M without stack overflow.
 * @note Performance: Identical to imperative loops (±1-2%) with TCO enabled.
 *
 * @see docs/benchmarks/RECURSION_VS_LOOP_RESULTS.md for performance data
 */
void fp_multinomial_nb_predict_batch(
    const MultinomialNBModel* model,
    const double* X,
    int n,
    int* predictions
);

/* ============================================================================
 * MEMORY MANAGEMENT
 * ============================================================================ */

/**
 * fp_nb_free_gaussian_model - Free internal arrays of GaussianNBModel
 *
 * @param model         Pointer to model to free
 */
void fp_nb_free_gaussian_model(GaussianNBModel* model);

/**
 * fp_nb_free_multinomial_model - Free internal arrays of MultinomialNBModel
 *
 * @param model         Pointer to model to free
 */
void fp_nb_free_multinomial_model(MultinomialNBModel* model);

/**
 * fp_nb_free_prediction - Free internal arrays of NBPrediction
 *
 * @param pred          Pointer to prediction to free
 */
void fp_nb_free_prediction(NBPrediction* pred);

/* ============================================================================
 * SAFE API (Either Monad) - GAUSSIAN NAIVE BAYES
 * ============================================================================ */

/**
 * fp_gaussian_nb_train_safe - Safe Gaussian NB training with validation
 *
 * Returns Either monad for explicit error handling:
 * - Right(GaussianNBModel*) on success
 * - Left(error_msg, error_code) on failure
 *
 * Error codes:
 *   1 = NULL input data (X or y)
 *   2 = Invalid parameters (n<=0, d<=0, n_classes<=0,
 *       or overflow in n_classes*d)
 *   3 = Memory allocation failed
 *
 * Overflow protection:
 *   - Checks n_classes * d < INT_MAX (means/variances matrices)
 *
 * @param X             n x d feature matrix (row-major)
 * @param y             Class labels (n elements, values 0 to n_classes-1)
 * @param n             Number of training samples (must be > 0)
 * @param d             Number of features (must be > 0)
 * @param n_classes     Number of classes (must be > 0)
 *
 * @return Either containing Right(GaussianNBModel*) or Left(error_msg, code)
 *
 * MEMORY: On success, caller must:
 *   1. fp_nb_free_gaussian_model(model) - Free internal arrays
 *   2. free(model) - Free the heap-allocated struct
 */
Either fp_gaussian_nb_train_safe(
    const double* X,
    const int* y,
    int n,
    int d,
    int n_classes
);

/* ============================================================================
 * SAFE API (Either Monad) - MULTINOMIAL NAIVE BAYES
 * ============================================================================ */

/**
 * fp_multinomial_nb_train_safe - Safe Multinomial NB training with validation
 *
 * Returns Either monad for explicit error handling:
 * - Right(MultinomialNBModel*) on success
 * - Left(error_msg, error_code) on failure
 *
 * Error codes:
 *   1 = NULL input data (X or y)
 *   2 = Invalid parameters (n<=0, d<=0, n_classes<=0, alpha<0,
 *       or overflow in n_classes*d)
 *   3 = Memory allocation failed
 *
 * Overflow protection:
 *   - Checks n_classes * d < INT_MAX (feature matrices)
 *
 * @param X             n x d feature matrix (row-major, non-negative counts)
 * @param y             Class labels (n elements, values 0 to n_classes-1)
 * @param n             Number of training samples (must be > 0)
 * @param d             Number of features (must be > 0)
 * @param n_classes     Number of classes (must be > 0)
 * @param alpha         Smoothing parameter (must be >= 0)
 *
 * @return Either containing Right(MultinomialNBModel*) or Left(error_msg, code)
 *
 * MEMORY: On success, caller must:
 *   1. fp_nb_free_multinomial_model(model) - Free internal arrays
 *   2. free(model) - Free the heap-allocated struct
 */
Either fp_multinomial_nb_train_safe(
    const double* X,
    const int* y,
    int n,
    int d,
    int n_classes,
    double alpha
);

#endif /* FP_NAIVE_BAYES_H */
