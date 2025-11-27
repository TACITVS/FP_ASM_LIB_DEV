#include "fp_gpu_math.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define ASSERT_TRUE(cond, msg) \
    do { \
        if (!(cond)) { \
            printf("FAIL: %s\n", msg); \
            return 0; \
        } \
    } while (0)

static int mat4_equal(const Mat4* a, const Mat4* b, float tol) {
    for (int i = 0; i < 16; i++) {
        float diff = a->m[i] - b->m[i];
        if (diff < -tol || diff > tol) {
            return 0;
        }
    }
    return 1;
}

static int test_cpu_fallback_matches_direct(void) {
    FpGpuMathContext* ctx = fp_gpu_math_init();

    Quaternion qs[2] = {
        {0.0f, 0.0f, 0.0f, 1.0f},                  // identity
        {0.0f, 0.70710677f, 0.0f, 0.70710677f}    // 90 deg yaw
    };

    Mat4 out[2];
    memset(out, 0, sizeof(out));

    ASSERT_TRUE(fp_gpu_quat_to_mat4_batch(ctx, out, qs, 2), "cpu fallback should succeed");

    Mat4 expected[2];
    fp_quat_to_mat4(&expected[0], &qs[0]);
    fp_quat_to_mat4(&expected[1], &qs[1]);

    ASSERT_TRUE(mat4_equal(&out[0], &expected[0], 1e-5f), "identity mat should match");
    ASSERT_TRUE(mat4_equal(&out[1], &expected[1], 1e-5f), "90deg yaw mat should match");

    fp_gpu_math_shutdown(ctx);
    return 1;
}

static int test_aliasing_is_rejected(void) {
    FpGpuMathContext* ctx = fp_gpu_math_init();
    Quaternion qs[1] = {{0.0f, 0.0f, 0.0f, 1.0f}};

    ASSERT_TRUE(!fp_gpu_quat_to_mat4_batch(ctx, (Mat4*)qs, qs, 1),
                "aliasing in/out must be rejected");

    fp_gpu_math_shutdown(ctx);
    return 1;
}

static int test_overflow_guard(void) {
    Mat4 dummy_out;
    Quaternion dummy_in;
    size_t huge = (SIZE_MAX / sizeof(Mat4)) + 1;
    ASSERT_TRUE(!fp_gpu_quat_to_mat4_batch(NULL, &dummy_out, &dummy_in, huge),
                "overflow input should be rejected");
    return 1;
}

static int test_null_params(void) {
    Quaternion q = {0.0f, 0.0f, 0.0f, 1.0f};
    Mat4 m;
    ASSERT_TRUE(!fp_gpu_quat_to_mat4_batch(NULL, NULL, &q, 1), "NULL out rejected");
    ASSERT_TRUE(!fp_gpu_quat_to_mat4_batch(NULL, &m, NULL, 1), "NULL in rejected");
    ASSERT_TRUE(!fp_gpu_quat_to_mat4_batch(NULL, &m, &q, 0), "zero n rejected");
    return 1;
}

static int test_ctx_optional(void) {
    Quaternion q = {0.0f, 0.0f, 0.0f, 1.0f};
    Mat4 m = {0};
    ASSERT_TRUE(fp_gpu_quat_to_mat4_batch(NULL, &m, &q, 1), "ctx NULL should still work (CPU fallback)");
    Mat4 expected;
    fp_quat_to_mat4(&expected, &q);
    ASSERT_TRUE(mat4_equal(&m, &expected, 1e-5f), "ctx NULL path should match direct");
    return 1;
}

int main(void) {
    int passed = 0, failed = 0;

    if (test_cpu_fallback_matches_direct()) passed++; else failed++;
    if (test_aliasing_is_rejected()) passed++; else failed++;
    if (test_overflow_guard()) passed++; else failed++;
    if (test_null_params()) passed++; else failed++;
    if (test_ctx_optional()) passed++; else failed++;

    printf("GPU math tests: passed=%d failed=%d\n", passed, failed);
    return failed ? 1 : 0;
}
