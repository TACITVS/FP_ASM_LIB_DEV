/**
 * demo_renderer_ssao.c
 *
 * Minimal FP-First Demo Using Renderer Library with SSAO
 *
 * Architecture:
 * - Immutable application state
 * - Pure functions for state transitions
 * - Library handles rendering side effects
 * - Demonstrates proper library usage
 *
 * Controls:
 *   O - Toggle SSAO on/off
 *   ESC - Exit
 */

#include <windows.h>
#include <gl/gl.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include "ecs.h"
#include "renderer_modern.h"
#include "gl_extensions.h"

//==============================================================================
// Immutable Application State
//==============================================================================

typedef struct {
    bool ssao_enabled;
    int ssao_samples;
    float ssao_radius;
    float rotation_angle;  // Camera rotation
    bool running;
} AppState;

typedef struct {
    HWND hwnd;
    HDC hdc;
    HGLRC hglrc;
    int width;
    int height;
} WindowContext;

//==============================================================================
// Pure Functions - State Transitions (No Side Effects)
//==============================================================================

/**
 * Create initial application state
 * Pure: Returns new state, no mutations
 */
AppState app_state_create(void) {
    AppState state = {
        .ssao_enabled = true,
        .ssao_samples = 16,
        .ssao_radius = 0.5f,
        .rotation_angle = 0.0f,
        .running = true
    };
    return state;
}

/**
 * Toggle SSAO (pure function, returns new state)
 */
AppState app_state_toggle_ssao(AppState state) {
    AppState new_state = state;  // Copy
    new_state.ssao_enabled = !new_state.ssao_enabled;
    return new_state;
}

/**
 * Update rotation (pure function)
 */
AppState app_state_update_rotation(AppState state, float delta_time) {
    AppState new_state = state;  // Copy
    new_state.rotation_angle += delta_time * 30.0f;  // 30 deg/sec
    if (new_state.rotation_angle >= 360.0f) {
        new_state.rotation_angle -= 360.0f;
    }
    return new_state;
}

/**
 * Request exit (pure function)
 */
AppState app_state_request_exit(AppState state) {
    AppState new_state = state;  // Copy
    new_state.running = false;
    return new_state;
}

//==============================================================================
// Pure Functions - Renderer Configuration
//==============================================================================

/**
 * Create renderer config from app state (pure)
 * Maps immutable app state → renderer configuration
 */
RendererConfig renderer_config_from_state(AppState state, int width, int height) {
    RendererConfig config = {
        .window_width = width,
        .window_height = height,
        .enable_shadows = false,      // Keep demo simple
        .enable_ssao = state.ssao_enabled,
        .enable_bloom = false,
        .enable_fxaa = false,
        .shadow_resolution = 0,
        .max_lights = 1,
        .shadow_bias = 0.005f,
        .ssao_samples = state.ssao_samples,
        .ssao_radius = state.ssao_radius
    };
    return config;
}

//==============================================================================
// Scene Setup (ECS)
//==============================================================================

/**
 * Create a simple test scene
 * Pure in spirit - creates data structures without global state
 */
