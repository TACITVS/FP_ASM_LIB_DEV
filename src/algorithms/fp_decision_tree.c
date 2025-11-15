// fp_decision_tree.c
//
// Decision Tree (CART - Classification and Regression Trees)
// Demonstrates recursive partitioning for interpretable ML
//
// This showcases:
// - Binary decision trees (recursive splitting)
// - Gini impurity for classification
// - MSE for regression
// - Greedy best-first splitting
// - Tree traversal and prediction
// - Feature importance calculation
//
// FP Primitives Used:
// - Reductions (counting, summing)
// - Predicates (filtering by threshold)
// - Statistical computations (variance, impurity)
//
// Applications:
// - Medical diagnosis (interpretable rules)
// - Credit scoring (explain decisions)
// - Fraud detection (rule-based)
// - Customer churn prediction
// - Foundation for Random Forests

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

// ============================================================================
// Data Structures
// ============================================================================

// Decision tree node
typedef struct DecisionNode {
    // Split information (for internal nodes)
    int is_leaf;
    int feature_index;      // Which feature to split on
    double threshold;       // Split threshold

    // Children (for internal nodes)
    struct DecisionNode* left;   // samples where feature <= threshold
    struct DecisionNode* right;  // samples where feature > threshold

    // Leaf information
    double value;           // Prediction (class or regression value)
    int n_samples;          // Number of samples at this node
    double impurity;        // Gini (classification) or variance (regression)

    // Classification-specific
    int* class_counts;      // Count per class (for classification)
    int n_classes;          // Number of classes
} DecisionNode;

// Decision tree model
typedef struct {
    DecisionNode* root;
    int max_depth;
    int min_samples_split;
    int is_classifier;      // 1 for classification, 0 for regression
    int n_features;
    int n_classes;          // For classification only

    double* feature_importances;  // Feature importance scores
} DecisionTreeModel;

// Split result
typedef struct {
    int feature_index;
    double threshold;
    double gain;            // Information gain from this split
    int n_left;
    int n_right;
} BestSplit;

// ============================================================================
// Utility Functions
// ============================================================================

// Create decision node
static DecisionNode* create_node(int is_classifier, int n_classes) {
    DecisionNode* node = (DecisionNode*)malloc(sizeof(DecisionNode));
    node->is_leaf = 0;
    node->feature_index = -1;
    node->threshold = 0.0;
    node->left = NULL;
    node->right = NULL;
    node->value = 0.0;
    node->n_samples = 0;
    node->impurity = 0.0;

    if (is_classifier && n_classes > 0) {
        node->class_counts = (int*)calloc(n_classes, sizeof(int));
        node->n_classes = n_classes;
    } else {
        node->class_counts = NULL;
        node->n_classes = 0;
    }

    return node;
}

// Free decision tree node recursively
static void free_node(DecisionNode* node) {
    if (node == NULL) return;

    free_node(node->left);
    free_node(node->right);

    if (node->class_counts != NULL) {
        free(node->class_counts);
    }

    free(node);
}

// ============================================================================
// Impurity Metrics
// ============================================================================

// Gini impurity for classification: 1 - Σ p_i²
static double gini_impurity(const int* y, int n, int n_classes) {
    int* counts = (int*)calloc(n_classes, sizeof(int));

    // Count samples per class
    for (int i = 0; i < n; i++) {
        counts[y[i]]++;
    }

    // Compute Gini: 1 - Σ (count/n)²
    double gini = 1.0;
    for (int c = 0; c < n_classes; c++) {
        double p = (double)counts[c] / n;
        gini -= p * p;
    }

    free(counts);
    return gini;
}

// Variance for regression
static double variance(const double* y, int n) {
    if (n == 0) return 0.0;

    // Compute mean
    double sum = 0.0;
    for (int i = 0; i < n; i++) {
        sum += y[i];
    }
    double mean = sum / n;

    // Compute variance
    double var = 0.0;
    for (int i = 0; i < n; i++) {
        double diff = y[i] - mean;
        var += diff * diff;
    }

    return var / n;
}

// ============================================================================
// Splitting Logic
// ============================================================================

