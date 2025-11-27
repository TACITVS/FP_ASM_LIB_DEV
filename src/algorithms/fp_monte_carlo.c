// fp_monte_carlo.c
//
// Monte Carlo Simulation Algorithms
// Demonstrates probabilistic methods for numerical computation
//
// This showcases:
// - π estimation (geometric probability)
// - Numerical integration (random sampling)
// - Option pricing (Black-Scholes simulation)
// - Random walk (statistical physics)
// - Convergence analysis (law of large numbers)
//
// FP Primitives Used:
// - Reduction operations (counting, summing)
// - Statistical computations (mean, variance)
// - Random number generation + transformations

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "fp_rng.h"  // Pure, deterministic RNG (replaces global state)

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================================
// Pattern 1: Array Statistics (Inline Helpers)
// ============================================================================

// Mean: sum / n
static inline double fp_mean_inline(const double* data, size_t n) {
    if (!data || n == 0) return 0.0;
    double sum = 0.0;
    for (size_t i = 0; i < n; i++) {
        sum += data[i];
    }
    return sum / (double)n;
}

// Variance: E[X²] - E[X]²
static inline double fp_variance_inline(const double* data, size_t n, double mean) {
    if (!data || n == 0) return 0.0;
    double sum_sq_diff = 0.0;
    for (size_t i = 0; i < n; i++) {
        double diff = data[i] - mean;
        sum_sq_diff += diff * diff;
    }
    return sum_sq_diff / (double)n;
}

// Variance from precomputed sums: Var = E[X²] - E[X]²
// This is useful for single-pass algorithms where sum and sum_sq are already computed
static inline double fp_variance_from_sums(double sum, double sum_sq, size_t n) {
    if (n == 0) return 0.0;
    double mean = sum / (double)n;
    double mean_sq = sum_sq / (double)n;
    return mean_sq - mean * mean;
}

// Standard error: sqrt(Var / n)
static inline double fp_std_error_inline(double variance, size_t n) {
    if (n == 0) return 0.0;
    return sqrt(variance / (double)n);
}

// ============================================================================
// Random Number Generation - Now uses fp_rng.h (pure, no global state)
// ============================================================================
// All functions now take fp_rng_t and return updated state via result structs.
// This ensures deterministic, reproducible results with any seed.

// ============================================================================
// 1. π Estimation (Circle in Square)
// ============================================================================

// Estimate π by sampling random points in unit square
// Count how many fall inside quarter circle (x²+y²≤1)
// π ≈ 4 * (inside_count / total_samples)
typedef struct {
    double pi_estimate;
    double error;              // |estimate - π|
    double relative_error;     // |estimate - π| / π
    int inside_count;
    int total_samples;
    double confidence_95;      // 95% confidence interval (±)
    fp_rng_t rng;              // Updated RNG state (for chaining)
} PiEstimationResult;

PiEstimationResult fp_monte_carlo_estimate_pi(int n_samples, uint64_t seed) {
    // HIGH-011 FIX: Add input validation
    if (n_samples <= 0) {
        PiEstimationResult error = {0};
        return error;
    }

    fp_rng_t rng = fp_rng_create(seed);
    int inside = 0;

    for (int i = 0; i < n_samples; i++) {
        double x, y;
        rng = fp_rng_next_f64(rng, &x);
        rng = fp_rng_next_f64(rng, &y);
        if (x * x + y * y <= 1.0) {
            inside++;
        }
    }

    double pi_est = 4.0 * inside / (double)n_samples;
    double pi_true = 3.14159265358979323846;

    // Confidence interval: π̂ ± 1.96·√(π̂(4-π̂)/n)
    double variance = pi_est * (4.0 - pi_est) / n_samples;
    double confidence_95 = 1.96 * sqrt(variance);

    PiEstimationResult result;
    result.pi_estimate = pi_est;
    result.error = fabs(pi_est - pi_true);
    result.relative_error = result.error / pi_true;
    result.inside_count = inside;
    result.total_samples = n_samples;
    result.confidence_95 = confidence_95;
    result.rng = rng;

    return result;
}

// Show convergence: estimate π with increasing sample sizes
// Each run uses a different seed derived from base_seed + index
void fp_monte_carlo_pi_convergence(int* sample_sizes, int n_sizes, PiEstimationResult* results, uint64_t base_seed) {
    for (int i = 0; i < n_sizes; i++) {
        results[i] = fp_monte_carlo_estimate_pi(sample_sizes[i], base_seed + (uint64_t)i);
    }
}