ECSWorld* create_test_scene(void) {
    ECSWorld* world = ecs_world_create();

    // Create a rotating cube
    Entity cube = ecs_entity_create(world);

    // Transform component
    TransformComponent transform;
    transform.position.x = 0.0f;
    transform.position.y = 0.0f;
    transform.position.z = -5.0f;
    transform.rotation = quat_identity();
    transform.scale.x = 1.0f;
    transform.scale.y = 1.0f;
    transform.scale.z = 1.0f;
    transform.dirty = true;
    ecs_add_transform(world, cube, &transform);

    // Material component (PBR)
    MaterialComponent material;
    material.albedo.x = 0.8f;  // Light gray
    material.albedo.y = 0.8f;
    material.albedo.z = 0.8f;
    material.metallic = 0.3f;
    material.roughness = 0.7f;
    material.ao = 1.0f;
    material.albedo_map = 0;
    material.normal_map = 0;
    material.metallic_roughness_map = 0;
    material.ao_map = 0;
    material.emissive.x = 0.0f;
    material.emissive.y = 0.0f;
    material.emissive.z = 0.0f;
    material.emissive_strength = 0.0f;
    ecs_add_material(world, cube, &material);

    // Mesh component - create actual cube geometry
    MeshComponent mesh = mesh_create_cube();
    ecs_add_mesh(world, cube, &mesh);

    // Ground plane for better AO visualization
    Entity plane = ecs_entity_create(world);

    TransformComponent plane_transform;
    plane_transform.position.x = 0.0f;
    plane_transform.position.y = -2.0f;
    plane_transform.position.z = -5.0f;
    plane_transform.rotation = quat_identity();
    plane_transform.scale.x = 10.0f;
    plane_transform.scale.y = 1.0f;
    plane_transform.scale.z = 10.0f;
    plane_transform.dirty = true;
    ecs_add_transform(world, plane, &plane_transform);

    MaterialComponent plane_material;
    plane_material.albedo.x = 0.5f;
    plane_material.albedo.y = 0.5f;
    plane_material.albedo.z = 0.5f;
    plane_material.metallic = 0.0f;
    plane_material.roughness = 0.9f;
    plane_material.ao = 1.0f;
    plane_material.albedo_map = 0;
    plane_material.normal_map = 0;
    plane_material.metallic_roughness_map = 0;
    plane_material.ao_map = 0;
    plane_material.emissive.x = 0.0f;
    plane_material.emissive.y = 0.0f;
    plane_material.emissive.z = 0.0f;
    plane_material.emissive_strength = 0.0f;
    ecs_add_material(world, plane, &plane_material);

    MeshComponent plane_mesh = mesh_create_plane(10.0f);  // 10x10 plane
    ecs_add_mesh(world, plane, &plane_mesh);

    // Create camera
    Entity camera = ecs_entity_create(world);

    CameraComponent cam;
    cam.type = CAMERA_PERSPECTIVE;
    cam.fov = 45.0f * 3.14159f / 180.0f;  // 45 degrees to radians
    cam.aspect = 1280.0f / 720.0f;
    cam.near_plane = 0.1f;
    cam.far_plane = 100.0f;
    cam.ortho_size = 10.0f;
    cam.dirty = true;
    ecs_add_camera(world, camera, &cam);

    // Set as active camera
    world->active_camera = camera;

    // Camera transform (positioned to view the scene)
    TransformComponent cam_transform;
    cam_transform.position.x = 0.0f;
    cam_transform.position.y = 1.0f;
    cam_transform.position.z = 2.0f;  // Back and up from origin
    cam_transform.rotation = quat_identity();
    cam_transform.scale.x = 1.0f;
    cam_transform.scale.y = 1.0f;
    cam_transform.scale.z = 1.0f;
    cam_transform.dirty = true;
    ecs_add_transform(world, camera, &cam_transform);

    // Create directional light
    Entity light = ecs_entity_create(world);

    LightComponent light_comp;
    light_comp.type = LIGHT_DIRECTIONAL;
    light_comp.color.x = 1.0f;  // White light
    light_comp.color.y = 1.0f;
    light_comp.color.z = 1.0f;
    light_comp.intensity = 3.0f;  // Brighter light for better visibility
    light_comp.range = 0.0f;
    light_comp.attenuation = 0.0f;
    light_comp.inner_cone = 0.0f;
    light_comp.outer_cone = 0.0f;
    light_comp.cast_shadows = false;
    light_comp.shadow_map = 0;
    light_comp.shadow_matrix = mat4_identity();  // Initialize matrix
    ecs_add_light(world, light, &light_comp);

    // Light transform (position above and in front)
    TransformComponent light_transform;
    light_transform.position.x = 2.0f;
    light_transform.position.y = 4.0f;
    light_transform.position.z = 2.0f;
    light_transform.rotation = quat_identity();
    light_transform.scale.x = 1.0f;
    light_transform.scale.y = 1.0f;
    light_transform.scale.z = 1.0f;
    light_transform.dirty = true;
    ecs_add_transform(world, light, &light_transform);

    return world;
}

/**
 * Update scene based on app state (functional approach)
 * Applies rotation from immutable state to ECS world
 */
void update_scene_from_state(ECSWorld* world, AppState state) {
    // Find cube entity and update rotation
    // In a real FP system, we'd return a new world, but ECS is inherently mutable
    // So we isolate mutations here and treat it as a "render" operation

    // First entity is the cube
    if (world->entity_count > 0) {
        Entity cube = world->entities[0];
        if (ecs_has_transform(world, cube)) {
            TransformComponent* transform = ecs_get_transform(world, cube);

            // Convert Euler angle to quaternion (rotate around Y axis)
            float angle_rad = state.rotation_angle * 3.14159f / 180.0f;
            Vec3 y_axis = {0.0f, 1.0f, 0.0f};
            transform->rotation = quat_from_axis_angle(y_axis, angle_rad);
            transform->dirty = true;
        }
    }
}

//==============================================================================
// Window Setup (Necessary Mutations for OS/OpenGL)
//==============================================================================

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

WindowContext create_window(const char* title, int width, int height) {
    WindowContext ctx = {0};
    ctx.width = width;
    ctx.height = height;

    // Register window class
    WNDCLASSA wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "RendererSSAODemo";
    wc.style = CS_OWNDC;
    RegisterClassA(&wc);

    // Create window
    ctx.hwnd = CreateWindowExA(
        0, "RendererSSAODemo", title,
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        NULL, NULL, GetModuleHandle(NULL), NULL
    );

    // Get device context
    ctx.hdc = GetDC(ctx.hwnd);

    // Set pixel format
    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR), 1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA, 32,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        24, 8, 0,
        PFD_MAIN_PLANE, 0, 0, 0, 0
    };
    int format = ChoosePixelFormat(ctx.hdc, &pfd);
    SetPixelFormat(ctx.hdc, format, &pfd);

    // Create OpenGL context
    ctx.hglrc = wglCreateContext(ctx.hdc);
    wglMakeCurrent(ctx.hdc, ctx.hglrc);

    return ctx;
}

