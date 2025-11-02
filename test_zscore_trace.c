#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include "include/fp_core.h"

// Create a traced version that mimics the actual implementation
size_t fp_detect_outliers_zscore_f64_TRACED(const double* data, size_t n,
                                             double threshold, uint8_t* is_outlier) {
    printf("TRACE: Entered function\n");
    printf("TRACE: n = %zu, threshold = %f\n", n, threshold);

    // Edge case: Need at least 2 points to compute standard deviation
    if (n < 2 || !is_outlier) {
        printf("TRACE: Edge case triggered! n < 2? %s, !is_outlier? %s\n",
               (n < 2) ? "YES" : "NO",
               (!is_outlier) ? "YES" : "NO");
        for (size_t i = 0; i < n; i++) {
            is_outlier[i] = 0;
        }
        return 0;
    }
    printf("TRACE: Passed edge case check 1\n");

    // COMPOSITION: Use existing descriptive stats function!
    DescriptiveStats stats;
    fp_descriptive_stats_f64(data, n, &stats);

    printf("TRACE: Got stats: mean=%f, std_dev=%f\n", stats.mean, stats.std_dev);

    // Edge case: All values identical (stddev = 0)
    if (stats.std_dev == 0.0 || !isfinite(stats.std_dev)) {
        printf("TRACE: Edge case 2 triggered! stddev==0? %s, !isfinite? %s\n",
               (stats.std_dev == 0.0) ? "YES" : "NO",
               (!isfinite(stats.std_dev)) ? "YES" : "NO");
        for (size_t i = 0; i < n; i++) {
            is_outlier[i] = 0;
        }
        return 0;
    }
    printf("TRACE: Passed edge case check 2\n");

    // Calculate Z-scores and mark outliers
    size_t outlier_count = 0;
    printf("TRACE: Entering main loop\n");
    for (size_t i = 0; i < n; i++) {
        double z_score = (data[i] - stats.mean) / stats.std_dev;
        double abs_z = fabs(z_score);
        printf("TRACE: i=%zu, data[%zu]=%f, z=%f, |z|=%f, |z|>threshold? %s\n",
               i, i, data[i], z_score, abs_z, (abs_z > threshold) ? "YES" : "NO");

        if (fabs(z_score) > threshold) {
            is_outlier[i] = 1;
            outlier_count++;
            printf("TRACE: Marked as outlier!\n");
        } else {
            is_outlier[i] = 0;
        }
    }

    printf("TRACE: Returning outlier_count = %zu\n", outlier_count);
    return outlier_count;
}

int main() {
    double data[] = {1.0, 2.0, 3.0, 4.0, 5.0, 100.0};
    size_t n = 6;
    uint8_t outliers[6];

    printf("=== Testing TRACED version ===\n");
    size_t count_traced = fp_detect_outliers_zscore_f64_TRACED(data, n, 2.0, outliers);
    printf("\nTRACED result: %zu outliers\n", count_traced);

    printf("\n=== Testing ACTUAL version ===\n");
    uint8_t outliers2[6];
    size_t count_actual = fp_detect_outliers_zscore_f64(data, n, 2.0, outliers2);
    printf("\nACTUAL result: %zu outliers\n", count_actual);

    return 0;
}
