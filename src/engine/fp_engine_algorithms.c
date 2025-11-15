#include "../../include/fp_engine_algorithms.h"
#include "../../include/fp_generic.h"
#include <math.h>
#include <string.h> // For memcpy

// Perlin noise for cube animation (copied from demo_engine_mvp.c for now)
static int perlin_p[512];
static bool perlin_initialized = false;

static void init_perlin() {
    if (perlin_initialized) return;
    int permutation[] = {
        151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,
        8,99,37,240,21,10,23,190,6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,
        35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,74,165,71,
        134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,
        55,46,245,40,244,102,143,54,65,25,63,161,1,216,80,73,209,76,132,187,208,89,
        18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,52,217,226,
        250,124,123,5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,
        189,28,42,223,183,170,213,119,248,152,2,44,154,163,70,221,153,101,155,167,43,
        172,9,129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,218,246,97,
        228,251,34,242,193,238,210,144,12,191,179,162,241,81,51,145,235,249,14,239,
        107,49,192,214,31,181,199,106,157,184,84,204,176,115,121,50,45,127,4,150,254,
        138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
    };
    for (int i = 0; i < 256; i++) {
        perlin_p[i] = perlin_p[256 + i] = permutation[i];
    }
    perlin_initialized = true;
}

static float fade(float t) { return t * t * t * (t * (t * 6 - 15) + 10); }
static float lerp_f(float t, float a, float b) { return a + t * (b - a); }

