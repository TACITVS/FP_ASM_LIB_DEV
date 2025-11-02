#include <stdio.h>
#include <math.h>
#include "../include/fp_core.h"

int main() {
    printf("Testing Algorithm #1: Descriptive Statistics\n\n");

    double data[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    size_t n = 5;

    DescriptiveStats stats;
    fp_descriptive_stats_f64(data, n, &stats);

    printf("Data: [1, 2, 3, 4, 5]\n\n");
    printf("Results:\n");
    printf("  Mean:     %.6f  (expected: 3.0)\n", stats.mean);
    printf("  Variance: %.6f  (expected: 2.0)\n", stats.variance);
    printf("  Std Dev:  %.6f  (expected: ~1.414)\n", stats.std_dev);
    printf("  Skewness: %.6f  (expected: 0.0, symmetric)\n", stats.skewness);
    printf("  Kurtosis: %.6f\n", stats.kurtosis);

    if (fabs(stats.mean - 3.0) < 0.001 && fabs(stats.variance - 2.0) < 0.001) {
        printf("\nRESULT: PASS!\n");
        return 0;
    } else {
        printf("\nRESULT: FAIL!\n");
        return 1;
    }
}
