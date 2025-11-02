// Minimal test for zip_add functions only
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "../include/fp_core.h"

int main(void) {
    const size_t n = 100;  // Small test
    
    int64_t* a = malloc(n * sizeof(int64_t));
    int64_t* b = malloc(n * sizeof(int64_t));
    int64_t* out = malloc(n * sizeof(int64_t));
    
    // Initialize
    for (size_t i = 0; i < n; i++) {
        a[i] = i;
        b[i] = i * 2;
    }
    
    printf("Testing fp_zip_add_i64 with n=%zu\n", n);
    printf("Input arrays initialized.\n");
    printf("About to call fp_zip_add_i64...\n");
    fflush(stdout);
    
    fp_zip_add_i64(a, b, out, n);
    
    printf("SUCCESS! fp_zip_add_i64 completed.\n");
    printf("First 5 results: %lld %lld %lld %lld %lld\n", 
           (long long)out[0], (long long)out[1], (long long)out[2], 
           (long long)out[3], (long long)out[4]);
    
    // Verify
    for (size_t i = 0; i < n; i++) {
        if (out[i] != a[i] + b[i]) {
            printf("FAIL at index %zu: expected %lld, got %lld\n", 
                   i, (long long)(a[i] + b[i]), (long long)out[i]);
            return 1;
        }
    }
    
    printf("All values correct!\n");
    
    free(a);
    free(b);
    free(out);
    return 0;
}