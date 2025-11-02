# FP-ASM Library: 30 High-Impact Algorithms Specification

**Purpose**: Demonstrate the practical power of functional programming in systems applications
**Target**: Convince FP purists and practitioners of real-world FP utility in C
**Status**: 📋 SPECIFICATION (Ready for Implementation)

---

## Implementation Strategy

### **TIER A: Most Impressive (20 algorithms)** - Implement First
High-impact, commonly used, showcase performance wins
- Statistical Computing (5)
- Financial Computing (5)
- Machine Learning (5)
- Signal Processing (5)

### **TIER B: Strong Practical Value (10 algorithms)** - Implement Second
Essential utilities with clear use cases
- Numerical Methods (5)
- Data Quality & Validation (5)

**Total**: 30 algorithms demonstrating FP across 6 critical domains

---

## TIER A: Statistical Computing (5 Algorithms)

### 1. **Descriptive Statistics Suite**

**What**: Calculate mean, variance, standard deviation, skewness, kurtosis in one pass

**Why Important**: Foundation of all statistical analysis, used everywhere

**FP Operations Used**:
```haskell
-- Single-pass calculation using folds
stats xs = let
    n = length xs
    sum1 = foldl (+) 0 xs
    sum2 = foldl (\acc x -> acc + x^2) 0 xs
    sum3 = foldl (\acc x -> acc + x^3) 0 xs
    sum4 = foldl (\acc x -> acc + x^4) 0 xs
    mean = sum1 / n
    variance = (sum2 / n) - mean^2
    stddev = sqrt variance
    in (mean, variance, stddev, skewness, kurtosis)
```

**Implementation Plan**:
- Use `fp_reduce_add_f64` for sum
- Create fused fold for sum of powers: `fp_fold_moments_f64(data, n, moments_out)`
- Returns all 4 moments in single pass
- Post-process to calculate statistics

**Complexity**: O(n) time, O(1) space

**Performance Target**: 1.5-2.0x vs naive C (fused operation, single pass)

**C API**:
```c
typedef struct {
    double mean;
    double variance;
    double std_dev;
    double skewness;
    double kurtosis;
} DescriptiveStats;

void fp_descriptive_stats(const double* data, size_t n, DescriptiveStats* stats);
```

---

### 2. **Percentile Calculations**

**What**: Calculate arbitrary percentiles (quartiles, median, deciles, etc.) from sorted data

**Why Important**: Box plots, outlier detection, distribution analysis

**FP Operations Used**:
```haskell
percentile p xs = sorted !! index
  where
    sorted = sort xs
    index = floor (p * fromIntegral (length xs))

quartiles xs = (percentile 0.25 xs, percentile 0.5 xs, percentile 0.75 xs)
```

**Implementation Plan**:
- Use existing `fp_sort_f64`
- Implement linear interpolation for exact percentiles
- Create batch percentile calculation for efficiency

**Complexity**: O(n log n) time (dominated by sort), O(1) space

**Performance Target**: 1.0-1.2x vs C qsort + indexing

**C API**:
```c
// Calculate single percentile (p in [0.0, 1.0])
double fp_percentile_f64(const double* sorted_data, size_t n, double p);

// Calculate multiple percentiles at once
void fp_percentiles_f64(const double* sorted_data, size_t n,
                        const double* p_values, size_t n_percentiles,
                        double* results);

// Calculate quartiles (Q1, median, Q3) + IQR
typedef struct {
    double q1, median, q3, iqr;
} Quartiles;

void fp_quartiles_f64(const double* sorted_data, size_t n, Quartiles* q);
```

---

### 3. **Correlation & Covariance**

**What**: Pearson correlation, covariance between two arrays

**Why Important**: Feature correlation in ML, multivariate statistics

**FP Operations Used**:
```haskell
covariance xs ys = (sumXY / n) - (meanX * meanY)
  where
    n = fromIntegral (length xs)
    meanX = mean xs
    meanY = mean ys
    sumXY = sum (zipWith (*) xs ys)

correlation xs ys = covariance xs ys / (stddev xs * stddev ys)
```

**Implementation Plan**:
- Create fused operation: `fp_fold_covariance_f64(x, y, n)`
- Calculate means, std devs, and covariance in minimal passes
- Use existing `fp_zip_mul_f64` for element-wise multiplication

**Complexity**: O(n) time, O(1) space

**Performance Target**: 2.0-2.5x vs naive C (fused operations)

**C API**:
```c
// Covariance between two arrays
double fp_covariance_f64(const double* x, const double* y, size_t n);

// Pearson correlation coefficient
double fp_correlation_f64(const double* x, const double* y, size_t n);

// Covariance matrix for multiple variables (n samples × m features)
void fp_covariance_matrix_f64(const double* data, size_t n_samples, size_t n_features,
                              double* cov_matrix);
```

---

### 4. **Linear Regression**

**What**: Least squares linear fit (y = mx + b), R², residuals

**Why Important**: Trend analysis, prediction, model evaluation

**FP Operations Used**:
```haskell
linearRegression xs ys = (slope, intercept, r_squared)
  where
    n = fromIntegral (length xs)
    meanX = mean xs
    meanY = mean ys
    sumXY = sum (zipWith (*) xs ys)
    sumX2 = sum (map (^2) xs)
    slope = (sumXY - n*meanX*meanY) / (sumX2 - n*meanX^2)
    intercept = meanY - slope*meanX
    predictions = map (\x -> slope*x + intercept) xs
    ss_res = sum (zipWith (\y pred -> (y-pred)^2) ys predictions)
    ss_tot = sum (map (\y -> (y-meanY)^2) ys)
    r_squared = 1 - (ss_res / ss_tot)
```

**Implementation Plan**:
- Use `fp_reduce_add_f64`, `fp_fold_dotp_f64`
- Create fused operation for regression coefficients
- Calculate R² using residuals

**Complexity**: O(n) time, O(1) space

**Performance Target**: 1.8-2.0x vs naive C (fused operations)

**C API**:
```c
typedef struct {
    double slope;
    double intercept;
    double r_squared;
    double std_error;
} LinearRegressionResult;

void fp_linear_regression_f64(const double* x, const double* y, size_t n,
                              LinearRegressionResult* result);

// Predict y values given x values using fitted model
void fp_linear_predict_f64(const double* x, size_t n,
                           double slope, double intercept,
                           double* predictions);
```

---

### 5. **Outlier Detection**

**What**: IQR method, z-score method, MAD (median absolute deviation)

**Why Important**: Data cleaning, anomaly detection, quality control

**FP Operations Used**:
```haskell
-- IQR Method
outliers_iqr xs = filter is_outlier xs
  where
    sorted = sort xs
    q1 = percentile 0.25 sorted
    q3 = percentile 0.75 sorted
    iqr = q3 - q1
    lower = q1 - 1.5*iqr
    upper = q3 + 1.5*iqr
    is_outlier x = x < lower || x > upper

-- Z-score Method
outliers_zscore threshold xs = filter (\x -> abs (zscore x) > threshold) xs
  where
    m = mean xs
    s = stddev xs
    zscore x = (x - m) / s
```

