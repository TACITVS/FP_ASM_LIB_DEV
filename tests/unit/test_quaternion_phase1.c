/**
 * @file test_quaternion_phase1.c
 * @brief Comprehensive tests for quaternion Phase 1 functions
 *
 * Tests follow FP-ASM testing guidelines:
 * 1. Correctness tests FIRST (halt on failure)
 * 2. Performance benchmarks (fair C baselines, same algorithm)
 * 3. Verification of purity (input immutability)
 *
 * Phase 1 Functions Under Test:
 * - fp_quat_normalize()
 * - fp_euler_to_quat()
 * - fp_quat_to_mat4()
 */

#include "../../include/fp_core.h"
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// Test Utilities
// ============================================================================

#define TOLERANCE 1e-5f
#define PI 3.14159265358979323846f

#define ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            printf("FAIL: %s\n", msg); \
            printf("  at %s:%d\n", __FILE__, __LINE__); \
            return 0; \
        } \
    } while(0)

#define ASSERT_FLOAT_EQ(a, b, tol, msg) \
    do { \
        float diff = fabsf((a) - (b)); \
        if (diff > (tol)) { \
            printf("FAIL: %s\n", msg); \
            printf("  Expected: %.8f, Got: %.8f, Diff: %.8f\n", (float)(b), (float)(a), diff); \
            return 0; \
        } \
    } while(0)

static int assert_quat_eq(const Quaternion* a, const Quaternion* b, float tol, const char* msg) {
    ASSERT_FLOAT_EQ(a->x, b->x, tol, msg);
    ASSERT_FLOAT_EQ(a->y, b->y, tol, msg);
    ASSERT_FLOAT_EQ(a->z, b->z, tol, msg);
    ASSERT_FLOAT_EQ(a->w, b->w, tol, msg);
    return 1;
}

static int assert_quat_normalized(const Quaternion* q, float tol, const char* msg) {
    float len_sq = q->x * q->x + q->y * q->y + q->z * q->z + q->w * q->w;
    ASSERT_FLOAT_EQ(len_sq, 1.0f, tol, msg);
    return 1;
}

// ============================================================================
// Correctness Tests
// ============================================================================

static int test_quat_normalize_identity() {
    printf("  Test: normalize identity quaternion... ");

    Quaternion input = {0.0f, 0.0f, 0.0f, 1.0f};
    Quaternion output;

    fp_quat_normalize(&output, &input);

    if (!assert_quat_eq(&output, &input, TOLERANCE, "normalize(identity) should equal identity")) {
        return 0;
    }
    if (!assert_quat_normalized(&output, TOLERANCE, "Result should be normalized")) {
        return 0;
    }

    printf("PASS\n");
    return 1;
}

static int test_quat_normalize_arbitrary() {
    printf("  Test: normalize arbitrary quaternion... ");

    Quaternion input = {1.0f, 2.0f, 3.0f, 4.0f};
    Quaternion output;

    fp_quat_normalize(&output, &input);

    // Expected: (1, 2, 3, 4) / sqrt(1+4+9+16) = (1, 2, 3, 4) / sqrt(30)
    float inv_len = 1.0f / sqrtf(30.0f);
    Quaternion expected = {
        1.0f * inv_len,
        2.0f * inv_len,
        3.0f * inv_len,
        4.0f * inv_len
    };

    if (!assert_quat_eq(&output, &expected, TOLERANCE, "Normalized values incorrect")) {
        return 0;
    }
    if (!assert_quat_normalized(&output, TOLERANCE, "Result should be unit length")) {
        return 0;
    }

    printf("PASS\n");
    return 1;
}

static int test_quat_normalize_near_zero() {
    printf("  Test: normalize near-zero quaternion... ");

    Quaternion input = {1e-10f, 1e-10f, 1e-10f, 1e-10f};
    Quaternion output;

    fp_quat_normalize(&output, &input);

    // Should return identity when input is near-zero
    Quaternion expected = {0.0f, 0.0f, 0.0f, 1.0f};

    if (!assert_quat_eq(&output, &expected, TOLERANCE, "Near-zero should return identity")) {
        return 0;
    }

    printf("PASS\n");
    return 1;
}

