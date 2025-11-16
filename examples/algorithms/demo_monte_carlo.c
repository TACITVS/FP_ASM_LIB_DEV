// demo_monte_carlo.c - Monte Carlo Simulation Demo
// Demonstrates probabilistic computation methods

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

// Forward declarations
typedef struct {
    double pi_estimate;
    double error;
    double relative_error;
    int inside_count;
    int total_samples;
    double confidence_95;
} PiEstimationResult;

typedef struct {
    double integral_estimate;
    double true_value;
    double error;
    double std_error;
    double confidence_95;
    int n_samples;
} IntegrationResult;

typedef struct {
    double option_price;
    double std_error;
    double confidence_95;
    int n_simulations;
    double S0, K, r, sigma, T;
} OptionPricingResult;

typedef struct {
    double final_x, final_y;
    double distance_from_origin;
    double max_distance;
    double mean_distance;
    int n_steps;
} RandomWalkResult;

typedef struct {
    double mean_final_distance;
    double std_final_distance;
    double max_final_distance;
    double mean_max_distance;
    int n_walks, n_steps_per_walk;
} RandomWalkEnsemble;

typedef double (*MonteCarloFunction)(double);

// Function declarations
void fp_monte_carlo_seed(unsigned long long seed);
PiEstimationResult fp_monte_carlo_estimate_pi(int n_samples);
void fp_monte_carlo_pi_convergence(int* sample_sizes, int n_sizes, PiEstimationResult* results);
IntegrationResult fp_monte_carlo_integrate(MonteCarloFunction f, double a, double b, int n_samples, double true_value);
OptionPricingResult fp_monte_carlo_option_price(double S0, double K, double r, double sigma, double T, int n_sims);
double fp_monte_carlo_black_scholes_exact(double S0, double K, double r, double sigma, double T);
RandomWalkResult fp_monte_carlo_random_walk_2d(int n_steps, double step_size);
RandomWalkEnsemble fp_monte_carlo_random_walk_ensemble(int n_walks, int n_steps_per_walk, double step_size);

void fp_monte_carlo_print_pi_result(const PiEstimationResult* result);
void fp_monte_carlo_print_integration_result(const IntegrationResult* result);
void fp_monte_carlo_print_option_result(const OptionPricingResult* result);
void fp_monte_carlo_print_random_walk_result(const RandomWalkResult* result);
void fp_monte_carlo_print_walk_ensemble_result(const RandomWalkEnsemble* result);

// ============================================================================
// TEST 1: π Estimation (Convergence Analysis)
// ============================================================================
void test_pi_estimation() {
    printf("================================================================\n");
    printf("TEST 1: π Estimation via Monte Carlo\n");
    printf("================================================================\n\n");

    printf("Method: Sample random points in unit square [0,1]×[0,1]\n");
    printf("Count how many fall inside quarter circle (x²+y²≤1)\n");
    printf("π ≈ 4 × (inside_count / total_samples)\n\n");

    // Test with increasing sample sizes to show convergence
    int sample_sizes[] = {100, 1000, 10000, 100000, 1000000};
    int n_sizes = sizeof(sample_sizes) / sizeof(sample_sizes[0]);

    printf("Convergence Analysis:\n");
    printf("%-12s | %-15s | %-12s | %-12s\n", "Samples", "π Estimate", "Error", "Rel Error %");
    printf("-------------|-----------------|--------------|-------------\n");

    for (int i = 0; i < n_sizes; i++) {
        PiEstimationResult result = fp_monte_carlo_estimate_pi(sample_sizes[i]);
        printf("%-12d | %.10f | %.10f | %.6f%%\n",
               result.total_samples,
               result.pi_estimate,
               result.error,
               result.relative_error * 100.0);
    }

    printf("\nDetailed results for 1M samples:\n");
    PiEstimationResult detailed = fp_monte_carlo_estimate_pi(1000000);
    fp_monte_carlo_print_pi_result(&detailed);

    printf("\nObservation: Error decreases as O(1/√n) - Law of Large Numbers!\n\n");
}

// ============================================================================
// TEST 2: Numerical Integration
// ============================================================================

// Test functions
double f_simple(double x) { return x * x; }              // ∫x² dx = x³/3
double f_sin(double x) { return sin(x); }                // ∫sin(x) dx = -cos(x)
double f_exp(double x) { return exp(-x * x); }           // ∫e^(-x²) - no closed form, but numerical value known
double f_sqrt(double x) { return sqrt(x); }              // ∫√x dx = (2/3)x^(3/2)

