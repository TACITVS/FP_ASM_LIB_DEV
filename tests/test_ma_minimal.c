#include <stdio.h>
#include "../include/fp_core.h"

int main(void) {
    double data[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    double output[3];

    fp_sma_f64(data, 5, 3, output);

    printf("SMA result: %.1f, %.1f, %.1f\n", output[0], output[1], output[2]);
    return 0;
}
