/**
 * Minimal test to debug fp_fold_dotp_f32 crash
 */

#include <stdio.h>
#include <stdlib.h>
#include "fp_core.h"

int main(void) {
    printf("=== DEBUG TEST ===\n\n");

    // Test 1: Single call (like Test 1 - this works)
    printf("[Test 1] Single call to fp_fold_dotp_f32\n");
    float normal1[3] = { 0.0f, 1.0f, 0.0f };
    float light1[3] = { 0.0f, 1.0f, 0.0f };
    float result1 = fp_fold_dotp_f32(normal1, light1, 3);
    printf("  Result: %.3f (expected 1.000)\n", result1);
    printf("  %s\n\n", (result1 > 0.99f && result1 < 1.01f) ? "PASS" : "FAIL");

    // Test 2: Call in a loop with array (like Test 2 - this crashes)
    printf("[Test 2] Loop with array access\n");
    const int count = 10;  // Start with just 10
    float* normals = malloc(count * 3 * sizeof(float));

    // Initialize
    printf("  Initializing %d normals...\n", count);
    for (int i = 0; i < count; ++i) {
        normals[i * 3 + 0] = 0.0f;
        normals[i * 3 + 1] = 1.0f;
        normals[i * 3 + 2] = 0.0f;
    }

    float light_dir[3] = { 0.0f, 1.0f, 0.0f };

    // Call in loop
    printf("  Calling fp_fold_dotp_f32 in loop...\n");
    for (int i = 0; i < count; ++i) {
        const float* normal = &normals[i * 3];
        printf("    Vertex %d: calling fp_fold_dotp_f32...", i);
        fflush(stdout);

        float dot = fp_fold_dotp_f32(normal, light_dir, 3);

        printf(" result=%.3f\n", dot);
    }

    printf("  PASS!\n\n");

    free(normals);

    printf("=== ALL TESTS PASSED ===\n");
    return 0;
}
