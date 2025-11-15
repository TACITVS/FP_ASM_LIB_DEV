#include "renderer.h"
#include "application.h"
#include "fp_core.h"
#include "fp_math.h"
#include "fp_graphics_engine.h"
#include "fp_mesh_generation.h"
#include <stdio.h>
#include <stdlib.h>
#include <windows.h> // For SwapBuffers
#include <stdio.h> // For fprintf, FILE, fopen, fclose
#include <stdlib.h> // For malloc, free

static FILE* g_opengl_error_log_file = NULL;

// Helper macro for OpenGL error checking
#define GL_CHECK_ERROR() \
    do { \
        GLenum err; \
        while ((err = glGetError()) != GL_NO_ERROR) { \
            if (g_opengl_error_log_file) { \
                fprintf(g_opengl_error_log_file, "[OpenGL Error] %s:%d - 0x%x\n", __FILE__, __LINE__, err); \
                fflush(g_opengl_error_log_file); \
            } \
            fprintf(stderr, "[OpenGL Error] %s:%d - 0x%x\n", __FILE__, __LINE__, err); \
        } \
    } while (0)

// Shaders for shadow mapping
const char* depth_vertex_src =
    "#version 330 core\n"
    "layout (location = 0) in vec3 a_pos;\n"
    "uniform mat4 u_light_space_matrix;\n"
    "void main() {\n"
    "   gl_Position = u_light_space_matrix * vec4(a_pos, 1.0);\n"
    "}\n";

const char* depth_fragment_src =
    "#version 330 core\n"
    "void main() { }\n"; // Empty, we only care about depth

// Shaders with Diffuse + Specular Lighting & Shadowing
const char* lighting_vertex_src =
    "#version 330 core\n"
    "layout (location = 0) in vec3 a_pos;\n"
    "void main() {\n"
    "   gl_Position = vec4(0.0, 0.0, 0.0, 1.0); // Fixed position at center of screen\n"
    "}\n";

const char* lighting_fragment_src =
    "#version 330 core\n"
    "in vec3 v_normal;\n"
    "in vec2 v_uv;\n"
    "in vec3 v_world_pos;\n"
    "in vec4 v_pos_in_light_space;\n"
    "out vec4 frag_color;\n"
    "uniform sampler2D u_texture;\n"
    "uniform sampler2D u_shadow_map;\n"
    "uniform vec3 u_light_dir;\n"
    "uniform vec3 u_view_pos;\n"
    "uniform vec3 u_object_color;\n"
    "float calculate_shadow(vec4 pos_in_light_space) {\n"
    "   vec3 proj_coords = pos_in_light_space.xyz / pos_in_light_space.w;\n"
    "   proj_coords = proj_coords * 0.5 + 0.5;\n"
    "   if(proj_coords.z > 1.0) return 0.0;\n"
    "   float closest_depth = texture(u_shadow_map, proj_coords.xy).r;\n"
    "   float current_depth = proj_coords.z;\n"
    "   float bias = 0.005;\n"
    "   float shadow = current_depth - bias > closest_depth  ? 1.0 : 0.0;\n"
    "   return shadow;\n"
    "}\n"
    "void main() {\n"
    "   frag_color = vec4(0.0, 1.0, 0.0, 1.0); // Solid green for debugging\n"
    "}\n";

// Helper function to create a shader program
GLuint create_shader_program(const char* vert_src, const char* frag_src) {
    GLuint v = glCreateShader(GL_VERTEX_SHADER);
    GL_CHECK_ERROR();
    glShaderSource(v, 1, &vert_src, NULL);
    GL_CHECK_ERROR();
    glCompileShader(v);
    GL_CHECK_ERROR();
    GLuint f = glCreateShader(GL_FRAGMENT_SHADER);
    GL_CHECK_ERROR();
    glShaderSource(f, 1, &frag_src, NULL);
    GL_CHECK_ERROR();
    glCompileShader(f);
    GL_CHECK_ERROR();
    GLuint p = glCreateProgram();
    GL_CHECK_ERROR();
    glAttachShader(p, v);
    GL_CHECK_ERROR();
    glAttachShader(p, f);
    GL_CHECK_ERROR();
    glLinkProgram(p);
    GL_CHECK_ERROR();
    glDeleteShader(v);
    GL_CHECK_ERROR();
    glDeleteShader(f);
    GL_CHECK_ERROR();
    return p;
}