// ============================================================================
// 2. Numerical Integration
// ============================================================================

// Estimate ∫[a,b] f(x) dx using Monte Carlo sampling
// Approximation: (b-a) * mean(f(x_i)) where x_i ~ Uniform[a,b]
typedef double (*MonteCarloFunction)(double);

typedef struct {
    double integral_estimate;
    double true_value;         // If known (0 if unknown)
    double error;              // |estimate - true| (0 if unknown)
    double std_error;          // Standard error of estimate
    double confidence_95;      // 95% confidence interval (±)
    int n_samples;
    fp_rng_t rng;              // Updated RNG state (for chaining)
} IntegrationResult;

// REFACTORED: Now uses fp_rng.h (pure, deterministic)
IntegrationResult fp_monte_carlo_integrate(
    MonteCarloFunction f,
    double a,
    double b,
    int n_samples,
    double true_value,  // Pass 0 if unknown
    uint64_t seed
) {
    // HIGH-011 FIX: Add input validation
    if (!f || n_samples <= 0 || b <= a) {
        IntegrationResult error = {0};
        return error;
    }

    fp_rng_t rng = fp_rng_create(seed);
    double sum = 0.0;
    double sum_sq = 0.0;

    // Sample random points and evaluate function
    for (int i = 0; i < n_samples; i++) {
        double x;
        rng = fp_rng_next_f64_range(rng, a, b, &x);
        double fx = f(x);
        sum += fx;
        sum_sq += fx * fx;
    }

    double mean = sum / n_samples;
    double variance = fp_variance_from_sums(sum, sum_sq, n_samples);
    double std_error = fp_std_error_inline(variance, n_samples);

    double integral = (b - a) * mean;

    IntegrationResult result;
    result.integral_estimate = integral;
    result.true_value = true_value;
    result.error = (true_value != 0.0) ? fabs(integral - true_value) : 0.0;
    result.std_error = (b - a) * std_error;
    result.confidence_95 = 1.96 * result.std_error;
    result.n_samples = n_samples;
    result.rng = rng;

    return result;
}

// ============================================================================
// 3. Option Pricing (Black-Scholes Monte Carlo)
// ============================================================================

// European Call Option Pricing via Monte Carlo
// S_T = S_0 * exp((r - σ²/2)T + σ√T·Z)  where Z ~ N(0,1)
// Payoff = max(S_T - K, 0)
// Option price = e^(-rT) * E[Payoff]
typedef struct {
    double option_price;       // Estimated option value
    double std_error;          // Standard error of estimate
    double confidence_95;      // 95% confidence interval (±)
    int n_simulations;

    // Input parameters (for reference)
    double S0;                 // Initial stock price
    double K;                  // Strike price
    double r;                  // Risk-free rate
    double sigma;              // Volatility
    double T;                  // Time to maturity
    fp_rng_t rng;              // Updated RNG state (for chaining)
} OptionPricingResult;

// REFACTORED: Now uses fp_rng.h (pure, deterministic)
OptionPricingResult fp_monte_carlo_option_price(
    double S0,      // Initial stock price
    double K,       // Strike price
    double r,       // Risk-free interest rate (annual)
    double sigma,   // Volatility (annual)
    double T,       // Time to maturity (years)
    int n_sims,     // Number of simulations
    uint64_t seed
) {
    // HIGH-011 FIX: Add input validation
    if (S0 <= 0.0 || K <= 0.0 || sigma <= 0.0 || T <= 0.0 || n_sims <= 0) {
        OptionPricingResult error = {0};
        return error;
    }

    fp_rng_t rng = fp_rng_create(seed);
    double sum_payoff = 0.0;
    double sum_payoff_sq = 0.0;

    double drift = (r - 0.5 * sigma * sigma) * T;
    double diffusion = sigma * sqrt(T);
    double discount = exp(-r * T);

    // Pre-allocate array for normal samples (Box-Muller generates pairs)
    // We'll generate samples in batches for efficiency
    double* normals = (double*)malloc((size_t)n_sims * sizeof(double));
    if (normals) {
        rng = fp_rng_fill_normal_f64(rng, normals, (size_t)n_sims, 0.0, 1.0);

        // Simulate stock price paths
        for (int i = 0; i < n_sims; i++) {
            double Z = normals[i];
            double ST = S0 * exp(drift + diffusion * Z);

            // European call payoff: max(ST - K, 0)
            double payoff = (ST > K) ? (ST - K) : 0.0;

            sum_payoff += payoff;
            sum_payoff_sq += payoff * payoff;
        }
        free(normals);
    }

    double mean_payoff = sum_payoff / n_sims;
    double variance = fp_variance_from_sums(sum_payoff, sum_payoff_sq, n_sims);
    double std_error = fp_std_error_inline(variance, n_sims);

    OptionPricingResult result;
    result.option_price = discount * mean_payoff;
    result.std_error = discount * std_error;
    result.confidence_95 = 1.96 * result.std_error;
    result.n_simulations = n_sims;
    result.S0 = S0;
    result.K = K;
    result.r = r;
    result.sigma = sigma;
    result.T = T;
    result.rng = rng;

    return result;
}

