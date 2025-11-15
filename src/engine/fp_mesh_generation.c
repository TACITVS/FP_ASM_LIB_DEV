#include "../../include/fp_mesh_generation.h"
#include "../../include/gl_extensions.h" // Include gl_extensions.h
#include <stdlib.h> // For malloc, free
#include <string.h> // For memcpy

#ifndef FP_MESH_ENABLE_OPENGL
#define FP_MESH_ENABLE_OPENGL 0
#endif

// Define GL_ARRAY_BUFFER, GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW if not defined
#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER 0x8892
#endif
#ifndef GL_ELEMENT_ARRAY_BUFFER
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#endif
#ifndef GL_STATIC_DRAW
#define GL_STATIC_DRAW 0x88E4
#endif
#ifndef GL_VERTEX_ARRAY_BINDING
#define GL_VERTEX_ARRAY_BINDING 0x85B5
#endif
#ifndef GL_ARRAY_BUFFER_BINDING
#define GL_ARRAY_BUFFER_BINDING 0x8894
#endif
#ifndef GL_ELEMENT_ARRAY_BUFFER_BINDING
#define GL_ELEMENT_ARRAY_BUFFER_BINDING 0x8895
#endif

static void fp_mesh_copy_geometry(FP_MeshData* mesh,
                                  const FP_Vertex* vertices,
                                  size_t vertex_count,
                                  const uint32_t* indices,
                                  size_t index_count,
                                  bool indexed) {
    if (vertex_count > 0 && vertices) {
        mesh->vertices = (FP_Vertex*)malloc(vertex_count * sizeof(FP_Vertex));
        if (mesh->vertices) {
            memcpy(mesh->vertices, vertices, vertex_count * sizeof(FP_Vertex));
        }
    }
    mesh->vertex_count = (uint32_t)vertex_count;

    mesh->indexed = indexed && index_count > 0;
    if (mesh->indexed && indices) {
        mesh->indices = (uint32_t*)malloc(index_count * sizeof(uint32_t));
        if (mesh->indices) {
            memcpy(mesh->indices, indices, index_count * sizeof(uint32_t));
        }
        mesh->index_count = (uint32_t)index_count;
    } else {
        mesh->indices = NULL;
        mesh->index_count = 0;
    }
}

FP_MeshData fp_mesh_create_cube() {
    FP_MeshData mesh = {0};

    // Cube vertices with positions, normals, UVs, and tangents
    FP_Vertex vertices[] = {
        // Front face
        {{-0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
        {{ 0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
        {{ 0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
        {{-0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
        // Back face
        {{ 0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}},
        {{-0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}},
        {{-0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}},
        {{ 0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}},
        // Top face
        {{-0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
        {{ 0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
        {{ 0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
        {{-0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
        // Bottom face
        {{-0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
        {{ 0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
        {{ 0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
        {{-0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
        // Right face
        {{ 0.5f, -0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
        {{ 0.5f, -0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
        {{ 0.5f,  0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}},
        {{ 0.5f,  0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}},
        // Left face
        {{-0.5f, -0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f, -0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f,  0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f,  0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
    };
    mesh.vertex_count = 24;

    uint32_t indices[] = {
        0, 1, 2,  2, 3, 0,    // Front
        4, 5, 6,  6, 7, 4,    // Back
        8, 9, 10, 10, 11, 8,  // Top
        12, 13, 14, 14, 15, 12, // Bottom
        16, 17, 18, 18, 19, 16, // Right
        20, 21, 22, 22, 23, 20  // Left
    };
    mesh.index_count = 36;

#if FP_MESH_ENABLE_OPENGL
    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glGenBuffers(1, &mesh.ebo);

    glBindVertexArray(mesh.vao);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(FP_Vertex), (void*)0);
    // Normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(FP_Vertex), (void*)(3 * sizeof(float)));
    // UV
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(FP_Vertex), (void*)(6 * sizeof(float)));
    // Tangent
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(FP_Vertex), (void*)(8 * sizeof(float)));

    glBindVertexArray(0); // Unbind VAO
#endif

    fp_mesh_copy_geometry(&mesh, vertices, sizeof(vertices)/sizeof(vertices[0]),
                          indices, sizeof(indices)/sizeof(indices[0]), true);

    return mesh;
}

FP_MeshData fp_mesh_create_plane(float size) {
    FP_MeshData mesh = {0};

    // Plane vertices with positions, normals, UVs (tangents are not strictly needed for a flat plane but included for consistency)
    FP_Vertex vertices[] = {
        {{-size / 2.0f, 0.0f,  size / 2.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
        {{ size / 2.0f, 0.0f,  size / 2.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
        {{ size / 2.0f, 0.0f, -size / 2.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
        {{-size / 2.0f, 0.0f, -size / 2.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
    };
    mesh.vertex_count = 4;

    uint32_t indices[] = {
        0, 1, 2,
        2, 3, 0
    };
    mesh.index_count = 6;

#if FP_MESH_ENABLE_OPENGL
    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glGenBuffers(1, &mesh.ebo);

    glBindVertexArray(mesh.vao);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(FP_Vertex), (void*)0);
    // Normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(FP_Vertex), (void*)(3 * sizeof(float)));
    // UV
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(FP_Vertex), (void*)(6 * sizeof(float)));
    // Tangent
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(FP_Vertex), (void*)(8 * sizeof(float)));

    glBindVertexArray(0); // Unbind VAO
#endif

    fp_mesh_copy_geometry(&mesh, vertices, sizeof(vertices)/sizeof(vertices[0]),
                          indices, sizeof(indices)/sizeof(indices[0]), true);

    return mesh;
}

void fp_mesh_free(FP_MeshData* mesh) {
    if (!mesh) return;
#if FP_MESH_ENABLE_OPENGL
    if (mesh->vao) {
        glDeleteVertexArrays(1, &mesh->vao);
    }
    if (mesh->vbo) {
        glDeleteBuffers(1, &mesh->vbo);
    }
    if (mesh->ebo) {
        glDeleteBuffers(1, &mesh->ebo);
    }
#endif
    if (mesh->vertices) {
        free(mesh->vertices);
        mesh->vertices = NULL;
    }
    if (mesh->indices) {
        free(mesh->indices);
        mesh->indices = NULL;
    }
    mesh->vao = 0;
    mesh->vbo = 0;
    mesh->ebo = 0;
    mesh->vertex_count = 0;
    mesh->index_count = 0;
    mesh->indexed = false;
}
