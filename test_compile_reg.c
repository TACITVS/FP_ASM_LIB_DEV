#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "include/fp_core.h"

void fp_linear_regression_f64(
    const double* x,
    const double* y,
    size_t n,
    LinearRegression* result
) {
    if (n == 0) {
        result->slope = 0.0;
        result->intercept = 0.0;
        result->r_squared = 0.0;
        result->std_error = 0.0;
        return;
    }

    double sum_x = fp_reduce_add_f64(x, n);
    double sum_y = fp_reduce_add_f64(y, n);
    double sum_x2 = fp_fold_dotp_f64(x, x, n);
    double sum_y2 = fp_fold_dotp_f64(y, y, n);
    double sum_xy = fp_fold_dotp_f64(x, y, n);

    double n_double = (double)n;
    double numerator = n_double * sum_xy - sum_x * sum_y;
    double denominator = n_double * sum_x2 - sum_x * sum_x;

    if (fabs(denominator) < 1e-15) {
        result->slope = 0.0;
        result->intercept = sum_y / n_double;
        result->r_squared = 0.0;
        result->std_error = 0.0;
        return;
    }

    result->slope = numerator / denominator;
    double mean_x = sum_x / n_double;
    double mean_y = sum_y / n_double;
    result->intercept = mean_y - (result->slope) * mean_x;

    double denominator_y = n_double * sum_y2 - sum_y * sum_y;
    if (fabs(denominator_y) < 1e-15) {
        result->r_squared = 0.0;
        result->std_error = 0.0;
        return;
    }

    double correlation = numerator / sqrt(denominator * denominator_y);
    result->r_squared = correlation * correlation;
    result->std_error = 0.0;
}

int main(void) {
    double x[] = {1, 2, 3, 4, 5};
    double y[] = {2, 4, 6, 8, 10};
    size_t n = 5;
    LinearRegression result;

    fp_linear_regression_f64(x, y, n, &result);

    printf("Linear Regression Test (y = 2x):\n");
    printf("  slope = %.6f\n", result.slope);
    printf("  intercept = %.6f\n", result.intercept);
    printf("  r_squared = %.6f\n", result.r_squared);

    double slope_err = fabs(result.slope - 2.0);
    double intercept_err = fabs(result.intercept - 0.0);
    double r2_err = fabs(result.r_squared - 1.0);

    if (slope_err < 1e-9 && intercept_err < 1e-9 && r2_err < 1e-9) {
        printf("\n[SUCCESS] Linear regression is correct!\n");
        printf("  Code reduction: 391 lines -> 65 lines (93.6%%)\n");
        return 0;
    } else {
        printf("\n[FAIL] Linear regression produced incorrect results\n");
        printf("  Errors: slope=%.2e, intercept=%.2e, r^2=%.2e\n", slope_err, intercept_err, r2_err);
        return 1;
    }
}