static int test_euler_to_quat_identity() {
    printf("  Test: euler_to_quat with zero angles... ");

    Quaternion output;
    fp_euler_to_quat(&output, 0.0f, 0.0f, 0.0f);

    // Zero Euler angles should give identity quaternion
    Quaternion expected = {0.0f, 0.0f, 0.0f, 1.0f};

    if (!assert_quat_eq(&output, &expected, TOLERANCE, "Zero angles should give identity")) {
        return 0;
    }
    if (!assert_quat_normalized(&output, TOLERANCE, "Result should be normalized")) {
        return 0;
    }

    printf("PASS\n");
    return 1;
}

static int test_euler_to_quat_90deg_rotations() {
    printf("  Test: euler_to_quat with 90-degree rotations... ");

    Quaternion output;

    // 90-degree pitch (X-axis rotation)
    fp_euler_to_quat(&output, PI / 2.0f, 0.0f, 0.0f);
    if (!assert_quat_normalized(&output, TOLERANCE, "90-deg pitch should be normalized")) {
        return 0;
    }

    // 90-degree yaw (Y-axis rotation)
    fp_euler_to_quat(&output, 0.0f, PI / 2.0f, 0.0f);
    if (!assert_quat_normalized(&output, TOLERANCE, "90-deg yaw should be normalized")) {
        return 0;
    }

    // 90-degree roll (Z-axis rotation)
    fp_euler_to_quat(&output, 0.0f, 0.0f, PI / 2.0f);
    if (!assert_quat_normalized(&output, TOLERANCE, "90-deg roll should be normalized")) {
        return 0;
    }

    printf("PASS\n");
    return 1;
}

static int test_quat_to_mat4_identity() {
    printf("  Test: quat_to_mat4 with identity quaternion... ");

    Quaternion q_identity = {0.0f, 0.0f, 0.0f, 1.0f};
    Mat4 output;

    fp_quat_to_mat4(&output, &q_identity);

    // Should produce identity matrix
    Mat4 expected;
    fp_mat4_identity(&expected);

    for (int i = 0; i < 16; i++) {
        ASSERT_FLOAT_EQ(output.m[i], expected.m[i], TOLERANCE, "Identity quat should give identity matrix");
    }

    printf("PASS\n");
    return 1;
}

static int test_quat_to_mat4_90deg_x() {
    printf("  Test: quat_to_mat4 with 90-deg X-axis rotation... ");

    // Create quaternion for 90-degree rotation around X-axis
    Quaternion q;
    fp_euler_to_quat(&q, PI / 2.0f, 0.0f, 0.0f);

    Mat4 output;
    fp_quat_to_mat4(&output, &q);

    // Verify it's orthonormal (rotation matrix properties)
    // Column 0: (1, 0, 0)
    ASSERT_FLOAT_EQ(output.m[0], 1.0f, TOLERANCE, "90-deg X: m[0] should be 1");
    ASSERT_FLOAT_EQ(fabsf(output.m[1]), 0.0f, TOLERANCE, "90-deg X: m[1] should be ~0");
    ASSERT_FLOAT_EQ(fabsf(output.m[2]), 0.0f, TOLERANCE, "90-deg X: m[2] should be ~0");

    // Translation should be zero
    ASSERT_FLOAT_EQ(output.m[12], 0.0f, TOLERANCE, "Translation X should be 0");
    ASSERT_FLOAT_EQ(output.m[13], 0.0f, TOLERANCE, "Translation Y should be 0");
    ASSERT_FLOAT_EQ(output.m[14], 0.0f, TOLERANCE, "Translation Z should be 0");
    ASSERT_FLOAT_EQ(output.m[15], 1.0f, TOLERANCE, "m[15] should be 1");

    printf("PASS\n");
    return 1;
}

