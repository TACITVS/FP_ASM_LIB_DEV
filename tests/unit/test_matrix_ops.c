/**
 * test_matrix_ops.c - Comprehensive Matrix Operations Test
 *
 * Tests all matrix creation functions for correctness
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "fp_core.h"

#define PI 3.14159265358979323846f
#define TOLERANCE 1e-5f

int passed = 0;
int failed = 0;

void print_mat4(const char* name, const Mat4* m) {
    printf("%s:\n", name);
    for (int row = 0; row < 4; row++) {
        printf("  [");
        for (int col = 0; col < 4; col++) {
            printf(" %7.3f", m->m[row + col*4]);
        }
        printf(" ]\n");
    }
}

int check_mat4(const char* test_name, const Mat4* result, const float* expected) {
    for (int i = 0; i < 16; i++) {
        float diff = fabsf(result->m[i] - expected[i]);
        if (diff > TOLERANCE) {
            printf("FAIL: %s at index %d: got %.6f, expected %.6f (diff %.6f)\n",
                   test_name, i, result->m[i], expected[i], diff);
            failed++;
            return 0;
        }
    }
    printf("PASS: %s\n", test_name);
    passed++;
    return 1;
}

void test_translation() {
    printf("\n=== Testing Translation ===\n");
    Mat4 m;
    fp_mat4_translation(&m, 5.0f, 10.0f, 15.0f);

    float expected[] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        5, 10, 15, 1
    };

    check_mat4("Translation", &m, expected);
}

void test_scale() {
    printf("\n=== Testing Scale ===\n");
    Mat4 m;
    fp_mat4_scale(&m, 2.0f, 3.0f, 4.0f);

    float expected[] = {
        2, 0, 0, 0,
        0, 3, 0, 0,
        0, 0, 4, 0,
        0, 0, 0, 1
    };

    check_mat4("Scale (non-uniform)", &m, expected);

    fp_mat4_scale_uniform(&m, 2.5f);
    float expected_uniform[] = {
        2.5f, 0, 0, 0,
        0, 2.5f, 0, 0,
        0, 0, 2.5f, 0,
        0, 0, 0, 1
    };

    check_mat4("Scale (uniform)", &m, expected_uniform);
}

void test_rotation_x() {
    printf("\n=== Testing Rotation X (90 degrees) ===\n");
    Mat4 m;
    fp_mat4_rotation_x(&m, PI / 2.0f);

    // Column-major storage: [col0, col1, col2, col3]
    float expected[] = {
        1, 0, 0, 0,      // Column 0: [1, 0, 0, 0]
        0, 0, 1, 0,      // Column 1: [0, cos, sin, 0] = [0, 0, 1, 0]
        0, -1, 0, 0,     // Column 2: [0, -sin, cos, 0] = [0, -1, 0, 0]
        0, 0, 0, 1       // Column 3: [0, 0, 0, 1]
    };

    check_mat4("Rotation X 90°", &m, expected);
}

void test_rotation_y() {
    printf("\n=== Testing Rotation Y (90 degrees) ===\n");
    Mat4 m;
    fp_mat4_rotation_y(&m, PI / 2.0f);

    // Column-major storage: [col0, col1, col2, col3]
    float expected[] = {
        0, 0, -1, 0,     // Column 0: [cos, 0, -sin, 0] = [0, 0, -1, 0]
        0, 1, 0, 0,      // Column 1: [0, 1, 0, 0]
        1, 0, 0, 0,      // Column 2: [sin, 0, cos, 0] = [1, 0, 0, 0]
        0, 0, 0, 1       // Column 3: [0, 0, 0, 1]
    };

    check_mat4("Rotation Y 90°", &m, expected);
}

void test_rotation_z() {
    printf("\n=== Testing Rotation Z (90 degrees) ===\n");
    Mat4 m;
    fp_mat4_rotation_z(&m, PI / 2.0f);

    // Column-major storage: [col0, col1, col2, col3]
    float expected[] = {
        0, 1, 0, 0,      // Column 0: [cos, sin, 0, 0] = [0, 1, 0, 0]
        -1, 0, 0, 0,     // Column 1: [-sin, cos, 0, 0] = [-1, 0, 0, 0]
        0, 0, 1, 0,      // Column 2: [0, 0, 1, 0]
        0, 0, 0, 1       // Column 3: [0, 0, 0, 1]
    };

    check_mat4("Rotation Z 90°", &m, expected);
}

void test_rotation_axis() {
    printf("\n=== Testing Rotation around arbitrary axis ===\n");
    Mat4 m;
    // Rotate 180° around (1,1,1) normalized
    fp_mat4_rotation_axis(&m, 1.0f, 1.0f, 1.0f, PI);

    // Transform (1,0,0)
    Vec3f test = {1.0f, 0.0f, 0.0f, 0.0f};
    Vec3f result;
    fp_mat4_mul_vec3(&result, &m, &test);

    printf("  Rotating (1,0,0) 180° around (1,1,1) -> (%.3f, %.3f, %.3f)\n",
           result.x, result.y, result.z);

    // Verify: result should be a unit vector and different from input
    float length = sqrtf(result.x * result.x + result.y * result.y + result.z * result.z);
    int is_unit = fabsf(length - 1.0f) < 0.01f;
    int is_different = fabsf(result.x - test.x) > 0.1f ||
                       fabsf(result.y - test.y) > 0.1f ||
                       fabsf(result.z - test.z) > 0.1f;

    if (is_unit && is_different) {
        printf("PASS: Rotation around axis (unit vector: %.3f, rotated)\n", length);
        passed++;
    } else {
        printf("FAIL: Rotation around axis (length: %.3f, expected ~1.0)\n", length);
        failed++;
    }
}

void test_perspective() {
    printf("\n=== Testing Perspective Projection ===\n");
    Mat4 m;
    fp_mat4_perspective(&m, PI / 2.0f, 16.0f/9.0f, 0.1f, 100.0f);

    printf("  Perspective (FOV=90°, Aspect=16:9, Near=0.1, Far=100):\n");
    print_mat4("  Result", &m);

    // Basic sanity checks
    if (m.m[0] > 0 && m.m[5] > 0 && m.m[10] < 0 && m.m[11] == -1.0f && m.m[15] == 0.0f) {
        printf("PASS: Perspective matrix structure\n");
        passed++;
    } else {
        printf("FAIL: Perspective matrix structure\n");
        failed++;
    }
}

void test_ortho() {
    printf("\n=== Testing Orthographic Projection ===\n");
    Mat4 m;
    fp_mat4_ortho(&m, -10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 100.0f);

    printf("  Ortho (L=-10, R=10, B=-10, T=10, N=0.1, F=100):\n");
    print_mat4("  Result", &m);

    // Basic sanity checks
    if (m.m[0] > 0 && m.m[5] > 0 && m.m[10] < 0 && m.m[15] == 1.0f) {
        printf("PASS: Orthographic matrix structure\n");
        passed++;
    } else {
        printf("FAIL: Orthographic matrix structure\n");
        failed++;
    }
}

void test_lookat() {
    printf("\n=== Testing LookAt View Matrix ===\n");
    Mat4 m;
    // Camera at (0, 0, 5) looking at origin, up is (0, 1, 0)
    fp_mat4_lookat(&m, 0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);

    printf("  LookAt (Eye=(0,0,5), Target=(0,0,0), Up=(0,1,0)):\n");
    print_mat4("  Result", &m);

    // Transform a point at origin - should be at (0, 0, -5) in view space
    Vec3f origin = {0.0f, 0.0f, 0.0f, 0.0f};
    Vec3f result;
    fp_mat4_mul_vec3(&result, &m, &origin);

    printf("  Origin in view space: (%.3f, %.3f, %.3f)\n", result.x, result.y, result.z);

    if (fabsf(result.x) < TOLERANCE &&
        fabsf(result.y) < TOLERANCE &&
        fabsf(result.z + 5.0f) < TOLERANCE) {
        printf("PASS: LookAt transformation\n");
        passed++;
    } else {
        printf("FAIL: LookAt transformation\n");
        failed++;
    }
}

void test_inverse() {
    printf("\n=== Testing Matrix Inverse ===\n");

    // Test with a simple translation
    Mat4 m, inv, identity;
    fp_mat4_translation(&m, 5.0f, 10.0f, 15.0f);

    int success = fp_mat4_inverse(&inv, &m);

    if (!success) {
        printf("FAIL: Inverse failed on simple translation\n");
        failed++;
        return;
    }

    // Multiply m * inv should give identity
    fp_mat4_mul(&identity, &m, &inv);

    float expected_identity[] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };

    check_mat4("Matrix inverse (M * M^-1 = I)", &identity, expected_identity);
}

void test_combined_transform() {
    printf("\n=== Testing Combined Transformations ===\n");

    // Create a typical model transform: Translate then Rotate then Scale
    Mat4 trans, rot, scale, temp, model;

    fp_mat4_translation(&trans, 10.0f, 0.0f, 0.0f);
    fp_mat4_rotation_z(&rot, PI / 4.0f);  // 45 degrees
    fp_mat4_scale_uniform(&scale, 2.0f);

    // Build: model = trans * rot * scale
    fp_mat4_mul(&temp, &rot, &scale);
    fp_mat4_mul(&model, &trans, &temp);

    printf("  Model = Translate(10,0,0) * RotateZ(45°) * Scale(2)\n");
    print_mat4("  Result", &model);

    // Transform point (1, 0, 0)
    Vec3f point = {1.0f, 0.0f, 0.0f, 0.0f};
    Vec3f result;
    fp_mat4_mul_vec3(&result, &model, &point);

    printf("  (1,0,0) transformed: (%.3f, %.3f, %.3f)\n", result.x, result.y, result.z);

    // Should be approximately: scale(2) -> (2,0,0), rotate 45° -> (√2, √2, 0), translate -> (10+√2, √2, 0)
    float sqrt2 = sqrtf(2.0f);
    float expected_x = 10.0f + sqrt2;
    float expected_y = sqrt2;

    if (fabsf(result.x - expected_x) < 0.01f &&
        fabsf(result.y - expected_y) < 0.01f &&
        fabsf(result.z) < 0.01f) {
        printf("PASS: Combined transformations\n");
        passed++;
    } else {
        printf("FAIL: Combined transformations (expected: %.3f, %.3f, 0)\n", expected_x, expected_y);
        failed++;
    }
}

int main(void) {
    printf("==============================================================\n");
    printf("  FP-ASM Matrix Operations Test Suite\n");
    printf("==============================================================\n");

    test_translation();
    test_scale();
    test_rotation_x();
    test_rotation_y();
    test_rotation_z();
    test_rotation_axis();
    test_perspective();
    test_ortho();
    test_lookat();
    test_inverse();
    test_combined_transform();

    printf("\n==============================================================\n");
    printf("  Test Results: %d passed, %d failed\n", passed, failed);
    printf("==============================================================\n");

    return (failed == 0) ? 0 : 1;
}
