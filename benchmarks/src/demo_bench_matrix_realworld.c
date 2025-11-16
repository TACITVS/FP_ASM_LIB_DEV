/**
 * demo_bench_matrix_realworld.c - REAL Game Engine Workload Benchmark
 *
 * Simulates actual 3D game engine frame rendering:
 * - Model-View-Projection matrix chains
 * - Vertex transformations (mesh rendering)
 * - Transform hierarchy updates
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "include/fp_core.h"

// Real-world transformation matrices

void create_perspective_matrix(Mat4* m, float fov, float aspect, float near, float far) {
    float f = 1.0f / tanf(fov * 0.5f);
    m->m[0] = f / aspect;  m->m[4] = 0.0f;  m->m[8] = 0.0f;                           m->m[12] = 0.0f;
    m->m[1] = 0.0f;        m->m[5] = f;     m->m[9] = 0.0f;                           m->m[13] = 0.0f;
    m->m[2] = 0.0f;        m->m[6] = 0.0f;  m->m[10] = (far + near) / (near - far);  m->m[14] = (2 * far * near) / (near - far);
    m->m[3] = 0.0f;        m->m[7] = 0.0f;  m->m[11] = -1.0f;                         m->m[15] = 0.0f;
}

void create_view_matrix(Mat4* m, float x, float y, float z) {
    // Simple translation for camera
    m->m[0] = 1.0f;  m->m[4] = 0.0f;  m->m[8] = 0.0f;   m->m[12] = -x;
    m->m[1] = 0.0f;  m->m[5] = 1.0f;  m->m[9] = 0.0f;   m->m[13] = -y;
    m->m[2] = 0.0f;  m->m[6] = 0.0f;  m->m[10] = 1.0f;  m->m[14] = -z;
    m->m[3] = 0.0f;  m->m[7] = 0.0f;  m->m[11] = 0.0f;  m->m[15] = 1.0f;
}

void create_model_matrix(Mat4* m, float x, float y, float z, float scale) {
    // Translation + uniform scale
    m->m[0] = scale;  m->m[4] = 0.0f;   m->m[8] = 0.0f;    m->m[12] = x;
    m->m[1] = 0.0f;   m->m[5] = scale;  m->m[9] = 0.0f;    m->m[13] = y;
    m->m[2] = 0.0f;   m->m[6] = 0.0f;   m->m[10] = scale;  m->m[14] = z;
    m->m[3] = 0.0f;   m->m[7] = 0.0f;   m->m[11] = 0.0f;   m->m[15] = 1.0f;
}

// Scalar baseline for MVP chain
void compute_mvp_scalar(Mat4* mvp, const Mat4* proj, const Mat4* view, const Mat4* model) {
    Mat4 temp;
    // temp = view * model
    for (int col = 0; col < 4; col++) {
        for (int row = 0; row < 4; row++) {
            float sum = 0.0f;
            for (int k = 0; k < 4; k++) {
                sum += view->m[row + k*4] * model->m[k + col*4];
            }
            temp.m[row + col*4] = sum;
        }
    }
    // mvp = proj * temp
    for (int col = 0; col < 4; col++) {
        for (int row = 0; row < 4; row++) {
            float sum = 0.0f;
            for (int k = 0; k < 4; k++) {
                sum += proj->m[row + k*4] * temp.m[k + col*4];
            }
            mvp->m[row + col*4] = sum;
        }
    }
}

// Scalar vertex transform
void transform_vertices_scalar(Vec3f* output, const Mat4* mvp, const Vec3f* input, int count) {
    for (int i = 0; i < count; i++) {
        output[i].x = mvp->m[0]*input[i].x + mvp->m[4]*input[i].y + mvp->m[8]*input[i].z + mvp->m[12];
        output[i].y = mvp->m[1]*input[i].x + mvp->m[5]*input[i].y + mvp->m[9]*input[i].z + mvp->m[13];
        output[i].z = mvp->m[2]*input[i].x + mvp->m[6]*input[i].y + mvp->m[10]*input[i].z + mvp->m[14];
    }
}

int main(void) {
    printf("==============================================================\n");
    printf("  REAL-WORLD Game Engine Benchmark\n");
    printf("  Simulating 60 FPS @ 100 objects with 1000 vertices each\n");
    printf("==============================================================\n\n");

    // Setup realistic matrices
    Mat4 projection, view;
    create_perspective_matrix(&projection, 1.57f, 16.0f/9.0f, 0.1f, 1000.0f);
    create_view_matrix(&view, 0.0f, 5.0f, -10.0f);

    // Create test mesh (1000 vertices)
    const int VERTEX_COUNT = 1000;
    Vec3f* vertices_in = malloc(VERTEX_COUNT * sizeof(Vec3f));
    Vec3f* vertices_out = malloc(VERTEX_COUNT * sizeof(Vec3f));

    for (int i = 0; i < VERTEX_COUNT; i++) {
        vertices_in[i].x = (float)(i % 10) - 5.0f;
        vertices_in[i].y = (float)((i / 10) % 10) - 5.0f;
        vertices_in[i].z = (float)(i / 100) - 5.0f;
        vertices_in[i]._pad = 0.0f;
    }

    const int NUM_OBJECTS = 100;
    const int FRAMES = 10000;  // Simulate 10000 frames (10x more for better timing)

    printf("=== Scenario 1: MVP Chain (per-object, per-frame) ===\n");
    printf("Computing Model-View-Projection for %d objects × %d frames\n", NUM_OBJECTS, FRAMES);
    printf("Total: %d MVP chains\n\n", NUM_OBJECTS * FRAMES);

    Mat4* models = malloc(NUM_OBJECTS * sizeof(Mat4));
    for (int i = 0; i < NUM_OBJECTS; i++) {
        create_model_matrix(&models[i], (float)(i % 10), (float)(i / 10), (float)i, 1.0f);
    }

    volatile float sink = 0.0f;

    // Benchmark ASM version
    clock_t start_asm = clock();
    for (int frame = 0; frame < FRAMES; frame++) {
        for (int obj = 0; obj < NUM_OBJECTS; obj++) {
            Mat4 mv, mvp;
            fp_mat4_mul(&mv, &view, &models[obj]);
            fp_mat4_mul(&mvp, &projection, &mv);
            sink += mvp.m[0];
        }
    }
    clock_t end_asm = clock();
    double time_asm = (double)(end_asm - start_asm) / CLOCKS_PER_SEC;

    // Benchmark scalar version
    clock_t start_scalar = clock();
    for (int frame = 0; frame < FRAMES; frame++) {
        for (int obj = 0; obj < NUM_OBJECTS; obj++) {
            Mat4 mvp;
            compute_mvp_scalar(&mvp, &projection, &view, &models[obj]);
            sink += mvp.m[0];
        }
    }
    clock_t end_scalar = clock();
    double time_scalar = (double)(end_scalar - start_scalar) / CLOCKS_PER_SEC;

    int total_mvp = NUM_OBJECTS * FRAMES;
    printf("ASM (AVX2):         %.3f seconds (%.1f ns per MVP chain)\n",
           time_asm, (time_asm / total_mvp) * 1e9);
    printf("GCC (-O3 -march=native):   %.3f seconds (%.1f ns per MVP chain)\n",
           time_scalar, (time_scalar / total_mvp) * 1e9);
    printf("Speedup:            %.2fx\n", time_scalar / time_asm);
    printf("FPS if this was the bottleneck (ASM):    %.1f FPS\n", 1.0 / (time_asm / FRAMES));
    printf("FPS if this was the bottleneck (Scalar): %.1f FPS\n\n", 1.0 / (time_scalar / FRAMES));

    // Scenario 2: Vertex Transformation (with anti-optimization)
    printf("=== Scenario 2: Vertex Transformation ===\n");
    printf("Transforming %d vertices per frame\n", VERTEX_COUNT);
    printf("Total frames: %d\n", FRAMES);
    printf("Total: %d vertex transforms\n\n", VERTEX_COUNT * FRAMES);

    Mat4 test_mvp;
    fp_mat4_mul(&test_mvp, &projection, &view);

    // Benchmark ASM vertex transform - BATCHED for world-class performance!
    start_asm = clock();
    for (int frame = 0; frame < FRAMES; frame++) {
        // Transform all vertices in ONE CALL (amortize matrix load cost!)
        fp_mat4_mul_vec3_batch(vertices_out, &test_mvp, vertices_in, VERTEX_COUNT);
        // Create dependency: modify input based on output (prevents optimization)
        vertices_in[0].x += vertices_out[VERTEX_COUNT-1].x * 0.0001f;
    }
    end_asm = clock();
    time_asm = (double)(end_asm - start_asm) / CLOCKS_PER_SEC;
    sink += vertices_out[0].x + vertices_in[0].x;

    // Reset input
    vertices_in[0].x = -5.0f;

    // Benchmark scalar vertex transform - same pattern
    start_scalar = clock();
    for (int frame = 0; frame < FRAMES; frame++) {
        // Transform all vertices
        transform_vertices_scalar(vertices_out, &test_mvp, vertices_in, VERTEX_COUNT);
        // Create dependency: modify input based on output (prevents optimization)
        vertices_in[0].x += vertices_out[VERTEX_COUNT-1].x * 0.0001f;
    }
    end_scalar = clock();
    time_scalar = (double)(end_scalar - start_scalar) / CLOCKS_PER_SEC;
    sink += vertices_out[0].x + vertices_in[0].x;

    int total_verts = VERTEX_COUNT * FRAMES;
    printf("ASM (AVX2):         %.3f seconds (%.2f ns per vertex)\n",
           time_asm, (time_asm / total_verts) * 1e9);
    printf("GCC (-O3 -march=native):   %.3f seconds (%.2f ns per vertex)\n",
           time_scalar, (time_scalar / total_verts) * 1e9);
    printf("Speedup:            %.2fx\n", time_scalar / time_asm);
    printf("Throughput (ASM):   %.1f million vertices/sec\n", (total_verts / time_asm) / 1e6);
    printf("Throughput (Scalar): %.1f million vertices/sec\n\n", (total_verts / time_scalar) / 1e6);

    printf("==============================================================\n");
    printf("  Result: %s\n", (time_scalar / time_asm) > 1.0 ? "AVX2 WINS!" : "Needs optimization");
    printf("==============================================================\n");

    free(vertices_in);
    free(vertices_out);
    free(models);

    return 0;
}
