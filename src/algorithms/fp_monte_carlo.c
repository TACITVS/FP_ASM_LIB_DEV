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

// ============================================================================
// Random Number Generation
// ============================================================================

// Simple LCG (Linear Congruential Generator)
// Fast, deterministic, sufficient for Monte Carlo
static unsigned long long rng_state = 12345;

void fp_monte_carlo_seed(unsigned long long seed) {
    rng_state = seed;
}

// Generate uniform random double in [0, 1)
static inline double rand_uniform() {
    rng_state = rng_state * 6364136223846793005ULL + 1442695040888963407ULL;
    return (rng_state >> 11) * 0x1.0p-53;  // 53 bits of precision
}

// Generate uniform random double in [a, b)
static inline double rand_uniform_range(double a, double b) {
    return a + (b - a) * rand_uniform();
}

// Box-Muller transform: generate standard normal N(0,1)
static double rand_normal() {
    static int has_spare = 0;
    static double spare;

    if (has_spare) {
        has_spare = 0;
        return spare;
    }

    double u, v, s;
    do {
        u = rand_uniform() * 2.0 - 1.0;
        v = rand_uniform() * 2.0 - 1.0;
        s = u * u + v * v;
    } while (s >= 1.0 || s == 0.0);

    s = sqrt(-2.0 * log(s) / s);
    spare = v * s;
    has_spare = 1;
    return u * s;
}

// Generate normal N(mu, sigma^2)
static inline double rand_normal_dist(double mu, double sigma) {
    return mu + sigma * rand_normal();
}

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
} PiEstimationResult;

PiEstimationResult fp_monte_carlo_estimate_pi(int n_samples) {
    int inside = 0;

    for (int i = 0; i < n_samples; i++) {
        double x = rand_uniform();
        double y = rand_uniform();
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

    return result;
}

// Show convergence: estimate π with increasing sample sizes
void fp_monte_carlo_pi_convergence(int* sample_sizes, int n_sizes, PiEstimationResult* results) {
    for (int i = 0; i < n_sizes; i++) {
        results[i] = fp_monte_carlo_estimate_pi(sample_sizes[i]);
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
} IntegrationResult;

IntegrationResult fp_monte_carlo_integrate(
    MonteCarloFunction f,
    double a,
    double b,
    int n_samples,
    double true_value  // Pass 0 if unknown
) {
    double sum = 0.0;
    double sum_sq = 0.0;

    // Sample random points and evaluate function
    for (int i = 0; i < n_samples; i++) {
        double x = rand_uniform_range(a, b);
        double fx = f(x);
        sum += fx;
        sum_sq += fx * fx;
    }

    double mean = sum / n_samples;
    double variance = (sum_sq / n_samples) - (mean * mean);
    double std_error = sqrt(variance / n_samples);

    double integral = (b - a) * mean;

    IntegrationResult result;
    result.integral_estimate = integral;
    result.true_value = true_value;
    result.error = (true_value != 0.0) ? fabs(integral - true_value) : 0.0;
    result.std_error = (b - a) * std_error;
    result.confidence_95 = 1.96 * result.std_error;
    result.n_samples = n_samples;

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
} OptionPricingResult;

OptionPricingResult fp_monte_carlo_option_price(
    double S0,      // Initial stock price
    double K,       // Strike price
    double r,       // Risk-free interest rate (annual)
    double sigma,   // Volatility (annual)
    double T,       // Time to maturity (years)
    int n_sims      // Number of simulations
) {
    double sum_payoff = 0.0;
    double sum_payoff_sq = 0.0;

    double drift = (r - 0.5 * sigma * sigma) * T;
    double diffusion = sigma * sqrt(T);
    double discount = exp(-r * T);

    // Simulate stock price paths
    for (int i = 0; i < n_sims; i++) {
        double Z = rand_normal();
        double ST = S0 * exp(drift + diffusion * Z);

        // European call payoff: max(ST - K, 0)
        double payoff = (ST > K) ? (ST - K) : 0.0;

        sum_payoff += payoff;
        sum_payoff_sq += payoff * payoff;
    }

    double mean_payoff = sum_payoff / n_sims;
    double variance = (sum_payoff_sq / n_sims) - (mean_payoff * mean_payoff);
    double std_error = sqrt(variance / n_sims);

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
    double distance_from_origin; // √(x² + y²)
    double max_distance;       // Maximum distance reached
    double mean_distance;      // Average distance over all steps
    int n_steps;
} RandomWalkResult;

RandomWalkResult fp_monte_carlo_random_walk_2d(int n_steps, double step_size) {
    double x = 0.0;
    double y = 0.0;
    double sum_distance = 0.0;
    double max_dist = 0.0;

    for (int i = 0; i < n_steps; i++) {
        // Random direction [0, 2π)
        double theta = rand_uniform() * 2.0 * M_PI;
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
} RandomWalkEnsemble;

RandomWalkEnsemble fp_monte_carlo_random_walk_ensemble(
    int n_walks,
    int n_steps_per_walk,
    double step_size
) {
    double sum_final_dist = 0.0;
    double sum_final_dist_sq = 0.0;
    double max_final_dist = 0.0;
    double sum_max_dist = 0.0;

    for (int i = 0; i < n_walks; i++) {
        RandomWalkResult walk = fp_monte_carlo_random_walk_2d(n_steps_per_walk, step_size);

        sum_final_dist += walk.distance_from_origin;
        sum_final_dist_sq += walk.distance_from_origin * walk.distance_from_origin;
        sum_max_dist += walk.max_distance;

        if (walk.distance_from_origin > max_final_dist) {
            max_final_dist = walk.distance_from_origin;
        }
    }

    double mean = sum_final_dist / n_walks;
    double variance = (sum_final_dist_sq / n_walks) - (mean * mean);

    RandomWalkEnsemble result;
    result.mean_final_distance = mean;
    result.std_final_distance = sqrt(variance);
    result.max_final_distance = max_final_dist;
    result.mean_max_distance = sum_max_dist / n_walks;
    result.n_walks = n_walks;
    result.n_steps_per_walk = n_steps_per_walk;

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
