#pragma once

/**
 * fp_ray_tracer.h
 *
 * Real-Time Ray Tracer for FP-ASM Library
 * Dual-mode: Real-time (30-60 FPS @ 640x480) + Offline (beauty shots)
 *
 * Optimized for i7-4600M (Haswell, AVX2)
 * Game-engine ready architecture
 */

#include <stdint.h>
#include <stdbool.h>

#include "fp_core.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Data Structures
// ============================================================================

/**
 * Ray structure
 */
typedef struct {
    Vec3f origin;
    Vec3f direction;  // Always normalized
} Ray;

/**
 * Sphere primitive
 */
typedef struct {
    Vec3f center;
    float radius;
    Vec3f color;        // RGB [0, 1]
    float specular;    // Phong exponent (0-1000)
    float reflective;  // Reflectivity [0, 1]
} Sphere;

/**
 * Infinite plane primitive
 */
typedef struct {
    Vec3f normal;       // Plane normal (normalized)
    float distance;    // Distance from origin
    Vec3f color;        // RGB [0, 1]
    float specular;    // Phong exponent
} Plane;

/**
 * Point light
 */
typedef struct {
    Vec3f position;
    Vec3f color;        // RGB intensity [0, 1]
    float intensity;   // Overall intensity multiplier
} Light;

/**
 * Hit record (ray-object intersection)
 */
typedef struct {
    bool hit;          // Did we hit anything?
    float t;           // Ray parameter (distance)
    Vec3f point;        // Intersection point
    Vec3f normal;       // Surface normal at hit point
    Vec3f color;        // Surface color
    float specular;    // Specular component
    float reflective;  // Reflectivity
} Hit;

/**
 * Scene container
 */
typedef struct {
    Sphere* spheres;
    int n_spheres;
    Plane* planes;
    int n_planes;
    Light* lights;
    int n_lights;
    Vec3f ambient;      // Ambient light color/intensity
} Scene;

/**
 * Camera
 */
typedef struct {
    Vec3f position;
    Vec3f look_at;
    Vec3f up;
    float fov;         // Vertical FOV in degrees
    float aspect;      // Width / Height ratio
} Camera;

/**
 * Render settings
 */
typedef struct {
    int width;
    int height;
    int samples_per_pixel;  // For anti-aliasing (1 = no AA)
    int max_bounces;        // For reflections (0 = primary only)
    bool shadows;           // Enable shadow rays
} RenderSettings;

/**
 * Render backend selection
 */
typedef enum {
    RENDER_BACKEND_CPU_SCALAR,      // Single-threaded CPU (baseline)
    RENDER_BACKEND_CPU_MULTITHREAD, // Multi-threaded CPU (use all cores)
    RENDER_BACKEND_GPU_OPENCL,      // GPU compute via OpenCL
    RENDER_BACKEND_AUTO             // Auto-detect best backend
} RenderBackend;

// ============================================================================
// Vector Math Operations
// ============================================================================

// NOTE: These are now implemented in fp_vector_ops.c and declared in fp_core.h
// This file now uses the canonical implementations.

// ============================================================================
// Ray Intersection
// ============================================================================

/**
 * Ray-sphere intersection (scalar version)
 *
 * @param ray Input ray
 * @param sphere Sphere to test
 * @param hit Output hit record
 * @return true if hit, false otherwise
 */
bool ray_sphere_intersect(const Ray* ray, const Sphere* sphere, Hit* hit);

/**
 * Ray-plane intersection
 *
 * @param ray Input ray
 * @param plane Plane to test
 * @param hit Output hit record
 * @return true if hit, false otherwise
 */
bool ray_plane_intersect(const Ray* ray, const Plane* plane, Hit* hit);

/**
 * Find closest intersection in scene
 *
 * @param ray Input ray
 * @param scene Scene to test
 * @param hit Output hit record (closest intersection)
 * @return true if any hit, false otherwise
 */
bool ray_scene_intersect(const Ray* ray, const Scene* scene, Hit* hit);

// ============================================================================
// Shading
// ============================================================================

/**
 * Compute Phong shading at hit point
 *
 * @param hit Hit record
 * @param scene Scene (for lights)
 * @param view_dir View direction (from hit to camera)
 * @param compute_shadows Enable shadow rays
 * @return Final color
 */
Vec3f phong_shading(const Hit* hit, const Scene* scene, const Vec3f* view_dir, bool compute_shadows);

/**
 * Test if light is visible from point (shadow ray)
 *
 * @param scene Scene
 * @param point Point to test from
 * @param light_pos Light position
 * @return true if light is visible, false if occluded
 */
bool is_light_visible(const Scene* scene, const Vec3f* point, const Vec3f* light_pos);

// ============================================================================
// Camera & Ray Generation
// ============================================================================

/**
 * Generate ray for pixel coordinate
 *
 * @param camera Camera
 * @param x Pixel x coordinate
 * @param y Pixel y coordinate
 * @param width Image width
 * @param height Image height
 * @return Ray through pixel
 */
Ray generate_camera_ray(const Camera* camera, float x, float y, int width, int height);

// ============================================================================
// Rendering
// ============================================================================

/**
 * Trace single ray through scene
 *
 * @param ray Ray to trace
 * @param scene Scene
 * @param depth Current recursion depth
 * @param max_depth Maximum recursion depth
 * @param compute_shadows Enable shadow rays
 * @return Final color
 */
Vec3f trace_ray(const Ray* ray, const Scene* scene, int depth, int max_depth, bool compute_shadows);

