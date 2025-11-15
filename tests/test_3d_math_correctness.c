#include "fp_core.h"
#include "fp_3d_math_wrapper.h" // For C-kernels and reference implementations
#include "fp_generic.h" // For Layer 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define TEST_N 1024

// Portable aligned_alloc for GCC on Windows
#if defined(__GNUC__) && defined(_WIN32)
#include <malloc.h> // For _aligned_malloc and _aligned_free
#define ALIGNED_MALLOC(size, alignment) _aligned_malloc(size, alignment)
#define ALIGNED_FREE _aligned_free
#else
// Fallback for other compilers or non-Windows GCC
#define ALIGNED_MALLOC(size, alignment) aligned_alloc(alignment, size)
#define ALIGNED_FREE free
#endif

// Macro for float comparison with tolerance
#define ASSERT_FLOAT_EQ(expected, actual, tolerance, test_name) \
    if (1) { \
        if (fabs((expected) - (actual)) > (tolerance)) { \
            printf("FAIL: %s - Expected %f, Got %f (Tolerance %f)\n", \
                   test_name, (double)(expected), (double)(actual), (double)(tolerance)); \
            exit(1); \
        } else { \
            printf("PASS: %s\n", test_name); \
        } \
    }

// Macro for float array comparison with tolerance
#define ASSERT_FLOAT_ARRAY_EQ(expected_arr, actual_arr, n, tolerance, test_name) \
    if (1) { \
        int fail = 0; \
        for (size_t i = 0; i < n; ++i) { \
            if (fabs((expected_arr)[i] - (actual_arr)[i]) > (tolerance)) { \
                printf("FAIL: %s[%zu] - Expected %f, Got %f (Tolerance %f)\n", \
                       test_name, i, (double)(expected_arr)[i], (double)(actual_arr)[i], (double)(tolerance)); \
                fail = 1; \
                break; \
            } \
        } \
        if (fail) { \
            exit(1); \
        } else { \
            printf("PASS: %s\n", test_name); \
        } \
    }

// Global test data
ALIGN16 Vec3f g_in_vecs[TEST_N];
ALIGN16 Vec3f g_out_vecs_ref[TEST_N];
ALIGN16 Vec3f g_out_vecs_asm[TEST_N];
ALIGN16 Vec3f g_in_a[TEST_N];
ALIGN16 Vec3f g_in_b[TEST_N];
ALIGN16 Mat4 g_matrix = {{ \
    1.0f, 0.0f, 0.0f, 0.0f, 
    0.0f, 1.0f, 0.0f, 0.0f, 
    0.0f, 0.0f, 1.0f, 0.0f, 
    10.0f, 20.0f, 30.0f, 1.0f // Translate +10, +20, +30 
}};
ALIGN16 Quaternion g_quat = { 0.0f, 0.707107f, 0.0f, 0.707107f }; // 90 deg rot around Y

void setup_test_data() {
    for (size_t i = 0; i < TEST_N; ++i) {
        g_in_vecs[i].x = (float)i;
        g_in_vecs[i].y = (float)i * 2.0f;
        g_in_vecs[i].z = (float)i * 3.0f;
        g_in_vecs[i]._pad = 0.0f;

        g_in_a[i].x = (float)i + 1.0f;
        g_in_a[i].y = (float)i * 0.5f;
        g_in_a[i].z = (float)i * 1.5f;
        g_in_a[i]._pad = 0.0f;

        g_in_b[i].x = (float)i * 2.0f;
        g_in_b[i].y = (float)i + 3.0f;
        g_in_b[i].z = (float)i * 0.25f;
        g_in_b[i]._pad = 0.0f;
    }
}

int main() {
    setup_test_data();
    printf("Test data setup complete.\n");

        printf("--- Running 3D Math Correctness Tests (N=%d) ---\n", TEST_N);
    // Test fp_map_transform_vec3_f32
    ref_map_transform_vec3_f32(g_in_vecs, g_out_vecs_ref, TEST_N, &g_matrix);
    fp_map_transform_vec3_f32(g_in_vecs, g_out_vecs_asm, TEST_N, &g_matrix);
    ASSERT_FLOAT_ARRAY_EQ((float*)g_out_vecs_ref, (float*)g_out_vecs_asm, TEST_N * 4, 0.001f, "fp_map_transform_vec3_f32");

    // Test fp_zipWith_vec3_add_f32
    ref_zipWith_vec3_add_f32(g_in_a, g_in_b, g_out_vecs_ref, TEST_N);
    fp_zipWith_vec3_add_f32(g_in_a, g_in_b, g_out_vecs_asm, TEST_N);
    ASSERT_FLOAT_ARRAY_EQ((float*)g_out_vecs_ref, (float*)g_out_vecs_asm, TEST_N * 4, 0.001f, "fp_zipWith_vec3_add_f32");

    // Test fp_map_quat_rotate_vec3_f32
    ref_map_quat_rotate_vec3_f32(g_in_vecs, g_out_vecs_ref, TEST_N, &g_quat);
    fp_map_quat_rotate_vec3_f32(g_in_vecs, g_out_vecs_asm, TEST_N, &g_quat);
    ASSERT_FLOAT_ARRAY_EQ((float*)g_out_vecs_ref, (float*)g_out_vecs_asm, TEST_N * 4, 0.001f, "fp_map_quat_rotate_vec3_f32");

    // Test fp_reduce_vec3_add_f32
    ALIGN16 Vec3f ref_sum = {0};
    ALIGN16 Vec3f asm_sum = {0};
    ref_reduce_vec3_add_f32(g_in_vecs, TEST_N, &ref_sum);
    fp_reduce_vec3_add_f32(g_in_vecs, TEST_N, &asm_sum);
    ASSERT_FLOAT_EQ(ref_sum.x, asm_sum.x, 0.0001f, "fp_reduce_vec3_add_f32.x");
    ASSERT_FLOAT_EQ(ref_sum.y, asm_sum.y, 0.0001f, "fp_reduce_vec3_add_f32.y");
    ASSERT_FLOAT_EQ(ref_sum.z, asm_sum.z, 0.0001f, "fp_reduce_vec3_add_f32.z");

    // Test fp_fold_vec3_dot_f32
    float ref_dot = ref_fold_vec3_dot_f32(g_in_a, g_in_b, TEST_N);
    float asm_dot = fp_fold_vec3_dot_f32(g_in_a, g_in_b, TEST_N);
    printf("Dot Product (Ref): %f, (ASM): %f\n", ref_dot, asm_dot);
    ASSERT_FLOAT_EQ(ref_dot, asm_dot, 0.0001f, "fp_fold_vec3_dot_f32");

    return 0;
}