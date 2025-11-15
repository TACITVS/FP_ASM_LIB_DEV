#ifndef RENDERER_H
#define RENDERER_H

#include "fp_graphics_engine.h"
#include "fp_msaa.h"
#include "fp_shadow_mapping.h"

// Forward-declare the AppState so the renderer knows about it
// without creating a circular dependency.
typedef struct AppState AppState;

typedef struct {
    GLuint lighting_shader;
    GLuint depth_shader;
    GLuint cube_vao, cube_vbo, cube_ebo;
    GLuint plane_vao, plane_vbo, plane_ebo;
    GLint mvp_location;
    GLint model_mat_location;
    GLint normal_mat_location;
    GLint light_dir_location;
    GLint object_color_location;
    GLint texture_sampler_location;
    GLint view_pos_location;
    GLint light_space_matrix_location;
    GLint shadow_map_sampler_location;
    GLint depth_light_space_matrix_location;
    GLuint cube_texture;
    FP_MSAA_Framebuffer msaa_fbo;
    FP_ShadowMap shadow_map;
} OpenGLResources;

OpenGLResources renderer_init(const AppState* state);
void renderer_shutdown(OpenGLResources* res);
void renderer_render(const AppState* state, OpenGLResources* res, HDC hdc);

#endif // RENDERER_H
