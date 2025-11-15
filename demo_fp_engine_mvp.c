/**
 * demo_fp_engine_mvp.c
 *
 * A command-line demonstration of the FP-First Engine's core pipeline.
 * This program verifies that all the ported modules can be composed
 * to transform and light a mesh correctly.
 *
 * Pipeline:
 * 1. Create a mesh (fp_mesh_generation)
 * 2. Define scene (transform, camera, light)
 * 3. Generate MVP matrix (fp_transforms)
 * 4. Simulate vertex shader by transforming all vertices.
 * 5. Simulate lighting calculation (fp_lighting)
 * 6. Print results for verification.
 */

#include <stdio.h>
#include <stdlib.h>
#include "fp_mesh_generation.h"
#include "fp_graphics_engine.h"
#include "fp_lighting.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// A pure function to simulate the vertex shader transformation.
void fp_transform_vertices(Vec3f* out_vertices, const FP_Vertex* in_vertices, size_t count, const Mat4* mvp) {
    for (size_t i = 0; i < count; ++i) {
        Vec3f pos_in = {
            .x = in_vertices[i].position[0],
            .y = in_vertices[i].position[1],
            .z = in_vertices[i].position[2],
            ._pad = 1.0f // Use padding for w component
        };
        fp_mat4_mul_vec3(&out_vertices[i], mvp, &pos_in);
    }
}

int main() {
    printf("==============================================\n");
    printf("   FP-First Engine MVP - Core Pipeline Test\n");
    printf("==============================================\n\n");

    // 1. Create a mesh
    printf("[1] Creating mesh...\n");
    FP_MeshData mesh = fp_mesh_create_cube();
    printf("    - Mesh created with %zu vertices.\n\n", mesh.vertex_count);

    // 2. Define the scene
    printf("[2] Defining scene...\n");
    Transform object_transform = {
        .position = {0.0f, 0.0f, 0.0f},
        .euler_rotation = {0.0f, 0.0f, 0.0f},
        .scale = {1.0f, 1.0f, 1.0f}
    };
    Camera camera = {
        .transform = { .position = {0.0f, 0.0f, 5.0f} },
        .projection = {
            .fov_radians = M_PI / 2.0f,
            .aspect_ratio = 1.0f,
            .near_plane = 0.1f,
            .far_plane = 100.0f
        }
    };
    DirectionalLight light = {
        .direction = {0.0f, 0.0f, -1.0f}, // Light shining from camera towards origin
        .intensity = 1.0f,
        .color = {1.0f, 1.0f, 1.0f}
    };
    printf("    - Scene defined with 1 object, 1 camera, 1 light.\n\n");

    // 3. Generate all matrices
    printf("[3] Generating MVP matrix...\n");
    Mat4 model_mat, view_mat, proj_mat, mvp_mat;
    fp_transform_to_matrix(&model_mat, &object_transform);
    fp_view_matrix(&view_mat, &camera);
    fp_projection_matrix(&proj_mat, &camera.projection);
    fp_get_mvp_matrix(&mvp_mat, &model_mat, &view_mat, &proj_mat);
    printf("    - MVP matrix generated.\n\n");

    // 4. Simulate Vertex Shader
    printf("[4] Transforming vertices (simulating vertex shader)...");
    Vec3f* transformed_vertices = (Vec3f*)malloc(mesh.vertex_count * sizeof(Vec3f));
    fp_transform_vertices(transformed_vertices, mesh.vertices, mesh.vertex_count, &mvp_mat);
    printf("    - First vertex transformed to clip space: (%.2f, %.2f, %.2f)\n\n",
           transformed_vertices[0].x, transformed_vertices[0].y, transformed_vertices[0].z);

    // 5. Simulate Lighting
    printf("[5] Calculating lighting...\n");
    float* lighting_values = (float*)malloc(mesh.vertex_count * sizeof(float));
    // Create a SIMD-padded array of normals for the lighting function
    Vec3f* normals = (Vec3f*)malloc(mesh.vertex_count * sizeof(Vec3f));
    for(size_t i = 0; i < mesh.vertex_count; ++i) {
        normals[i].x = mesh.vertices[i].normal[0];
        normals[i].y = mesh.vertices[i].normal[1];
        normals[i].z = mesh.vertices[i].normal[2];
        normals[i]._pad = 0.0f;
    }
    fp_lighting_directional(normals, lighting_values, &light, mesh.vertex_count);
    printf("    - Lighting for first vertex (front face, normal=[0,0,1]): %.2f\n\n", lighting_values[0]);

    // Verification
    printf("[6] Verification...\n");
    // The front face of the cube has normal (0,0,1). The light has direction (0,0,-1).
    // The dot product should be -1, which clamps to 0.
    // Let's test a different face. The back face normal is (0,0,-1).
    // The dot product should be 1.
    printf("    - Lighting for back face vertex (normal=[0,0,-1]): %.2f (Expected: 1.00)\n\n", lighting_values[4]);

    // Cleanup
    fp_mesh_free(&mesh);
    free(transformed_vertices);
    free(lighting_values);
    free(normals);

    printf("--- Pipeline Test Complete ---\n");
    return 0;
}
