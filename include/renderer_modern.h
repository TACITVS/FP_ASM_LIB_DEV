/**
 * renderer_modern.h
 *
 * Modern OpenGL 3.3+ Renderer with PBR, Shadows, and Post-Processing
 *
 * Features:
 * - Physically-Based Rendering (PBR) with metallic/roughness workflow
 * - Shadow mapping with PCF (Percentage Closer Filtering) for soft shadows
 * - Screen-Space Ambient Occlusion (SSAO)
 * - Post-processing (FXAA, bloom, tone mapping)
 * - Deferred rendering for multiple lights
 *
 * Render Pipeline:
 * 1. Shadow pass: Render depth from light's POV
 * 2. Geometry pass: Render scene with PBR
 * 3. SSAO pass: Calculate ambient occlusion
 * 4. Lighting pass: Apply lighting with shadows
 * 5. Post-process: FXAA, bloom, tone mapping
 */

#ifndef RENDERER_MODERN_H
#define RENDERER_MODERN_H

#include <stdint.h>
#include <stdbool.h>
#include "fp_engine_types.h" // Include FP_AppState, FP_Light, FP_Camera, FP_SceneObject
#include "ecs.h" // Include ECS for MeshComponent definition

// OpenGL types (platform-independent)
typedef unsigned int GLuint;
typedef int GLint;
typedef unsigned int GLenum;
typedef char GLchar;
typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;

// Forward declarations (structs defined below)
struct Renderer;
struct Shader;
struct Framebuffer;

//==============================================================================
// Renderer Configuration
//==============================================================================

typedef struct {
    int window_width;
    int window_height;
    bool enable_shadows;
    bool enable_ssao;
    bool enable_bloom;
    bool enable_fxaa;
    int shadow_resolution;   // Per-light shadow map size
    int max_lights;          // Maximum number of lights
    float shadow_bias;
    int ssao_samples;
    float ssao_radius;
} RendererConfig;

//==============================================================================
// Shader System
//==============================================================================

typedef struct Shader {
    GLuint program;
    GLuint vertex_shader;
    GLuint fragment_shader;

    // Common uniform locations
    GLint u_model;
    GLint u_view;
    GLint u_projection;
    GLint u_view_pos;

    // PBR material uniforms
    GLint u_albedo;
    GLint u_metallic;
    GLint u_roughness;
    GLint u_ao;

    // Lighting uniforms
    GLint u_light_count;
    GLint u_light_positions;
    GLint u_light_colors;
    GLint u_light_intensities;

    // Shadow uniforms
    GLint u_shadow_map;
    GLint u_light_space_matrix;

    // Post-processing uniforms
    GLint u_screen_texture;
    GLint u_ssao_texture;
} Shader;

Shader* shader_create_from_source(const char* vertex_src, const char* fragment_src);
Shader* shader_create_from_files(const char* vertex_path, const char* fragment_path);
void shader_destroy(Shader* shader);
void shader_bind(Shader* shader);
void shader_set_int(Shader* shader, const char* name, int value);
void shader_set_float(Shader* shader, const char* name, float value);
void shader_set_vec3(Shader* shader, const char* name, Vec3f value);
void shader_set_mat4(Shader* shader, const char* name, const Mat4* value);

//==============================================================================
// Framebuffer System
//==============================================================================

typedef enum {
    FB_ATTACHMENT_COLOR,
    FB_ATTACHMENT_DEPTH,
    FB_ATTACHMENT_COLOR_DEPTH,
} FramebufferAttachment;

typedef struct Framebuffer {
    GLuint fbo;
    GLuint color_texture;
    GLuint depth_texture;
    int width;
    int height;
} Framebuffer;

Framebuffer* framebuffer_create(int width, int height, FramebufferAttachment attachment);
void framebuffer_destroy(Framebuffer* fb);
void framebuffer_bind(Framebuffer* fb);
void framebuffer_unbind(void);

//==============================================================================
// Mesh Management
//==============================================================================

typedef struct {
    float position[3];
    float normal[3];
    float uv[2];
    float tangent[3];
} Vertex;

// Create mesh from vertex data
MeshComponent mesh_create_cube(void);
MeshComponent mesh_create_sphere(uint32_t segments);
MeshComponent mesh_create_plane(float size);
void mesh_destroy(MeshComponent* mesh);

// Upload mesh data to GPU
void mesh_upload(MeshComponent* mesh, Vertex* vertices, uint32_t vertex_count,
                uint32_t* indices, uint32_t index_count);

//==============================================================================
// Renderer System
//==============================================================================

typedef struct {
    // Configuration
    RendererConfig config;

    // Shaders
    Shader* pbr_shader;
    Shader* shadow_shader;
    Shader* ssao_shader;
    Shader* blur_shader;
    Shader* composite_shader;
    Shader* postprocess_shader;

    // Framebuffers
    Framebuffer** shadow_maps;      // One per light
    uint32_t shadow_map_count;
    Framebuffer* geometry_buffer;   // For deferred rendering
    Framebuffer* ssao_buffer;
    Framebuffer* ssao_blur_buffer;
    Framebuffer* hdr_buffer;        // HDR accumulation
    Framebuffer* bloom_buffers[2];  // Ping-pong for bloom

    // Fullscreen quad (for post-processing)
    GLuint quad_vao;
    GLuint quad_vbo;

    // SSAO kernel
    Vec3f ssao_kernel[64];
    GLuint ssao_noise_texture;

    // Stats
    uint32_t draw_calls;
    uint32_t triangles;
    float frame_time;
} Renderer;

//==============================================================================
// Renderer API
//==============================================================================

// Initialization
Renderer* renderer_create(RendererConfig config);
void renderer_destroy(Renderer* renderer);

// Frame rendering
void renderer_begin_frame(Renderer* renderer);
void renderer_end_frame(Renderer* renderer);

// Render the world
void renderer_render_world(Renderer* renderer, FP_AppState* app_state);

// SSAO System
void renderer_init_ssao(Renderer* renderer);
void renderer_render_ssao_pass(Renderer* renderer, Framebuffer* scene_fb);
void renderer_apply_ssao(Renderer* renderer, Framebuffer* scene_fb);
void renderer_cleanup_ssao(Renderer* renderer);

// Statistics
void renderer_get_stats(Renderer* renderer, uint32_t* draw_calls, uint32_t* triangles, float* frame_time);
void renderer_reset_stats(Renderer* renderer);

//==============================================================================
// Default Shaders (Embedded)
//==============================================================================

// PBR vertex shader
extern const char* SHADER_PBR_VERT;

// PBR fragment shader
extern const char* SHADER_PBR_FRAG;

// Shadow mapping vertex shader
extern const char* SHADER_SHADOW_VERT;

// Shadow mapping fragment shader
extern const char* SHADER_SHADOW_FRAG;

// SSAO fragment shader
extern const char* SHADER_SSAO_FRAG;

// SSAO composite fragment shader
extern const char* SHADER_SSAO_COMPOSITE_FRAG;

// Gaussian blur fragment shader
extern const char* SHADER_BLUR_FRAG;

// Post-processing fragment shader
extern const char* SHADER_POSTPROCESS_FRAG;

// Fullscreen quad vertex shader
extern const char* SHADER_QUAD_VERT;

//==============================================================================
// OpenGL Extension Loading (Windows)
//==============================================================================

bool renderer_load_gl_extensions(void);

#endif // RENDERER_MODERN_H