void test_numerical_integration() {
    printf("================================================================\n");
    printf("TEST 2: Numerical Integration via Monte Carlo\n");
    printf("================================================================\n\n");

    printf("Method: Estimate ∫[a,b] f(x)dx ≈ (b-a) × mean(f(x_i))\n");
    printf("where x_i are uniformly sampled from [a,b]\n\n");

    int n_samples = 100000;

    // Test 1: ∫[0,1] x² dx = 1/3
    printf("Test 1: ∫[0,1] x² dx\n");
    double true_val_1 = 1.0 / 3.0;
    IntegrationResult result1 = fp_monte_carlo_integrate(f_simple, 0.0, 1.0, n_samples, true_val_1);
    fp_monte_carlo_print_integration_result(&result1);
    printf("\n");

    // Test 2: ∫[0,π] sin(x) dx = 2
    printf("Test 2: ∫[0,π] sin(x) dx\n");
    double true_val_2 = 2.0;
    IntegrationResult result2 = fp_monte_carlo_integrate(f_sin, 0.0, M_PI, n_samples, true_val_2);
    fp_monte_carlo_print_integration_result(&result2);
    printf("\n");

    // Test 3: ∫[0,1] √x dx = 2/3
    printf("Test 3: ∫[0,1] √x dx\n");
    double true_val_3 = 2.0 / 3.0;
    IntegrationResult result3 = fp_monte_carlo_integrate(f_sqrt, 0.0, 1.0, n_samples, true_val_3);
    fp_monte_carlo_print_integration_result(&result3);
    printf("\n");

    // Test 4: ∫[-2,2] e^(-x²) dx (Gaussian-like, known value ≈ 3.54491)
    printf("Test 4: ∫[-2,2] e^(-x²) dx (Gaussian integral)\n");
    double true_val_4 = 3.54491;  // Numerical value
    IntegrationResult result4 = fp_monte_carlo_integrate(f_exp, -2.0, 2.0, n_samples, true_val_4);
    fp_monte_carlo_print_integration_result(&result4);
    printf("\n");

    printf("Observation: Monte Carlo integration works for ANY function!\n");
    printf("No need for analytical derivatives or special methods.\n\n");
}

// ============================================================================
// TEST 3: Option Pricing (Black-Scholes Monte Carlo)
// ============================================================================
void test_option_pricing() {
    printf("================================================================\n");
    printf("TEST 3: European Option Pricing (Black-Scholes Monte Carlo)\n");
    printf("================================================================\n\n");

    printf("Method: Simulate stock price paths using geometric Brownian motion\n");
    printf("S_T = S_0 × exp((r - σ²/2)T + σ√T·Z) where Z ~ N(0,1)\n");
    printf("Option value = e^(-rT) × E[max(S_T - K, 0)]\n\n");

    // Test case: At-the-money call option
    double S0 = 100.0;      // Initial stock price
    double K = 100.0;       // Strike price (at-the-money)
    double r = 0.05;        // 5% risk-free rate
    double sigma = 0.20;    // 20% volatility
    double T = 1.0;         // 1 year to maturity

    printf("Option Parameters:\n");
    printf("  Initial stock price (S0): $%.2f\n", S0);
    printf("  Strike price (K): $%.2f\n", K);
    printf("  Risk-free rate (r): %.2f%%\n", r * 100.0);
    printf("  Volatility (σ): %.2f%%\n", sigma * 100.0);
    printf("  Time to maturity (T): %.2f years\n\n", T);

    // Monte Carlo simulation
    int n_sims = 100000;
    printf("Running %d simulations...\n\n", n_sims);

    OptionPricingResult mc_result = fp_monte_carlo_option_price(S0, K, r, sigma, T, n_sims);
    fp_monte_carlo_print_option_result(&mc_result);

    // Compare with Black-Scholes analytical solution
    double bs_exact = fp_monte_carlo_black_scholes_exact(S0, K, r, sigma, T);
    printf("\nBlack-Scholes Analytical Price: $%.4f\n", bs_exact);
    printf("Monte Carlo vs Analytical Error: $%.4f (%.2f%%)\n",
           fabs(mc_result.option_price - bs_exact),
           100.0 * fabs(mc_result.option_price - bs_exact) / bs_exact);

    printf("\nReal-World Impact:\n");
    printf("  - This method values trillions of dollars in derivatives\n");
    printf("  - Used by every major investment bank and hedge fund\n");
    printf("  - Can handle exotic options without analytical solutions\n\n");

    // Test 2: In-the-money option
    printf("Test 2: In-the-Money Call (K=$90)\n");
    OptionPricingResult itm_result = fp_monte_carlo_option_price(S0, 90.0, r, sigma, T, n_sims);
    double bs_itm = fp_monte_carlo_black_scholes_exact(S0, 90.0, r, sigma, T);
    printf("  Monte Carlo: $%.4f\n", itm_result.option_price);
    printf("  Black-Scholes: $%.4f\n", bs_itm);
    printf("  Error: $%.4f\n\n", fabs(itm_result.option_price - bs_itm));
}

