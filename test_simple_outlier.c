#include <stdio.h>
#include <stdint.h>
#include "include/fp_core.h"

int main() {
    double data[] = {1.0, 2.0, 3.0, 100.0};
    uint8_t outliers[4];
    
    size_t count = fp_detect_outliers_zscore_f64(data, 4, 3.0, outliers);
    printf("Detected %zu outliers\n", count);
    
    return 0;
}