**Implementation Plan**:
- Use quartiles, mean, std dev from previous functions
- Implement `fp_filter_outliers_iqr` and `fp_filter_outliers_zscore`
- Return both outlier indices and filtered array

**Complexity**: O(n log n) time (IQR requires sort), O(n) space

**Performance Target**: 1.5x vs C (efficient filtering)

**C API**:
```c
// IQR-based outlier detection
size_t fp_detect_outliers_iqr_f64(const double* data, size_t n,
                                  int64_t* outlier_indices,
                                  double iqr_multiplier);

// Z-score based outlier detection
size_t fp_detect_outliers_zscore_f64(const double* data, size_t n,
                                     int64_t* outlier_indices,
                                     double threshold);

// MAD (Median Absolute Deviation) based
size_t fp_detect_outliers_mad_f64(const double* data, size_t n,
                                  int64_t* outlier_indices,
                                  double threshold);
```

---

## TIER A: Financial Computing (5 Algorithms)

### 6. **Technical Indicators Suite**

**What**: SMA, EMA, RSI, MACD, Bollinger Bands

**Why Important**: Trading algorithms, market analysis, strategy backtesting

**FP Operations Used**:
```haskell
-- Simple Moving Average
sma period prices = map average (windows period prices)
  where windows n xs = [take n (drop i xs) | i <- [0..length xs - n]]

-- Exponential Moving Average
ema period prices = scanl1 (\prev price -> alpha*price + (1-alpha)*prev) prices
  where alpha = 2.0 / (fromIntegral period + 1)

-- RSI (Relative Strength Index)
rsi period prices = map calc_rsi (windows period gains_losses)
  where
    changes = zipWith (-) (tail prices) prices
    gains = map (max 0) changes
    losses = map (abs . min 0) changes
    calc_rsi window = 100 - (100 / (1 + avg_gain/avg_loss))
```

**Implementation Plan**:
- Implement sliding window helper using scans
- SMA: use `fp_scan_add_f64` with window subtraction
- EMA: use custom scan with exponential weighting
- RSI: combine gains/losses filtering with moving averages
- Bollinger: SMA ± k*std_dev over window

**Complexity**: O(n) time for each indicator

**Performance Target**: 2.0-3.0x vs Python/pandas (cache-friendly, fused)

**C API**:
```c
// Simple Moving Average
void fp_sma_f64(const double* prices, size_t n, size_t period, double* sma);

// Exponential Moving Average
void fp_ema_f64(const double* prices, size_t n, size_t period, double* ema);

// RSI (Relative Strength Index)
void fp_rsi_f64(const double* prices, size_t n, size_t period, double* rsi);

// MACD (Moving Average Convergence Divergence)
typedef struct {
    double* macd_line;
    double* signal_line;
    double* histogram;
} MACD;

void fp_macd_f64(const double* prices, size_t n, MACD* result);

// Bollinger Bands
typedef struct {
    double* middle;  // SMA
    double* upper;   // SMA + k*stddev
    double* lower;   // SMA - k*stddev
} BollingerBands;

void fp_bollinger_bands_f64(const double* prices, size_t n, size_t period,
                            double k, BollingerBands* bands);
```

---

### 7. **Portfolio Analytics**

**What**: Sharpe ratio, Sortino ratio, portfolio variance, beta

**Why Important**: Risk-adjusted returns, portfolio optimization

**FP Operations Used**:
```haskell
-- Sharpe Ratio
sharpe returns risk_free_rate = (mean excess_returns) / (stddev excess_returns)
  where excess_returns = map (\r -> r - risk_free_rate) returns

-- Sortino Ratio (only penalizes downside volatility)
sortino returns target = (mean excess_returns) / downside_deviation
  where
    excess_returns = map (\r -> r - target) returns
    downside = filter (< 0) excess_returns
    downside_deviation = stddev downside

-- Portfolio Variance
portfolio_variance weights returns covariance_matrix =
  sum [weights!!i * weights!!j * covariance_matrix!!i!!j | i <- ids, j <- ids]
```

**Implementation Plan**:
- Use descriptive stats for mean/std dev
- Filter for downside returns using `fp_filter`
- Matrix operations for portfolio variance
- Combine into single analytics function

**Complexity**: O(n) for ratios, O(m²) for portfolio variance (m assets)

**Performance Target**: 1.5-2.0x vs Python (efficient filtering, SIMD)

**C API**:
```c
// Sharpe Ratio
double fp_sharpe_ratio_f64(const double* returns, size_t n, double risk_free_rate);

// Sortino Ratio
double fp_sortino_ratio_f64(const double* returns, size_t n, double target_return);

// Portfolio variance given weights and covariance matrix
double fp_portfolio_variance_f64(const double* weights, size_t n_assets,
                                 const double* cov_matrix);

// Portfolio beta (vs market)
double fp_portfolio_beta_f64(const double* portfolio_returns,
                             const double* market_returns, size_t n);

// Complete portfolio analytics
typedef struct {
    double sharpe_ratio;
    double sortino_ratio;
    double variance;
    double beta;
    double alpha;
} PortfolioMetrics;

void fp_portfolio_analytics_f64(const double* returns, size_t n,
                                const double* market_returns,
                                double risk_free_rate,
                                PortfolioMetrics* metrics);
```

---

### 8. **Risk Metrics**

**What**: Value at Risk (VaR), Conditional VaR, Maximum Drawdown, Volatility

**Why Important**: Risk management, regulatory compliance, position sizing

**FP Operations Used**:
```haskell
-- Value at Risk (parametric)
var_parametric confidence returns = mean returns - z_score * stddev returns
  where z_score = inverse_normal confidence

-- Historical VaR
var_historical confidence returns = percentile (1 - confidence) (sort returns)

-- Maximum Drawdown
max_drawdown prices = maximum drawdowns
  where
    cumulative = scanl1 (+) prices
    running_max = scanl1 max cumulative
    drawdowns = zipWith (\peak val -> (peak - val) / peak) running_max cumulative

-- Conditional VaR (Expected Shortfall)
cvar confidence returns = mean (filter (< var_threshold) returns)
  where var_threshold = var_historical confidence returns
```

**Implementation Plan**:
- VaR: use percentile on sorted returns
- CVaR: filter + mean of tail
- Drawdown: use scans for cumulative and running max
- Volatility: standard deviation of returns

**Complexity**: O(n log n) for historical VaR (sorting), O(n) for others

**Performance Target**: 1.5-2.0x vs Python/R

**C API**:
```c
// Value at Risk (historical method)
double fp_var_historical_f64(const double* returns, size_t n, double confidence);

// Conditional VaR (Expected Shortfall)
double fp_cvar_f64(const double* returns, size_t n, double confidence);

// Maximum Drawdown
double fp_max_drawdown_f64(const double* prices, size_t n);

// Volatility (annualized)
double fp_volatility_f64(const double* returns, size_t n, size_t periods_per_year);

// Complete risk metrics
typedef struct {
    double var_95;
    double var_99;
    double cvar_95;
    double max_drawdown;
    double volatility;
} RiskMetrics;

void fp_risk_metrics_f64(const double* returns, size_t n, RiskMetrics* metrics);
```

---

### 9. **Returns Analysis**

**What**: Cumulative returns, log returns, annualized returns, rolling windows