// Cumulative normal distribution (approximation)
static double norm_cdf(double x) {
    return 0.5 * (1.0 + erf(x / sqrt(2.0)));
}

// Black-Scholes closed-form solution (for comparison)
static double black_scholes_call(double S0, double K, double r, double sigma, double T) {
    double d1 = (log(S0 / K) + (r + 0.5 * sigma * sigma) * T) / (sigma * sqrt(T));
    double d2 = d1 - sigma * sqrt(T);

    return S0 * norm_cdf(d1) - K * exp(-r * T) * norm_cdf(d2);
}

double fp_monte_carlo_black_scholes_exact(double S0, double K, double r, double sigma, double T) {
    return black_scholes_call(S0, K, r, sigma, T);
}

// ============================================================================
// 4. Random Walk (2D Brownian Motion)
// ============================================================================

// Simulate 2D random walk and compute statistics
typedef struct {
    double final_x;            // Final x position
    double final_y;            // Final y position
    double distance_from_origin; // sqrt(x^2 + y^2)
    double max_distance;       // Maximum distance reached
    double mean_distance;      // Average distance over all steps
    int n_steps;
    fp_rng_t rng;              // Updated RNG state (for chaining)
} RandomWalkResult;

RandomWalkResult fp_monte_carlo_random_walk_2d(int n_steps, double step_size, fp_rng_t rng) {
    // HIGH-011 FIX: Add input validation
    if (n_steps <= 0 || step_size <= 0.0) {
        RandomWalkResult error = {0};
        return error;
    }

    double x = 0.0;
    double y = 0.0;
    double sum_distance = 0.0;
    double max_dist = 0.0;

    for (int i = 0; i < n_steps; i++) {
        // Random direction [0, 2*pi)
        double theta;
        rng = fp_rng_next_f64(rng, &theta);
        theta *= 2.0 * M_PI;
        x += step_size * cos(theta);
        y += step_size * sin(theta);

        double dist = sqrt(x * x + y * y);
        sum_distance += dist;
        if (dist > max_dist) max_dist = dist;
    }

    RandomWalkResult result;
    result.final_x = x;
    result.final_y = y;
    result.distance_from_origin = sqrt(x * x + y * y);
    result.max_distance = max_dist;
    result.mean_distance = sum_distance / n_steps;
    result.n_steps = n_steps;
    result.rng = rng;

    return result;
}

// Simulate multiple random walks and collect statistics
typedef struct {
    double mean_final_distance;
    double std_final_distance;
    double max_final_distance;
    double mean_max_distance;
    int n_walks;
    int n_steps_per_walk;
    fp_rng_t rng;              // Updated RNG state (for chaining)
} RandomWalkEnsemble;