// Helper function to create a procedural texture
GLuint create_checkerboard_texture(int width, int height) {
    GLubyte* data = (GLubyte*)malloc(width * height * 3);
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            int c = (((i & 0x8) == 0) ^ ((j & 0x8) == 0)) * 255;
            data[(j * width + i) * 3 + 0] = (GLubyte)c;
            data[(j * width + i) * 3 + 1] = (GLubyte)c;
            data[(j * width + i) * 3 + 2] = (GLubyte)c;
        }
    }

    GLuint texture_id;
    glGenTextures(1, &texture_id);
    GL_CHECK_ERROR();
    glBindTexture(GL_TEXTURE_2D, texture_id);
    GL_CHECK_ERROR();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    GL_CHECK_ERROR();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    GL_CHECK_ERROR();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    GL_CHECK_ERROR();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    GL_CHECK_ERROR();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    GL_CHECK_ERROR();
    glGenerateMipmap(GL_TEXTURE_2D);
    GL_CHECK_ERROR();

    free(data);
    return texture_id;
}

// Helper function to create a procedural bumpy normal map
GLuint create_bumpy_normal_map(int width, int height) {
    GLubyte* data = (GLubyte*)malloc(width * height * 3);
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            // Simple sine wave based bumpiness
            float x_val = (float)i / width * 2.0f * M_PI;
            float y_val = (float)j / height * 2.0f * M_PI;
            float z_val = sinf(x_val * 5.0f) * 0.5f + sinf(y_val * 5.0f) * 0.5f; // -1 to 1 range

            // Convert height to normal vector (simplified for demonstration)
            // Normal map stores (nx, ny, nz) mapped to (0-255)
            // For a simple bump, assume normal is mostly (0,0,1) and perturb x,y
            // This is a very basic procedural normal map, not derived from height map
            Vec3f normal = {
                sinf(x_val * 10.0f) * 0.2f, // Small perturbation in x
                sinf(y_val * 10.0f) * 0.2f, // Small perturbation in y
                1.0f - fabsf(sinf(x_val * 10.0f) * 0.2f) - fabsf(sinf(y_val * 10.0f) * 0.2f) // Z component
            };
            normal = vec3f_normalize(normal);

            data[(j * width + i) * 3 + 0] = (GLubyte)((normal.x + 1.0f) * 0.5f * 255.0f);
            data[(j * width + i) * 3 + 1] = (GLubyte)((normal.y + 1.0f) * 0.5f * 255.0f);
            data[(j * width + i) * 3 + 2] = (GLubyte)((normal.z + 1.0f) * 0.5f * 255.0f);
        }
    }

    GLuint texture_id;
    glGenTextures(1, &texture_id);
    GL_CHECK_ERROR();
    glBindTexture(GL_TEXTURE_2D, texture_id);
    GL_CHECK_ERROR();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    GL_CHECK_ERROR();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    GL_CHECK_ERROR();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    GL_CHECK_ERROR();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GL_CHECK_ERROR();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    GL_CHECK_ERROR();
    glGenerateMipmap(GL_TEXTURE_2D);
    GL_CHECK_ERROR();

    free(data);
    return texture_id;
}