// Find best split for a feature (classification)
static BestSplit find_best_split_classification(
    const double* X,
    const int* y,
    const int* indices,
    int n,
    int n_features,
    int n_classes,
    int feature_idx
) {
    BestSplit best;
    best.feature_index = feature_idx;
    best.threshold = 0.0;
    best.gain = -DBL_MAX;
    best.n_left = 0;
    best.n_right = 0;

    if (n < 2) return best;

    // Current impurity
    double parent_impurity = gini_impurity(y, n, n_classes);

    // Sort indices by feature value
    double* feature_values = (double*)malloc(n * sizeof(double));
    int* sorted_indices = (int*)malloc(n * sizeof(int));

    for (int i = 0; i < n; i++) {
        sorted_indices[i] = i;
        feature_values[i] = X[indices[i] * n_features + feature_idx];
    }

    // Simple bubble sort (good enough for small n)
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            if (feature_values[j] > feature_values[j + 1]) {
                double temp = feature_values[j];
                feature_values[j] = feature_values[j + 1];
                feature_values[j + 1] = temp;

                int temp_idx = sorted_indices[j];
                sorted_indices[j] = sorted_indices[j + 1];
                sorted_indices[j + 1] = temp_idx;
            }
        }
    }

    // Try splits between consecutive unique values
    for (int i = 0; i < n - 1; i++) {
        if (feature_values[i] == feature_values[i + 1]) continue;

        double threshold = (feature_values[i] + feature_values[i + 1]) / 2.0;

        // Split samples
        int n_left = i + 1;
        int n_right = n - n_left;

        // Allocate temporary arrays for left/right labels
        int* y_left = (int*)malloc(n_left * sizeof(int));
        int* y_right = (int*)malloc(n_right * sizeof(int));

        for (int j = 0; j <= i; j++) {
            y_left[j] = y[sorted_indices[j]];
        }
        for (int j = i + 1; j < n; j++) {
            y_right[j - i - 1] = y[sorted_indices[j]];
        }

        // Compute weighted impurity
        double left_impurity = gini_impurity(y_left, n_left, n_classes);
        double right_impurity = gini_impurity(y_right, n_right, n_classes);

        double weighted_impurity = (n_left * left_impurity + n_right * right_impurity) / n;
        double gain = parent_impurity - weighted_impurity;

        if (gain > best.gain) {
            best.threshold = threshold;
            best.gain = gain;
            best.n_left = n_left;
            best.n_right = n_right;
        }

        free(y_left);
        free(y_right);
    }

    free(feature_values);
    free(sorted_indices);

    return best;
}

// Find best split across all features
static BestSplit find_best_split(
    const double* X,
    const int* y,
    const int* indices,
    int n,
    int n_features,
    int n_classes
) {
    BestSplit best;
    best.gain = -DBL_MAX;

    for (int f = 0; f < n_features; f++) {
        BestSplit split = find_best_split_classification(X, y, indices, n, n_features, n_classes, f);

        if (split.gain > best.gain) {
            best = split;
        }
    }

    return best;
}

// ============================================================================
// Tree Building (Recursive)
// ============================================================================

// Build decision tree recursively
static DecisionNode* build_tree(
    const double* X,
    const int* y,
    const int* indices,
    int n,
    int n_features,
    int n_classes,
    int depth,
    int max_depth,
    int min_samples_split,
    double* feature_importances
) {
    DecisionNode* node = create_node(1, n_classes);
    node->n_samples = n;

    // Create temporary label array for this subset
    int* y_subset = (int*)malloc(n * sizeof(int));
    for (int i = 0; i < n; i++) {
        y_subset[i] = y[indices[i]];
    }

    // Count classes
    for (int i = 0; i < n; i++) {
        node->class_counts[y_subset[i]]++;
    }

    // Compute current impurity
    node->impurity = gini_impurity(y_subset, n, n_classes);

    // Find majority class
    int max_count = 0;
    int majority_class = 0;
    for (int c = 0; c < n_classes; c++) {
        if (node->class_counts[c] > max_count) {
            max_count = node->class_counts[c];
            majority_class = c;
        }
    }
    node->value = (double)majority_class;

    // Stopping criteria
    if (depth >= max_depth || n < min_samples_split || node->impurity == 0.0) {
        node->is_leaf = 1;
        free(y_subset);
        return node;
    }

    // Find best split (pass y_subset instead of y)
    BestSplit split = find_best_split(X, y_subset, indices, n, n_features, n_classes);

    if (split.gain <= 0.0) {
        node->is_leaf = 1;
        free(y_subset);
        return node;
    }

    // Record split
    node->feature_index = split.feature_index;
    node->threshold = split.threshold;

    // Update feature importance
    feature_importances[split.feature_index] += split.gain * n;

    // Partition samples
    int* left_indices = (int*)malloc(n * sizeof(int));
    int* right_indices = (int*)malloc(n * sizeof(int));
    int n_left = 0;
    int n_right = 0;

    for (int i = 0; i < n; i++) {
        int idx = indices[i];
        if (X[idx * n_features + split.feature_index] <= split.threshold) {
            left_indices[n_left++] = idx;
        } else {
            right_indices[n_right++] = idx;
        }
    }

    // Recursively build children
    if (n_left > 0) {
        node->left = build_tree(X, y, left_indices, n_left, n_features, n_classes,
                               depth + 1, max_depth, min_samples_split, feature_importances);
    }

    if (n_right > 0) {
        node->right = build_tree(X, y, right_indices, n_right, n_features, n_classes,
                                depth + 1, max_depth, min_samples_split, feature_importances);
    }

    free(left_indices);
    free(right_indices);
    free(y_subset);

    return node;
}

// ============================================================================
// Public API
// ============================================================================