void destroy_window(WindowContext ctx) {
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(ctx.hglrc);
    ReleaseDC(ctx.hwnd, ctx.hdc);
    DestroyWindow(ctx.hwnd);
}

//==============================================================================
// Main Loop (Functional Style with Immutable State)
//==============================================================================

// Global state for window proc (necessary evil for Windows API)
static AppState* g_app_state = NULL;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CLOSE:
            if (g_app_state) {
                *g_app_state = app_state_request_exit(*g_app_state);
            }
            return 0;

        case WM_KEYDOWN:
            if (g_app_state) {
                if (wParam == VK_ESCAPE) {
                    *g_app_state = app_state_request_exit(*g_app_state);
                }
                else if (wParam == 'O') {
                    *g_app_state = app_state_toggle_ssao(*g_app_state);
                    printf("[SSAO] %s\n", g_app_state->ssao_enabled ? "ENABLED" : "DISABLED");
                }
            }
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}

int main(void) {
    printf("=== SSAO Renderer Library Demo ===\n");
    printf("Controls:\n");
    printf("  O - Toggle SSAO\n");
    printf("  ESC - Exit\n\n");

    // Initialize immutable state
    AppState state = app_state_create();
    g_app_state = &state;  // For window proc

    // Create window
    WindowContext window = create_window("SSAO Demo - Renderer Library", 1280, 720);

    // Load OpenGL extensions
    if (!gl_load_extensions()) {
        fprintf(stderr, "Failed to load OpenGL extensions\n");
        return 1;
    }

    // Create renderer with config from state
    RendererConfig config = renderer_config_from_state(state, window.width, window.height);
    Renderer* renderer = renderer_create(config);
    if (!renderer) {
        fprintf(stderr, "Failed to create renderer\n");
        return 1;
    }

    // Initialize SSAO system
    renderer_init_ssao(renderer);

    // Create scene
    ECSWorld* world = create_test_scene();

    // Main loop (functional style)
    LARGE_INTEGER freq, last_time, current_time;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&last_time);

    while (state.running) {
        // Process messages
        MSG msg;
        while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }

        // Calculate delta time
        QueryPerformanceCounter(&current_time);
        float delta_time = (float)(current_time.QuadPart - last_time.QuadPart) / (float)freq.QuadPart;
        last_time = current_time;

        // Update state (functional transformations)
        state = app_state_update_rotation(state, delta_time);

        // Update renderer config if SSAO state changed
        RendererConfig new_config = renderer_config_from_state(state, window.width, window.height);
        renderer->config = new_config;  // Update config (necessary mutation for library)

        // Update scene from state
        update_scene_from_state(world, state);

        // Update camera matrices
        if (world->active_camera != ENTITY_INVALID) {
            CameraComponent* cam = ecs_get_camera(world, world->active_camera);
            TransformComponent* cam_transform = ecs_get_transform(world, world->active_camera);

            if (cam && cam_transform && cam->dirty) {
                // Update projection matrix
                cam->projection_matrix = mat4_perspective(cam->fov, cam->aspect, cam->near_plane, cam->far_plane);

                // Update view matrix (camera looks at origin from its position)
                Vec3 eye = cam_transform->position;
                Vec3 target = {0.0f, 0.0f, -5.0f};  // Look at center of scene
                Vec3 up = {0.0f, 1.0f, 0.0f};
                cam->view_matrix = mat4_look_at(eye, target, up);

                // Calculate view-projection matrix
                cam->view_projection_matrix = mat4_mul(cam->projection_matrix, cam->view_matrix);
                cam->dirty = false;
            }
        }

        // Update transform matrices
        for (Entity id = 0; id < world->entity_count; ++id) {
            Entity entity = world->entities[id];
            if (ecs_has_transform(world, entity)) {
                TransformComponent* transform = ecs_get_transform(world, entity);
                if (transform && transform->dirty) {
                    transform_update_matrix(transform);
                }
            }
        }

        // Render (side effects isolated in library)
        renderer_begin_frame(renderer);
        renderer_render_world(renderer, world);
        renderer_end_frame(renderer);

        // Present
        SwapBuffers(window.hdc);
    }

    // Cleanup
    printf("\nCleaning up...\n");
    renderer_cleanup_ssao(renderer);
    renderer_destroy(renderer);
    ecs_world_destroy(world);
    destroy_window(window);

    printf("Demo complete.\n");
    return 0;
}
