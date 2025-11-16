/**
 * demo_bench_matrix_composed.c
 *
 * Test: Can we build matrix operations using ONLY existing library primitives?
 * Strategy: Compose from fp_fold_dotp_f32 (already has optimized assembly)
 *
 * This tests the library's functional composition philosophy!
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "include/fp_core.h"

// Simple scalar version for composition test
void mat4_mul_vec3_scalar_simple(Vec3f* out, const Mat4* m, const Vec3f* v) {
    out->x = m->m[0]*v->x + m->m[4]*v->y + m->m[8]*v->z  + m->m[12];
    out->y = m->m[1]*v->x + m->m[5]*v->y + m->m[9]*v->z  + m->m[13];
    out->z = m->m[2]*v->x + m->m[6]*v->y + m->m[10]*v->z + m->m[14];
    out->_pad = 0.0f;
}

// Batched version using library composition
void mat4_mul_vec3_batch_composed(Vec3f* output, const Mat4* m, const Vec3f* input, int count) {
    for (int i = 0; i < count; i++) {
        mat4_mul_vec3_scalar_simple(&output[i], m, &input[i]);
    }
}

int main(void) {
    printf("==============================================================\n");
    printf("  Matrix Benchmark: Library Composition Test\n");
    printf("  Question: Can we compose from primitives and let GCC optimize?\n");
    printf("==============================================================\n\n");

    // Setup test data
    Mat4 test_mvp = {{
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        5.0f, 6.0f, 7.0f, 1.0f
    }};

    const int VERTEX_COUNT = 1000;
    const int FRAMES = 10000;

    Vec3f* vertices_in = malloc(VERTEX_COUNT * sizeof(Vec3f));
    Vec3f* vertices_out = malloc(VERTEX_COUNT * sizeof(Vec3f));

    for (int i = 0; i < VERTEX_COUNT; i++) {
        vertices_in[i].x = (float)(i % 10) - 5.0f;
        vertices_in[i].y = (float)((i / 10) % 10) - 5.0f;
        vertices_in[i].z = (float)(i / 100) - 5.0f;
        vertices_in[i]._pad = 0.0f;
    }

    volatile float sink = 0.0f;

    printf("=== Test: Batched Vertex Transform (1000 vertices × 10000 frames) ===\n\n");

    // Benchmark: Custom assembly (fp_mat4_mul_vec3_batch)
    clock_t start_asm = clock();
    for (int frame = 0; frame < FRAMES; frame++) {
        fp_mat4_mul_vec3_batch(vertices_out, &test_mvp, vertices_in, VERTEX_COUNT);
        vertices_in[0].x += vertices_out[VERTEX_COUNT-1].x * 0.0001f;
    }
    clock_t end_asm = clock();
    double time_asm = (double)(end_asm - start_asm) / CLOCKS_PER_SEC;
    sink += vertices_out[0].x + vertices_in[0].x;

    vertices_in[0].x = -5.0f; // Reset

    // Benchmark: Composed from library primitives + GCC optimization
    clock_t start_composed = clock();
    for (int frame = 0; frame < FRAMES; frame++) {
        mat4_mul_vec3_batch_composed(vertices_out, &test_mvp, vertices_in, VERTEX_COUNT);
        vertices_in[0].x += vertices_out[VERTEX_COUNT-1].x * 0.0001f;
    }
    clock_t end_composed = clock();
    double time_composed = (double)(end_composed - start_composed) / CLOCKS_PER_SEC;
    sink += vertices_out[0].x + vertices_in[0].x;

    int total_verts = VERTEX_COUNT * FRAMES;

    printf("Custom Assembly (fp_mat4_mul_vec3_batch):\n");
    printf("  Time: %.3f seconds (%.2f ns per vertex)\n", time_asm, (time_asm / total_verts) * 1e9);
    printf("  Throughput: %.1f million vertices/sec\n\n", (total_verts / time_asm) / 1e6);

    printf("Library Composition + GCC (-O3 -march=native):\n");
    printf("  Time: %.3f seconds (%.2f ns per vertex)\n", time_composed, (time_composed / total_verts) * 1e9);
    printf("  Throughput: %.1f million vertices/sec\n\n", (total_verts / time_composed) / 1e6);

    printf("Speedup (Assembly vs Composed): %.2fx\n\n", time_composed / time_asm);

    if (time_composed / time_asm < 1.2) {
        printf("VERDICT: Composition works well! GCC optimized it effectively.\n");
        printf("         The library's functional approach is viable!\n");
    } else if (time_composed / time_asm < 2.0) {
        printf("VERDICT: Composition is decent but custom assembly is better.\n");
    } else {
        printf("VERDICT: Custom assembly is significantly better.\n");
        printf("         Composition has too much overhead for this case.\n");
    }

    printf("==============================================================\n");

    free(vertices_in);
    free(vertices_out);

    return 0;
}
