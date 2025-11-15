# FP-First Graphics Engine Architecture

**Mission:** Build a graphics engine that showcases the FP-ASM library through pure functional composition.

**Core Principle:** Every computation must be expressible as composition of FP library functions.

---

## Architectural Foundation

### Rule #1: Immutable State Everywhere

```c
// ✅ GOOD - Immutable config
typedef struct {
    const int width;
    const int height;
    const float fov;
    const float near_plane;
    const float far_plane;
} CameraConfig;

// ❌ BAD - Mutable state
typedef struct {
    int width;      // Can be mutated!
    int height;     // Can be mutated!
    bool dirty;     // Mutation flag!
} Camera;
```

### Rule #2: Pure Functions Return New State

```c
// ✅ GOOD - Pure transformation
CameraConfig camera_set_fov(CameraConfig config, float new_fov) {
    CameraConfig new_config = config;  // Copy
    new_config.fov = new_fov;          // Modify copy
    return new_config;                  // Return new state
}

// ❌ BAD - Mutation
void camera_set_fov(Camera* camera, float fov) {
    camera->fov = fov;  // MUTATION!
    camera->dirty = true;  // MORE MUTATION!
}
```

### Rule #3: Computation Through FP Library

```c
// ✅ GOOD - Uses FP library
float compute_total_brightness(const float* pixels, size_t count) {
    return fp_reduce_add_f32(pixels, count);  // FP library!
}

// ❌ BAD - Imperative loop
float compute_total_brightness(const float* pixels, size_t count) {
    float total = 0.0f;
    for (size_t i = 0; i < count; ++i) {  // IMPERATIVE!
        total += pixels[i];                 // MUTATION!
    }
    return total;
}
```

---

## Core Data Structures (All Immutable)

### Scene State

```c
/**
 * Immutable scene configuration
 * Created once, passed everywhere by value
 */
typedef struct {
    const CameraConfig camera;
    const LightConfig lights[MAX_LIGHTS];
    const int light_count;
    const RenderConfig render_settings;
} SceneConfig;
```

### Render Buffers (Read-Only)

```c
/**
 * Immutable buffer references
 * Pointers are const, data is const
 */
typedef struct {
    const float* const positions;    // Vertex positions
    const float* const normals;      // Vertex normals
    const float* const uvs;          // Texture coordinates
    const size_t vertex_count;
} MeshData;
```

### Transform State

```c
/**
 * Immutable transform
 * Matrix computed from position/rotation/scale
 */
typedef struct {
    const float position[3];
    const float rotation[4];  // Quaternion
    const float scale[3];
    const float matrix[16];   // Computed once, immutable
} Transform;
```

---

## FP-First Operations

### Matrix Operations (Using FP Library)

```c
/**
 * Matrix-vector multiply using fp_fold_dotp_f32
 *
 * FP Composition:
 *   Each row × vector = fp_fold_dotp_f32(row, vec, 4)
 *   Result[i] = dot(matrix[i], vec)
 */
void matrix_vec_multiply_fp(
    const float matrix[16],  // 4x4 matrix
    const float vec[4],       // Input vector
    float result[4]           // Output vector
) {
    // Row 0
    result[0] = fp_fold_dotp_f32(&matrix[0], vec, 4);
    // Row 1
    result[1] = fp_fold_dotp_f32(&matrix[4], vec, 4);
    // Row 2
    result[2] = fp_fold_dotp_f32(&matrix[8], vec, 4);
    // Row 3
    result[3] = fp_fold_dotp_f32(&matrix[12], vec, 4);
}
```

### Vertex Transformation (Using FP Library)

```c
/**
 * Transform all vertices using FP library
 *
 * FP Composition:
 *   For each vertex: matrix × vertex_pos
 *   Using fp_fold_dotp_f32 for each component
 */
void transform_vertices_fp(
    const float* positions,    // Input vertex positions [x,y,z,w,...]
    float* transformed,        // Output transformed positions
    const float matrix[16],    // Transform matrix
    size_t vertex_count
) {
    for (size_t i = 0; i < vertex_count; ++i) {
        const float* vertex = &positions[i * 4];
        float* result = &transformed[i * 4];

        // Each component uses fp_fold_dotp_f32
        result[0] = fp_fold_dotp_f32(&matrix[0], vertex, 4);
        result[1] = fp_fold_dotp_f32(&matrix[4], vertex, 4);
        result[2] = fp_fold_dotp_f32(&matrix[8], vertex, 4);
        result[3] = fp_fold_dotp_f32(&matrix[12], vertex, 4);
    }
}
```

### Lighting Calculation (Using FP Library)

```c
/**
 * Compute diffuse lighting using FP library
 *
 * FP Composition:
 *   1. Dot product (normal · light_dir) = fp_fold_dotp_f32
 *   2. Clamp to [0,1] = max(0, dot)
 *   3. Scale by intensity = fp_map_scale_f32
 */
float compute_diffuse_fp(
    const float normal[3],
    const float light_dir[3],
    float intensity
) {
    // FP library: dot product
    float dot = fp_fold_dotp_f32(normal, light_dir, 3);

    // Clamp to [0, 1]
    float clamped = (dot > 0.0f) ? dot : 0.0f;

    // Scale by intensity
    return clamped * intensity;
}
```

### SSAO (Already Implemented Correctly!)