// ============================================================================
// TEST 4: Random Walk (2D Brownian Motion)
// ============================================================================
void test_random_walk() {
    printf("================================================================\n");
    printf("TEST 4: 2D Random Walk (Brownian Motion)\n");
    printf("================================================================\n\n");

    printf("Method: Particle starts at origin, takes random steps\n");
    printf("Each step: random direction θ ∈ [0,2π), fixed step size\n");
    printf("Theory: E[distance] ≈ √n (for step_size=1, n steps)\n\n");

    // Single walk demonstration
    printf("Single Random Walk (10,000 steps):\n");
    RandomWalkResult single = fp_monte_carlo_random_walk_2d(10000, 1.0);
    fp_monte_carlo_print_random_walk_result(&single);
    printf("\n");

    // Ensemble of walks
    printf("Ensemble of Random Walks:\n");
    int steps_per_walk[] = {100, 1000, 10000};
    int n_tests = sizeof(steps_per_walk) / sizeof(steps_per_walk[0]);

    printf("%-12s | %-18s | %-18s | %-18s\n",
           "Steps", "Mean Distance", "Theoretical √n", "Ratio");
    printf("-------------|--------------------|--------------------|-------------------\n");

    for (int i = 0; i < n_tests; i++) {
        int n_steps = steps_per_walk[i];
        RandomWalkEnsemble ensemble = fp_monte_carlo_random_walk_ensemble(1000, n_steps, 1.0);

        double theoretical = sqrt((double)n_steps);
        double ratio = ensemble.mean_final_distance / theoretical;

        printf("%-12d | %.2f ± %.2f       | %.2f               | %.3f\n",
               n_steps,
               ensemble.mean_final_distance,
               ensemble.std_final_distance,
               theoretical,
               ratio);
    }

    printf("\n");
    printf("Detailed Ensemble Results (10,000 steps, 1,000 walks):\n");
    RandomWalkEnsemble detailed = fp_monte_carlo_random_walk_ensemble(1000, 10000, 1.0);
    fp_monte_carlo_print_walk_ensemble_result(&detailed);

    printf("\nPhysics Connection:\n");
    printf("  - Models diffusion of particles in fluids\n");
    printf("  - Einstein's theory of Brownian motion\n");
    printf("  - Stock price movements (continuous time limit)\n");
    printf("  - Polymer chain configurations\n\n");
}

// ============================================================================
// Main
// ============================================================================
int main() {
    printf("================================================================\n");
    printf("  Monte Carlo Simulation Demo\n");
    printf("  FP-ASM Library - Probabilistic Computation\n");
    printf("================================================================\n\n");

    // Seed RNG for reproducibility
    fp_monte_carlo_seed(42);

    printf("Monte Carlo Methods Demonstrated:\n");
    printf("  1. π Estimation - Geometric probability\n");
    printf("  2. Numerical Integration - Random sampling\n");
    printf("  3. Option Pricing - Financial simulation (Black-Scholes)\n");
    printf("  4. Random Walk - Statistical physics (Brownian motion)\n\n");

    printf("Key Principle: Law of Large Numbers\n");
    printf("  As sample size → ∞, estimate → true value\n");
    printf("  Error decreases as O(1/√n)\n\n");

    // Run all tests
    test_pi_estimation();
    test_numerical_integration();
    test_option_pricing();
    test_random_walk();

    printf("================================================================\n");
    printf("  Monte Carlo Simulation Demo Complete!\n");
    printf("================================================================\n\n");

    printf("Key Takeaways:\n");
    printf("  ✓ Monte Carlo works for problems without analytical solutions\n");
    printf("  ✓ Convergence guaranteed by Law of Large Numbers\n");
    printf("  ✓ Error decreases as 1/√n (need 100x samples for 10x accuracy)\n");
    printf("  ✓ Versatile: geometry, integration, finance, physics, AI\n\n");

    printf("Real-World Applications:\n");
    printf("  - Quantitative Finance: Derivatives pricing, risk management\n");
    printf("  - Machine Learning: Bayesian inference, MCMC sampling\n");
    printf("  - Physics: Particle simulations, quantum mechanics\n");
    printf("  - Engineering: Reliability analysis, uncertainty quantification\n");
    printf("  - Computer Graphics: Ray tracing, global illumination\n\n");

    printf("FP-ASM Primitives Used:\n");
    printf("  - Reductions: counting, summing, averaging\n");
    printf("  - Maps: function application to random samples\n");
    printf("  - Statistical operations: mean, variance, confidence intervals\n\n");

    return 0;
}
