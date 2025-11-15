#include <stdio.h>                          
#include <stdlib.h>                        
#include <math.h>                          
#include "../include/fp_core.h"             
                                             
int main(void) {                             
    size_t n = 100;                          
    size_t window = 5;                       
    double* data = malloc(n * sizeof(double)); 
    double* output = malloc((n-window+1) * sizeof(double)); 
    for (size_t i = 0; i < n; i++) data[i] = (double)(i+1); 
    fp_map_sma_f64(data, n, window, output);    
    printf("SMA Test Results (n=%zu, window=%zu):\n", n, window); 
    for (size_t i = 0; i < 10; i++) {       
        printf("  output[%zu] = %.6f\n", i, output[i]); 
    }                                         
    printf("Expected: output[0] = 3.000000 (mean of 1,2,3,4,5)\n"); 
    printf("          output[1] = 4.000000 (mean of 2,3,4,5,6)\n"); 
    double diff = fabs(output[0] - 3.0);     
    if (diff < 1e-9) {                       
        printf("\n[SUCCESS] Composition-based SMA is correct!\n"); 
        return 0;                             
    } else {                                  
        printf("\n[FAIL] SMA produced incorrect result\n"); 
        return 1;                             
    }                                         
}                                             