```c
/**
 * SSAO computation - CORRECT FP-FIRST PATTERN
 *
 * See demo_fp_ssao.c for full implementation
 * Uses fp_reduce_add_f32 for occlusion accumulation
 */
float ssao_compute_pixel_fp(/* ... */) {
    // ... generate occlusion flags ...

    // FP LIBRARY: Sum occlusion
    float total = fp_reduce_add_f32(occlusion_flags, sample_count);

    return 1.0f - (total / (float)sample_count);
}
```

---

## Module Design

### Module 1: fp_graphics_transforms.c

```c
// Pure transformation functions using FP library

// Matrix operations
void fp_mat4_mul_vec4(const float m[16], const float v[4], float r[4]);
void fp_mat4_mul_mat4(const float a[16], const float b[16], float r[16]);

// Vertex transformations
void fp_transform_positions(const float* in, float* out, const float m[16], size_t n);
void fp_transform_normals(const float* in, float* out, const float m[16], size_t n);

// All use fp_fold_dotp_f32 internally
```

### Module 2: fp_graphics_lighting.c

```c
// Lighting calculations using FP library

// Diffuse lighting for all vertices
void fp_lighting_diffuse(const float* normals, float* intensities,
                         const float light_dir[3], float strength, size_t n);

// Uses fp_fold_dotp_f32 for normal·light_dir
// Uses fp_map_scale_f32 for intensity scaling
```

### Module 3: fp_graphics_ssao.c

```c
// SSAO implementation (already done!)
// See demo_fp_ssao.c

float fp_ssao_compute_pixel(/* ... */);
void fp_ssao_compute_image(/* ... */);

// Uses fp_reduce_add_f32, fp_reduce_min_f32, fp_reduce_max_f32
```

### Module 4: fp_graphics_post.c

```c
// Post-processing effects using FP library

// Bloom/blur - convolution using FP operations
void fp_post_blur(const float* in, float* out, int w, int h);

// Tone mapping
void fp_post_tonemap(const float* hdr, float* ldr, size_t n, float exposure);

// Uses fp_map_scale_f32, fp_reduce_max_f32, etc.
```

---

## Example: FP-First Render Pipeline

```c
/**
 * Complete render pipeline - FP-first style
 *
 * All computation through FP library functions
 * Immutable state throughout
 */

typedef struct {
    const SceneConfig scene;
    const MeshData mesh;
    const float* depth_buffer;
    float* output_image;
    const int width;
    const int height;
} RenderContext;

void fp_render_pipeline(const RenderContext ctx) {
    // Step 1: Transform vertices (using fp_fold_dotp_f32)
    float* transformed = malloc(ctx.mesh.vertex_count * 4 * sizeof(float));
    fp_transform_positions(
        ctx.mesh.positions,
        transformed,
        ctx.scene.camera.view_projection_matrix,
        ctx.mesh.vertex_count
    );

    // Step 2: Lighting (using fp_fold_dotp_f32, fp_map_scale_f32)
    float* lit_vertices = malloc(ctx.mesh.vertex_count * sizeof(float));
    fp_lighting_diffuse(
        ctx.mesh.normals,
        lit_vertices,
        ctx.scene.lights[0].direction,
        ctx.scene.lights[0].intensity,
        ctx.mesh.vertex_count
    );

    // Step 3: SSAO (using fp_reduce_add_f32)
    fp_ssao_compute_image(
        ctx.depth_buffer,
        ctx.output_image,
        /* ... kernel data ... */,
        ctx.width,
        ctx.height
    );

    // Step 4: Post-processing (using fp_map_*, fp_reduce_*)
    fp_post_tonemap(
        ctx.output_image,
        ctx.output_image,
        ctx.width * ctx.height,
        ctx.scene.render_settings.exposure
    );

    // Cleanup
    free(transformed);
    free(lit_vertices);
}
```

---

## FP Library Usage Map

| Graphics Operation | FP Library Function |
|-------------------|---------------------|
| Matrix × Vector | `fp_fold_dotp_f32` |
| Normal · Light Direction | `fp_fold_dotp_f32` |
| SSAO Occlusion Sum | `fp_reduce_add_f32` |
| Find Brightest Pixel | `fp_reduce_max_f32` |
| Scale Light Intensity | `fp_map_scale_f32` |
| Offset Brightness | `fp_map_offset_f32` |
| Combine Buffers | `fp_zip_add_f32` |
| Variance Calculation | `fp_fold_sumsq_f32` |

---

## Success Criteria

✅ **Zero imperative loops in hot paths**
✅ **All buffers marked `const`**
✅ **Every operation uses FP library**
✅ **Pure functions everywhere**
✅ **Immutable configuration**
✅ **Functional composition visible**

---

## Implementation Plan

### Phase 1: Core Transforms
- `fp_graphics_transforms.c` - Matrix ops using `fp_fold_dotp_f32`
- Test: Transform 10,000 vertices, verify correctness

### Phase 2: Lighting
- `fp_graphics_lighting.c` - Lighting using `fp_fold_dotp_f32` + `fp_map_scale_f32`
- Test: Compute lighting for sphere, visual output

### Phase 3: SSAO (Already Done!)
- ✅ `demo_fp_ssao.c` - Complete working example
- ✅ Uses `fp_reduce_add_f32` properly

### Phase 4: Post-Processing
- `fp_graphics_post.c` - Blur, tonemap using FP library
- Test: Apply effects to image, visual output

### Phase 5: Integration
- Combine all modules into unified pipeline
- Demo: Render scene with all effects
- Visual proof of FP-first graphics

---

**This is the path forward: FP library everywhere, immutability always, pure functions only.**