**Why Important**: Performance measurement, comparison, benchmarking

**FP Operations Used**:
```haskell
-- Cumulative returns
cumulative_returns prices = scanl1 (*) (map return_factor prices)
  where return_factor p = 1 + p

-- Log returns
log_returns prices = map log (zipWith (/) (tail prices) prices)

-- Annualized return
annualized_return total_return periods = (1 + total_return) ** (365/periods) - 1

-- Rolling returns
rolling_returns period prices =
  map (\window -> (last window / head window) - 1) (windows period prices)
```

**Implementation Plan**:
- Use `fp_scan_mul_f64` for cumulative returns
- Implement log returns with `zipWith` and log map
- Rolling returns using sliding window scan

**Complexity**: O(n) time

**Performance Target**: 2.0x vs Python (vectorized operations)

**C API**:
```c
// Convert prices to simple returns
void fp_simple_returns_f64(const double* prices, size_t n, double* returns);

// Convert prices to log returns
void fp_log_returns_f64(const double* prices, size_t n, double* log_returns);

// Cumulative returns from return series
void fp_cumulative_returns_f64(const double* returns, size_t n, double* cumulative);

// Annualized return
double fp_annualized_return_f64(double total_return, size_t periods,
                                size_t periods_per_year);

// Rolling period returns
void fp_rolling_returns_f64(const double* prices, size_t n, size_t period,
                            double* rolling_returns);
```

---

### 10. **Backtesting Metrics**

**What**: Win rate, profit factor, max consecutive losses, expectancy

**Why Important**: Strategy evaluation, system validation

**FP Operations Used**:
```haskell
-- Win rate
win_rate trades = (fromIntegral wins / fromIntegral total) * 100
  where
    wins = length (filter (> 0) trades)
    total = length trades

-- Profit factor
profit_factor trades = gross_profit / gross_loss
  where
    gross_profit = sum (filter (> 0) trades)
    gross_loss = abs (sum (filter (< 0) trades))

-- Max consecutive losses
max_consecutive_losses trades = maximum (map length (group losses))
  where
    losses = filter (< 0) trades

-- Expectancy
expectancy trades = mean trades
```

**Implementation Plan**:
- Use `fp_filter`, `fp_reduce_add`, `fp_count`
- Max consecutive: use `fp_group` + count lengths
- Combine into single backtesting report

**Complexity**: O(n) time

**Performance Target**: 1.5x vs Python

**C API**:
```c
// Win rate (percentage of profitable trades)
double fp_win_rate_f64(const double* trades, size_t n);

// Profit factor (gross profit / gross loss)
double fp_profit_factor_f64(const double* trades, size_t n);

// Maximum consecutive wins/losses
typedef struct {
    size_t max_consecutive_wins;
    size_t max_consecutive_losses;
} ConsecutiveStats;

void fp_consecutive_stats_f64(const double* trades, size_t n, ConsecutiveStats* stats);

// Average win/loss
typedef struct {
    double avg_win;
    double avg_loss;
    double expectancy;
} WinLossStats;

void fp_win_loss_stats_f64(const double* trades, size_t n, WinLossStats* stats);

// Complete backtesting report
typedef struct {
    double win_rate;
    double profit_factor;
    size_t max_consecutive_wins;
    size_t max_consecutive_losses;
    double avg_win;
    double avg_loss;
    double expectancy;
    double sharpe_ratio;
} BacktestReport;

void fp_backtest_report_f64(const double* trades, size_t n,
                            double risk_free_rate,
                            BacktestReport* report);
```

---

## TIER A: Machine Learning (5 Algorithms)

### 11. **K-Means Clustering**

**What**: Lloyd's algorithm for partitioning data into k clusters

**Why Important**: Customer segmentation, image compression, data exploration

**FP Operations Used**:
```haskell
-- K-means iteration
kmeans k data initial_centers = iterate update initial_centers
  where
    update centers = new_centers
      where
        assignments = map (nearest centers) data
        new_centers = [centroid (points_in_cluster i) | i <- [0..k-1]]
        points_in_cluster i = [p | (p, c) <- zip data assignments, c == i]

    nearest centers point = argmin (map (distance point) centers)
    centroid points = map average (transpose points)
```

**Implementation Plan**:
- Distance calculation using `fp_fold_euclidean_distance`
- Assignment step: find nearest center for each point
- Update step: calculate mean of each cluster
- Iterate until convergence (centers stable)

**Complexity**: O(nkdi) where n=points, k=clusters, d=dimensions, i=iterations

**Performance Target**: 1.5-2.0x vs sklearn (cache-friendly, SIMD distances)

**C API**:
```c
// K-means clustering (returns cluster assignments and final centers)
typedef struct {
    int64_t* assignments;     // n elements
    double* centers;          // k × d elements
    size_t iterations;
    double inertia;          // Sum of squared distances to centers
} KMeansResult;

void fp_kmeans_f64(const double* data, size_t n_samples, size_t n_features,
                  size_t k_clusters, size_t max_iterations, double tolerance,
                  KMeansResult* result);

// Helper: calculate inertia (sum of squared distances)
double fp_kmeans_inertia_f64(const double* data, size_t n_samples, size_t n_features,
                             const int64_t* assignments, const double* centers, size_t k);
```

---

### 12. **K-Nearest Neighbors**

**What**: Classification/regression based on k nearest points

**Why Important**: Simple, effective, interpretable, no training needed

**FP Operations Used**:
```haskell
-- KNN classification
knn_classify k train_data train_labels query = majority_vote nearest_labels
  where
    distances = map (\point -> (distance query point, label point)) train_data
    nearest = take k (sort distances)
    nearest_labels = map snd nearest
    majority_vote = mode

-- KNN regression
knn_regress k train_data train_values query = mean nearest_values
  where
    distances = map (\point -> (distance query point, value point)) train_data
    nearest_values = map snd (take k (sort distances))
```

**Implementation Plan**:
- Distance matrix using `fp_fold_euclidean_distance`
- Partial sort to find k nearest (heap or quickselect)
- Classification: mode of labels
- Regression: mean of values

**Complexity**: O(nd) for prediction (n=training points, d=dimensions)

**Performance Target**: 1.5x vs sklearn (efficient distance computation)

**C API**:
```c
// KNN classification (returns most common label among k nearest)
int64_t fp_knn_classify_f64(const double* train_data, const int64_t* train_labels,
                            size_t n_train, size_t n_features,
                            const double* query, size_t k);

// KNN regression (returns mean of k nearest values)
double fp_knn_regress_f64(const double* train_data, const double* train_values,
                         size_t n_train, size_t n_features,
                         const double* query, size_t k);

// Batch predictions
void fp_knn_classify_batch_f64(const double* train_data, const int64_t* train_labels,
                               size_t n_train, size_t n_features,
                               const double* queries, size_t n_queries,
                               size_t k, int64_t* predictions);
```

---

### 13. **Linear Models with Gradient Descent**

**What**: Linear/logistic regression trained via gradient descent

**Why Important**: Foundation of ML, online learning, large datasets

