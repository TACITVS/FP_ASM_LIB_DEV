#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "include/fp_core.h"

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
        return 0;
    } else {
        printf("\n[FAIL] Linear regression produced incorrect results\n");
        printf("  Errors: slope=%.2e, intercept=%.2e, r^2=%.2e\n", slope_err, intercept_err, r2_err);
        return 1;
    }
}
