#include <stdio.h>
#include <stdint.h>
#include "include/fp_core.h"

int main() {
    double data[] = {1.0, 2.0, 3.0, 4.0, 5.0, 100.0};
    size_t n = 6;
    
    // Check what stats we get
    DescriptiveStats stats;
    fp_descriptive_stats_f64(data, n, &stats);
    
    printf("Data: 1, 2, 3, 4, 5, 100\n");
    printf("Mean: %f\n", stats.mean);
    printf("Std Dev: %f\n", stats.std_dev);
    printf("Variance: %f\n", stats.variance);
    
    // Calculate z-scores manually
    for (size_t i = 0; i < n; i++) {
        double z = (data[i] - stats.mean) / stats.std_dev;
        printf("Data[%zu] = %.0f, Z-score = %.4f\n", i, data[i], z);
    }
    
    return 0;
}
