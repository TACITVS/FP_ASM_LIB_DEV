#include <stdio.h>
#include "../include/fp_core.h"

int main() {
    double x[] = {1, 2, 3, 4, 5};
    double y[] = {2, 4, 6, 8, 10};

    double cov = fp_covariance_f64(x, y, 5);
    double corr = fp_correlation_f64(x, y, 5);

    printf("Covariance: %f\n", cov);
    printf("Correlation: %f\n", corr);

    return 0;
}
