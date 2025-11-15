/**
 * demo_bench_matrix.c - Module 7: 3D Matrix Math Benchmarks
 *
 * Tests correctness and performance of AVX2-optimized matrix operations.
 * Foundation for 3D game engines!
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "include/fp_core.h"

// Baseline C implementations for correctness testing

void mat4_identity_scalar(Mat4* m) {
    memset(m->m, 0, sizeof(m->m));
    m->m[0] = 1.0f;  m->m[5] = 1.0f;  m->m[10] = 1.0f;  m->m[15] = 1.0f;
}

void mat4_mul_scalar(Mat4* out, const Mat4* a, const Mat4* b) {
    Mat4 temp;
    for (int col = 0; col < 4; col++) {
        for (int row = 0; row < 4; row++) {
            float sum = 0.0f;
            for (int k = 0; k < 4; k++) {
                sum += a->m[row + k*4] * b->m[k + col*4];
            }
            temp.m[row + col*4] = sum;
        }
    }
    *out = temp;
}

void mat4_mul_vec3_scalar(Vec3f* out, const Mat4* m, const Vec3f* v) {
    out->x = m->m[0]*v->x + m->m[4]*v->y + m->m[8]*v->z  + m->m[12];
    out->y = m->m[1]*v->x + m->m[5]*v->y + m->m[9]*v->z  + m->m[13];
    out->z = m->m[2]*v->x + m->m[6]*v->y + m->m[10]*v->z + m->m[14];
}

void mat4_transpose_scalar(Mat4* out, const Mat4* m) {
    Mat4 temp;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            temp.m[j*4 + i] = m->m[i*4 + j];
        }
    }
    *out = temp;
}

// Utility functions

int mat4_equals(const Mat4* a, const Mat4* b, float epsilon) {
    for (int i = 0; i < 16; i++) {
        if (fabsf(a->m[i] - b->m[i]) > epsilon) {
            return 0;
        }
    }
    return 1;
}

int vec3_equals(const Vec3f* a, const Vec3f* b, float epsilon) {
    return fabsf(a->x - b->x) < epsilon &&
           fabsf(a->y - b->y) < epsilon &&
           fabsf(a->z - b->z) < epsilon;
}

void print_mat4(const char* name, const Mat4* m) {
    printf("%s:\n", name);
    for (int row = 0; row < 4; row++) {
        printf("  [");
        for (int col = 0; col < 4; col++) {
            printf("%7.3f", m->m[row + col*4]);
            if (col < 3) printf(" ");
        }
        printf("]\n");
    }
}

void print_vec3(const char* name, const Vec3f* v) {
    printf("%s: [%.3f, %.3f, %.3f]\n", name, v->x, v->y, v->z);
}

// Correctness Tests

int test_identity(void) {
    printf("Testing fp_mat4_identity... ");

    Mat4 m1, m2;
    fp_mat4_identity(&m1);
    mat4_identity_scalar(&m2);

    if (mat4_equals(&m1, &m2, 1e-6f)) {
        printf("PASS\n");
        return 1;
    } else {
        printf("FAIL\n");
        print_mat4("ASM", &m1);
        print_mat4("Scalar", &m2);
        return 0;
    }
}

int test_mat4_mul(void) {
    printf("Testing fp_mat4_mul... ");

    // Create test matrices
    Mat4 a = {{
        1, 2, 3, 4,
        5, 6, 7, 8,
        9, 10, 11, 12,
        13, 14, 15, 16
    }};

    Mat4 b = {{
        2, 0, 0, 0,
        0, 2, 0, 0,
        0, 0, 2, 0,
        0, 0, 0, 2
    }};

    Mat4 result_asm, result_scalar;
    fp_mat4_mul(&result_asm, &a, &b);
    mat4_mul_scalar(&result_scalar, &a, &b);

    if (mat4_equals(&result_asm, &result_scalar, 1e-5f)) {
        printf("PASS\n");
        return 1;
    } else {
        printf("FAIL\n");
        print_mat4("ASM", &result_asm);
        print_mat4("Scalar", &result_scalar);
        return 0;
    }
}

int test_mat4_mul_vec3(void) {
    printf("Testing fp_mat4_mul_vec3... ");

    Mat4 m = {{
        2, 0, 0, 0,
        0, 3, 0, 0,
        0, 0, 4, 0,
        5, 6, 7, 1
    }};

    Vec3f v = {1.0f, 2.0f, 3.0f, 0.0f};
    Vec3f result_asm, result_scalar;

    fp_mat4_mul_vec3(&result_asm, &m, &v);
    mat4_mul_vec3_scalar(&result_scalar, &m, &v);

    if (vec3_equals(&result_asm, &result_scalar, 1e-5f)) {
        printf("PASS\n");
        return 1;
    } else {
        printf("FAIL\n");
        print_vec3("ASM", &result_asm);
        print_vec3("Scalar", &result_scalar);
        return 0;
    }
}

int test_transpose(void) {
    printf("Testing fp_mat4_transpose... ");

    Mat4 m = {{
        1, 2, 3, 4,
        5, 6, 7, 8,
        9, 10, 11, 12,
        13, 14, 15, 16
    }};

    Mat4 result_asm, result_scalar;
    fp_mat4_transpose(&result_asm, &m);
    mat4_transpose_scalar(&result_scalar, &m);

    if (mat4_equals(&result_asm, &result_scalar, 1e-6f)) {
        printf("PASS\n");
        return 1;
    } else {
        printf("FAIL\n");
        print_mat4("ASM", &result_asm);
        print_mat4("Scalar", &result_scalar);
        return 0;
    }
}

// Performance Benchmarks

void benchmark_mat4_mul(int iterations) {
    printf("\n=== Benchmarking Matrix Multiplication ===\n");

    // Use DENSE matrices (no zeros!) to prevent compiler optimizations
    Mat4 a = {{1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16}};
    Mat4 b = {{1,2,1,2,2,1,2,1,1,2,1,2,2,1,2,1}};  // Dense!
    Mat4 result;

    volatile float sink;

    // Warm up
    for (int i = 0; i < 100; i++) {
        fp_mat4_mul(&result, &a, &b);
    }

    // Benchmark ASM version - use result as input to create dependencies
    Mat4 acc_asm = a;
    clock_t start_asm = clock();
    for (int i = 0; i < iterations; i++) {
        fp_mat4_mul(&acc_asm, &acc_asm, &b);  // acc = acc * b (chain dependency!)
    }
    clock_t end_asm = clock();
    double time_asm = (double)(end_asm - start_asm) / CLOCKS_PER_SEC;
    sink = acc_asm.m[0];  // Use result to prevent dead code elimination

    // Benchmark scalar version - same pattern
    Mat4 acc_scalar = a;
    clock_t start_scalar = clock();
    for (int i = 0; i < iterations; i++) {
        mat4_mul_scalar(&acc_scalar, &acc_scalar, &b);  // acc = acc * b (chain dependency!)
    }
    clock_t end_scalar = clock();
    double time_scalar = (double)(end_scalar - start_scalar) / CLOCKS_PER_SEC;
    sink = acc_scalar.m[0];  // Use result

    printf("Iterations: %d\n", iterations);
    printf("ASM (AVX2):         %.6f seconds (%.3f ns/op)\n", time_asm, (time_asm/iterations)*1e9);
    printf("Scalar (no SIMD):   %.6f seconds (%.3f ns/op)\n", time_scalar, (time_scalar/iterations)*1e9);
    printf("Speedup:            %.2fx\n", time_scalar / time_asm);
}

void benchmark_mat4_mul_vec3(int iterations) {
    printf("\n=== Benchmarking Matrix-Vector Multiplication ===\n");

    // Use DENSE matrix (no zeros!) to prevent compiler optimizations
    Mat4 m = {{1,2,1,2,2,1,2,1,1,2,1,2,2,1,2,1}};  // Dense!
    Vec3f v = {1.0f, 2.0f, 3.0f, 0.0f};
    Vec3f result;
    volatile float sink;

    // Warm up
    for (int i = 0; i < 100; i++) {
        fp_mat4_mul_vec3(&result, &m, &v);
    }

    // Benchmark ASM version - chain dependencies
    Vec3f vec_asm = v;
    clock_t start_asm = clock();
    for (int i = 0; i < iterations; i++) {
        fp_mat4_mul_vec3(&vec_asm, &m, &vec_asm);  // vec = m * vec (chain!)
    }
    clock_t end_asm = clock();
    double time_asm = (double)(end_asm - start_asm) / CLOCKS_PER_SEC;
    sink = vec_asm.x;  // Use result

    // Benchmark scalar version - chain dependencies
    Vec3f vec_scalar = v;
    clock_t start_scalar = clock();
    for (int i = 0; i < iterations; i++) {
        mat4_mul_vec3_scalar(&vec_scalar, &m, &vec_scalar);  // vec = m * vec (chain!)
    }
    clock_t end_scalar = clock();
    double time_scalar = (double)(end_scalar - start_scalar) / CLOCKS_PER_SEC;
    sink = vec_scalar.x;  // Use result

    printf("Iterations: %d\n", iterations);
    printf("ASM (AVX2):         %.6f seconds (%.3f ns/op)\n", time_asm, (time_asm/iterations)*1e9);
    printf("Scalar (no SIMD):   %.6f seconds (%.3f ns/op)\n", time_scalar, (time_scalar/iterations)*1e9);
    printf("Speedup:            %.2fx\n", time_scalar / time_asm);
}

int main(void) {
    printf("==============================================================\n");
    printf("  FP-ASM Module 7: 3D Matrix Math - Benchmarks\n");
    printf("  Foundation for Game Engines!\n");
    printf("==============================================================\n\n");

    // Run correctness tests FIRST
    printf("--- Correctness Tests ---\n");
    int passed = 0;
    passed += test_identity();
    passed += test_mat4_mul();
    passed += test_mat4_mul_vec3();
    passed += test_transpose();

    if (passed != 4) {
        printf("\n*** %d/%d tests FAILED - Fix errors before benchmarking! ***\n", 4-passed, 4);
        return 1;
    }

    printf("\nAll correctness tests PASSED!\n");

    // Run performance benchmarks
    int iterations = 10000000;  // 10M iterations for accurate timing

    benchmark_mat4_mul(iterations);
    benchmark_mat4_mul_vec3(iterations);

    printf("\n==============================================================\n");
    printf("  Module 7: Matrix Math - COMPLETE!\n");
    printf("==============================================================\n");
    printf("\nYour game engine foundation is ready! 🎮\n");
    printf("Next: Build full transformation pipeline!\n");

    return 0;
}