// Train decision tree classifier
DecisionTreeModel fp_decision_tree_train(
    const double* X,
    const int* y,
    int n,
    int d,
    int n_classes,
    int max_depth,
    int min_samples_split
) {
    DecisionTreeModel model;
    model.max_depth = max_depth;
    model.min_samples_split = min_samples_split;
    model.is_classifier = 1;
    model.n_features = d;
    model.n_classes = n_classes;

    // Initialize feature importances
    model.feature_importances = (double*)calloc(d, sizeof(double));

    // Create indices array
    int* indices = (int*)malloc(n * sizeof(int));
    for (int i = 0; i < n; i++) {
        indices[i] = i;
    }

    // Build tree
    model.root = build_tree(X, y, indices, n, d, n_classes, 0, max_depth,
                           min_samples_split, model.feature_importances);

    // Normalize feature importances
    double total_importance = 0.0;
    for (int f = 0; f < d; f++) {
        total_importance += model.feature_importances[f];
    }

    if (total_importance > 0.0) {
        for (int f = 0; f < d; f++) {
            model.feature_importances[f] /= total_importance;
        }
    }

    free(indices);

    return model;
}

// Predict single sample
int fp_decision_tree_predict(const DecisionTreeModel* model, const double* x) {
    DecisionNode* node = model->root;

    while (node != NULL && !node->is_leaf) {
        if (x[node->feature_index] <= node->threshold) {
            node = node->left;
        } else {
            node = node->right;
        }
    }

    // If we ended up at NULL (shouldn't happen with proper tree), return 0
    if (node == NULL) return 0;

    return (int)node->value;
}

// Predict multiple samples
void fp_decision_tree_predict_batch(
    const DecisionTreeModel* model,
    const double* X,
    int n,
    int* predictions
) {
    for (int i = 0; i < n; i++) {
        predictions[i] = fp_decision_tree_predict(model, &X[i * model->n_features]);
    }
}

// Compute accuracy on test set
double fp_decision_tree_accuracy(
    const DecisionTreeModel* model,
    const double* X_test,
    const int* y_test,
    int n_test
) {
    int correct = 0;

    for (int i = 0; i < n_test; i++) {
        int pred = fp_decision_tree_predict(model, &X_test[i * model->n_features]);
        if (pred == y_test[i]) {
            correct++;
        }
    }

    return (double)correct / n_test;
}

// ============================================================================
// Tree Visualization and Analysis
// ============================================================================

// Print tree structure (helper)
static void print_node(DecisionNode* node, int depth, const char* prefix) {
    if (node == NULL) return;

    for (int i = 0; i < depth; i++) printf("  ");
    printf("%s", prefix);

    if (node->is_leaf) {
        printf("Leaf: class=%d, samples=%d, gini=%.4f\n",
               (int)node->value, node->n_samples, node->impurity);
    } else {
        printf("Node: feature_%d <= %.2f, samples=%d, gini=%.4f\n",
               node->feature_index, node->threshold, node->n_samples, node->impurity);

        print_node(node->left, depth + 1, "L: ");
        print_node(node->right, depth + 1, "R: ");
    }
}

// Print decision tree
void fp_decision_tree_print(const DecisionTreeModel* model) {
    printf("Decision Tree (max_depth=%d, min_samples_split=%d)\n",
           model->max_depth, model->min_samples_split);
    printf("\n");
    print_node(model->root, 0, "Root: ");
}

// Get tree depth (helper)
static int get_depth(DecisionNode* node) {
    if (node == NULL || node->is_leaf) return 0;

    int left_depth = get_depth(node->left);
    int right_depth = get_depth(node->right);

    return 1 + (left_depth > right_depth ? left_depth : right_depth);
}

// Get tree depth
int fp_decision_tree_depth(const DecisionTreeModel* model) {
    return get_depth(model->root);
}

// Count nodes (helper)
static int count_nodes(DecisionNode* node) {
    if (node == NULL) return 0;
    return 1 + count_nodes(node->left) + count_nodes(node->right);
}

// Get number of nodes
int fp_decision_tree_n_nodes(const DecisionTreeModel* model) {
    return count_nodes(model->root);
}

// Count leaf nodes (helper)
static int count_leaves(DecisionNode* node) {
    if (node == NULL) return 0;
    if (node->is_leaf) return 1;
    return count_leaves(node->left) + count_leaves(node->right);
}

// Get number of leaf nodes
int fp_decision_tree_n_leaves(const DecisionTreeModel* model) {
    return count_leaves(model->root);
}

// Print feature importances
void fp_decision_tree_print_feature_importances(const DecisionTreeModel* model) {
    printf("Feature Importances:\n");
    for (int f = 0; f < model->n_features; f++) {
        printf("  Feature %d: %.4f\n", f, model->feature_importances[f]);
    }
}

// ============================================================================
// Memory Management
// ============================================================================

// Free decision tree model
void fp_decision_tree_free(DecisionTreeModel* model) {
    free_node(model->root);
    free(model->feature_importances);
}
