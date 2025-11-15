/**
 * demo_engine_mvp.c
 *
 * Modern Engine MVP Demo - ECS Architecture Showcase
 *
 * Features:
 * - Entity Component System (ECS) architecture
 * - OpenGL 3.3 rendering with modern pipeline
 * - 500 cubes with PBR materials (metallic/roughness workflow)
 * - 2 directional lights with shadow mapping
 * - CAD-style camera controls (orbit/pan/zoom)
 * - Real-time parameter tweaking via keyboard
 * - Performance metrics (FPS counter)
 *
 * Controls:
 *   MOUSE:
 *     Left-drag   - Orbit camera around scene
 *     Right-drag  - Pan camera (move view)
 *     Mouse wheel - Zoom in/out
 *
 *   KEYBOARD:
 *     QUALITY PRESETS:
 *       F1 - Low (60+ FPS)   | F2 - Medium (45-60 FPS)
 *       F3 - High (30-45 FPS) | F4 - Ultra (Raytracing-like)
 *
 *     QUALITY TOGGLES:
 *       O - SSAO     | P - PBR      | H - Shadows
 *       B - Bloom    | M - MSAA     | G - Gamma
 *
 *     MATERIAL/LIGHTING:
 *       UP/DOWN     - Global roughness offset
 *       LEFT/RIGHT  - Global metallic offset
 *       1/2         - Light 1 intensity
 *       3/4         - Light 2 intensity
 *
 *     OTHER:
 *       SPACE       - Pause/play animation
 *       CTRL+W      - Toggle wireframe mode
 *       ESC         - Exit
 *
 * Target: 60 FPS @ 1920x1080
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <windows.h>
#include <GL/gl.h>

// Include engine headers
#include "include/fp_engine_types.h"
#include "include/fp_mesh_generation.h"
#include "include/fp_engine_algorithms.h"
#include "include/gl_extensions.h"
#include "include/renderer_modern.h" // Include the modern renderer header

#define PI 3.14159265358979323846f
#define NUM_CUBES 1
#define SHADOW_MAP_SIZE 2048

// OpenGL constants that might not be defined
#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE 0x809D
#endif

// Window dimensions
#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080

// Globals
HWND hwnd;
HDC hdc;
HGLRC hglrc;
// ECSWorld* world = NULL; // No longer used
// Entity camera_entity = ENTITY_INVALID; // No longer used
Renderer* renderer = NULL; // Global renderer instance
FP_AppState app_state = {0}; // Global AppState instance

// Performance tracking
int frame_count = 0;
clock_t last_fps_time;
float fps = 0.0f;
float frame_time = 0.0f;

// Camera state (spherical coordinates for CAD-style orbit)
float camera_distance = 10.0f;  // Distance from look-at point
float camera_yaw = 180.0f;     // Horizontal rotation
float camera_pitch = -20.0f;   // Vertical rotation
Vec3f camera_target = {0.0f, 0.0f, 0.0f, 0.0f};  // Look-at point (changed to Vec3f)

// Mouse state
BOOL mouse_left_down = FALSE;
BOOL mouse_right_down = FALSE;
int last_mouse_x = 0;
int last_mouse_y = 0;

// Timer for rendering during window drag
#define TIMER_ID 1
BOOL is_moving = FALSE;

// Animation and parameter controls
BOOL animation_paused = TRUE; // Disable animation initially
BOOL show_wireframe = FALSE;
float global_roughness_offset = 0.0f;  // -1.0 to 1.0
float global_metallic_offset = 0.0f;   // -1.0 to 1.0
float light1_intensity = 1.0f;
float light2_intensity = 0.5f;

//==============================================================================
// Modular Quality Settings System
//==============================================================================

typedef struct {
    // Lighting & Shading
    BOOL enable_pbr;              // PBR vs simple lighting
    BOOL enable_ssao;             // Ambient occlusion
    BOOL enable_shadows;          // Shadow mapping
    BOOL enable_soft_shadows;     // PCF filtering

    // Anti-Aliasing
    BOOL enable_msaa;             // Hardware MSAA
    BOOL enable_fxaa;             // Post-process FXAA

    // Post-Processing
    BOOL enable_bloom;            // HDR bloom
    BOOL enable_tone_mapping;     // HDR to LDR
    BOOL enable_gamma_correction; // sRGB conversion

    // Performance Settings
    int ssao_samples;             // 8, 16, 32, 64
    int shadow_resolution;        // 512, 1024, 2048, 4096
    int msaa_samples;             // 0, 2, 4, 8
} RenderQualitySettings;

// Global quality settings (starts at High preset)
RenderQualitySettings quality = {
    .enable_pbr = TRUE, // Enable PBR
    .enable_ssao = FALSE, // Disable SSAO
    .enable_shadows = FALSE, // Disable Shadows
    .enable_soft_shadows = FALSE,
    .enable_msaa = FALSE, // Disable MSAA
    .enable_fxaa = FALSE, // Disable FXAA
    .enable_bloom = FALSE, // Disable Bloom
    .enable_tone_mapping = FALSE, // Disable Tone Mapping
    .enable_gamma_correction = FALSE, // Disable Gamma Correction
    .ssao_samples = 0,
    .shadow_resolution = 0,
    .msaa_samples = 0
};

// Quality presets
void apply_preset_low(void) {
    quality.enable_pbr = FALSE;
    quality.enable_ssao = FALSE;
    quality.enable_shadows = FALSE;
    quality.enable_msaa = FALSE;
    quality.enable_fxaa = FALSE;
    quality.enable_bloom = FALSE;
    quality.ssao_samples = 0;
    quality.shadow_resolution = 0;
    quality.msaa_samples = 0;
    printf("Quality: LOW (60+ FPS target)\n");
}

void apply_preset_medium(void) {
    quality.enable_pbr = TRUE;
    quality.enable_ssao = TRUE;
    quality.enable_shadows = TRUE;
    quality.enable_soft_shadows = FALSE;
    quality.enable_msaa = FALSE;
    quality.enable_fxaa = TRUE;
    quality.enable_bloom = FALSE;
    quality.ssao_samples = 16;
    quality.shadow_resolution = 1024;
    quality.msaa_samples = 0;
    printf("Quality: MEDIUM (45-60 FPS target)\n");
}

void apply_preset_high(void) {
    quality.enable_pbr = TRUE;
    quality.enable_ssao = TRUE;
    quality.enable_shadows = TRUE;
    quality.enable_soft_shadows = TRUE;
    quality.enable_msaa = TRUE;
    quality.enable_bloom = TRUE;
    quality.ssao_samples = 32;
    quality.shadow_resolution = 2048;
    quality.msaa_samples = 4;
    printf("Quality: HIGH (30-45 FPS target)\n");
}

void apply_preset_ultra(void) {
    quality.enable_pbr = TRUE;
    quality.enable_ssao = TRUE;
    quality.enable_shadows = TRUE;
    quality.enable_soft_shadows = TRUE;
    quality.enable_msaa = TRUE;
    quality.enable_fxaa = TRUE;
    quality.enable_bloom = TRUE;
    quality.ssao_samples = 64;
    quality.shadow_resolution = 4096;
    quality.msaa_samples = 8;
    printf("Quality: ULTRA (Raytracing-like, 30+ FPS target)\n");
}

// Simple shader programs (we'll create minimal versions)
// GLuint pbr_shader_program = 0;
// GLuint shadow_shader_program = 0;

// Shadow mapping
GLuint shadow_fbo = 0;
GLuint shadow_texture = 0;

// Perlin noise for cube animation
static int perlin_p[512];

void init_perlin() {
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
}

float fade(float t) { return t * t * t * (t * (t * 6 - 15) + 10); }
float lerp_f(float t, float a, float b) { return a + t * (b - a); }

float grad(int hash, float x, float y, float z) {
    int h = hash & 15;
    float u = h < 8 ? x : y;
    float v = h < 4 ? y : h == 12 || h == 14 ? x : z;
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

float perlin_noise(float x, float y, float z) {
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

//==============================================================================
// ECS Scene Setup
//==============================================================================

void create_scene() {
    // Initialize app_state with default values
    app_state.object_count = 0;
    app_state.light_count = 0;
    app_state.current_time = 0.0f;

    // Create shared meshes
    FP_MeshData cube_mesh = fp_mesh_create_cube();
    // FP_MeshData plane_mesh = fp_mesh_create_plane(100.0f); // No plane for now

    // Add a single cube at the origin
    if (app_state.object_count < MAX_SCENE_OBJECTS) {
        FP_SceneObject obj = {0};

        // Transform
        obj.transform.position = (Vec3f){0.0f, 0.0f, 0.0f, 0.0f};
        obj.transform.rotation = (Quaternion){0.0f, 0.0f, 0.0f, 1.0f}; // Identity quaternion
        obj.transform.scale = (Vec3f){1.0f, 1.0f, 1.0f, 0.0f};
        obj.transform.dirty = true; // Mark as dirty for initial matrix update

        // Material (simple white)
        obj.material.albedo = (Vec3f){1.0f, 1.0f, 1.0f, 0.0f};
        obj.material.metallic = 0.0f;
        obj.material.roughness = 0.5f;
        obj.material.ao = 1.0f;

        // Mesh
        obj.mesh = &cube_mesh;

        app_state.objects[app_state.object_count++] = obj;
    }

    // Populate app_state.lights with a single light
    if (app_state.light_count < MAX_LIGHTS) {
        FP_Light light1 = {0};
        light1.position = (Vec3f){5.0f, 5.0f, 5.0f, 0.0f}; // Position the light
        light1.target = (Vec3f){0.0f, 0.0f, 0.0f, 0.0f}; // Look at the origin
        light1.color = (Vec3f){1.0f, 1.0f, 1.0f, 0.0f}; // White light
        light1.type = FP_LIGHT_DIRECTIONAL;
        fp_mat4_identity(&light1.shadow_matrix); // Initialize
        app_state.lights[app_state.light_count++] = light1;
    }

    // Populate app_state.camera
    app_state.camera.position = (Vec3f){0.0f, 0.0f, 0.0f, 0.0f}; // Will be set by update_camera
    app_state.camera.target = (Vec3f){0.0f, 0.0f, 0.0f, 0.0f};
    app_state.camera.up = (Vec3f){0.0f, 1.0f, 0.0f, 0.0f};
    fp_mat4_identity(&app_state.camera.view_matrix);
    fp_mat4_perspective(&app_state.camera.projection_matrix,
                        60.0f * PI / 180.0f, (float)WINDOW_WIDTH / WINDOW_HEIGHT, 0.1f, 500.0f);

    printf("Scene created: %d objects, %d lights, 1 camera\n", app_state.object_count, app_state.light_count);
}

//==============================================================================
// Rendering
//==============================================================================

void update_camera() {
    // Use global camera_yaw, camera_pitch, camera_distance, camera_target for now
    // These will eventually be part of a functional input state
    app_state = fp_update_camera(&app_state, camera_yaw * PI / 180.0f, camera_pitch * PI / 180.0f, camera_distance, camera_target);
}

void animate_cubes(float dt) {
    app_state = fp_animate_objects(&app_state, dt);
}

void render_world() {
    // Delegate rendering to the modern renderer
    renderer_render_world(renderer, &app_state);
}

//==============================================================================
// Light Matrix Updates
//==============================================================================

void update_light_matrices() {
    app_state = fp_update_light_matrices(&app_state);
}

//==============================================================================
// OpenGL Setup
//==============================================================================

void setup_opengl() {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);

    // Enable MSAA
    glEnable(GL_MULTISAMPLE);

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    // Initialize modern renderer
    RendererConfig config = {
        .window_width = WINDOW_WIDTH,
        .window_height = WINDOW_HEIGHT,
        .enable_shadows = quality.enable_shadows,
        .enable_ssao = quality.enable_ssao,
        .enable_bloom = quality.enable_bloom,
        .enable_fxaa = quality.enable_fxaa,
        .shadow_resolution = quality.shadow_resolution,
        .max_lights = 2, // Assuming 2 lights for now
        .shadow_bias = 0.005f,
        .ssao_samples = quality.ssao_samples,
        .ssao_radius = 0.5f // Default value, can be exposed later
    };

    renderer = renderer_create(config);
    if (!renderer) {
        printf("Failed to create modern renderer!\n");
        exit(1);
    }

    printf("OpenGL setup complete\n");
}

//==============================================================================
// Window Procedure
//==============================================================================

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) {
                PostQuitMessage(0);
            }
            else if (wParam == VK_SPACE) {
                animation_paused = !animation_paused;
                printf("Animation %s\n", animation_paused ? "PAUSED" : "PLAYING");
            }
            else if (wParam == 'W' && (GetKeyState(VK_CONTROL) & 0x8000)) {
                show_wireframe = !show_wireframe;
                glPolygonMode(GL_FRONT_AND_BACK, show_wireframe ? GL_LINE : GL_FILL);
                printf("Wireframe %s\n", show_wireframe ? "ON" : "OFF");
            }
            else if (wParam == VK_UP) {
                global_roughness_offset += 0.1f;
                if (global_roughness_offset > 1.0f) global_roughness_offset = 1.0f;
                printf("Roughness offset: %.2f\n", global_roughness_offset);
            }
            else if (wParam == VK_DOWN) {
                global_roughness_offset -= 0.1f;
                if (global_roughness_offset < -1.0f) global_roughness_offset = -1.0f;
                printf("Roughness offset: %.2f\n", global_roughness_offset);
            }
            else if (wParam == VK_RIGHT) {
                global_metallic_offset += 0.1f;
                if (global_metallic_offset > 1.0f) global_metallic_offset = 1.0f;
                printf("Metallic offset: %.2f\n", global_metallic_offset);
            }
            else if (wParam == VK_LEFT) {
                global_metallic_offset -= 0.1f;
                if (global_metallic_offset < -1.0f) global_metallic_offset = -1.0f;
                printf("Metallic offset: %.2f\n", global_metallic_offset);
            }
            else if (wParam == '1') {
                app_state.lights[0].color.x += 0.1f;
                if (app_state.lights[0].color.x > 1.0f) app_state.lights[0].color.x = 1.0f;
                printf("Light 1 R intensity: %.2f\n", app_state.lights[0].color.x);
            }
            else if (wParam == '2') {
                app_state.lights[0].color.x -= 0.1f;
                if (app_state.lights[0].color.x < 0.0f) app_state.lights[0].color.x = 0.0f;
                printf("Light 1 R intensity: %.2f\n", app_state.lights[0].color.x);
            }
            else if (wParam == '3') {
                app_state.lights[1].color.x += 0.1f;
                if (app_state.lights[1].color.x > 1.0f) app_state.lights[1].color.x = 1.0f;
                printf("Light 2 R intensity: %.2f\n", app_state.lights[1].color.x);
            }
            else if (wParam == '4') {
                app_state.lights[1].color.x -= 0.1f;
                if (app_state.lights[1].color.x < 0.0f) app_state.lights[1].color.x = 0.0f;
                printf("Light 2 R intensity: %.2f\n", app_state.lights[1].color.x);
            }
            // Quality presets
            else if (wParam == VK_F1) {
                apply_preset_low();
            }
            else if (wParam == VK_F2) {
                apply_preset_medium();
            }
            else if (wParam == VK_F3) {
                apply_preset_high();
            }
            else if (wParam == VK_F4) {
                apply_preset_ultra();
            }
            // Individual quality toggles
            else if (wParam == 'O') {
                quality.enable_ssao = !quality.enable_ssao;
                printf("SSAO: %s%s\n", quality.enable_ssao ? "ON" : "OFF",
                       quality.enable_ssao ? "" : " (will be implemented)");
            }
            else if (wParam == 'P') {
                quality.enable_pbr = !quality.enable_pbr;
                printf("PBR: %s\n", quality.enable_pbr ? "ON (Cook-Torrance)" : "OFF (Simple lighting)");
            }
            else if (wParam == 'H') {
                quality.enable_shadows = !quality.enable_shadows;
                printf("Shadows: %s%s\n", quality.enable_shadows ? "ON" : "OFF",
                       quality.enable_shadows ? " (will be implemented)" : "");
            }
            else if (wParam == 'B') {
                quality.enable_bloom = !quality.enable_bloom;
                printf("Bloom: %s%s\n", quality.enable_bloom ? "ON" : "OFF",
                       quality.enable_bloom ? " (will be implemented)" : "");
            }
            else if (wParam == 'M') {
                quality.enable_msaa = !quality.enable_msaa;
                if (quality.enable_msaa) {
                    glEnable(GL_MULTISAMPLE);
                } else {
                    glDisable(GL_MULTISAMPLE);
                }
                printf("MSAA: %s\n", quality.enable_msaa ? "ON (8x)" : "OFF");
            }
            else if (wParam == 'G') {
                quality.enable_gamma_correction = !quality.enable_gamma_correction;
                printf("Gamma Correction: %s (always active in shader)\n",
                       quality.enable_gamma_correction ? "ON" : "OFF");
            }
            return 0;

        case WM_LBUTTONDOWN:
            mouse_left_down = TRUE;
            last_mouse_x = LOWORD(lParam);
            last_mouse_y = HIWORD(lParam);
            SetCapture(hwnd);
            return 0;

        case WM_LBUTTONUP:
            mouse_left_down = FALSE;
            ReleaseCapture();
            return 0;

        case WM_RBUTTONDOWN:
            mouse_right_down = TRUE;
            last_mouse_x = LOWORD(lParam);
            last_mouse_y = HIWORD(lParam);
            SetCapture(hwnd);
            return 0;

        case WM_RBUTTONUP:
            mouse_right_down = FALSE;
            ReleaseCapture();
            return 0;

        case WM_MOUSEMOVE: {
            int mouse_x = LOWORD(lParam);
            int mouse_y = HIWORD(lParam);
            int dx = mouse_x - last_mouse_x;
            int dy = mouse_y - last_mouse_y;

            if (mouse_left_down) {
                // Orbit: rotate camera around target
                camera_yaw += dx * 0.5f;
                camera_pitch -= dy * 0.5f;

                // Clamp pitch to avoid gimbal lock
                if (camera_pitch > 89.0f) camera_pitch = 89.0f;
                if (camera_pitch < -89.0f) camera_pitch = -89.0f;
            }
            else if (mouse_right_down) {
                // Pan: move target point
                float pan_speed = camera_distance * 0.001f;
                float yaw_rad = camera_yaw * PI / 180.0f;

                // Calculate right and up vectors
                Vec3f right = {cosf(yaw_rad), 0.0f, -sinf(yaw_rad), 0.0f};
                Vec3f up = {0.0f, 1.0f, 0.0f, 0.0f};

                // Move target
                camera_target.x -= right.x * dx * pan_speed;
                camera_target.y += up.y * dy * pan_speed;
                camera_target.z -= right.z * dx * pan_speed;
            }

            last_mouse_x = mouse_x;
            last_mouse_y = mouse_y;
            return 0;
        }

        case WM_MOUSEWHEEL: {
            // Zoom: adjust camera distance
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            camera_distance -= delta * 0.05f;

            // Clamp distance
            if (camera_distance < 10.0f) camera_distance = 10.0f;
            if (camera_distance > 500.0f) camera_distance = 500.0f;
            return 0;
        }

        case WM_ENTERSIZEMOVE:
            // Start timer to keep rendering during window drag
            SetTimer(hwnd, TIMER_ID, 16, NULL);  // ~60 FPS
            is_moving = FALSE;
            return 0;

        case WM_EXITSIZEMOVE:
            // Stop timer when done moving
            KillTimer(hwnd, TIMER_ID);
            is_moving = FALSE;
            return 0;

        case WM_TIMER:
            if (wParam == TIMER_ID && is_moving) {
                // Render during window drag
                static clock_t last_time = 0;
                clock_t current_time = clock();
                float dt = (float)(current_time - last_time) / CLOCKS_PER_SEC * 1000.0f;
                last_time = current_time;

                update_camera();

                // Respect animation_paused flag even during window drag
                if (!animation_paused) {
                    animate_cubes(dt);
                }

                render_world();
                SwapBuffers(hdc);
            }
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

//==============================================================================
// Main Entry Point
//==============================================================================

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Allocate console for debugging
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);

    // Register window class
    WNDCLASSA wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "EngineDemo";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassA(&wc);

    // Create window
    hwnd = CreateWindowExA(
        0,
        "EngineDemo",
        "FP-ASM Engine MVP - ECS Architecture Showcase",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        100, 100, WINDOW_WIDTH, WINDOW_HEIGHT,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) {
        printf("Failed to create window!\n");
        return 1;
    }

    // Get device context
    hdc = GetDC(hwnd);

    // Set pixel format with MSAA support
    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR), 1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA, 32,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        24, 8, 0,
        PFD_MAIN_PLANE, 0, 0, 0, 0
    };

    int pixelFormat = ChoosePixelFormat(hdc, &pfd);
    if (!pixelFormat) {
        printf("Failed to choose pixel format!\n");
        return 1;
    }

    if (!SetPixelFormat(hdc, pixelFormat, &pfd)) {
        printf("Failed to set pixel format!\n");
        return 1;
    }

    // Create temporary context to get extension functions
    HGLRC tempContext = wglCreateContext(hdc);
    wglMakeCurrent(hdc, tempContext);

    // Try to enable MSAA if available
    typedef BOOL (WINAPI * PFNWGLCHOOSEPIXELFORMATARBPROC)(HDC, const int*, const FLOAT*, UINT, int*, UINT*);
    PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB =
        (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");

    if (wglChoosePixelFormatARB) {
        // MSAA attributes
        int msaaPixelFormat;
        UINT numFormats;
        int attribs[] = {
            0x2001, 1,      // WGL_DRAW_TO_WINDOW_ARB
            0x2010, 1,      // WGL_SUPPORT_OPENGL_ARB
            0x2011, 1,      // WGL_DOUBLE_BUFFER_ARB
            0x2013, 0x2027, // WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB
            0x2014, 32,     // WGL_COLOR_BITS_ARB
            0x2022, 24,     // WGL_DEPTH_BITS_ARB
            0x2023, 8,      // WGL_STENCIL_BITS_ARB
            0x2041, 1,      // WGL_SAMPLE_BUFFERS_ARB
            0x2042, 8,      // WGL_SAMPLES_ARB (8x MSAA)
            0
        };

        if (wglChoosePixelFormatARB(hdc, attribs, NULL, 1, &msaaPixelFormat, &numFormats) && numFormats > 0) {
            // Delete temp context and recreate with MSAA
            wglMakeCurrent(NULL, NULL);
            wglDeleteContext(tempContext);
            ReleaseDC(hwnd, hdc);
            DestroyWindow(hwnd);

            // Recreate window with MSAA pixel format
            hwnd = CreateWindowExA(
                0, "EngineDemo",
                "FP-ASM Engine MVP - ECS Architecture Showcase",
                WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                100, 100, WINDOW_WIDTH, WINDOW_HEIGHT,
                NULL, NULL, hInstance, NULL
            );
            hdc = GetDC(hwnd);
            SetPixelFormat(hdc, msaaPixelFormat, &pfd);
            printf("MSAA 8x enabled successfully!\n");
        } else {
            printf("MSAA not available, using standard pixel format\n");
            wglMakeCurrent(NULL, NULL);
            wglDeleteContext(tempContext);
        }
    } else {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(tempContext);
    }

    // Create OpenGL context
    hglrc = wglCreateContext(hdc);
    if (!hglrc) {
        printf("Failed to create OpenGL context!\n");
        return 1;
    }

    wglMakeCurrent(hdc, hglrc);

    // Load OpenGL extensions
    if (!gl_load_extensions()) {
        printf("Failed to load OpenGL extensions!\n");
        return 1;
    }

    printf("==============================================\n");
    printf("  FP-ASM Engine MVP Demo\n");
    printf("==============================================\n");
    printf("Architecture: Entity Component System (ECS)\n");
    printf("Resolution: %dx%d\n", WINDOW_WIDTH, WINDOW_HEIGHT);
    printf("Target: 60 FPS\n");
    printf("\n");
    printf("Features:\n");
    printf("  - %d cube entities with PBR materials\n", NUM_CUBES);
    printf("  - 2 directional lights\n");
    printf("  - Physically-based rendering (Cook-Torrance BRDF)\n");
    printf("  - Procedural animation (Perlin noise)\n");
    printf("  - Real-time parameter tweaking\n");
    printf("  - 8x MSAA anti-aliasing\n");
    printf("\n");
    printf("MOUSE Controls (CAD-style):\n");
    printf("  Left-drag   - Orbit camera around scene\n");
    printf("  Right-drag  - Pan camera (move view)\n");
    printf("  Mouse wheel - Zoom in/out\n");
    printf("\n");
    printf("QUALITY PRESETS:\n");
    printf("  F1 - Low (60+ FPS)    F2 - Medium (45-60 FPS)\n");
    printf("  F3 - High (30-45 FPS)  F4 - Ultra (Raytracing)\n");
    printf("\n");
    printf("QUALITY TOGGLES:\n");
    printf("  O - SSAO     P - PBR        H - Shadows\n");
    printf("  B - Bloom    M - MSAA       G - Gamma\n");
    printf("\n");
    printf("KEYBOARD Controls:\n");
    printf("  SPACE       - Pause/play animation\n");
    printf("  CTRL+W      - Toggle wireframe mode\n");
    printf("  UP/DOWN     - Adjust global roughness\n");
    printf("  LEFT/RIGHT  - Adjust global metallic\n");
    printf("  1/2         - Adjust light 1 intensity\n");
    printf("  3/4         - Adjust light 2 intensity\n");
    printf("  ESC         - Exit\n");
    printf("==============================================\n\n");

    // Initialize Perlin noise
    init_perlin();

    // Setup OpenGL
    setup_opengl();

    // Create ECS world
    // world = ecs_world_create();
    // if (!world) {
    //     printf("Failed to create ECS world!\n");
    //     return 1;
    // }

    // Create scene
    create_scene();

    // Main loop
    MSG msg;
    BOOL running = TRUE;
    clock_t last_frame_time = clock();
    last_fps_time = clock();

    while (running) {
        // Handle messages
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                running = FALSE;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // Calculate delta time
        clock_t current_time = clock();
        float dt = (float)(current_time - last_frame_time) / CLOCKS_PER_SEC * 1000.0f;
        last_frame_time = current_time;
        frame_time = dt;

        // Update FPS counter and window title
        frame_count++;
        float elapsed = (float)(current_time - last_fps_time) / CLOCKS_PER_SEC;
        if (elapsed >= 0.5f) {  // Update twice per second
            fps = frame_count / elapsed;
            printf("FPS: %.1f | Frame time: %.2f ms | Objects: %d\n",
                   fps, frame_time, app_state.object_count);

            // Update window title with FPS and object count
            char title[256];
            sprintf(title, "FP-ASM Engine MVP [FP] - %.1f FPS | %.2f ms | %d objects | Left-drag: Orbit | Right-drag: Pan | Scroll: Zoom",
                    fps, frame_time, app_state.object_count);
            SetWindowTextA(hwnd, title);

            frame_count = 0;
            last_fps_time = current_time;
        }

        // Update systems
        update_camera();
        update_light_matrices(); // Update light matrices before rendering

        // Only animate if not paused
        // if (!animation_paused) {
        //     animate_cubes(dt);
        // }

        // Render
        render_world();

        // Swap buffers
        SwapBuffers(hdc);

        // Limit framerate (roughly 60 FPS)
        Sleep(1);
    }

    // Cleanup
    printf("\nCleaning up...\n");

    if (renderer) {
        renderer_destroy(renderer);
    }

    // if (pbr_shader_program) {
    //     glDeleteProgram(pbr_shader_program);
    // }

    // if (world) {
    //     ecs_world_destroy(world);
    // }

    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hglrc);
    ReleaseDC(hwnd, hdc);

    printf("Demo finished.\n");
    return 0;
}