**FP Operations Used**:
```haskell
-- Gradient descent step
gradient_descent_step learning_rate weights data labels =
  zipWith (\w g -> w - learning_rate * g) weights gradients
  where
    predictions = map (predict weights) data
    errors = zipWith (-) predictions labels
    gradients = map (\feature -> mean (zipWith (*) errors feature)) (transpose data)

-- Iterate until convergence
train max_iters learning_rate data labels initial_weights =
  iterate (gradient_descent_step learning_rate data labels) initial_weights
  !! max_iters
```

**Implementation Plan**:
- Matrix-vector multiplication for predictions
- Error calculation using `zipWith`
- Gradient computation using folds
- Iterate using scans with early stopping

**Complexity**: O(ndi) where n=samples, d=features, i=iterations

**Performance Target**: 1.5-2.0x vs numpy (SIMD matrix ops)

**C API**:
```c
// Linear regression via gradient descent
typedef struct {
    double* weights;          // d+1 elements (includes intercept)
    size_t iterations;
    double final_loss;
} GradientDescentResult;

void fp_linear_regression_gd_f64(const double* X, const double* y,
                                 size_t n_samples, size_t n_features,
                                 double learning_rate, size_t max_iterations,
                                 double tolerance,
                                 GradientDescentResult* result);

// Logistic regression via gradient descent
void fp_logistic_regression_gd_f64(const double* X, const int64_t* y,
                                   size_t n_samples, size_t n_features,
                                   double learning_rate, size_t max_iterations,
                                   double tolerance,
                                   GradientDescentResult* result);

// Predict using trained weights
void fp_linear_predict_batch_f64(const double* X, size_t n_samples, size_t n_features,
                                 const double* weights, double* predictions);
```

---

### 14. **Distance & Similarity Metrics**

**What**: Euclidean, Manhattan, Cosine, Hamming, Jaccard distances

**Why Important**: Foundation for clustering, KNN, similarity search

**FP Operations Used**:
```haskell
-- Euclidean distance
euclidean xs ys = sqrt (sum (zipWith (\x y -> (x-y)^2) xs ys))

-- Manhattan distance
manhattan xs ys = sum (zipWith (\x y -> abs (x-y)) xs ys)

-- Cosine similarity
cosine xs ys = dot_product xs ys / (norm xs * norm ys)
  where
    dot_product as bs = sum (zipWith (*) as bs)
    norm as = sqrt (sum (map (^2) as))

-- Hamming distance (for binary/categorical)
hamming xs ys = length (filter id (zipWith (/=) xs ys))
```

**Implementation Plan**:
- Euclidean: fused map-reduce `(x-y)²`
- Manhattan: use absolute difference + sum
- Cosine: use `fp_fold_dotp_f64` and norms
- Hamming: use XOR + popcount for binary

**Complexity**: O(d) where d=dimensions

**Performance Target**: 2.0-3.0x vs numpy (fused operations, SIMD)

**C API**:
```c
// Euclidean distance
double fp_euclidean_distance_f64(const double* x, const double* y, size_t n);

// Manhattan (L1) distance
double fp_manhattan_distance_f64(const double* x, const double* y, size_t n);

// Cosine similarity (returns value in [-1, 1])
double fp_cosine_similarity_f64(const double* x, const double* y, size_t n);

// Cosine distance (1 - similarity)
double fp_cosine_distance_f64(const double* x, const double* y, size_t n);

// Hamming distance (for integer arrays)
size_t fp_hamming_distance_i64(const int64_t* x, const int64_t* y, size_t n);

// Jaccard similarity for binary vectors
double fp_jaccard_similarity_binary(const int64_t* x, const int64_t* y, size_t n);

// Pairwise distance matrix
void fp_pairwise_distances_f64(const double* X, size_t n_samples, size_t n_features,
                               double* distances, const char* metric);
```

---

### 15. **Feature Scaling**

**What**: Min-max normalization, z-score standardization, robust scaling

**Why Important**: Essential preprocessing for ML, improves convergence

**FP Operations Used**:
```haskell
-- Min-max normalization
minmax_normalize xs = map (\x -> (x - min_val) / (max_val - min_val)) xs
  where
    min_val = minimum xs
    max_val = maximum xs

-- Z-score standardization
standardize xs = map (\x -> (x - mean_val) / stddev_val) xs
  where
    mean_val = mean xs
    stddev_val = stddev xs

-- Robust scaling (using median and IQR)
robust_scale xs = map (\x -> (x - median_val) / iqr) xs
  where
    sorted = sort xs
    median_val = percentile 0.5 sorted
    q1 = percentile 0.25 sorted
    q3 = percentile 0.75 sorted
    iqr = q3 - q1
```

**Implementation Plan**:
- Min-max: use `fp_reduce_max`, `fp_reduce_min`, then map
- Z-score: use descriptive stats, then map
- Robust: use quartiles, then map
- Create inverse transforms for denormalization

**Complexity**: O(n) for min-max/z-score, O(n log n) for robust (sorting)

**Performance Target**: 1.5-2.0x vs sklearn

**C API**:
```c
// Min-max normalization to [0, 1]
void fp_minmax_normalize_f64(const double* data, size_t n, double* normalized);

// Min-max to custom range [a, b]
void fp_minmax_scale_f64(const double* data, size_t n, double a, double b,
                        double* scaled);

// Z-score standardization (mean=0, stddev=1)
void fp_standardize_f64(const double* data, size_t n, double* standardized);

// Robust scaling (median and IQR based)
void fp_robust_scale_f64(const double* data, size_t n, double* scaled);

// Fit scaler and transform
typedef struct {
    double min, max;        // For min-max
    double mean, stddev;    // For z-score
    double median, iqr;     // For robust
} ScalerParams;

void fp_fit_scaler_f64(const double* data, size_t n, const char* method,
                      ScalerParams* params);

void fp_transform_f64(const double* data, size_t n, const ScalerParams* params,
                     const char* method, double* transformed);

void fp_inverse_transform_f64(const double* transformed, size_t n,
                             const ScalerParams* params,
                             const char* method, double* data);
```

---

## TIER A: Signal Processing (5 Algorithms)

### 16. **1D Convolution**

**What**: Discrete convolution for filtering, smoothing, feature detection

**Why Important**: Foundation of signal processing, used in filters, edge detection

**FP Operations Used**:
```haskell
-- 1D convolution
convolve signal kernel = [sum (zipWith (*) window kernel) | window <- windows]
  where
    windows = [take k (drop i padded) | i <- [0..length signal - 1]]
    padded = replicate (k `div` 2) 0 ++ signal ++ replicate (k `div` 2) 0
    k = length kernel
```

**Implementation Plan**:
- Implement sliding window convolution
- Use `fp_fold_dotp_f64` for each window
- Support 'valid', 'same', 'full' padding modes
- Optimize with SIMD for common kernel sizes

**Complexity**: O(nk) where n=signal length, k=kernel size

**Performance Target**: 2.0-3.0x vs numpy (SIMD dot products, cache efficiency)

