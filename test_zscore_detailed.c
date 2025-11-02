#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include "include/fp_core.h"

int main() {
    double data[] = {1.0, 2.0, 3.0, 4.0, 5.0, 100.0};
    size_t n = 6;
    uint8_t outliers[6];

    printf("=== Manual Z-score calculation ===\n");

    // Get stats
    DescriptiveStats stats;
    fp_descriptive_stats_f64(data, n, &stats);

    printf("Mean: %f\n", stats.mean);
    printf("Std Dev: %f\n", stats.std_dev);
    printf("Threshold: 2.0\n\n");

    // Calculate z-scores manually and check condition
    for (size_t i = 0; i < n; i++) {
        double z_score = (data[i] - stats.mean) / stats.std_dev;
        double abs_z = fabs(z_score);
        int is_outlier_manual = (abs_z > 2.0) ? 1 : 0;
        printf("data[%zu]=%.0f: z=%.4f, |z|=%.4f, |z|>2.0? %s\n",
               i, data[i], z_score, abs_z, is_outlier_manual ? "YES" : "NO");
    }

    printf("\n=== Calling fp_detect_outliers_zscore_f64 ===\n");
    size_t count = fp_detect_outliers_zscore_f64(data, n, 2.0, outliers);

    printf("Detected %zu outliers\n", count);
    printf("is_outlier array: ");
    for (size_t i = 0; i < n; i++) {
        printf("%d ", outliers[i]);
    }
    printf("\n");

    return 0;
}