OpenGLResources renderer_init(const AppState* state) {
    g_opengl_error_log_file = fopen("temp_opengl_errors.txt", "w");
    if (!g_opengl_error_log_file) {
        fprintf(stderr, "[ERROR] Failed to open temp_opengl_errors.txt for writing.\n");
    }

    OpenGLResources res = {0};
    res.lighting_shader = create_shader_program(lighting_vertex_src, lighting_fragment_src);
    res.depth_shader = create_shader_program(depth_vertex_src, depth_fragment_src);
    if (res.lighting_shader == 0 || res.depth_shader == 0) { printf("Failed to create shaders.\n"); return (OpenGLResources){0}; }

    res.mvp_location = glGetUniformLocation(res.lighting_shader, "u_mvp");
    GL_CHECK_ERROR();
    res.model_mat_location = glGetUniformLocation(res.lighting_shader, "u_model");
    GL_CHECK_ERROR();
    res.normal_mat_location = glGetUniformLocation(res.lighting_shader, "u_normal_matrix");
    GL_CHECK_ERROR();
    res.light_dir_location = glGetUniformLocation(res.lighting_shader, "u_light_dir");
    GL_CHECK_ERROR();
    res.object_color_location = glGetUniformLocation(res.lighting_shader, "u_object_color");
    GL_CHECK_ERROR();
    res.texture_sampler_location = glGetUniformLocation(res.lighting_shader, "u_texture");
    GL_CHECK_ERROR();
    res.view_pos_location = glGetUniformLocation(res.lighting_shader, "u_view_pos");
    GL_CHECK_ERROR();
    res.light_space_matrix_location = glGetUniformLocation(res.lighting_shader, "u_light_space_matrix");
    GL_CHECK_ERROR();
    res.shadow_map_sampler_location = glGetUniformLocation(res.lighting_shader, "u_shadow_map");
    GL_CHECK_ERROR();
    res.depth_light_space_matrix_location = glGetUniformLocation(res.depth_shader, "u_light_space_matrix");
    GL_CHECK_ERROR();

    res.cube_texture = create_checkerboard_texture(64, 64);
    res.shadow_map = fp_shadow_create_map();

    // Cube buffers - use existing VAO/VBO/EBO from FP_MeshData
    res.cube_vao = state->cube_mesh.vao;
    res.cube_vbo = state->cube_mesh.vbo;
    res.cube_ebo = state->cube_mesh.ebo;

    // Plane buffers - use existing VAO/VBO/EBO from FP_MeshData
    res.plane_vao = state->plane_mesh.vao;
    res.plane_vbo = state->plane_mesh.vbo;
    res.plane_ebo = state->plane_mesh.ebo;

    res.msaa_fbo = fp_msaa_create_framebuffer(1280, 720, 4);
    if (res.msaa_fbo.fbo == 0) { printf("Failed to create MSAA FBO.\n"); return (OpenGLResources){0}; }

    glEnable(GL_DEPTH_TEST);
    GL_CHECK_ERROR();
    glEnable(GL_MULTISAMPLE);
    GL_CHECK_ERROR();
    glClearColor(.1f, .1f, .1f, 1);
    GL_CHECK_ERROR();
    return res;
}

void renderer_shutdown(OpenGLResources* res) {
    fp_msaa_destroy_framebuffer(&res->msaa_fbo);
    fp_shadow_destroy_map(&res->shadow_map);
    glDeleteTextures(1, &res->cube_texture);
    glDeleteVertexArrays(1, &res->cube_vao);
    glDeleteBuffers(1, &res->cube_vbo);
    glDeleteBuffers(1, &res->cube_ebo);
    glDeleteVertexArrays(1, &res->plane_vao);
    glDeleteBuffers(1, &res->plane_vbo);
    glDeleteBuffers(1, &res->plane_ebo);
    glDeleteProgram(res->lighting_shader);
    glDeleteProgram(res->depth_shader);

    if (g_opengl_error_log_file) {
        fclose(g_opengl_error_log_file);
        g_opengl_error_log_file = NULL;
    }
}

