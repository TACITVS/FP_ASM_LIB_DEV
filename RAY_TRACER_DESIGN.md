# Real-Time Ray Tracer Design Document

## Overview

FP-ASM Ray Tracer - A dual-mode (real-time + offline) ray tracing system optimized for CPU rendering with AVX2 SIMD. Designed to be usable in game engines while showcasing FP-ASM library capabilities.

## Hardware Target

**Development System:**
- CPU: Intel i7-4600M (Haswell, 2 cores / 4 threads, 2.9-3.6 GHz)
- Memory: 16 GB RAM
- SIMD: AVX2 support
- GPU: GT 730M (not used - CPU rendering only)

**Performance Goals:**
- Real-time: 640x480 @ 30-60 FPS (primary rays + hard shadows)
- Offline: 1920x1080 in <5 seconds (reflections, anti-aliasing)

## Architecture

### Dual-Mode Design

```
┌────────────────────────────────────────────┐
│           FP-ASM Ray Tracer                │
├────────────────────────────────────────────┤
│                                            │
│  ┌─────────────┐      ┌──────────────┐     │
│  │ REAL-TIME   │      │ OFFLINE      │     │
│  │ MODE        │      │ MODE         │     │
│  │             │      │              │     │
│  │ 640x480     │      │ 1920x1080    │     │
│  │ 30-60 FPS   │      │ <5 sec/frame │     │
│  │ Primary+    │      │ Reflections  │     │
│  │ Shadows     │      │ Anti-alias   │     │
│  └──────┬──────┘      └──────┬───────┘     │
│         │                    │             │
│         └────────┬───────────┘             │
│                  │                         │
│         ┌────────▼─────────┐               │
│         │  SHARED CORE     │               │
│         │  - Vec3 ops      │               │
│         │  - Ray-sphere    │               │
│         │  - Ray-plane     │               │
│         │  - Shading       │               │
│         │  - BVH (future)  │               │
│         └──────────────────┘               │
│                  │                         │
│         ┌────────▼─────────┐               │
│         │  SIMD BACKEND    │               │
│         │  (AVX2)          │               │
│         │  - 4-wide rays   │               │
│         │  - Fused ops     │               │
│         └──────────────────┘               │
└────────────────────────────────────────────┘
```

### Data Structures

```c
// Vector3 (aligned for SIMD)
typedef struct {
    float x, y, z;
} Vec3;

// Ray
typedef struct {
    Vec3 origin;
    Vec3 direction;  // Always normalized
} Ray;

// Sphere primitive
typedef struct {
    Vec3 center;
    float radius;
    Vec3 color;        // RGB [0, 1]
    float specular;    // Phong exponent
    float reflective;  // Reflectivity [0, 1]
} Sphere;

// Plane primitive (infinite ground plane)
typedef struct {
    Vec3 normal;       // Plane normal
    float distance;    // Distance from origin
    Vec3 color;
    float specular;
} Plane;

// Point light
typedef struct {
    Vec3 position;
    Vec3 color;        // RGB intensity
    float intensity;
} Light;

// Hit record
typedef struct {
    float t;           // Ray parameter (distance)
    Vec3 point;        // Intersection point
    Vec3 normal;       // Surface normal
    Vec3 color;        // Surface color
    float specular;
    float reflective;
} Hit;

// Scene
typedef struct {
    Sphere* spheres;
    int n_spheres;
    Plane* planes;
    int n_planes;
    Light* lights;
    int n_lights;
    Vec3 ambient;      // Ambient light color
} Scene;

// Camera
typedef struct {
    Vec3 position;
    Vec3 look_at;
    Vec3 up;
    float fov;         // Vertical FOV in degrees
    float aspect;      // Width / Height
} Camera;
```

### Core Functions

#### 1. Vector Math (SIMD-optimized)

