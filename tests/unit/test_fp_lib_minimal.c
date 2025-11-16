/**
 * test_fp_lib_minimal.c
 *
 * Minimal test to verify FP library linkage and functionality
 */

#include <stdio.h>
#include <math.h>
#include "include/fp_core.h"

int main() {
    printf("Testing FP Library Linkage...\n\n");

    // Test 1: Simple dot product
    printf("Test 1: fp_fold_dotp_f32\n");
    float a[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    float b[4] = {1.0f, 1.0f, 1.0f, 1.0f};

    float result = fp_fold_dotp_f32(a, b, 4);
    float expected = 10.0f; // 1*1 + 2*1 + 3*1 + 4*1 = 10

    printf("  Input A: [%.1f, %.1f, %.1f, %.1f]\n", a[0], a[1], a[2], a[3]);
    printf("  Input B: [%.1f, %.1f, %.1f, %.1f]\n", b[0], b[1], b[2], b[3]);
    printf("  Result: %.1f\n", result);
    printf("  Expected: %.1f\n", expected);

    if (fabsf(result - expected) < 0.001f) {
        printf("  PASS\n\n");
    } else {
        printf("  FAIL - Result doesn't match expected!\n\n");
        return 1;
    }

    // Test 2: 3D vector dot product (for lighting)
    printf("Test 2: 3D dot product (normal * light)\n");
    float normal[3] = {0.0f, 0.0f, 1.0f};
    float light[3] = {0.57735f, 0.57735f, 0.57735f}; // Normalized (1,1,1)

    result = fp_fold_dotp_f32(normal, light, 3);
    expected = 0.57735f; // Only Z component contributes

    printf("  Normal: [%.3f, %.3f, %.3f]\n", normal[0], normal[1], normal[2]);
    printf("  Light:  [%.3f, %.3f, %.3f]\n", light[0], light[1], light[2]);
    printf("  Result: %.5f\n", result);
    printf("  Expected: %.5f\n", expected);

    if (fabsf(result - expected) < 0.001f) {
        printf("  PASS\n\n");
    } else {
        printf("  FAIL - Result doesn't match expected!\n\n");
        return 1;
    }

    // Test 3: Matrix row * vector (for transforms)
    printf("Test 3: Matrix row * vector (4D)\n");
    float matrix_row[4] = {1.0f, 0.0f, 0.0f, 0.0f}; // Identity row
    float vertex[4] = {5.0f, 10.0f, 15.0f, 1.0f};

    result = fp_fold_dotp_f32(matrix_row, vertex, 4);
    expected = 5.0f; // Only X component

    printf("  Matrix row: [%.1f, %.1f, %.1f, %.1f]\n",
           matrix_row[0], matrix_row[1], matrix_row[2], matrix_row[3]);
    printf("  Vertex:     [%.1f, %.1f, %.1f, %.1f]\n",
           vertex[0], vertex[1], vertex[2], vertex[3]);
    printf("  Result: %.1f\n", result);
    printf("  Expected: %.1f\n", expected);

    if (fabsf(result - expected) < 0.001f) {
        printf("  PASS\n\n");
    } else {
        printf("  FAIL - Result doesn't match expected!\n\n");
        return 1;
    }

    printf("========================================\n");
    printf("  ALL TESTS PASSED!\n");
    printf("  FP library is working correctly.\n");
    printf("========================================\n");

    return 0;
}