void renderer_render(const AppState* state, OpenGLResources* res, HDC hdc) {
    // Temporarily disable shadow pass for debugging
    // glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    // GL_CHECK_ERROR();
    // glBindFramebuffer(GL_FRAMEBUFFER, res->shadow_map.depth_map_fbo);
    // GL_CHECK_ERROR();
    // glClear(GL_DEPTH_BUFFER_BIT);
    // GL_CHECK_ERROR();

    // Mat4 light_proj, light_view, light_space_matrix;
    // fp_mat4_ortho(&light_proj, -10.0f, 10.0f, -10.0f, 10.0f, 1.0f, 40.0f);
    // fp_mat4_lookat(&light_view, state->light.position.x, state->light.position.y, state->light.position.z, state->light.target.x, state->light.target.y, state->light.target.z, 0.0f, 1.0f, 0.0f);
    // fp_mat4_mul(&light_space_matrix, &light_proj, &light_view);

    // glUseProgram(res->depth_shader);
    // GL_CHECK_ERROR();
    // glUniformMatrix4fv(res->depth_light_space_matrix_location, 1, GL_FALSE, light_space_matrix.m);
    // GL_CHECK_ERROR();

    // // Render all cubes to the depth map
    // for (int i = 0; i < state->object_count; ++i) {
    //     if (state->objects[i].mesh == &state->cube_mesh) {
    //         Mat4 model_mat;
    //         fp_transform_to_matrix(&model_mat, &state->objects[i].transform);
    //         // The light space matrix is already set, we just need to draw the object
    //         glBindVertexArray(res->cube_vao);
    //         GL_CHECK_ERROR();
    //         glDrawElements(GL_TRIANGLES, state->objects[i].mesh->index_count, GL_UNSIGNED_INT, 0);
    //         GL_CHECK_ERROR();
    //     }
    // }
    
    // 2. Render scene normally with shadows (directly to default framebuffer for debugging)
    glViewport(0, 0, 1280, 720);
    GL_CHECK_ERROR();
    // glBindFramebuffer(GL_FRAMEBUFFER, res->msaa_fbo.fbo); // Bypass MSAA FBO
    glBindFramebuffer(GL_FRAMEBUFFER, 0); // Render directly to default framebuffer
    GL_CHECK_ERROR();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    GL_CHECK_ERROR();

    glUseProgram(res->lighting_shader);
    GL_CHECK_ERROR();
    // glUniformMatrix4fv(res->light_space_matrix_location, 1, GL_FALSE, light_space_matrix.m); // Shadow map related
    // GL_CHECK_ERROR();
    
    Vec3f light_dir_temp = vec3f_sub(state->light.target, state->light.position);
    Vec3f light_dir = vec3f_normalize(light_dir_temp);
    glUniform3fv(res->light_dir_location, 1, &light_dir.x);
    GL_CHECK_ERROR();
    glUniform3fv(res->view_pos_location, 1, &state->camera.transform.position.x);
    GL_CHECK_ERROR();
    
    glActiveTexture(GL_TEXTURE0);
    GL_CHECK_ERROR();
    glBindTexture(GL_TEXTURE_2D, res->cube_texture);
    GL_CHECK_ERROR();
    glUniform1i(res->texture_sampler_location, 0);
    GL_CHECK_ERROR();
    
    // glActiveTexture(GL_TEXTURE1); // Shadow map related
    // GL_CHECK_ERROR();
    // glBindTexture(GL_TEXTURE_2D, res->shadow_map.depth_map_texture); // Shadow map related
    // GL_CHECK_ERROR();
    // glUniform1i(res->shadow_map_sampler_location, 1); // Shadow map related
    // GL_CHECK_ERROR();

    Mat4 view_mat, proj_mat;
    fp_view_matrix(&view_mat, &state->camera);
    fp_projection_matrix(&proj_mat, &state->camera.projection);

    for (int i = 0; i < state->object_count; ++i) {
        const SceneObject* obj = &state->objects[i];

        Mat4 obj_model_mat, mvp_mat, normal_mat, inv_model_mat;
        fp_transform_to_matrix(&obj_model_mat, &obj->transform);
        fp_get_mvp_matrix(&mvp_mat, &obj_model_mat, &view_mat, &proj_mat);
        fp_mat4_inverse(&inv_model_mat, &obj_model_mat);
        fp_mat4_transpose(&normal_mat, &inv_model_mat);

        glUniformMatrix4fv(res->mvp_location, 1, GL_FALSE, mvp_mat.m);
        GL_CHECK_ERROR();
        glUniformMatrix4fv(res->model_mat_location, 1, GL_FALSE, obj_model_mat.m);
        GL_CHECK_ERROR();
        glUniformMatrix4fv(res->normal_mat_location, 1, GL_FALSE, normal_mat.m);
        GL_CHECK_ERROR();

        glUniform3fv(res->object_color_location, 1, &obj->material_color.x);
        GL_CHECK_ERROR();

        if (obj->mesh == &state->cube_mesh) {
            glBindVertexArray(res->cube_vao);
            GL_CHECK_ERROR();
        } else {
            glBindVertexArray(res->plane_vao);
            GL_CHECK_ERROR();
        }
        
        glDrawElements(GL_TRIANGLES, obj->mesh->index_count, GL_UNSIGNED_INT, 0);
        GL_CHECK_ERROR();
    }
    glBindVertexArray(0);
    GL_CHECK_ERROR();

    // 3. Resolve MSAA framebuffer to default framebuffer (Bypassed)
    // fp_msaa_resolve_framebuffer(&res->msaa_fbo, 0);
    // GL_CHECK_ERROR();

    SwapBuffers(hdc);
    GL_CHECK_ERROR();
}
