#include "fp_graphics_engine.h"
#include "fp_core.h" // For the actual fp_mat4_mul implementation
#include <stdio.h>
#include <string.h>
#include <math.h>

void print_matrix(const char* name, const Mat4* m) {
    printf("Matrix %s:\n", name);
    for (int i = 0; i < 4; ++i) {
        printf("  [ %.2f, %.2f, %.2f, %.2f ]\n",
               m->m[i], m->m[i + 4], m->m[i + 8], m->m[i + 12]);
    }
    printf("\n");
}

int test_matrix_multiplication() {
    printf("--- Test 1: Matrix Multiplication ---\n\n");
    Mat4 mat_a, mat_b;
    fp_mat4_translation(&mat_a, 1.0f, 2.0f, 3.0f);
    print_matrix("A (Translation)", &mat_a);
    fp_mat4_scale(&mat_b, 2.0f, 2.0f, 2.0f);
    print_matrix("B (Scale)", &mat_b);

    Mat4 result;
    fp_mat4_mul_pure(&result, &mat_a, &mat_b);
    print_matrix("Result (A * B)", &result);

    Mat4 expected;
    fp_mat4_identity(&expected);
    expected.m[0] = 2.0f;
    expected.m[5] = 2.0f;
    expected.m[10] = 2.0f;
    expected.m[12] = 1.0f;
    expected.m[13] = 2.0f;
    expected.m[14] = 3.0f;
    
    int mismatch = 0;
    for (int i = 0; i < 16; ++i) {
        if (fabs(result.m[i] - expected.m[i]) > 1e-6) {
            mismatch = 1;
            break;
        }
    }

    if (mismatch) {
        printf("--- !!! TEST 1 FAILED !!! ---\n\n");
    } else {
        printf("--- TEST 1 PASSED ---\n\n");
    }
    return mismatch;
}

int test_transform_to_matrix() {
    printf("--- Test 2: Transform to Matrix ---\n\n");

    Transform t = {
        .position = {10.0f, 20.0f, 30.0f},
        .euler_rotation = {0.0f, M_PI / 2.0f, 0.0f}, // 90 degree yaw
        .scale = {2.0f, 2.0f, 2.0f}
    };

    Mat4 result_model_matrix;
    fp_transform_to_matrix(&result_model_matrix, &t);
    print_matrix("Result Model Matrix", &result_model_matrix);

    // Manually calculate expected result
    Mat4 trans_mat, rot_mat, scale_mat, temp_mat, expected_model_matrix;
    fp_mat4_translation(&trans_mat, 10.0f, 20.0f, 30.0f);
    fp_mat4_rotation_y(&rot_mat, M_PI / 2.0f);
    fp_mat4_scale(&scale_mat, 2.0f, 2.0f, 2.0f);
    fp_mat4_mul(&temp_mat, &rot_mat, &scale_mat);
    fp_mat4_mul(&expected_model_matrix, &trans_mat, &temp_mat);
    print_matrix("Expected Model Matrix", &expected_model_matrix);

    int mismatch = 0;
    for (int i = 0; i < 16; ++i) {
        if (fabs(result_model_matrix.m[i] - expected_model_matrix.m[i]) > 1e-6) {
            mismatch = 1;
            break;
        }
    }

    if (mismatch) {
        printf("--- !!! TEST 2 FAILED !!! ---\n\n");
    } else {
        printf("--- TEST 2 PASSED ---\n\n");
    }
    return mismatch;
}