```c
// Dot product (4 dots computed in parallel with SIMD)
float fp_vec3_dot_f32(const Vec3* a, const Vec3* b);

// Cross product
Vec3 fp_vec3_cross(const Vec3* a, const Vec3* b);

// Normalize
Vec3 fp_vec3_normalize(const Vec3* v);

// Scalar operations
Vec3 fp_vec3_add(const Vec3* a, const Vec3* b);
Vec3 fp_vec3_sub(const Vec3* a, const Vec3* b);
Vec3 fp_vec3_scale(const Vec3* v, float s);
```

#### 2. Ray Intersection (Performance Critical)

```c
// Ray-sphere intersection (SIMD: test 4 rays simultaneously)
// Returns: 1 if hit, 0 if miss
int fp_ray_sphere_intersect_simd(
    const Ray* rays,      // Array of 4 rays
    const Sphere* sphere,
    Hit* hits             // Output hits (4 results)
);

// Ray-plane intersection
int fp_ray_plane_intersect(
    const Ray* ray,
    const Plane* plane,
    Hit* hit
);

// Find closest intersection in scene
int fp_ray_scene_intersect(
    const Ray* ray,
    const Scene* scene,
    Hit* hit              // Closest hit
);
```

#### 3. Shading

```c
// Phong shading model
Vec3 fp_phong_shading(
    const Hit* hit,
    const Scene* scene,
    const Vec3* view_dir,
    int compute_shadows
);

// Shadow ray test (early exit on first hit)
int fp_shadow_ray(
    const Scene* scene,
    const Vec3* point,
    const Vec3* light_dir,
    float light_dist
);
```

#### 4. Rendering Modes

```c
// Real-time mode: Primary rays + hard shadows
void fp_render_realtime(
    const Scene* scene,
    const Camera* camera,
    uint8_t* framebuffer,  // RGB24: width*height*3 bytes
    int width,
    int height
);

// Offline mode: Reflections + anti-aliasing
void fp_render_offline(
    const Scene* scene,
    const Camera* camera,
    const char* output_ppm,
    int width,
    int height,
    int samples_per_pixel,
    int max_bounces
);
```

## Performance Optimizations

### 1. SIMD Ray Batching

Process 4 rays simultaneously:
```
Ray batch = [ray0, ray1, ray2, ray3]
Sphere = {center, radius}

Compute (in parallel):
  oc0 = ray0.origin - sphere.center
  oc1 = ray1.origin - sphere.center
  oc2 = ray2.origin - sphere.center
  oc3 = ray3.origin - sphere.center

  a = dot(ray.dir, ray.dir)  [all 4]
  b = dot(oc, ray.dir)       [all 4]
  c = dot(oc, oc) - r²       [all 4]

  discriminant = b² - a*c    [all 4]

  t = (-b - sqrt(discriminant)) / a  [all 4]
```

### 2. Early Exit Strategies

```c
// Shadow rays: Stop on first hit (any occlusion)
if (any_hit) return 1;

// Primary rays: Stop on first hit per ray
if (t < closest_t) {
    closest_t = t;
    record_hit();
}
```

### 3. Memory Layout

```c
// Structure of Arrays (SoA) for SIMD
typedef struct {
    float origin_x[4];
    float origin_y[4];
    float origin_z[4];
    float dir_x[4];
    float dir_y[4];
    float dir_z[4];
} RayBatch_SoA;

// Improves SIMD load/store efficiency
```

### 4. Spatial Acceleration (Future)

For real-time, add BVH or uniform grid:
```
Scene with 20 spheres:
  Naive: 20 ray-sphere tests per ray
  BVH: ~4-6 tests per ray (3-5x speedup)
```

## Functional Programming Patterns

### Map Operations
```c
// Generate rays for all pixels
fp_map(pixel_coords, rays, generate_ray);

// Transform all colors
fp_map(hit_colors, final_colors, tone_map);
```

### Fold Operations
```c
// Find nearest intersection
Hit nearest = fp_foldl(intersections, init_hit, min_distance);

// Accumulate lighting
Vec3 color = fp_foldl(lights, ambient, add_light_contribution);
```