// REFACTORED: Now uses fp_rng.h (pure, deterministic)
RandomWalkEnsemble fp_monte_carlo_random_walk_ensemble(
    int n_walks,
    int n_steps_per_walk,
    double step_size,
    uint64_t seed
) {
    fp_rng_t rng = fp_rng_create(seed);
    double sum_final_dist = 0.0;
    double sum_final_dist_sq = 0.0;
    double max_final_dist = 0.0;
    double sum_max_dist = 0.0;

    for (int i = 0; i < n_walks; i++) {
        RandomWalkResult walk = fp_monte_carlo_random_walk_2d(n_steps_per_walk, step_size, rng);
        rng = walk.rng;  // Chain the RNG state

        sum_final_dist += walk.distance_from_origin;
        sum_final_dist_sq += walk.distance_from_origin * walk.distance_from_origin;
        sum_max_dist += walk.max_distance;

        if (walk.distance_from_origin > max_final_dist) {
            max_final_dist = walk.distance_from_origin;
        }
    }

    double mean = sum_final_dist / n_walks;
    double variance = fp_variance_from_sums(sum_final_dist, sum_final_dist_sq, n_walks);

    RandomWalkEnsemble result;
    result.mean_final_distance = mean;
    result.std_final_distance = sqrt(variance);
    result.max_final_distance = max_final_dist;
    result.mean_max_distance = sum_max_dist / n_walks;
    result.n_walks = n_walks;
    result.n_steps_per_walk = n_steps_per_walk;
    result.rng = rng;

    return result;
}

// ============================================================================
// Printing and Visualization
// ============================================================================

void fp_monte_carlo_print_pi_result(const PiEstimationResult* result) {
    printf("π Estimation Results:\n");
    printf("  Samples: %d\n", result->total_samples);
    printf("  Inside circle: %d (%.2f%%)\n",
           result->inside_count,
           100.0 * result->inside_count / result->total_samples);
    printf("  π estimate: %.10f\n", result->pi_estimate);
    printf("  True π:     %.10f\n", 3.14159265358979323846);
    printf("  Error: %.10f (%.4f%%)\n", result->error, result->relative_error * 100.0);
    printf("  95%% CI: %.10f ± %.10f\n", result->pi_estimate, result->confidence_95);
}

void fp_monte_carlo_print_integration_result(const IntegrationResult* result) {
    printf("Integration Results:\n");
    printf("  Samples: %d\n", result->n_samples);
    printf("  Estimate: %.10f\n", result->integral_estimate);
    if (result->true_value != 0.0) {
        printf("  True value: %.10f\n", result->true_value);
        printf("  Error: %.10f (%.4f%%)\n",
               result->error,
               100.0 * result->error / fabs(result->true_value));
    }
    printf("  Std Error: %.10f\n", result->std_error);
    printf("  95%% CI: %.10f ± %.10f\n",
           result->integral_estimate, result->confidence_95);
}

void fp_monte_carlo_print_option_result(const OptionPricingResult* result) {
    printf("Option Pricing Results:\n");
    printf("  Simulations: %d\n", result->n_simulations);
    printf("  Parameters: S0=$%.2f, K=$%.2f, r=%.2f%%, σ=%.2f%%, T=%.2fy\n",
           result->S0, result->K, result->r * 100.0, result->sigma * 100.0, result->T);
    printf("  Option Price: $%.4f\n", result->option_price);
    printf("  Std Error: $%.4f\n", result->std_error);
    printf("  95%% CI: $%.4f ± $%.4f\n", result->option_price, result->confidence_95);
}

void fp_monte_carlo_print_random_walk_result(const RandomWalkResult* result) {
    printf("Random Walk Results:\n");
    printf("  Steps: %d\n", result->n_steps);
    printf("  Final position: (%.2f, %.2f)\n", result->final_x, result->final_y);
    printf("  Final distance from origin: %.2f\n", result->distance_from_origin);
    printf("  Maximum distance reached: %.2f\n", result->max_distance);
    printf("  Mean distance: %.2f\n", result->mean_distance);
}

void fp_monte_carlo_print_walk_ensemble_result(const RandomWalkEnsemble* result) {
    printf("Random Walk Ensemble Results:\n");
    printf("  Number of walks: %d\n", result->n_walks);
    printf("  Steps per walk: %d\n", result->n_steps_per_walk);
    printf("  Mean final distance: %.2f ± %.2f\n",
           result->mean_final_distance, result->std_final_distance);
    printf("  Maximum final distance: %.2f\n", result->max_final_distance);
    printf("  Mean of max distances: %.2f\n", result->mean_max_distance);

    // Theoretical expectation: E[distance] ≈ √n for step_size=1
    double theoretical = sqrt((double)result->n_steps_per_walk);
    printf("  Theoretical √n: %.2f\n", theoretical);
}