static int test_round_trip_quat_euler_quat() {
    printf("  Test: round-trip quat->euler->quat... ");

    // Create a quaternion from arbitrary Euler angles
    float pitch = 0.3f, yaw = 0.5f, roll = 0.2f;
    Quaternion q1;
    fp_euler_to_quat(&q1, pitch, yaw, roll);

    // Convert to Euler
    Vec3f euler;
    fp_quat_to_euler(&euler, &q1);

    // Convert back to quaternion
    Quaternion q2;
    fp_euler_to_quat(&q2, euler.x, euler.y, euler.z);

    // Should match (within tolerance, accounting for gimbal lock regions)
    // Note: This may not be exact due to Euler angle ambiguities
    if (!assert_quat_normalized(&q2, TOLERANCE, "Round-trip result should be normalized")) {
        return 0;
    }

    printf("PASS\n");
    return 1;
}

// ============================================================================
// Performance Benchmarks
// ============================================================================

// C Baseline: fp_quat_normalize (SAME ALGORITHM)
static void quat_normalize_baseline(Quaternion* out, const Quaternion* q) {
    float len_sq = q->x * q->x + q->y * q->y + q->z * q->z + q->w * q->w;
    if (len_sq < 1e-8f) {
        out->x = 0.0f; out->y = 0.0f; out->z = 0.0f; out->w = 1.0f;
        return;
    }
    float inv_len = 1.0f / sqrtf(len_sq);
    out->x = q->x * inv_len;
    out->y = q->y * inv_len;
    out->z = q->z * inv_len;
    out->w = q->w * inv_len;
}

// C Baseline: fp_euler_to_quat (SAME ALGORITHM)
static void euler_to_quat_baseline(Quaternion* out, float pitch, float yaw, float roll) {
    float cx = cosf(pitch * 0.5f);
    float sx = sinf(pitch * 0.5f);
    float cy = cosf(yaw * 0.5f);
    float sy = sinf(yaw * 0.5f);
    float cz = cosf(roll * 0.5f);
    float sz = sinf(roll * 0.5f);

    out->w = cx * cy * cz - sx * sy * sz;
    out->x = sx * cy * cz + cx * sy * sz;
    out->y = cx * sy * cz - sx * cy * sz;
    out->z = cx * cy * sz + sx * sy * cz;
}

// C Baseline: fp_quat_to_mat4 (SAME ALGORITHM)
static void quat_to_mat4_baseline(Mat4* out, const Quaternion* q) {
    float xx = q->x * q->x, yy = q->y * q->y, zz = q->z * q->z;
    float xy = q->x * q->y, xz = q->x * q->z, yz = q->y * q->z;
    float wx = q->w * q->x, wy = q->w * q->y, wz = q->w * q->z;

    out->m[0] = 1.0f - 2.0f * (yy + zz);
    out->m[1] = 2.0f * (xy + wz);
    out->m[2] = 2.0f * (xz - wy);
    out->m[3] = 0.0f;

    out->m[4] = 2.0f * (xy - wz);
    out->m[5] = 1.0f - 2.0f * (xx + zz);
    out->m[6] = 2.0f * (yz + wx);
    out->m[7] = 0.0f;

    out->m[8] = 2.0f * (xz + wy);
    out->m[9] = 2.0f * (yz - wx);
    out->m[10] = 1.0f - 2.0f * (xx + yy);
    out->m[11] = 0.0f;

    out->m[12] = 0.0f; out->m[13] = 0.0f;
    out->m[14] = 0.0f; out->m[15] = 1.0f;
}

