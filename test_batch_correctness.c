/**
 * Quick correctness test for fp_mat4_mul_vec3_batch
 */

#include <stdio.h>
#include <math.h>
#include "include/fp_core.h"

int main(void) {
    printf("Testing fp_mat4_mul_vec3_batch correctness...\n\n");

    // Create a simple test matrix
    Mat4 m = {{
        2.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 3.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 4.0f, 0.0f,
        5.0f, 6.0f, 7.0f, 1.0f
    }};

    // Create test vertices
    const int COUNT = 5;
    Vec3f input[5] = {
        {1.0f, 2.0f, 3.0f, 0.0f},
        {4.0f, 5.0f, 6.0f, 0.0f},
        {7.0f, 8.0f, 9.0f, 0.0f},
        {10.0f, 11.0f, 12.0f, 0.0f},
        {13.0f, 14.0f, 15.0f, 0.0f}
    };

    Vec3f output_single[5];
    Vec3f output_batch[5];

    // Transform using single calls
    for (int i = 0; i < COUNT; i++) {
        fp_mat4_mul_vec3(&output_single[i], &m, &input[i]);
    }

    // Transform using batch call
    fp_mat4_mul_vec3_batch(output_batch, &m, input, COUNT);

    // Compare results
    printf("Comparing single vs batch results:\n");
    int all_match = 1;
    for (int i = 0; i < COUNT; i++) {
        float dx = fabsf(output_single[i].x - output_batch[i].x);
        float dy = fabsf(output_single[i].y - output_batch[i].y);
        float dz = fabsf(output_single[i].z - output_batch[i].z);

        printf("Vertex %d:\n", i);
        printf("  Single: [%.2f, %.2f, %.2f]\n", output_single[i].x, output_single[i].y, output_single[i].z);
        printf("  Batch:  [%.2f, %.2f, %.2f]\n", output_batch[i].x, output_batch[i].y, output_batch[i].z);
        printf("  Diff:   [%.6f, %.6f, %.6f]\n", dx, dy, dz);

        if (dx > 1e-5f || dy > 1e-5f || dz > 1e-5f) {
            printf("  ERROR: Mismatch!\n");
            all_match = 0;
        } else {
            printf("  OK\n");
        }
        printf("\n");
    }

    if (all_match) {
        printf("SUCCESS: All vertices match!\n");
        return 0;
    } else {
        printf("FAILURE: Vertices don't match!\n");
        return 1;
    }
}