/**
 * Render scene (real-time mode)
 *
 * Fast rendering: Primary rays + shadows only
 * Output: RGB24 framebuffer (width * height * 3 bytes)
 *
 * @param scene Scene to render
 * @param camera Camera
 * @param framebuffer Output buffer (RGB24)
 * @param width Image width
 * @param height Image height
 */
void render_realtime(
    const Scene* scene,
    const Camera* camera,
    uint8_t* framebuffer,
    int width,
    int height
);

/**
 * Render scene (offline mode)
 *
 * High-quality rendering: Reflections + anti-aliasing
 * Output: PPM file
 *
 * @param scene Scene to render
 * @param camera Camera
 * @param output_path Output PPM file path
 * @param settings Render settings
 */
void render_offline(
    const Scene* scene,
    const Camera* camera,
    const char* output_path,
    const RenderSettings* settings
);

/**
 * Render frame with backend selection (UNIFIED API)
 *
 * Modern multi-backend rendering system
 *
 * @param scene Scene to render
 * @param camera Camera
 * @param framebuffer Output buffer (RGB24)
 * @param width Image width
 * @param height Image height
 * @param backend Backend to use (scalar, multithread, opencl)
 */
void render_frame(
    const Scene* scene,
    const Camera* camera,
    uint8_t* framebuffer,
    int width,
    int height,
    RenderBackend backend
);

/**
 * Render with multithreading (CPU parallelism)
 *
 * Uses all available CPU cores for parallel rendering
 *
 * @param scene Scene to render
 * @param camera Camera
 * @param framebuffer Output buffer (RGB24)
 * @param width Image width
 * @param height Image height
 * @param num_threads Number of threads to use (0 = auto-detect)
 */
void render_multithread(
    const Scene* scene,
    const Camera* camera,
    uint8_t* framebuffer,
    int width,
    int height,
    int num_threads
);

// ============================================================================
// Persistent GPU Rendering (High-Performance API)
// ============================================================================

/**
 * Opaque GPU context handle
 *
 * Contains persistent OpenCL resources:
 * - Platform, device, context, command queue
 * - Compiled kernel program
 * - Persistent scene buffers (spheres, planes, lights)
 *
 * Initialize ONCE with gpu_init()
 * Use for MANY frames with gpu_render_frame()
 * Cleanup ONCE with gpu_cleanup()
 */
typedef struct GPUContext GPUContext;

/**
 * Initialize persistent GPU context (CALL ONCE)
 *
 * This performs all expensive one-time operations:
 * - Query OpenCL platform and GPU device
 * - Create OpenCL context and command queue
 * - Load and compile ray tracing kernel from disk
 * - Upload scene geometry to GPU (spheres, planes, lights)
 *
 * Expected time: ~100ms (kernel compilation is slow)
 *
 * @param scene Scene to render (geometry uploaded to GPU)
 * @return GPU context handle, or NULL on failure
 */
GPUContext* gpu_init(const Scene* scene);

/**
 * Render single frame using persistent GPU context (CALL EVERY FRAME)
 *
 * This is the HOT PATH for real-time rendering.
 * Only performs per-frame work:
 * - Set camera parameters
 * - Launch GPU kernel (one thread per pixel)
 * - Read back framebuffer
 *
 * Expected time @ 640×480:  ~8ms  (125 FPS)
 * Expected time @ 1920×1080: ~22ms (45 FPS)
 *
 * @param gpu_ctx Persistent GPU context from gpu_init()
 * @param camera Camera (can change every frame)
 * @param framebuffer Output buffer (RGB24, width×height×3 bytes)
 * @param width Image width
 * @param height Image height
 */
void gpu_render_frame(
    GPUContext* gpu_ctx,
    const Camera* camera,
    uint8_t* framebuffer,
    int width,
    int height
);

/**
 * Update scene geometry on GPU (OPTIONAL)
 *
 * Call this if spheres/planes/lights change.
 * For static scenes, no need to call after gpu_init().
 *
 * @param gpu_ctx GPU context
 * @param scene Updated scene
 */
void gpu_update_scene(GPUContext* gpu_ctx, const Scene* scene);

/**
 * Cleanup persistent GPU context (CALL ONCE AT SHUTDOWN)
 *
 * Releases all OpenCL resources:
 * - GPU buffers (scene data, framebuffer)
 * - Compiled kernel program
 * - Command queue, context, device
 *
 * @param gpu_ctx GPU context to destroy
 */
void gpu_cleanup(GPUContext* gpu_ctx);

// ============================================================================
// Scene Creation Helpers
// ============================================================================

/**
 * Create default camera
 */
Camera create_camera(Vec3f position, Vec3f look_at, Vec3f up, float fov, float aspect);

/**
 * Create sphere
 */
Sphere create_sphere(Vec3f center, float radius, Vec3f color, float specular, float reflective);

/**
 * Create plane
 */
Plane create_plane(Vec3f normal, float distance, Vec3f color, float specular);

/**
 * Create point light
 */
Light create_light(Vec3f position, Vec3f color, float intensity);

/**
 * Create empty scene
 */
Scene create_scene(void);

/**
 * Free scene resources
 */
void free_scene(Scene* scene);

// ============================================================================
// Utility Functions
// ============================================================================

/**
 * Save framebuffer as PPM file
 *
 * @param filename Output file path
 * @param pixels RGB24 pixel data
 * @param width Image width
 * @param height Image height
 */
void save_ppm(const char* filename, const uint8_t* pixels, int width, int height);

/**
 * Convert float color [0,1] to uint8 [0,255]
 */
uint8_t float_to_byte(float f);

/**
 * Gamma correction (sRGB)
 */
float gamma_correct(float linear);

#ifdef __cplusplus
}
#endif