static float grad(int hash, float x, float y, float z) {
    int h = hash & 15;
    float u = h < 8 ? x : y;
    float v = h < 4 ? y : h == 12 || h == 14 ? x : z;
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

static float perlin_noise(float x, float y, float z) {
    init_perlin(); // Ensure Perlin noise is initialized
    int X = (int)floor(x) & 255, Y = (int)floor(y) & 255, Z = (int)floor(z) & 255;
    x -= floor(x); y -= floor(y); z -= floor(z);
    float u = fade(x), v = fade(y), w = fade(z);
    int A = perlin_p[X] + Y, AA = perlin_p[A] + Z, AB = perlin_p[A + 1] + Z;
    int B = perlin_p[X + 1] + Y, BA = perlin_p[B] + Z, BB = perlin_p[B + 1] + Z;
    return lerp_f(w, lerp_f(v, lerp_f(u, grad(perlin_p[AA], x, y, z),
                                   grad(perlin_p[BA], x - 1, y, z)),
                           lerp_f(u, grad(perlin_p[AB], x, y - 1, z),
                                   grad(perlin_p[BB], x - 1, y - 1, z))),
                   lerp_f(v, lerp_f(u, grad(perlin_p[AA + 1], x, y, z - 1),
                                   grad(perlin_p[BA + 1], x - 1, y, z - 1)),
                           lerp_f(u, grad(perlin_p[AB + 1], x, y - 1, z - 1),
                                   grad(perlin_p[BB + 1], x - 1, y - 1, z - 1))));
}

typedef struct {
    float near_plane;
    float far_plane;
    float ortho_size;
    Vec3f up;
} LightMatrixContext;

static void update_light_shadow_matrix(void* out_ptr, const void* in_ptr, void* ctx_ptr) {
    const FP_Light* src = (const FP_Light*)in_ptr;
    FP_Light* dst = (FP_Light*)out_ptr;
    const LightMatrixContext* ctx = (const LightMatrixContext*)ctx_ptr;

    *dst = *src;

    Mat4 light_view;
    fp_mat4_lookat(&light_view,
                   src->position.x, src->position.y, src->position.z,
                   src->target.x, src->target.y, src->target.z,
                   ctx->up.x, ctx->up.y, ctx->up.z);

    Mat4 light_projection;
    fp_mat4_ortho(&light_projection,
                  -ctx->ortho_size, ctx->ortho_size,
                  -ctx->ortho_size, ctx->ortho_size,
                  ctx->near_plane, ctx->far_plane);

    fp_mat4_mul(&dst->shadow_matrix, &light_projection, &light_view);
}

typedef struct {
    float current_time;
    uint32_t index;
} AnimateContext;

static void animate_object(void* out_ptr, const void* in_ptr, void* ctx_ptr) {
    const FP_SceneObject* src = (const FP_SceneObject*)in_ptr;
    FP_SceneObject* dst = (FP_SceneObject*)out_ptr;
    AnimateContext* anim = (AnimateContext*)ctx_ptr;

    *dst = *src;

    const uint32_t idx = anim->index++;

    if (dst->mesh == NULL) {
        return;
    }

    float time_seed = (anim->current_time + idx * 100.0f) * 0.5f;
    float noise_x = perlin_noise(dst->transform.position.x * 0.1f,
                                 dst->transform.position.z * 0.1f,
                                 time_seed);
    float noise_y = perlin_noise(dst->transform.position.x * 0.1f + 100.0f,
                                 dst->transform.position.z * 0.1f,
                                 time_seed);
    float noise_z = perlin_noise(dst->transform.position.x * 0.1f + 200.0f,
                                 dst->transform.position.z * 0.1f,
                                 time_seed);

    Vec3f axis = { noise_x, noise_y, noise_z, 0.0f };
    float angle = anim->current_time * 0.5f;
    fp_quat_from_axis_angle(&dst->transform.rotation, &axis, angle);
}

// Function to update the camera based on controls
FP_AppState fp_update_camera(const FP_AppState* current_state,
                             float camera_yaw_rad, float camera_pitch_rad,
                             float camera_distance, Vec3f camera_target) {
    FP_AppState new_state = *current_state; // Start with a copy of the current state

    // Calculate camera position from spherical coordinates
    new_state.camera.position.x = camera_target.x + camera_distance * sinf(camera_yaw_rad) * cosf(camera_pitch_rad);
    new_state.camera.position.y = camera_target.y + camera_distance * sinf(camera_pitch_rad);
    new_state.camera.position.z = camera_target.z + camera_distance * cosf(camera_yaw_rad) * cosf(camera_pitch_rad);
    new_state.camera.position._pad = 0.0f; // Ensure pad is initialized

    // Update view matrix to look at target
    Vec3f up = {0.0f, 1.0f, 0.0f, 0.0f};
    fp_mat4_lookat(&new_state.camera.view_matrix,
                   new_state.camera.position.x, new_state.camera.position.y, new_state.camera.position.z,
                   camera_target.x, camera_target.y, camera_target.z,
                   up.x, up.y, up.z);

    return new_state;
}

// Function to update light matrices (e.g., shadow matrices)
FP_AppState fp_update_light_matrices(const FP_AppState* current_state) {
    FP_AppState new_state = *current_state; // Start with a copy of the current state

    if (new_state.light_count > 0) {
        LightMatrixContext ctx = {
            .near_plane = 1.0f,
            .far_plane = 100.0f,
            .ortho_size = 100.0f,
            .up = {0.0f, 1.0f, 0.0f, 0.0f}
        };
        fp_map_generic(current_state->lights,
                       new_state.lights,
                       current_state->light_count,
                       sizeof(FP_Light),
                       sizeof(FP_Light),
                       update_light_shadow_matrix,
                       &ctx);
    }

    return new_state;
}

// Function to animate scene objects
FP_AppState fp_animate_objects(const FP_AppState* current_state, float dt) {
    FP_AppState new_state = *current_state; // Start with a copy of the current state
    new_state.current_time += dt * 0.001f; // Update time

    if (new_state.object_count > 0) {
        AnimateContext ctx = {
            .current_time = new_state.current_time,
            .index = 0
        };
        fp_map_generic(current_state->objects,
                       new_state.objects,
                       current_state->object_count,
                       sizeof(FP_SceneObject),
                       sizeof(FP_SceneObject),
                       animate_object,
                       &ctx);
    }

    return new_state;
}

// Function to update the local and world matrices of a transform
void fp_transform_update_matrix(FP_Transform* transform) {
    if (!transform->dirty) {
        return;
    }

    Mat4 translation_matrix;
    fp_mat4_translation(&translation_matrix, transform->position.x, transform->position.y, transform->position.z);

    Mat4 scale_matrix;
    fp_mat4_scale(&scale_matrix, transform->scale.x, transform->scale.y, transform->scale.z);

    // Convert quaternion to Euler angles for fp_mat4_rotation_euler
    Vec3f euler_angles;
    fp_quat_to_euler(&euler_angles, &transform->rotation);
    Mat4 rotation_matrix;
    fp_mat4_rotation_euler(&rotation_matrix, euler_angles.x, euler_angles.y, euler_angles.z);

    // Combine transformations: Translation * Rotation * Scale
    // fp_mat4_mul computes output = a * b.
    // So, temp = rotation_matrix * scale_matrix
    // local_matrix = translation_matrix * temp

    Mat4 temp_matrix;
    fp_mat4_mul(&temp_matrix, &rotation_matrix, &scale_matrix);
    fp_mat4_mul(&transform->local_matrix, &translation_matrix, &temp_matrix);

    transform->dirty = false;
}