**C API**:
```c
// 1D convolution with various modes
void fp_convolve_f64(const double* signal, size_t signal_len,
                    const double* kernel, size_t kernel_len,
                    const char* mode,  // "valid", "same", "full"
                    double* output);

// Common filters implemented as optimized kernels
void fp_gaussian_filter_f64(const double* signal, size_t n, double sigma,
                           double* filtered);

void fp_box_filter_f64(const double* signal, size_t n, size_t window_size,
                      double* filtered);

void fp_savitzky_golay_f64(const double* signal, size_t n, size_t window_size,
                          size_t polynomial_order, double* filtered);
```

---

### 17. **Moving Window Operations**

**What**: SMA, weighted MA, exponential smoothing, rolling statistics

**Why Important**: Trend detection, noise reduction, time series forecasting

**FP Operations Used**:
```haskell
-- Simple moving average
sma window_size signal = map average (windows window_size signal)

-- Exponential moving average
ema alpha signal = scanl1 (\prev x -> alpha*x + (1-alpha)*prev) signal

-- Rolling standard deviation
rolling_std window_size signal = map stddev (windows window_size signal)
```

**Implementation Plan**:
- SMA: use cumulative sum trick (subtract tail, add head)
- EMA: use scan with exponential weighting
- Rolling stats: use sliding window with incremental updates
- Weighted MA: use convolution with custom weights

**Complexity**: O(n) for SMA/EMA (amortized), O(nw) for rolling stats

**Performance Target**: 2.0-3.0x vs pandas (incremental computation)

**C API**:
```c
// Simple moving average (optimized with cumsum trick)
void fp_moving_average_f64(const double* signal, size_t n, size_t window,
                          double* ma);

// Exponential moving average
void fp_exponential_moving_average_f64(const double* signal, size_t n,
                                      double alpha, double* ema);

// Weighted moving average (custom weights)
void fp_weighted_moving_average_f64(const double* signal, size_t n,
                                   const double* weights, size_t window,
                                   double* wma);

// Rolling statistics
void fp_rolling_mean_f64(const double* signal, size_t n, size_t window, double* means);
void fp_rolling_std_f64(const double* signal, size_t n, size_t window, double* stds);
void fp_rolling_min_f64(const double* signal, size_t n, size_t window, double* mins);
void fp_rolling_max_f64(const double* signal, size_t n, size_t window, double* maxs);
```

---

### 18. **Cross-Correlation & Autocorrelation**

**What**: Measure similarity between signals at different lags, detect periodicity

**Why Important**: Pattern matching, time series analysis, signal alignment

**FP Operations Used**:
```haskell
-- Cross-correlation at lag k
cross_corr lag signal1 signal2 =
  sum (zipWith (*) (drop lag signal1) signal2) / normalizer

-- Autocorrelation function
autocorr signal = [cross_corr k signal signal | k <- [0..max_lag]]

-- Find dominant period
find_period signal = argmax (tail autocorr_values)
  where autocorr_values = autocorr signal
```

**Implementation Plan**:
- Use `fp_fold_dotp_f64` for each lag
- Autocorrelation: self-correlation at multiple lags
- Normalize by signal energy
- Find peaks in autocorrelation for periodicity

**Complexity**: O(nL) where n=signal length, L=max lag

**Performance Target**: 2.0x vs scipy (SIMD dot products)

**C API**:
```c
// Cross-correlation between two signals
void fp_cross_correlation_f64(const double* signal1, size_t n1,
                             const double* signal2, size_t n2,
                             double* correlation);

// Autocorrelation function (lags 0 to max_lag)
void fp_autocorrelation_f64(const double* signal, size_t n, size_t max_lag,
                           double* autocorr);

// Find dominant period using autocorrelation
size_t fp_find_period_f64(const double* signal, size_t n, size_t min_period,
                         size_t max_period);

// Normalized cross-correlation (Pearson correlation at each lag)
void fp_normalized_cross_correlation_f64(const double* signal1, size_t n1,
                                        const double* signal2, size_t n2,
                                        double* normalized_corr);
```

---

### 19. **Peak Detection**

**What**: Find local maxima/minima with prominence, width constraints

**Why Important**: Feature extraction, event detection, QRS detection in ECG

**FP Operations Used**:
```haskell
-- Simple peak detection (local maxima)
find_peaks signal = [i | (i, val) <- zip [1..] (init (tail signal)),
                         val > signal!!(i-1) && val > signal!!(i+1)]

-- Peak with prominence
prominent_peaks min_prominence signal = filter has_prominence (find_peaks signal)
  where
    has_prominence i = prominence i > min_prominence
    prominence i = min (left_drop i) (right_drop i)
    left_drop i = signal!!i - minimum (take i signal)
    right_drop i = signal!!i - minimum (drop (i+1) signal)
```

**Implementation Plan**:
- First pass: find local maxima (compare with neighbors)
- Calculate prominence for each peak
- Filter by prominence threshold
- Optional: filter by width, distance constraints

**Complexity**: O(n) for simple peaks, O(n²) for prominence (can optimize)

**Performance Target**: 1.5-2.0x vs scipy.signal

**C API**:
```c
// Simple peak detection (local maxima)
size_t fp_find_peaks_f64(const double* signal, size_t n,
                        int64_t* peak_indices);

// Peak detection with prominence threshold
size_t fp_find_peaks_prominence_f64(const double* signal, size_t n,
                                   double min_prominence,
                                   int64_t* peak_indices,
                                   double* prominences);

// Peak detection with multiple constraints
typedef struct {
    double min_prominence;
    double min_height;
    size_t min_distance;
    size_t min_width;
} PeakConstraints;

size_t fp_find_peaks_constrained_f64(const double* signal, size_t n,
                                    const PeakConstraints* constraints,
                                    int64_t* peak_indices);

// Find valleys (local minima)
size_t fp_find_valleys_f64(const double* signal, size_t n,
                          int64_t* valley_indices);
```

---

### 20. **Numerical Differentiation/Integration**

**What**: Approximate derivatives and integrals using finite differences, trapezoidal/Simpson's rule

**Why Important**: Rate of change, area under curve, solving ODEs numerically

**FP Operations Used**:
```haskell
-- First derivative (central difference)
derivative signal dx = [((signal!!(i+1)) - (signal!!(i-1))) / (2*dx)
                       | i <- [1..length signal - 2]]

-- Second derivative
second_derivative signal dx = derivative (derivative signal dx) dx

-- Trapezoidal integration
trapz ys dx = dx * (sum (init (tail ys)) + 0.5*(head ys + last ys))

-- Simpson's rule (requires odd number of points)
simpson ys dx = (dx/3) * (first + 4*odds + 2*evens + last)
  where
    odds = sum [ys!!i | i <- [1,3..length ys - 2]]
    evens = sum [ys!!i | i <- [2,4..length ys - 3]]
```

**Implementation Plan**:
- Forward/backward/central differences using `zipWith`
- Higher-order derivatives recursively
- Trapezoidal: sum with endpoint weighting
- Simpson's: weighted sum with 1-4-2-4...-2-4-1 pattern

**Complexity**: O(n) time

**Performance Target**: 1.5-2.0x vs numpy

