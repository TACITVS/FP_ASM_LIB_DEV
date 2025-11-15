/**
 * renderer_modern.c
 *
 * Modern OpenGL 3.3+ Renderer Implementation
 * Implements PBR, shadow mapping, and post-processing pipeline
 */

#include "renderer_modern.h"
#include "ecs.h"
#include "gl_extensions.h"
#include "fp_core.h" // For Vec3f, Mat4, and FP_ASM math functions
#include "fp_engine_types.h" // For FP_AppState, FP_Light, FP_Camera, FP_SceneObject
#include "fp_mesh_generation.h" // For FP_MeshData
#include "fp_engine_algorithms.h" // For fp_transform_update_matrix
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

// Helper macro for OpenGL error checking
#define GL_CHECK_ERROR() \
    do { \
        GLenum err; \
        while ((err = glGetError()) != GL_NO_ERROR) { \
            fprintf(stderr, "[OpenGL Error] %s:%d - 0x%x\n", __FILE__, __LINE__, err); \
        } \
    } while (0)

// External shader strings from shaders_embedded.c
extern const char* SHADER_PBR_VERT;
extern const char* SHADER_PBR_FRAG;
extern const char* SHADER_SHADOW_VERT;
extern const char* SHADER_SHADOW_FRAG;
extern const char* SHADER_QUAD_VERT;
extern const char* SHADER_SSAO_FRAG;
extern const char* SHADER_SSAO_COMPOSITE_FRAG;
extern const char* SHADER_POSTPROCESS_FRAG;

// OpenGL 3.0+ texture format constants (may not be in older gl.h)
#ifndef GL_RGB16F
#define GL_RGB16F 0x881B
#endif
#ifndef GL_R16F
#define GL_R16F 0x822D
#endif

//==============================================================================
// Shader System Implementation
//==============================================================================

static bool shader_compile(GLuint shader, const char* source, const char* shader_type) {
    glShaderSource(shader, 1, &source, NULL);
    GL_CHECK_ERROR();
    glCompileShader(shader);
    GL_CHECK_ERROR();

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar info_log[1024];
        glGetShaderInfoLog(shader, sizeof(info_log), NULL, info_log);
        fprintf(stderr, "[ERROR] %s shader compilation failed:\n%s\n", shader_type, info_log);
        return false;
    }
    return true;
}

static bool shader_link(GLuint program) {
    glLinkProgram(program);
    GL_CHECK_ERROR();

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar info_log[1024];
        glGetProgramInfoLog(program, sizeof(info_log), NULL, info_log);
        fprintf(stderr, "[ERROR] Shader program linking failed:\n%s\n", info_log);
        return false;
    }
    return true;
}

Shader* shader_create_from_source(const char* vertex_src, const char* fragment_src) {
    Shader* shader = (Shader*)malloc(sizeof(Shader));
    if (!shader) {
        fprintf(stderr, "[ERROR] Failed to allocate shader\n");
        return NULL;
    }

    // Create shaders
    shader->vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    GL_CHECK_ERROR();
    shader->fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    GL_CHECK_ERROR();

    // Compile vertex shader
    if (!shader_compile(shader->vertex_shader, vertex_src, "Vertex")) {
        glDeleteShader(shader->vertex_shader);
        glDeleteShader(shader->fragment_shader);
        free(shader);
        return NULL;
    }

    // Compile fragment shader
    if (!shader_compile(shader->fragment_shader, fragment_src, "Fragment")) {
        glDeleteShader(shader->vertex_shader);
        glDeleteShader(shader->fragment_shader);
        free(shader);
        return NULL;
    }

    // Create and link program
    shader->program = glCreateProgram();
    GL_CHECK_ERROR();
    glAttachShader(shader->program, shader->vertex_shader);
    GL_CHECK_ERROR();
    glAttachShader(shader->program, shader->fragment_shader);
    GL_CHECK_ERROR();

    if (!shader_link(shader->program)) {
        glDeleteShader(shader->vertex_shader);
        glDeleteShader(shader->fragment_shader);
        glDeleteProgram(shader->program);
        free(shader);
        return NULL;
    }

    // Cache common uniform locations
    shader->u_model = glGetUniformLocation(shader->program, "uModel");
    shader->u_view = glGetUniformLocation(shader->program, "uView");
    shader->u_projection = glGetUniformLocation(shader->program, "uProjection");
    shader->u_view_pos = glGetUniformLocation(shader->program, "uViewPos");
    shader->u_albedo = glGetUniformLocation(shader->program, "uAlbedo");
    shader->u_metallic = glGetUniformLocation(shader->program, "uMetallic");
    shader->u_roughness = glGetUniformLocation(shader->program, "uRoughness");
    shader->u_ao = glGetUniformLocation(shader->program, "uAO");
    shader->u_shadow_map = glGetUniformLocation(shader->program, "uShadowMap");
    shader->u_light_space_matrix = glGetUniformLocation(shader->program, "uLightSpaceMatrix");
    shader->u_screen_texture = glGetUniformLocation(shader->program, "uSceneTexture");

    printf("[INFO] Shader created successfully (Program ID: %u)\n", shader->program);
    return shader;
}

