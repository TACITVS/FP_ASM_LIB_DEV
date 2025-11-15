#ifndef FP_MESH_GENERATION_H
#define FP_MESH_GENERATION_H

#include <stdint.h>
#include <stdbool.h> // For bool type
#include <GL/gl.h> // For GLuint

// Vertex structure (position, normal, uv, tangent)
typedef struct {
    float position[3];
    float normal[3];
    float uv[2];
    float tangent[3];
} FP_Vertex;

// FP_MeshData: Contains CPU-accessible geometry plus optional GL handles
typedef struct FP_MeshData {
    GLuint vao;        // Vertex Array Object (0 if not created)
    GLuint vbo;        // Vertex Buffer Object
    GLuint ebo;        // Element Buffer Object
    FP_Vertex* vertices;   // CPU copy of vertices (Null if not provided)
    uint32_t* indices;     // CPU copy of indices (Null if non-indexed)
    uint32_t vertex_count;
    uint32_t index_count;
    bool indexed;
} FP_MeshData;

// Function to create a cube mesh
FP_MeshData fp_mesh_create_cube();

// Function to create a plane mesh
FP_MeshData fp_mesh_create_plane(float size);

// Function to destroy mesh (free CPU/GL resources)
void fp_mesh_free(FP_MeshData* mesh);
static inline void fp_mesh_destroy(FP_MeshData* mesh) { fp_mesh_free(mesh); }

#endif // FP_MESH_GENERATION_H