int test_camera_matrices() {
    printf("--- Test 3: Camera Matrices ---\n\n");

    Camera cam = {
        .transform = {
            .position = {0.0f, 0.0f, 5.0f},
            .euler_rotation = {0.0f, 0.0f, 0.0f},
            .scale = {1.0f, 1.0f, 1.0f}
        },
        .projection = {
            .fov_radians = M_PI / 2.0f, // 90 degrees
            .aspect_ratio = 16.0f / 9.0f,
            .near_plane = 0.1f,
            .far_plane = 100.0f
        }
    };

    Mat4 view_matrix, proj_matrix;
    fp_view_matrix(&view_matrix, &cam);
    fp_projection_matrix(&proj_matrix, &cam.projection);

    print_matrix("Result View Matrix", &view_matrix);
    print_matrix("Result Projection Matrix", &proj_matrix);

    // Manually calculate expected matrices
    Mat4 expected_view, expected_proj;
    fp_mat4_lookat(&expected_view, 0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    fp_mat4_perspective(&expected_proj, M_PI / 2.0f, 16.0f / 9.0f, 0.1f, 100.0f);

    int mismatch = 0;
    for (int i = 0; i < 16; ++i) {
        if (fabs(view_matrix.m[i] - expected_view.m[i]) > 1e-6) {
            mismatch = 1;
            break;
        }
    }
    if (mismatch) {
        printf("--- !!! View Matrix TEST FAILED !!! ---\n\n");
        return 1;
    }

    for (int i = 0; i < 16; ++i) {
        if (fabs(proj_matrix.m[i] - expected_proj.m[i]) > 1e-6) {
            mismatch = 1;
            break;
        }
    }

    if (mismatch) {
        printf("--- !!! Projection Matrix TEST FAILED !!! ---\n\n");
    } else {
        printf("--- TEST 3 PASSED ---\n\n");
    }
    return mismatch;
}

int test_mvp_matrix() {
    printf("--- Test 4: MVP Matrix Composition ---\n\n");

    // 1. Create Model matrix
    Transform t = {
        .position = {1.0f, 0.0f, 0.0f},
        .euler_rotation = {0.0f, 0.0f, 0.0f},
        .scale = {1.0f, 1.0f, 1.0f}
    };
    Mat4 model_matrix;
    fp_transform_to_matrix(&model_matrix, &t);

    // 2. Create View matrix
    Camera cam = {
        .transform = { .position = {0.0f, 0.0f, 5.0f} }
    };
    Mat4 view_matrix;
    fp_view_matrix(&view_matrix, &cam);

    // 3. Create Projection matrix
    CameraProjection proj = {
        .fov_radians = M_PI / 2.0f,
        .aspect_ratio = 1.0f,
        .near_plane = 0.1f,
        .far_plane = 100.0f
    };
    Mat4 proj_matrix;
    fp_projection_matrix(&proj_matrix, &proj);

    // 4. Get MVP matrix
    Mat4 mvp_matrix;
    fp_get_mvp_matrix(&mvp_matrix, &model_matrix, &view_matrix, &proj_matrix);
    print_matrix("Result MVP Matrix", &mvp_matrix);

    // 5. Manually calculate expected MVP
    Mat4 expected_mvp, temp;
    fp_mat4_mul(&temp, &view_matrix, &model_matrix);
    fp_mat4_mul(&expected_mvp, &proj_matrix, &temp);
    print_matrix("Expected MVP Matrix", &expected_mvp);

    int mismatch = 0;
    for (int i = 0; i < 16; ++i) {
        if (fabs(mvp_matrix.m[i] - expected_mvp.m[i]) > 1e-6) {
            mismatch = 1;
            break;
        }
    }

    if (mismatch) {
        printf("--- !!! TEST 4 FAILED !!! ---\n\n");
    } else {
        printf("--- TEST 4 PASSED ---\n\n");
    }
    return mismatch;
}

int main() {
    printf("--- Testing FP Graphics Transforms ---\n\n");

    int failed = 0;
    failed |= test_matrix_multiplication();
    failed |= test_transform_to_matrix();
    failed |= test_camera_matrices();
    failed |= test_mvp_matrix();

    printf("--- All Tests Complete ---\n");
    if (failed) {
        printf("Overall Status: FAILED\n");
    } else {
        printf("Overall Status: PASSED\n");
    }

    return failed;
}