Shader* shader_create_from_files(const char* vertex_path, const char* fragment_path) {
    // Read vertex shader file
    FILE* vf = fopen(vertex_path, "r");
    if (!vf) {
        fprintf(stderr, "[ERROR] Failed to open vertex shader: %s\n", vertex_path);
        return NULL;
    }
    fseek(vf, 0, SEEK_END);
    long vlen = ftell(vf);
    fseek(vf, 0, SEEK_SET);
    char* vsrc = (char*)malloc(vlen + 1);
    fread(vsrc, 1, vlen, vf);
    vsrc[vlen] = '\0';
    fclose(vf);

    // Read fragment shader file
    FILE* ff = fopen(fragment_path, "r");
    if (!ff) {
        fprintf(stderr, "[ERROR] Failed to open fragment shader: %s\n", fragment_path);
        free(vsrc);
        return NULL;
    }
    fseek(ff, 0, SEEK_END);
    long flen = ftell(ff);
    fseek(ff, 0, SEEK_SET);
    char* fsrc = (char*)malloc(flen + 1);
    fread(fsrc, 1, flen, ff);
    fsrc[flen] = '\0';
    fclose(ff);

    Shader* shader = shader_create_from_source(vsrc, fsrc);
    free(vsrc);
    free(fsrc);
    return shader;
}

void shader_destroy(Shader* shader) {
    if (!shader) return;
    glDeleteShader(shader->vertex_shader);
    glDeleteShader(shader->fragment_shader);
    glDeleteProgram(shader->program);
    free(shader);
}

void shader_bind(Shader* shader) {
    if (shader) {
        glUseProgram(shader->program);
        GL_CHECK_ERROR();
    }
}

void shader_set_int(Shader* shader, const char* name, int value) {
    GLint location = glGetUniformLocation(shader->program, name);
    glUniform1i(location, value);
    GL_CHECK_ERROR();
}

void shader_set_float(Shader* shader, const char* name, float value) {
    GLint location = glGetUniformLocation(shader->program, name);
    glUniform1f(location, value);
    GL_CHECK_ERROR();
}

void shader_set_vec3(Shader* shader, const char* name, Vec3f value) {
    GLint location = glGetUniformLocation(shader->program, name);
    glUniform3f(location, value.x, value.y, value.z);
    GL_CHECK_ERROR();
}

void shader_set_mat4(Shader* shader, const char* name, const Mat4* value) {
    GLint location = glGetUniformLocation(shader->program, name);
    glUniformMatrix4fv(location, 1, GL_FALSE, value->m);
    GL_CHECK_ERROR();
}

// Helper function to print a Mat4 for debugging
static void print_mat4(const char* name, const Mat4* mat) {
    printf("Matrix: %s\n", name);
    for (int i = 0; i < 4; ++i) {
        printf("  %.2f %.2f %.2f %.2f\n", mat->m[i*4+0], mat->m[i*4+1], mat->m[i*4+2], mat->m[i*4+3]);
    }
}

//==============================================================================
// Framebuffer System Implementation
//==============================================================================

Framebuffer* framebuffer_create(int width, int height, FramebufferAttachment attachment) {
    Framebuffer* fb = (Framebuffer*)malloc(sizeof(Framebuffer));
    if (!fb) {
        fprintf(stderr, "[ERROR] Failed to allocate framebuffer\n");
        return NULL;
    }

    fb->width = width;
    fb->height = height;
    fb->color_texture = 0;
    fb->depth_texture = 0;

    // Create framebuffer object
    glGenFramebuffers(1, &fb->fbo);
    GL_CHECK_ERROR();
    glBindFramebuffer(GL_FRAMEBUFFER, fb->fbo);
    GL_CHECK_ERROR();

    // Create color attachment if requested
    if (attachment == FB_ATTACHMENT_COLOR || attachment == FB_ATTACHMENT_COLOR_DEPTH) {
        glGenTextures(1, &fb->color_texture);
        GL_CHECK_ERROR();
        glBindTexture(GL_TEXTURE_2D, fb->color_texture);
        GL_CHECK_ERROR();
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        GL_CHECK_ERROR();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        GL_CHECK_ERROR();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        GL_CHECK_ERROR();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        GL_CHECK_ERROR();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        GL_CHECK_ERROR();
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fb->color_texture, 0);
        GL_CHECK_ERROR();
    }

    // Create depth attachment if requested
    if (attachment == FB_ATTACHMENT_DEPTH || attachment == FB_ATTACHMENT_COLOR_DEPTH) {
        glGenTextures(1, &fb->depth_texture);
        GL_CHECK_ERROR();
        glBindTexture(GL_TEXTURE_2D, fb->depth_texture);
        GL_CHECK_ERROR();
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        GL_CHECK_ERROR();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        GL_CHECK_ERROR();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        GL_CHECK_ERROR();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        GL_CHECK_ERROR();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        GL_CHECK_ERROR();
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, fb->depth_texture, 0);
        GL_CHECK_ERROR();
    }

    // Check framebuffer completeness
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "[ERROR] Framebuffer incomplete (status: 0x%X)\n", status);
        framebuffer_destroy(fb);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return NULL;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    printf("[INFO] Framebuffer created (%dx%d, FBO ID: %u)\n", width, height, fb->fbo);
    return fb;
}