**C API**:
```c
// Numerical derivatives (finite differences)
void fp_derivative_f64(const double* signal, size_t n, double dx,
                      const char* method,  // "forward", "backward", "central"
                      double* derivative);

// Second derivative
void fp_second_derivative_f64(const double* signal, size_t n, double dx,
                             double* second_deriv);

// Gradient (multi-dimensional derivative)
void fp_gradient_f64(const double* signal, size_t n, double dx, double* gradient);

// Numerical integration
double fp_trapz_f64(const double* y, size_t n, double dx);

double fp_simpson_f64(const double* y, size_t n, double dx);

// Cumulative integration (running integral)
void fp_cumulative_trapz_f64(const double* y, size_t n, double dx,
                            double* cumulative);
```

---

## TIER B: Numerical Methods (5 Algorithms)

### 21. **Numerical Integration (Advanced)**

**What**: Adaptive quadrature, composite rules, Richardson extrapolation

**Why Important**: Accurate integral approximation, solving definite integrals

**FP Operations Used**:
```haskell
-- Adaptive Simpson's
adaptive_simpson f a b epsilon = integrate a b
  where
    integrate low high
      | abs (s_full - s_half1 - s_half2) < 15*epsilon = s_full
      | otherwise = integrate low mid + integrate mid high
      where
        mid = (low + high) / 2
        s_full = simpson f low high
        s_half1 = simpson f low mid
        s_half2 = simpson f mid high
```

**Implementation Plan**:
- Implement composite trapezoidal/Simpson's
- Adaptive subdivision using recursion
- Richardson extrapolation for error estimation
- Support for function pointers

**Complexity**: O(n) for fixed-step, adaptive varies

**Performance Target**: 1.2-1.5x vs GSL (optimized evaluation)

**C API**:
```c
// Function pointer type for integration
typedef double (*integrand_func)(double x, void* params);

// Composite trapezoidal rule
double fp_integrate_trapz(integrand_func f, void* params,
                         double a, double b, size_t n_intervals);

// Composite Simpson's rule
double fp_integrate_simpson(integrand_func f, void* params,
                           double a, double b, size_t n_intervals);

// Adaptive integration with error control
typedef struct {
    double result;
    double error_estimate;
    size_t function_evaluations;
} IntegrationResult;

void fp_integrate_adaptive(integrand_func f, void* params,
                          double a, double b, double tolerance,
                          IntegrationResult* result);
```

---

### 22. **Root Finding**

**What**: Newton-Raphson, bisection, secant method for finding zeros

**Why Important**: Solve equations, optimization, inverse functions

**FP Operations Used**:
```haskell
-- Newton-Raphson
newton f f' x0 tolerance = until converged iterate x0
  where
    iterate x = x - (f x) / (f' x)
    converged x = abs (f x) < tolerance

-- Bisection
bisection f a b tolerance = until converged subdivide (a, b)
  where
    subdivide (low, high)
      | f mid * f low < 0 = (low, mid)
      | otherwise = (mid, high)
      where mid = (low + high) / 2
    converged (low, high) = high - low < tolerance
```

**Implementation Plan**:
- Newton: iterative update using derivative
- Bisection: interval halving with sign check
- Secant: finite difference approximation
- Convergence checking with max iterations

**Complexity**: O(log(1/ε)) for bisection, faster for Newton

**Performance Target**: 1.1-1.3x vs GSL (fewer function calls)

**C API**:
```c
// Function and derivative types
typedef double (*scalar_func)(double x, void* params);
typedef double (*scalar_derivative)(double x, void* params);

// Newton-Raphson method
typedef struct {
    double root;
    size_t iterations;
    double final_error;
} RootResult;

void fp_newton_raphson(scalar_func f, scalar_derivative df, void* params,
                      double x0, double tolerance, size_t max_iterations,
                      RootResult* result);

// Bisection method
void fp_bisection(scalar_func f, void* params,
                 double a, double b, double tolerance,
                 RootResult* result);

// Secant method (no derivative needed)
void fp_secant(scalar_func f, void* params,
              double x0, double x1, double tolerance, size_t max_iterations,
              RootResult* result);

// Brent's method (hybrid, robust)
void fp_brent(scalar_func f, void* params,
             double a, double b, double tolerance,
             RootResult* result);
```

---

### 23. **Polynomial Operations**

**What**: Horner's method for evaluation, root finding, interpolation

**Why Important**: Efficient polynomial evaluation, curve fitting

**FP Operations Used**:
```haskell
-- Horner's method for evaluation
horner coeffs x = foldr (\c acc -> c + x*acc) 0 coeffs

-- Polynomial interpolation (Lagrange)
lagrange points x = sum [y * basis i | (i, (_, y)) <- zip [0..] points]
  where
    basis i = product [(x - xj) / (xi - xj) | (j, (xj, _)) <- zip [0..] points, i /= j]
    (xi, _) = points !! i
```

**Implementation Plan**:
- Horner: use `foldr` for efficient evaluation
- Differentiation: shift and multiply coefficients
- Interpolation: Lagrange or Newton divided differences
- Root finding: companion matrix eigenvalues

**Complexity**: O(n) for evaluation, O(n²) for interpolation

**Performance Target**: 1.5x vs numpy.polyval

**C API**:
```c
// Evaluate polynomial using Horner's method
double fp_polynomial_eval_f64(const double* coefficients, size_t degree, double x);

// Evaluate at multiple points
void fp_polynomial_eval_batch_f64(const double* coefficients, size_t degree,
                                 const double* x_values, size_t n,
                                 double* results);

// Polynomial operations
void fp_polynomial_add_f64(const double* p1, size_t deg1,
                          const double* p2, size_t deg2,
                          double* result);

void fp_polynomial_multiply_f64(const double* p1, size_t deg1,
                               const double* p2, size_t deg2,
                               double* result);

void fp_polynomial_derivative_f64(const double* coefficients, size_t degree,
                                 double* derivative);

// Polynomial interpolation (Lagrange)
void fp_polynomial_interpolate_f64(const double* x_points, const double* y_points,
                                  size_t n_points, double* coefficients);
```

---

### 24. **Matrix Operations**

**What**: Matrix-vector multiply, transpose, rank-1 updates (BLAS Level 2)

**Why Important**: Foundation of linear algebra, ML, scientific computing

**FP Operations Used**:
```haskell
-- Matrix-vector multiply
matvec matrix vector = [dot row vector | row <- matrix]
  where dot xs ys = sum (zipWith (*) xs ys)

-- Transpose
transpose matrix = [[matrix!!i!!j | i <- [0..rows-1]] | j <- [0..cols-1]]

-- Rank-1 update: A := A + alpha * x * y^T
rank1_update alpha x y matrix =
  [[matrix!!i!!j + alpha * x!!i * y!!j | j <- [0..cols-1]] | i <- [0..rows-1]]
```

**Implementation Plan**:
- Matrix-vector: use `fp_fold_dotp_f64` for each row
- Transpose: efficient cache-blocked algorithm
- Rank-1: outer product update
- Use SIMD for row/column operations

**Complexity**: O(mn) for m×n matrix operations

**Performance Target**: 1.5-2.0x vs naive C (cache optimization, SIMD)