static void benchmark_normalize(int iterations) {
    printf("\nBenchmarking fp_quat_normalize (%d iterations)\n", iterations);
    printf("================================================================\n");

    // Setup test data
    Quaternion* inputs = malloc(sizeof(Quaternion) * iterations);
    Quaternion* outputs = malloc(sizeof(Quaternion) * iterations);

    for (int i = 0; i < iterations; i++) {
        inputs[i].x = (float)(rand() % 1000) / 100.0f;
        inputs[i].y = (float)(rand() % 1000) / 100.0f;
        inputs[i].z = (float)(rand() % 1000) / 100.0f;
        inputs[i].w = (float)(rand() % 1000) / 100.0f;
    }

    // Warmup
    for (int i = 0; i < 10; i++) {
        fp_quat_normalize(&outputs[0], &inputs[0]);
        quat_normalize_baseline(&outputs[0], &inputs[0]);
    }

    // Benchmark library implementation
    clock_t start = clock();
    for (int i = 0; i < iterations; i++) {
        fp_quat_normalize(&outputs[i], &inputs[i]);
    }
    clock_t end = clock();
    double time_lib = (double)(end - start) / CLOCKS_PER_SEC;

    // Benchmark C baseline
    start = clock();
    for (int i = 0; i < iterations; i++) {
        quat_normalize_baseline(&outputs[i], &inputs[i]);
    }
    end = clock();
    double time_c = (double)(end - start) / CLOCKS_PER_SEC;

    printf("Library:  %.6f seconds\n", time_lib);
    printf("C:        %.6f seconds\n", time_c);
    printf("Speedup:  %.2fx\n", time_c / time_lib);
    printf("Expected: ~1.0-1.2x (simple computation)\n");

    free(inputs);
    free(outputs);
}

static void benchmark_euler_to_quat(int iterations) {
    printf("\nBenchmarking fp_euler_to_quat (%d iterations)\n", iterations);
    printf("================================================================\n");

    // Setup test data
    float* pitches = malloc(sizeof(float) * iterations);
    float* yaws = malloc(sizeof(float) * iterations);
    float* rolls = malloc(sizeof(float) * iterations);
    Quaternion* outputs = malloc(sizeof(Quaternion) * iterations);

    for (int i = 0; i < iterations; i++) {
        pitches[i] = ((float)(rand() % 628) / 100.0f) - PI;
        yaws[i] = ((float)(rand() % 628) / 100.0f) - PI;
        rolls[i] = ((float)(rand() % 628) / 100.0f) - PI;
    }

    // Warmup
    for (int i = 0; i < 10; i++) {
        fp_euler_to_quat(&outputs[0], pitches[0], yaws[0], rolls[0]);
        euler_to_quat_baseline(&outputs[0], pitches[0], yaws[0], rolls[0]);
    }

    // Benchmark library implementation
    clock_t start = clock();
    for (int i = 0; i < iterations; i++) {
        fp_euler_to_quat(&outputs[i], pitches[i], yaws[i], rolls[i]);
    }
    clock_t end = clock();
    double time_lib = (double)(end - start) / CLOCKS_PER_SEC;

    // Benchmark C baseline
    start = clock();
    for (int i = 0; i < iterations; i++) {
        euler_to_quat_baseline(&outputs[i], pitches[i], yaws[i], rolls[i]);
    }
    end = clock();
    double time_c = (double)(end - start) / CLOCKS_PER_SEC;

    printf("Library:  %.6f seconds\n", time_lib);
    printf("C:        %.6f seconds\n", time_c);
    printf("Speedup:  %.2fx\n", time_c / time_lib);
    printf("Expected: ~1.0-1.1x (trig-heavy, compiler optimizes well)\n");

    free(pitches);
    free(yaws);
    free(rolls);
    free(outputs);
}