void framebuffer_destroy(Framebuffer* fb) {
    if (!fb) return;
    if (fb->color_texture) glDeleteTextures(1, &fb->color_texture);
    if (fb->depth_texture) glDeleteTextures(1, &fb->depth_texture);
    glDeleteFramebuffers(1, &fb->fbo);
    free(fb);
}

void framebuffer_bind(Framebuffer* fb) {
    if (fb) {
        glBindFramebuffer(GL_FRAMEBUFFER, fb->fbo);
        GL_CHECK_ERROR();
        glViewport(0, 0, fb->width, fb->height);
        GL_CHECK_ERROR();
    }
}

void framebuffer_unbind(void) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    GL_CHECK_ERROR();
}

//==============================================================================
// Mesh Management Implementation
//==============================================================================

void mesh_upload(MeshComponent* mesh, Vertex* vertices, uint32_t vertex_count,
                uint32_t* indices, uint32_t index_count) {
    mesh->vertex_count = vertex_count;
    mesh->index_count = index_count;
    mesh->indexed = (indices != NULL && index_count > 0);

    // Generate VAO
    glGenVertexArrays(1, &mesh->vao);
    GL_CHECK_ERROR();
    glBindVertexArray(mesh->vao);
    GL_CHECK_ERROR();

    // Generate and upload VBO
    glGenBuffers(1, &mesh->vbo);
    GL_CHECK_ERROR();
    glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
    GL_CHECK_ERROR();
    glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(Vertex), vertices, GL_STATIC_DRAW);
    GL_CHECK_ERROR();

    // Setup vertex attributes
    // Position (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    GL_CHECK_ERROR();
    glEnableVertexAttribArray(0);
    GL_CHECK_ERROR();

    // Normal (location 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));
    GL_CHECK_ERROR();
    glEnableVertexAttribArray(1);
    GL_CHECK_ERROR();

    // UV (location 2)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(6 * sizeof(float)));
    GL_CHECK_ERROR();
    glEnableVertexAttribArray(2);
    GL_CHECK_ERROR();

    // Tangent (location 3)
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(8 * sizeof(float)));
    GL_CHECK_ERROR();
    glEnableVertexAttribArray(3);
    GL_CHECK_ERROR();

    // Generate and upload EBO if using indices
    if (mesh->indexed) {
        glGenBuffers(1, &mesh->ebo);
        GL_CHECK_ERROR();
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);
        GL_CHECK_ERROR();
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_count * sizeof(uint32_t), indices, GL_STATIC_DRAW);
        GL_CHECK_ERROR();
    } else {
        mesh->ebo = 0;
    }

    glBindVertexArray(0);
    GL_CHECK_ERROR();
}

void mesh_destroy(MeshComponent* mesh) {
    if (!mesh) return;
    if (mesh->vao) glDeleteVertexArrays(1, &mesh->vao);
    if (mesh->vbo) glDeleteBuffers(1, &mesh->vbo);
    if (mesh->ebo) glDeleteBuffers(1, &mesh->ebo);
    mesh->vao = mesh->vbo = mesh->ebo = 0;
}