**C API**:
```c
// Matrix-vector multiply: y = A * x
void fp_matvec_f64(const double* A, size_t rows, size_t cols,
                  const double* x, double* y);

// Matrix-vector multiply with transpose: y = A^T * x
void fp_matvec_transpose_f64(const double* A, size_t rows, size_t cols,
                            const double* x, double* y);

// Matrix transpose
void fp_matrix_transpose_f64(const double* A, size_t rows, size_t cols,
                            double* A_T);

// Rank-1 update: A := A + alpha * x * y^T
void fp_rank1_update_f64(double* A, size_t rows, size_t cols,
                        double alpha, const double* x, const double* y);

// Matrix-matrix multiply: C = A * B (simple version)
void fp_matmul_f64(const double* A, size_t m, size_t k,
                  const double* B, size_t n,
                  double* C);
```

---

### 25. **Convergence Analysis**

**What**: Norms (L1, L2, L∞), residual checking, relative error

**Why Important**: Iterative solver convergence, error estimation

**FP Operations Used**:
```haskell
-- L1 norm (Manhattan)
l1_norm xs = sum (map abs xs)

-- L2 norm (Euclidean)
l2_norm xs = sqrt (sum (map (^2) xs))

-- L-infinity norm (maximum absolute value)
linf_norm xs = maximum (map abs xs)

-- Relative error
rel_error approx exact = l2_norm (zipWith (-) approx exact) / l2_norm exact

-- Check convergence
converged tolerance xs ys = rel_error xs ys < tolerance
```

**Implementation Plan**:
- L1: use `fp_map_abs` + `fp_reduce_add`
- L2: use `fp_fold_sumsq` + sqrt
- L∞: use `fp_map_abs` + `fp_reduce_max`
- Relative error: combine norms

**Complexity**: O(n) time

**Performance Target**: 1.5-2.0x vs BLAS dnrm2

**C API**:
```c
// Vector norms
double fp_norm_l1_f64(const double* x, size_t n);
double fp_norm_l2_f64(const double* x, size_t n);
double fp_norm_linf_f64(const double* x, size_t n);
double fp_norm_lp_f64(const double* x, size_t n, double p);

// Distance between vectors
double fp_distance_l1_f64(const double* x, const double* y, size_t n);
double fp_distance_l2_f64(const double* x, const double* y, size_t n);
double fp_distance_linf_f64(const double* x, const double* y, size_t n);

// Relative error
double fp_relative_error_f64(const double* approx, const double* exact, size_t n);

// Convergence check
bool fp_has_converged_f64(const double* x_new, const double* x_old,
                         size_t n, double tolerance, const char* norm);
```

---

## TIER B: Data Quality & Validation (5 Algorithms)

### 26. **Hash & Checksum**

**What**: CRC32, simple hash functions, rolling hash (Rabin-Karp)

**Why Important**: Data integrity, fast equality checks, pattern matching

**FP Operations Used**:
```haskell
-- Simple polynomial hash
poly_hash xs = foldl (\h x -> (h * base + x) `mod` prime) 0 xs

-- Rolling hash (Rabin-Karp)
rolling_hash xs = scanl update initial_hash xs
  where
    update h x = ((h - h_remove) * base + x_add) `mod` prime
```

**Implementation Plan**:
- CRC32: use lookup table + XOR
- Polynomial hash: use Horner's method
- Rolling hash: efficient window updates
- Support for both data arrays and strings

**Complexity**: O(n) time

**Performance Target**: 1.5-2.0x vs reference implementations

**C API**:
```c
// CRC32 checksum
uint32_t fp_crc32(const uint8_t* data, size_t n);

// Simple polynomial hash (64-bit)
uint64_t fp_hash_polynomial_i64(const int64_t* data, size_t n, uint64_t base);

// Rolling hash for substring matching
typedef struct {
    uint64_t* hashes;  // Hash at each position
    size_t count;
} RollingHashResult;

void fp_rolling_hash_i64(const int64_t* data, size_t n, size_t window_size,
                        uint64_t base, RollingHashResult* result);

// FNV-1a hash (fast, good distribution)
uint64_t fp_hash_fnv1a(const uint8_t* data, size_t n);
```

---

### 27. **Data Profiling**

**What**: Missing value detection, cardinality, distribution analysis

**Why Important**: Data quality assessment, ETL validation

**FP Operations Used**:
```haskell
-- Missing value statistics
missing_stats xs special_value = (count, percentage)
  where
    count = length (filter (== special_value) xs)
    percentage = 100.0 * fromIntegral count / fromIntegral (length xs)

-- Cardinality (unique count)
cardinality xs = length (nub xs)

-- Value frequencies
value_freqs xs = map (\g -> (head g, length g)) (group (sort xs))
```

**Implementation Plan**:
- Missing count: use `fp_count_i64`
- Cardinality: use `fp_unique_i64` after sorting
- Frequencies: use `fp_group_i64`
- Distribution: histogram binning

**Complexity**: O(n log n) for unique/frequencies (sorting)

**Performance Target**: 1.5x vs pandas

**C API**:
```c
// Data profiling statistics
typedef struct {
    size_t total_count;
    size_t missing_count;
    size_t unique_count;
    double missing_percentage;
    int64_t mode;
    size_t mode_count;
} DataProfile;

void fp_profile_data_i64(const int64_t* data, size_t n,
                        int64_t missing_value,
                        DataProfile* profile);

// Value frequency distribution
typedef struct {
    int64_t* values;
    size_t* counts;
    size_t n_unique;
} FrequencyTable;

void fp_frequency_table_i64(const int64_t* data, size_t n,
                           FrequencyTable* freq_table);

// Histogram binning
void fp_histogram_f64(const double* data, size_t n, size_t n_bins,
                     double* bin_edges, size_t* bin_counts);
```

---

### 28. **Deduplication**

**What**: Near-duplicate detection, fuzzy matching, similarity threshold

**Why Important**: Data cleaning, record linkage, duplicate removal

**FP Operations Used**:
```haskell
-- Find near-duplicates using similarity threshold
near_duplicates threshold xs =
  [(i, j) | i <- [0..n-1], j <- [i+1..n-1], similarity (xs!!i) (xs!!j) > threshold]
  where n = length xs

-- Deduplicate within tolerance
deduplicate_fuzzy tolerance xs = nub_by similar (sort xs)
  where similar a b = abs (a - b) <= tolerance
```

**Implementation Plan**:
- Exact deduplication: use `fp_unique_i64`
- Fuzzy deduplication: sort + merge with tolerance
- Near-duplicate pairs: pairwise similarity with threshold
- Efficient using sorting + sliding window

**Complexity**: O(n log n) for fuzzy, O(n²) for all pairs

**Performance Target**: 1.5x vs pandas.drop_duplicates

**C API**:
```c
// Exact deduplication (returns unique sorted values)
size_t fp_deduplicate_exact_i64(const int64_t* data, size_t n,
                               int64_t* unique_values);

// Fuzzy deduplication (within tolerance)
size_t fp_deduplicate_fuzzy_f64(const double* data, size_t n,
                               double tolerance,
                               double* unique_values);

// Find near-duplicate pairs
typedef struct {
    size_t* index1;
    size_t* index2;
    double* similarity;
    size_t n_pairs;
} DuplicatePairs;

void fp_find_near_duplicates_f64(const double* data, size_t n,
                                double similarity_threshold,
                                DuplicatePairs* pairs);
```

---

### 29. **Validation Rules**