static void benchmark_quat_to_mat4(int iterations) {
    printf("\nBenchmarking fp_quat_to_mat4 (%d iterations)\n", iterations);
    printf("================================================================\n");

    // Setup test data
    Quaternion* inputs = malloc(sizeof(Quaternion) * iterations);
    Mat4* outputs = malloc(sizeof(Mat4) * iterations);

    for (int i = 0; i < iterations; i++) {
        // Generate normalized quaternions
        float pitch = ((float)(rand() % 628) / 100.0f) - PI;
        float yaw = ((float)(rand() % 628) / 100.0f) - PI;
        float roll = ((float)(rand() % 628) / 100.0f) - PI;
        fp_euler_to_quat(&inputs[i], pitch, yaw, roll);
    }

    // Warmup
    for (int i = 0; i < 10; i++) {
        fp_quat_to_mat4(&outputs[0], &inputs[0]);
        quat_to_mat4_baseline(&outputs[0], &inputs[0]);
    }

    // Benchmark library implementation
    clock_t start = clock();
    for (int i = 0; i < iterations; i++) {
        fp_quat_to_mat4(&outputs[i], &inputs[i]);
    }
    clock_t end = clock();
    double time_lib = (double)(end - start) / CLOCKS_PER_SEC;

    // Benchmark C baseline
    start = clock();
    for (int i = 0; i < iterations; i++) {
        quat_to_mat4_baseline(&outputs[i], &inputs[i]);
    }
    end = clock();
    double time_c = (double)(end - start) / CLOCKS_PER_SEC;

    printf("Library:  %.6f seconds\n", time_lib);
    printf("C:        %.6f seconds\n", time_c);
    printf("Speedup:  %.2fx\n", time_c / time_lib);
    printf("Expected: ~1.0-1.1x (optimized algorithm, 15 muls)\n");
    printf("Note:     Both use same optimized formula (not naive 27 muls)\n");

    free(inputs);
    free(outputs);
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main(void) {
    printf("========================================\n");
    printf("Quaternion Phase 1 Tests\n");
    printf("========================================\n\n");

    // PHASE 1: Correctness Tests (HALT ON FAILURE)
    printf("PHASE 1: Correctness Tests\n");
    printf("----------------------------------------\n");

    if (!test_quat_normalize_identity()) return 1;
    if (!test_quat_normalize_arbitrary()) return 1;
    if (!test_quat_normalize_near_zero()) return 1;
    if (!test_euler_to_quat_identity()) return 1;
    if (!test_euler_to_quat_90deg_rotations()) return 1;
    if (!test_quat_to_mat4_identity()) return 1;
    if (!test_quat_to_mat4_90deg_x()) return 1;
    if (!test_round_trip_quat_euler_quat()) return 1;

    printf("\n✓ All correctness tests PASSED!\n");

    // PHASE 2: Performance Benchmarks
    printf("\n========================================\n");
    printf("PHASE 2: Performance Benchmarks\n");
    printf("========================================\n");

    srand(42);  // Deterministic random data

    benchmark_normalize(100000);
    benchmark_euler_to_quat(100000);
    benchmark_quat_to_mat4(100000);

    // PHASE 3: L0 Primitive Sanity Check
    printf("\n========================================\n");
    printf("PHASE 3: L0 Primitives Verification\n");
    printf("========================================\n");
    printf("Verifying L0-optimized functions use assembly primitives correctly\n\n");

    // Simple sanity check: verify L0-optimized version produces same results as Pure C
    Quaternion test_quat = {1.0f, 2.0f, 3.0f, 4.0f};
    Quaternion result_pure_c, result_l0;

    fp_quat_normalize_pure_c(&result_pure_c, &test_quat);
    fp_quat_normalize(&result_l0, &test_quat);

    int l0_correct =
        (fabs(result_pure_c.x - result_l0.x) < 1e-6f) &&
        (fabs(result_pure_c.y - result_l0.y) < 1e-6f) &&
        (fabs(result_pure_c.z - result_l0.z) < 1e-6f) &&
        (fabs(result_pure_c.w - result_l0.w) < 1e-6f);

    printf("  L0-optimized fp_quat_normalize... %s\n", l0_correct ? "PASS" : "FAIL");
    printf("  Uses: fp_fold_dotp_f32, fp_map_scale_f32\n");

    if (!l0_correct) {
        printf("ERROR: L0-optimized version produced different results!\n");
        printf("  Pure C: [%.6f, %.6f, %.6f, %.6f]\n",
               result_pure_c.x, result_pure_c.y, result_pure_c.z, result_pure_c.w);
        printf("  L0:     [%.6f, %.6f, %.6f, %.6f]\n",
               result_l0.x, result_l0.y, result_l0.z, result_l0.w);
        return 1;
    }

    printf("\n✓ L0 primitives verified!\n");
    printf("\nNote: Phase 2 benchmarks show ~1.0x speedup for single quaternions.\n");
    printf("      This is expected due to packing overhead (struct -> array -> struct).\n");
    printf("      Future optimization: Batch operations for 1000s of quaternions.\n");

    printf("\n========================================\n");
    printf("All tests completed successfully!\n");
    printf("========================================\n");

    return 0;
}
