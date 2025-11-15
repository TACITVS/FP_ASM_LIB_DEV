#include "fp_mesh_generation.h"
#include <stdio.h>

void print_mesh_stats(const char* name, FP_MeshData* data) {
    if (data && data->vertices && data->indices) {
        printf("Stats for %s:\n", name);
        printf("  - Vertex count: %zu\n", data->vertex_count);
        printf("  - Index count:  %zu\n", data->index_count);
        printf("  - First vertex position: (%.2f, %.2f, %.2f)\n",
               data->vertices[0].position[0],
               data->vertices[0].position[1],
               data->vertices[0].position[2]);
        printf("  - First index: %u\n", data->indices[0]);
        printf("  - Status: OK\n\n");
    } else {
        printf("Stats for %s:\n", name);
        printf("  - Status: FAILED or NOT IMPLEMENTED\n\n");
    }
}

int main() {
    printf("--- Testing FP Mesh Generation ---\\n\n");

    // Test Cube
    FP_MeshData cube_data = fp_mesh_create_cube();
    print_mesh_stats("Cube", &cube_data);
    fp_mesh_free(&cube_data);

    // Test Plane (stub)
    FP_MeshData plane_data = fp_mesh_create_plane(10.0f);
    print_mesh_stats("Plane", &plane_data);
    fp_mesh_free(&plane_data);

    // Test Sphere (stub)
    FP_MeshData sphere_data = fp_mesh_create_sphere(16);
    print_mesh_stats("Sphere", &sphere_data);
    fp_mesh_free(&sphere_data);

    printf("--- Test Complete ---\\n");

    return 0;
}