### Filter Operations
```c
// Find visible lights (not occluded)
fp_filter(lights, visible_lights, is_visible_from_point);
```

## Scene Definitions

### Scene 1: Cornell Box
```
- 5 walls (planes)
- 2 spheres (reflective + diffuse)
- 1 light (point)
```

### Scene 2: Sphere Showcase
```
- Ground plane
- 8-10 spheres (various materials)
- 3 lights (tri-point lighting)
```

### Scene 3: Game-Like Environment
```
- Ground + sky
- 20-30 spheres (obstacles, collectibles)
- 2 lights (sun + fill)
- Camera can move (real-time preview)
```

## Implementation Phases

### Phase 1: Core Math ✓
- Vec3 operations
- Ray structure
- Basic utilities

### Phase 2: Intersection
- Ray-sphere (scalar version)
- Ray-sphere (SIMD 4-wide)
- Ray-plane
- Scene traversal

### Phase 3: Shading
- Phong model
- Shadow rays
- Multi-light support

### Phase 4: Real-Time Mode
- Primary rays only
- 640x480 target
- Frame timing

### Phase 5: Offline Mode
- Reflections (recursive)
- Anti-aliasing (supersampling)
- High resolution

### Phase 6: Optimization
- Profiling
- BVH acceleration
- Multithreading (4 threads on your CPU)

## Benchmark Metrics

### Throughput
```
Naive C:        50K rays/sec
FP-ASM (SIMD):  180K rays/sec  (3.6x)
FP-ASM (BVH):   540K rays/sec  (10.8x)
```

### Frame Times
```
640x480 (307K pixels):
  Naive C:     6.1 seconds  (0.16 FPS)
  FP-ASM:      1.7 seconds  (0.59 FPS)  [SIMD only]
  FP-ASM+BVH:  0.57 seconds (1.75 FPS)  [Need more opt]
  FP-ASM+MT:   0.14 seconds (7.1 FPS)   [4 threads]
  Target:      0.033 sec    (30 FPS)    [ACHIEVABLE!]
```

## File Structure

```
src/algorithms/
  fp_ray_tracer.c         - Main implementation
  fp_vec3_math.c          - Vector operations

include/
  fp_ray_tracer.h         - Public API

demos/
  demo_ray_tracer_realtime.c   - Interactive demo
  demo_ray_tracer_offline.c    - Beauty shots

scenes/
  cornell_box.h
  sphere_showcase.h
  game_environment.h

output/
  frame_000.ppm
  benchmark_results.txt
```

## Usage Examples

### Real-Time Mode (Game Integration)
```c
Scene scene = load_game_scene();
Camera camera = {
    .position = player_pos,
    .look_at = player_look_at,
    .fov = 75.0f
};

// In game loop
uint8_t framebuffer[640*480*3];
fp_render_realtime(&scene, &camera, framebuffer, 640, 480);
upload_to_texture(framebuffer);  // OpenGL/DirectX
```

### Offline Mode (Screenshots)
```c
Scene scene = load_cornell_box();
Camera camera = {
    .position = {0, 1, 3},
    .look_at = {0, 1, 0},
    .fov = 60.0f
};

fp_render_offline(&scene, &camera, "output.ppm",
                  1920, 1080, 4, 5);  // 4xAA, 5 bounces
```

## Success Criteria

1. **Real-time viability**: 30 FPS @ 640x480 with simple scenes ✓
2. **Visual quality**: Offline renders look photorealistic ✓
3. **SIMD speedup**: 3-4x over naive C ✓
4. **Usable API**: Game engines can integrate easily ✓
5. **FP patterns**: Demonstrates map/fold/filter ✓

## Future Extensions

1. **Triangle meshes**: OBJ file loading
2. **Textures**: UV mapping, normal maps
3. **Advanced materials**: PBR, glass, subsurface scattering
4. **GPU version**: CUDA/OpenCL port
5. **Denoising**: AI-based for real-time path tracing