**What**: Range checks, format validation, consistency checking

**Why Important**: Input validation, data integrity, error prevention

**FP Operations Used**:
```haskell
-- Range validation
in_range lower upper xs = and (map (\x -> x >= lower && x <= upper) xs)

-- Monotonic increasing check
is_monotonic_increasing xs = and (zipWith (<=) xs (tail xs))

-- Consistency check (e.g., sum constraint)
sum_equals target tolerance xs = abs (sum xs - target) <= tolerance
```

**Implementation Plan**:
- Range: use `fp_reduce_and_bool` with comparisons
- Monotonic: `zipWith` comparison + and
- Custom rules: combine primitive predicates
- Return validation report with failed indices

**Complexity**: O(n) time

**Performance Target**: 1.5-2.0x vs pandas validation

**C API**:
```c
// Range validation
bool fp_validate_range_f64(const double* data, size_t n,
                          double min, double max,
                          size_t* n_violations,
                          size_t* violation_indices);

// Monotonic sequence check
bool fp_validate_monotonic_f64(const double* data, size_t n,
                              bool increasing,  // true for increasing
                              size_t* first_violation_index);

// Sum constraint validation
bool fp_validate_sum_f64(const double* data, size_t n,
                        double expected_sum, double tolerance);

// Custom predicate validation
typedef bool (*validation_predicate)(double value, void* params);

size_t fp_validate_custom_f64(const double* data, size_t n,
                             validation_predicate pred, void* params,
                             size_t* violation_indices);

// Complete validation report
typedef struct {
    bool range_valid;
    bool monotonic_valid;
    bool sum_valid;
    size_t n_violations;
    size_t* violation_indices;
} ValidationReport;

void fp_validate_data_f64(const double* data, size_t n,
                         const void* constraints,
                         ValidationReport* report);
```

---

### 30. **Anomaly Detection**

**What**: Statistical outliers, isolation concepts, density-based detection

**Why Important**: Fraud detection, quality control, system monitoring

**FP Operations Used**:
```haskell
-- Z-score based anomaly
anomalies_zscore threshold xs = filter is_anomaly (zip [0..] xs)
  where
    m = mean xs
    s = stddev xs
    is_anomaly (i, x) = abs ((x - m) / s) > threshold

-- Local outlier factor (simplified)
lof k xs = map (local_density k xs) xs
  where
    local_density k point neighbors =
      average (map (density point) (knn k point neighbors))
```

**Implementation Plan**:
- Z-score: use descriptive stats + threshold
- IQR method: use quartiles (already in stats)
- Distance-based: k-nearest neighbors
- Return anomaly indices and scores

**Complexity**: O(n) for statistical, O(n²) for distance-based

**Performance Target**: 1.5x vs scikit-learn (for statistical methods)

**C API**:
```c
// Z-score based anomaly detection
size_t fp_detect_anomalies_zscore_f64(const double* data, size_t n,
                                     double threshold,
                                     int64_t* anomaly_indices,
                                     double* anomaly_scores);

// IQR-based anomaly detection (already implemented in stats)
size_t fp_detect_anomalies_iqr_f64(const double* data, size_t n,
                                  double iqr_multiplier,
                                  int64_t* anomaly_indices);

// Distance-based anomaly detection
size_t fp_detect_anomalies_distance_f64(const double* data, size_t n_samples,
                                       size_t n_features, size_t k_neighbors,
                                       double contamination,
                                       int64_t* anomaly_indices,
                                       double* anomaly_scores);

// Ensemble anomaly detection (combines multiple methods)
typedef struct {
    double zscore_weight;
    double iqr_weight;
    double distance_weight;
} AnomalyEnsembleParams;

size_t fp_detect_anomalies_ensemble_f64(const double* data, size_t n,
                                       const AnomalyEnsembleParams* params,
                                       double threshold,
                                       int64_t* anomaly_indices,
                                       double* anomaly_scores);
```

---

## Summary: 30 High-Impact Algorithms

### TIER A (20 algorithms) - Most Impressive:

**Statistical Computing (5)**:
1. Descriptive Statistics Suite
2. Percentile Calculations
3. Correlation & Covariance
4. Linear Regression
5. Outlier Detection

**Financial Computing (5)**:
6. Technical Indicators Suite
7. Portfolio Analytics
8. Risk Metrics
9. Returns Analysis
10. Backtesting Metrics

**Machine Learning (5)**:
11. K-Means Clustering
12. K-Nearest Neighbors
13. Linear Models with Gradient Descent
14. Distance & Similarity Metrics
15. Feature Scaling

**Signal Processing (5)**:
16. 1D Convolution
17. Moving Window Operations
18. Cross-Correlation & Autocorrelation
19. Peak Detection
20. Numerical Differentiation/Integration

### TIER B (10 algorithms) - Strong Practical Value:

**Numerical Methods (5)**:
21. Numerical Integration (Advanced)
22. Root Finding
23. Polynomial Operations
24. Matrix Operations
25. Convergence Analysis

**Data Quality & Validation (5)**:
26. Hash & Checksum
27. Data Profiling
28. Deduplication
29. Validation Rules
30. Anomaly Detection

---

## Implementation Roadmap

### Phase 1: Foundation (Weeks 1-2)
Implement supporting utilities needed by multiple algorithms:
- Enhanced matrix operations
- Window/sliding operations
- Additional distance metrics

### Phase 2: TIER A Implementation (Weeks 3-8)
Implement in order of impact:
1. Statistical Computing (Week 3)
2. Financial Computing (Week 4-5)
3. Machine Learning (Week 6)
4. Signal Processing (Week 7-8)

### Phase 3: TIER B Implementation (Weeks 9-11)
1. Numerical Methods (Week 9-10)
2. Data Quality (Week 11)

### Phase 4: Testing & Documentation (Week 12)
- Comprehensive test suites for all 30
- Performance benchmarks vs numpy/scipy/sklearn/pandas
- Complete documentation with examples
- Real-world application demos

---

## Expected Performance Summary

| Domain | Avg Speedup | Why |
|--------|-------------|-----|
| Statistical Computing | 1.5-2.0x | Fused operations, single-pass |
| Financial Computing | 2.0-3.0x | Cache-friendly sliding windows |
| Machine Learning | 1.5-2.0x | SIMD distances, optimized kernels |
| Signal Processing | 2.0-3.0x | SIMD convolution, incremental updates |
| Numerical Methods | 1.2-1.5x | Reduced function calls, efficient iteration |
| Data Quality | 1.5-2.0x | Efficient filtering, SIMD hashing |

**Overall**: Competitive to superior performance across all domains while demonstrating pure FP style!

---

## Next Steps

This specification is now complete and ready for implementation. Each algorithm has:
- ✅ Clear purpose and importance
- ✅ Haskell-style FP expression
- ✅ Implementation strategy
- ✅ Complexity analysis
- ✅ Performance targets
- ✅ Complete C API design

**Ready to proceed with implementation whenever you have credits available!**

---

*Document Version*: 1.0
*Status*: ✅ SPECIFICATION COMPLETE
*Total Algorithms*: 30 across 6 domains
*Estimated Lines of Code*: ~8,000 lines of C/ASM + ~12,000 lines of tests/docs