MeshComponent mesh_create_cube(void) {
    MeshComponent mesh = {0};

    // Cube vertices with normals and UVs
    Vertex vertices[24] = {
        // Front face (+Z)
        {{-0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
        {{ 0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
        {{ 0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
        {{-0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},

        // Back face (-Z)
        {{ 0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}},
        {{-0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}},
        {{-0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}},
        {{ 0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}},

        // Top face (+Y)
        {{-0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
        {{ 0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
        {{ 0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
        {{-0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},

        // Bottom face (-Y)
        {{-0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
        {{ 0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
        {{ 0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
        {{-0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},

        // Right face (+X)
        {{ 0.5f, -0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
        {{ 0.5f, -0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
        {{ 0.5f,  0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}},
        {{ 0.5f,  0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}},

        // Left face (-X)
        {{-0.5f, -0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f, -0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f,  0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f,  0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
    };

    uint32_t indices[36] = {
        0, 1, 2, 2, 3, 0,       // Front
        4, 5, 6, 6, 7, 4,       // Back
        8, 9, 10, 10, 11, 8,    // Top
        12, 13, 14, 14, 15, 12, // Bottom
        16, 17, 18, 18, 19, 16, // Right
        20, 21, 22, 22, 23, 20  // Left
    };

    mesh_upload(&mesh, vertices, 24, indices, 36);
    return mesh;
}

MeshComponent mesh_create_sphere(uint32_t segments) {
    MeshComponent mesh = {0};

    if (segments < 4) segments = 4;
    if (segments > 128) segments = 128;

    uint32_t vertex_count = (segments + 1) * (segments + 1);
    uint32_t index_count = segments * segments * 6;

    Vertex* vertices = (Vertex*)malloc(vertex_count * sizeof(Vertex));
    uint32_t* indices = (uint32_t*)malloc(index_count * sizeof(uint32_t));

    // Generate sphere vertices
    uint32_t idx = 0;
    for (uint32_t y = 0; y <= segments; y++) {
        for (uint32_t x = 0; x <= segments; x++) {
            float xSegment = (float)x / (float)segments;
            float ySegment = (float)y / (float)segments;
            float xPos = cosf(xSegment * 2.0f * 3.14159265359f) * sinf(ySegment * 3.14159265359f);
            float yPos = cosf(ySegment * 3.14159265359f);
            float zPos = sinf(xSegment * 2.0f * 3.14159265359f) * sinf(ySegment * 3.14159265359f);

            vertices[idx].position[0] = xPos * 0.5f;
            vertices[idx].position[1] = yPos * 0.5f;
            vertices[idx].position[2] = zPos * 0.5f;
            vertices[idx].normal[0] = xPos;
            vertices[idx].normal[1] = yPos;
            vertices[idx].normal[2] = zPos;
            vertices[idx].uv[0] = xSegment;
            vertices[idx].uv[1] = ySegment;
            vertices[idx].tangent[0] = -sinf(xSegment * 2.0f * 3.14159265359f);
            vertices[idx].tangent[1] = 0.0f;
            vertices[idx].tangent[2] = cosf(xSegment * 2.0f * 3.14159265359f);
            idx++;
        }
    }

    // Generate sphere indices
    idx = 0;
    for (uint32_t y = 0; y < segments; y++) {
        for (uint32_t x = 0; x < segments; x++) {
            indices[idx++] = y * (segments + 1) + x;
            indices[idx++] = (y + 1) * (segments + 1) + x;
            indices[idx++] = (y + 1) * (segments + 1) + x + 1;
            indices[idx++] = y * (segments + 1) + x;
            indices[idx++] = (y + 1) * (segments + 1) + x + 1;
            indices[idx++] = y * (segments + 1) + x + 1;
        }
    }

    mesh_upload(&mesh, vertices, vertex_count, indices, index_count);
    free(vertices);
    free(indices);
    return mesh;
}

MeshComponent mesh_create_plane(float size) {
    MeshComponent mesh = {0};

    float half = size * 0.5f;
    Vertex vertices[4] = {
        {{-half, 0.0f, -half}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
        {{ half, 0.0f, -half}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
        {{ half, 0.0f,  half}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
        {{-half, 0.0f,  half}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
    };

    uint32_t indices[6] = {0, 1, 2, 2, 3, 0};

    mesh_upload(&mesh, vertices, 4, indices, 6);
    return mesh;
}

//==============================================================================
// Fullscreen Quad for Post-Processing
//==============================================================================

static void create_fullscreen_quad(GLuint* vao, GLuint* vbo) {
    // NDC quad vertices with UVs
    float quad_vertices[] = {
        // Positions   // UVs
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };

    glGenVertexArrays(1, vao);
    glGenBuffers(1, vbo);

    glBindVertexArray(*vao);
    glBindBuffer(GL_ARRAY_BUFFER, *vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // UV attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

//==============================================================================
// Renderer Initialization
//==============================================================================

Renderer* renderer_create(RendererConfig config) {
    printf("[INFO] Creating renderer...\n");

    // Load OpenGL extensions
    if (!gl_load_extensions()) {
        fprintf(stderr, "[ERROR] Failed to load OpenGL extensions\n");
        return NULL;
    }

    Renderer* renderer = (Renderer*)calloc(1, sizeof(Renderer));
    if (!renderer) {
        fprintf(stderr, "[ERROR] Failed to allocate renderer\n");
        return NULL;
    }

    renderer->config = config;

    // Create shaders
    printf("[INFO] Creating PBR shader...\n");
    renderer->pbr_shader = shader_create_from_source(SHADER_PBR_VERT, SHADER_PBR_FRAG);
    GL_CHECK_ERROR();
    if (!renderer->pbr_shader) {
        fprintf(stderr, "[ERROR] Failed to create PBR shader\n");
        renderer_destroy(renderer);
        return NULL;
    }

    printf("[INFO] Creating shadow shader...\n");
    renderer->shadow_shader = shader_create_from_source(SHADER_SHADOW_VERT, SHADER_SHADOW_FRAG);
    GL_CHECK_ERROR();
    if (!renderer->shadow_shader) {
        fprintf(stderr, "[ERROR] Failed to create shadow shader\n");
        renderer_destroy(renderer);
        return NULL;
    }

    printf("[INFO] Creating post-process shader...\n");
    renderer->postprocess_shader = shader_create_from_source(SHADER_QUAD_VERT, SHADER_POSTPROCESS_FRAG);
    GL_CHECK_ERROR();
    if (!renderer->postprocess_shader) {
        fprintf(stderr, "[ERROR] Failed to create post-process shader\n");
        renderer_destroy(renderer);
        return NULL;
    }

    // Create framebuffers
    if (config.enable_shadows) {
        printf("[INFO] Creating shadow maps (%d x %dx%d)...\n",
               config.max_lights, config.shadow_resolution, config.shadow_resolution);
        renderer->shadow_map_count = config.max_lights;
        renderer->shadow_maps = (Framebuffer**)malloc(config.max_lights * sizeof(Framebuffer*));
        for (int i = 0; i < config.max_lights; i++) {
            renderer->shadow_maps[i] = framebuffer_create(
                config.shadow_resolution,
                config.shadow_resolution,
                FB_ATTACHMENT_DEPTH
            );
            if (!renderer->shadow_maps[i]) {
                fprintf(stderr, "[ERROR] Failed to create shadow map %d\n", i);
                renderer_destroy(renderer);
                return NULL;
            }
        }
    }

    printf("[INFO] Creating HDR framebuffer (%dx%d)...\n", config.window_width, config.window_height);
    renderer->hdr_buffer = framebuffer_create(
        config.window_width,
        config.window_height,
        FB_ATTACHMENT_COLOR_DEPTH
    );
    if (!renderer->hdr_buffer) {
        fprintf(stderr, "[ERROR] Failed to create HDR framebuffer\n");
        renderer_destroy(renderer);
        return NULL;
    }

    // Create fullscreen quad for post-processing
    printf("[INFO] Creating fullscreen quad...\n");
    create_fullscreen_quad(&renderer->quad_vao, &renderer->quad_vbo);
    GL_CHECK_ERROR();

    printf("[INFO] Renderer initialized successfully\n");
    return renderer;
}

void renderer_destroy(Renderer* renderer) {
    if (!renderer) return;

    // Destroy shaders
    shader_destroy(renderer->pbr_shader);
    shader_destroy(renderer->shadow_shader);
    shader_destroy(renderer->postprocess_shader);

    // Destroy framebuffers
    if (renderer->shadow_maps) {
        for (uint32_t i = 0; i < renderer->shadow_map_count; i++) {
            framebuffer_destroy(renderer->shadow_maps[i]);
        }
        free(renderer->shadow_maps);
    }
    framebuffer_destroy(renderer->hdr_buffer);

    // Destroy fullscreen quad
    if (renderer->quad_vao) glDeleteVertexArrays(1, &renderer->quad_vao);
    if (renderer->quad_vbo) glDeleteBuffers(1, &renderer->quad_vbo);

    free(renderer);
    printf("[INFO] Renderer destroyed\n");
}

//==============================================================================
// Render Pass: Shadow Map
//==============================================================================

static void render_shadow_pass(Renderer* renderer, FP_AppState* app_state,
                               const FP_Light* light, Framebuffer* shadow_fb) {
    // Bind shadow framebuffer
    framebuffer_bind(shadow_fb);
    glClear(GL_DEPTH_BUFFER_BIT);
    GL_CHECK_ERROR();

    // Use shadow shader
    shader_bind(renderer->shadow_shader);

    // Set light space matrix
    shader_set_mat4(renderer->shadow_shader, "uLightSpaceMatrix", &light->shadow_matrix);

    for (uint32_t i = 0; i < app_state->object_count; i++) {
        FP_SceneObject* obj = &app_state->objects[i];

        // Only render objects that have a mesh
        if (!obj->mesh) continue;

        // Set model matrix
        shader_set_mat4(renderer->shadow_shader, "uModel", &obj->transform.world_matrix);

        // Draw mesh
        glBindVertexArray(obj->mesh->vao);
        GL_CHECK_ERROR();
        if (obj->mesh->indexed) {
            glDrawElements(GL_TRIANGLES, obj->mesh->index_count, GL_UNSIGNED_INT, 0);
            GL_CHECK_ERROR();
        } else {
            glDrawArrays(GL_TRIANGLES, 0, obj->mesh->vertex_count);
            GL_CHECK_ERROR();
        }
        glBindVertexArray(0);
        GL_CHECK_ERROR();

        renderer->draw_calls++;
        renderer->triangles += obj->mesh->indexed ? obj->mesh->index_count / 3 : obj->mesh->vertex_count / 3;
    }

    framebuffer_unbind();
}

//==============================================================================
// Render Pass: Geometry (PBR)
//==============================================================================

static void render_geometry_pass(Renderer* renderer, FP_AppState* app_state,
                                 FP_Camera* camera, Vec3f view_pos) {
    // Bind HDR framebuffer
    framebuffer_bind(renderer->hdr_buffer);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    GL_CHECK_ERROR();

    // Use PBR shader
    shader_bind(renderer->pbr_shader);

    // Set camera matrices
    shader_set_mat4(renderer->pbr_shader, "uView", &camera->view_matrix);
    shader_set_mat4(renderer->pbr_shader, "uProjection", &camera->projection_matrix);
    shader_set_vec3(renderer->pbr_shader, "uViewPos", view_pos);

    // Debug prints for matrices
    printf("--- Frame Matrices ---\n");
    print_mat4("View Matrix", &camera->view_matrix);
    print_mat4("Projection Matrix", &camera->projection_matrix);
    printf("----------------------\n");

    // Get first light (MVP: single light support)
    FP_Light* light = NULL;
    Vec3f light_direction = {0.0f, -1.0f, 0.0f, 0.0f}; // Default downward direction
    Vec3f light_color = {1.0f, 1.0f, 1.0f, 0.0f};

    if (app_state->light_count > 0) {
        light = &app_state->lights[0];

        // For directional light, position is direction
        Vec3f default_forward_ecs = {0.0f, 0.0f, -1.0f, 0.0f}; // Assuming light's forward is -Z
        Vec3f rotated_vec;
        // Assuming light->position is actually the direction for directional lights
        // For now, we'll use a fixed rotation for the light direction
        // In a full ECS, light would have a transform component
        // For this MVP, we'll derive direction from light->position and light->target
        Vec3f light_dir_vec;
        vec3_sub(&light_dir_vec, &light->target, &light->position);
        vec3_normalize(&light_direction, &light_dir_vec);

        light_color.x = light->color.x;
        light_color.y = light->color.y;
        light_color.z = light->color.z;
    }

    shader_set_vec3(renderer->pbr_shader, "uLightDir", light_direction);
    shader_set_vec3(renderer->pbr_shader, "uLightColor", light_color);

    // Bind shadow map if available
    if (renderer->config.enable_shadows && light && renderer->shadow_map_count > 0) {
        glActiveTexture(GL_TEXTURE0);
        GL_CHECK_ERROR();
        glBindTexture(GL_TEXTURE_2D, renderer->shadow_maps[0]->depth_texture);
        GL_CHECK_ERROR();
        shader_set_int(renderer->pbr_shader, "uShadowMap", 0);
        shader_set_mat4(renderer->pbr_shader, "uLightSpaceMatrix", &light->shadow_matrix);
    }

    // Render all meshes
    for (uint32_t i = 0; i < app_state->object_count; i++) {
        FP_SceneObject* obj = &app_state->objects[i];

        // Only render objects that have a mesh
        if (!obj->mesh) continue;

        // Set model matrix
        shader_set_mat4(renderer->pbr_shader, "uModel", &obj->transform.world_matrix);
        print_mat4("Model Matrix", &obj->transform.world_matrix); // Debug print for model matrix

        // Set material properties
        shader_set_vec3(renderer->pbr_shader, "uAlbedo", obj->material.albedo);
        shader_set_float(renderer->pbr_shader, "uMetallic", obj->material.metallic);
        shader_set_float(renderer->pbr_shader, "uRoughness", obj->material.roughness);
        shader_set_float(renderer->pbr_shader, "uAO", obj->material.ao);

        // Draw mesh
        glBindVertexArray(obj->mesh->vao);
        GL_CHECK_ERROR();
        if (obj->mesh->indexed) {
            glDrawElements(GL_TRIANGLES, obj->mesh->index_count, GL_UNSIGNED_INT, 0);
            GL_CHECK_ERROR();
        } else {
            glDrawArrays(GL_TRIANGLES, 0, obj->mesh->vertex_count);
            GL_CHECK_ERROR();
        }
        glBindVertexArray(0);
        GL_CHECK_ERROR();

        renderer->draw_calls++;
        renderer->triangles += obj->mesh->indexed ? obj->mesh->index_count / 3 : obj->mesh->vertex_count / 3;
    }

    framebuffer_unbind();
}

//==============================================================================
// Render Pass: Post-Processing
//==============================================================================

static void render_postprocess_pass(Renderer* renderer) {
    // Render to default framebuffer
    glViewport(0, 0, renderer->config.window_width, renderer->config.window_height);
    GL_CHECK_ERROR();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    GL_CHECK_ERROR();

    // Use post-process shader
    shader_bind(renderer->postprocess_shader);

    // Bind HDR texture
    glActiveTexture(GL_TEXTURE0);
    GL_CHECK_ERROR();
    glBindTexture(GL_TEXTURE_2D, renderer->hdr_buffer->color_texture);
    GL_CHECK_ERROR();
    shader_set_int(renderer->postprocess_shader, "uSceneTexture", 0);
    shader_set_float(renderer->postprocess_shader, "uExposure", 1.0f);

    // Draw fullscreen quad
    glBindVertexArray(renderer->quad_vao);
    GL_CHECK_ERROR();
    glDrawArrays(GL_TRIANGLES, 0, 6);
    GL_CHECK_ERROR();
    glBindVertexArray(0);
    GL_CHECK_ERROR();
}

//==============================================================================
// Main Render Loop
//==============================================================================

void renderer_begin_frame(Renderer* renderer) {
    renderer->draw_calls = 0;
    renderer->triangles = 0;
}

void renderer_end_frame(Renderer* renderer) {
    // Stats are already accumulated during rendering
}

void renderer_render_world(Renderer* renderer, FP_AppState* app_state) {
    if (!renderer || !app_state) return;

    // Update all dirty transform matrices
    for (uint32_t i = 0; i < app_state->object_count; i++) {
        FP_SceneObject* obj = &app_state->objects[i];
        if (obj->transform.dirty) {
            // This transform_update_matrix function needs to be updated to take FP_Transform*
            // For now, we'll assume it works with the FP_Transform struct directly
            // and update its local_matrix.
            // NOTE: This is a temporary workaround. Ideally, transform_update_matrix
            // should be a pure function that takes FP_Transform and returns a new one.
            // Or, it should be part of the FP_Engine_Algorithms.
            // For now, we're modifying the object in place.
            fp_transform_update_matrix(&obj->transform);
            // Assuming world_matrix is the same as local_matrix if no hierarchy
            obj->transform.world_matrix = obj->transform.local_matrix;
        }
    }

    // Get active camera
    FP_Camera* camera = &app_state->camera;
    Vec3f view_pos = app_state->camera.position;

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    GL_CHECK_ERROR();
    glDepthFunc(GL_LESS);
    GL_CHECK_ERROR();

    // 1. Shadow pass: Render depth from each light's perspective
    // if (renderer->config.enable_shadows) {
    //     for (uint32_t i = 0; i < app_state->light_count && i < renderer->shadow_map_count; i++) {
    //         const FP_Light* light = &app_state->lights[i];

    //         if (light->type == FP_LIGHT_DIRECTIONAL) { // Only directional lights cast shadows for now
    //             render_shadow_pass(renderer, app_state, light, renderer->shadow_maps[i]);
    //         }
    //     }
    // }

    // 2. Geometry pass: Render scene with PBR
    render_geometry_pass(renderer, app_state, camera, view_pos);

    // 3. Post-process pass: FXAA + tone mapping
    // if (renderer->config.enable_fxaa) {
    //     render_postprocess_pass(renderer);
    // } else {
        // Just blit HDR buffer to screen without post-processing
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        GL_CHECK_ERROR();
        glViewport(0, 0, renderer->config.window_width, renderer->config.window_height);
        GL_CHECK_ERROR();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        GL_CHECK_ERROR();

        // Simple blit (would need a simple pass-through shader in production)
        // For now, still use postprocess shader but that's fine
        render_postprocess_pass(renderer);
    // }
}

void renderer_get_stats(Renderer* renderer, uint32_t* draw_calls, uint32_t* triangles, float* frame_time) {
    if (!renderer) return;
    if (draw_calls) *draw_calls = renderer->draw_calls;
    if (triangles) *triangles = renderer->triangles;
    if (frame_time) *frame_time = renderer->frame_time;
}

void renderer_reset_stats(Renderer* renderer) {
    if (!renderer) return;
    renderer->draw_calls = 0;
    renderer->triangles = 0;
    renderer->frame_time = 0.0f;
}

//==============================================================================
// SSAO System Implementation (DISABLED FOR TROUBLESHOOTING)
//==============================================================================

/*
/**
 * Generate hemisphere-distributed sample kernel for SSAO
 * FP approach: Pure function that generates deterministic samples
 */
/*
static void ssao_generate_kernel(Vec3f* kernel, int sample_count) {
    for (int i = 0; i < sample_count; ++i) {
        // Generate random sample in hemisphere
        float scale = (float)i / (float)sample_count;

        // Lerp to concentrate samples near origin (more realistic AO)
        scale = 0.1f + scale * scale * 0.9f;  // Quadratic falloff

        // Random direction (would use proper random in production)
        // For deterministic behavior, use quasi-random sequence
        float theta = 2.0f * 3.14159f * ((float)i / (float)sample_count);
        int pseudo_rand = (i * 7 + 13) % sample_count;  // Integer modulo
        float phi = acosf(1.0f - 2.0f * ((float)pseudo_rand / (float)sample_count));

        kernel[i].x = sinf(phi) * cosf(theta) * scale;
        kernel[i].y = sinf(phi) * sinf(theta) * scale;
        kernel[i].z = cosf(phi) * scale;
    }
}
*/
/*
/**
 * Generate 4x4 noise texture for random rotations
 * FP approach: Pure function generating deterministic noise pattern
 */
/*
static GLuint ssao_generate_noise_texture(void) {
    const int NOISE_SIZE = 4;
    Vec3f noise[NOISE_SIZE * NOISE_SIZE];

    for (int i = 0; i < NOISE_SIZE * NOISE_SIZE; ++i) {
        // Random rotation vectors in tangent space
        // Z component is 0 (rotation in XY plane)
        float angle = 2.0f * 3.14159f * ((float)i / (float)(NOISE_SIZE * NOISE_SIZE));
        noise[i].x = cosf(angle);
        noise[i].y = sinf(angle);
        noise[i].z = 0.0f;
    }

    // Create OpenGL texture
    GLuint texture_id;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, NOISE_SIZE, NOISE_SIZE, 0,
                 GL_RGB, GL_FLOAT, noise);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    return texture_id;
}
*/
/*
/**
 * Initialize SSAO system
 * Creates kernel, noise texture, framebuffers, and shaders
 */
/*
void renderer_init_ssao(Renderer* renderer) {
    if (!renderer || !renderer->config.enable_ssao) {
        return;
    }

    printf("[RENDERER] Initializing SSAO (samples: %d, radius: %.2f)...\n",
           renderer->config.ssao_samples,
           renderer->config.ssao_radius);

    // Generate SSAO kernel (hemisphere samples)
    int sample_count = renderer->config.ssao_samples;
    if (sample_count > 64) sample_count = 64;  // Max 64 samples
    if (sample_count < 8) sample_count = 8;    // Min 8 samples

    ssao_generate_kernel(renderer->ssao_kernel, sample_count);

    // Generate noise texture for random rotations
    renderer->ssao_noise_texture = ssao_generate_noise_texture();

    // Create SSAO framebuffer (single-channel R16F)
    renderer->ssao_buffer = (Framebuffer*)malloc(sizeof(Framebuffer));
    renderer->ssao_buffer->width = renderer->config.window_width;
    renderer->ssao_buffer->height = renderer->config.window_height;

    glGenFramebuffers(1, &renderer->ssao_buffer->fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, renderer->ssao_buffer->fbo);

    // Create AO texture (single channel)
    glGenTextures(1, &renderer->ssao_buffer->color_texture);
    glBindTexture(GL_TEXTURE_2D, renderer->ssao_buffer->color_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F,
                 renderer->config.window_width,
                 renderer->config.window_height,
                 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                          GL_TEXTURE_2D, renderer->ssao_buffer->color_texture, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "[ERROR] SSAO framebuffer not complete!\n");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Create SSAO shader
    renderer->ssao_shader = shader_create_from_source(SHADER_QUAD_VERT, SHADER_SSAO_FRAG);
    if (!renderer->ssao_shader) {
        fprintf(stderr, "[ERROR] Failed to create SSAO shader\n");
        return;
    }

    // Create composite shader (applies AO to scene)
    renderer->composite_shader = shader_create_from_source(SHADER_QUAD_VERT, SHADER_SSAO_COMPOSITE_FRAG);
    if (!renderer->composite_shader) {
        fprintf(stderr, "[ERROR] Failed to create SSAO composite shader\n");
        return;
    }

    printf("[RENDERER] SSAO initialized successfully\n");
}
*/
/*
/**
 * Render SSAO pass
 * Computes ambient occlusion from depth buffer
 */
/*
void renderer_render_ssao_pass(Renderer* renderer, Framebuffer* scene_fb) {
    if (!renderer || !renderer->config.enable_ssao ||
        !renderer->ssao_buffer || !renderer->ssao_shader) {
        return;
    }

    // Bind SSAO framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, renderer->ssao_buffer->fbo);
    glClear(GL_COLOR_BUFFER_BIT);

    // Use SSAO shader
    shader_bind(renderer->ssao_shader);

    // Bind depth texture from scene
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, scene_fb->depth_texture);
    shader_set_int(renderer->ssao_shader, "uDepthTexture", 0);

    // Bind noise texture
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, renderer->ssao_noise_texture);
    shader_set_int(renderer->ssao_shader, "uNoiseTexture", 1);

    // Set SSAO uniforms
    int sample_count = renderer->config.ssao_samples;
    if (sample_count > 64) sample_count = 64;

    for (int i = 0; i < sample_count; ++i) {
        char uniform_name[32];
        snprintf(uniform_name, sizeof(uniform_name), "uSamples[%d]", i);
        shader_set_vec3(renderer->ssao_shader, uniform_name, renderer->ssao_kernel[i]);
    }

    shader_set_float(renderer->ssao_shader, "uRadius", renderer->config.ssao_radius);
    shader_set_float(renderer->ssao_shader, "uBias", 0.025f);

    // Set noise scale (screen size / 4)
    Vec3f noise_scale = {
        (float)renderer->config.window_width / 4.0f,
        (float)renderer->config.window_height / 4.0f,
        0.0f, 0.0f // _pad
    };
    shader_set_vec3(renderer->ssao_shader, "uNoiseScale", noise_scale);

    // TODO: Set projection and view matrices from camera
    // For now, shader will use identity or default

    // Render fullscreen quad
    glBindVertexArray(renderer->quad_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
*/
/*
/**
 * Apply SSAO to scene (composite pass)
 */
/*
void renderer_apply_ssao(Renderer* renderer, Framebuffer* scene_fb) {
    if (!renderer || !renderer->config.enable_ssao ||
        !renderer->composite_shader) {
        return;
    }

    shader_bind(renderer->composite_shader);

    // Bind scene texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, scene_fb->color_texture);
    shader_set_int(renderer->composite_shader, "uSceneTexture", 0);

    // Bind SSAO texture
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, renderer->ssao_buffer->color_texture);
    shader_set_int(renderer->composite_shader, "uSSAOTexture", 1);

    // Set AO strength (1.0 = full effect)
    shader_set_float(renderer->composite_shader, "uSSAOStrength", 1.0f);

    // Render to screen
    glBindVertexArray(renderer->quad_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}
*/
/*
/**
 * Cleanup SSAO resources
 */
/*
void renderer_cleanup_ssao(Renderer* renderer) {
    if (!renderer) return;

    if (renderer->ssao_noise_texture) {
        glDeleteTextures(1, &renderer->ssao_noise_texture);
        renderer->ssao_noise_texture = 0;
    }

    if (renderer->ssao_buffer) {
        framebuffer_destroy(renderer->ssao_buffer);
        renderer->ssao_buffer = NULL;
    }

    if (renderer->ssao_shader) {
        shader_destroy(renderer->ssao_shader);
        renderer->ssao_shader = NULL;
    }

    if (renderer->composite_shader) {
        shader_destroy(renderer->composite_shader);
        renderer->composite_shader = NULL;
    }

    printf("[RENDERER] SSAO cleanup complete\n");
}
*/

//==============================================================================
// OpenGL Extension Loading Wrapper
//==============================================================================

bool renderer_load_gl_extensions(void) {
    return gl_load_extensions();
}
