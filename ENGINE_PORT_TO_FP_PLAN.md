# 3D Engine: Port Old Imperative Code to FP-First

**Date**: November 2025
**Goal**: Port ALL algorithms from `src/engine_old/` to FP-first style in `src/engine/`

---

## Current Status

### ✅ Already Ported (src/engine/)
1. **Lighting** (`fp_graphics_lighting.c` - 291 lines)
   - Diffuse lighting (single + batch)
   - Directional lights
   - Point lights with attenuation
   - Multi-light accumulation
   - Ambient lighting

2. **Post-Processing** (`fp_graphics_postprocess.c` - 300 lines)
   - Box blur (horizontal)
   - Bloom effect
   - Tone mapping (Reinhard + Exposure)
   - Brightness adjustment
   - Contrast adjustment
   - Gamma correction

### ❌ NOT YET PORTED (Still in src/engine_old/)

Based on `renderer_modern.c` (1,090 lines), here are ALL algorithms that need FP-first implementations:

---

## ALGORITHMS TO PORT

### 1. SSAO (Screen-Space Ambient Occlusion)

**Location**: `src/engine_old/renderer_modern.c:842-1064` (223 lines)

#### Current Implementation (IMPERATIVE - VIOLATES FP):
```c
// SSAO kernel generation - IMPERATIVE LOOPS!
static void ssao_generate_kernel(Vec3* kernel, int sample_count) {
    for (int i = 0; i < sample_count; ++i) {  // ❌ IMPERATIVE LOOP
        float scale = (float)i / (float)sample_count;
        scale = 0.1f + scale * scale * 0.9f;

        float theta = 2.0f * M_PI * ((float)i / (float)sample_count);
        int pseudo_rand = (i * 7 + 13) % sample_count;
        float phi = acosf(1.0f - 2.0f * ((float)pseudo_rand / (float)sample_count));

        kernel[i].x = sinf(phi) * cosf(theta) * scale;  // ❌ MUTATION
        kernel[i].y = sinf(phi) * sinf(theta) * scale;  // ❌ MUTATION
        kernel[i].z = cosf(phi) * scale;                 // ❌ MUTATION
    }
}
```

#### FP-First Implementation Needed:
```c
/**
 * Generate SSAO kernel using FP library
 *
 * FP Composition:
 *   - Use fp_map to generate all samples in parallel
 *   - Return immutable kernel array
 *   - No loops, no mutations
 */
void fp_ssao_generate_kernel_fp(
    float* kernel_x,        // Output: X components
    float* kernel_y,        // Output: Y components
    float* kernel_z,        // Output: Z components
    int sample_count
) {
    // Generate indices [0, 1, 2, ..., sample_count-1]
    float* indices = malloc(sample_count * sizeof(float));
    for (int i = 0; i < sample_count; ++i) {
        indices[i] = (float)i;
    }

    // FP LIBRARY: Scale indices to [0, 1]
    float* scales = malloc(sample_count * sizeof(float));
    fp_map_scale_f32(indices, scales, sample_count, 1.0f / (float)sample_count);

    // FP LIBRARY: Apply quadratic falloff (scale^2)
    // scales = 0.1 + scales^2 * 0.9
    // This needs fp_map_square_f32 or similar

    // Generate theta and phi using FP primitives
    // ...

    // FP LIBRARY: Compute sin/cos using vectorized operations
    // kernel_x = sin(phi) * cos(theta) * scale
    // kernel_y = sin(phi) * sin(theta) * scale
    // kernel_z = cos(phi) * scale

    free(indices);
    free(scales);
}
```

#### Functions to Implement:
- `fp_ssao_generate_kernel()` - Hemisphere sample generation using FP library
- `fp_ssao_generate_noise()` - 4x4 noise texture using FP library
- `fp_ssao_compute_occlusion()` - AO calculation using `fp_reduce_add_f32`
- `fp_ssao_blur()` - Blur AO texture using `fp_zip_add_f32`

**Reference**: See `demo_fp_ssao.c` for correct FP-first SSAO implementation

---

### 2. Shadow Mapping

**Location**: `src/engine_old/renderer_modern.c:604-643` (40 lines)

#### Current Implementation (IMPERATIVE):
```c
static void render_shadow_pass(Renderer* renderer, ECSWorld* world,
                               LightComponent* light, Framebuffer* shadow_fb) {
    framebuffer_bind(shadow_fb);  // ❌ SIDE EFFECT
    glClear(GL_DEPTH_BUFFER_BIT);  // ❌ SIDE EFFECT

    shader_bind(renderer->shadow_shader);  // ❌ SIDE EFFECT

    // ❌ IMPERATIVE LOOP
    for (uint32_t i = 0; i < world->mesh_count; i++) {
        Entity entity = world->mesh_entities[i];
        // ... render mesh to shadow map
        glDrawElements(...);  // ❌ SIDE EFFECT
    }
}
```

#### FP-First Implementation Needed:
```c
/**
 * Shadow mapping using FP principles
 *
 * FP Approach:
 *   - Immutable light configuration
 *   - Pure function to compute light space matrices
 *   - Batch shadow map generation
 */

// PURE FUNCTION: Compute light space matrix
Mat4 fp_shadow_compute_light_matrix(
    const Vec3* light_pos,
    const Vec3* light_dir,
    float ortho_size
) {
    // Use fp_fold_dotp_f32 for view/projection computation
    // Return new matrix (no mutation)
}

// PURE FUNCTION: Check if vertex is in shadow
float fp_shadow_test_visibility(
    const float* vertex_world_pos,
    const float* light_space_matrix,
    const float* shadow_map_data,
    int shadow_map_width
) {
    // Use FP library for matrix × vector
    // Use FP library for depth comparison
    // Return 0.0 (shadow) or 1.0 (lit)
}
```

#### Functions to Implement:
- `fp_shadow_compute_light_matrix()` - Light space transform (pure function)
- `fp_shadow_test_visibility()` - Shadow test using FP primitives
- `fp_shadow_pcf()` - Percentage Closer Filtering using `fp_reduce_add_f32`

---

### 3. PBR (Physically-Based Rendering)

**Location**: `src/engine_old/renderer_modern.c:649-734` (86 lines) + shaders

#### Current Implementation (IMPERATIVE):
```c
static void render_geometry_pass(Renderer* renderer, ECSWorld* world,
                                 CameraComponent* camera, Vec3 view_pos) {
    // ❌ IMPERATIVE LOOP
    for (uint32_t i = 0; i < world->mesh_count; i++) {
        Entity entity = world->mesh_entities[i];

        TransformComponent* transform = ecs_get_transform(world, entity);
        MeshComponent* mesh = ecs_get_mesh(world, entity);
        MaterialComponent* material = ecs_get_material(world, entity);

        // Set uniforms (side effects)
        shader_set_mat4(...);  // ❌ SIDE EFFECT
        shader_set_vec3(...);  // ❌ SIDE EFFECT

        glDrawElements(...);   // ❌ SIDE EFFECT
    }
}
```

#### FP-First Implementation Needed:
```c
/**
 * PBR BRDF computation using FP library
 *
 * Cook-Torrance BRDF = (D * F * G) / (4 * (N·L) * (N·V))
 *
 * FP Composition:
 *   - All dot products via fp_fold_dotp_f32
 *   - Fresnel via fp_map (vectorized)
 *   - GGX normal distribution via fp_map
 */

// PURE FUNCTION: Fresnel-Schlick approximation
void fp_pbr_fresnel_schlick(
    const float* cos_theta,     // N·V values
    const float* F0,            // Base reflectivity
    float* fresnel_out,         // Output Fresnel values
    int count
) {
    // F = F0 + (1 - F0) * (1 - cos_theta)^5
    // Use fp_map for (1 - cos_theta)
    // Use fp_map_pow for ^5 (or fp_map × fp_map × fp_map...)
}

// PURE FUNCTION: GGX normal distribution
void fp_pbr_distribution_ggx(
    const float* NdotH,         // Normal · Half-vector
    float roughness,
    float* distribution_out,
    int count
) {
    // D = alpha^2 / (PI * ((N·H)^2 * (alpha^2 - 1) + 1)^2)
    // Use FP library for all operations
}

// PURE FUNCTION: Full PBR shading for batch
void fp_pbr_compute_lighting(
    const float* normals,       // Vertex normals
    const float* view_dirs,     // View directions
    const float* light_dirs,    // Light directions
    const float* albedo,        // Material albedo
    float metallic,
    float roughness,
    float* radiance_out,        // Output radiance
    int vertex_count
) {
    // Compute N·L, N·V, N·H using fp_fold_dotp_f32
    // Compute Fresnel, Distribution, Geometry using FP maps
    // Combine using fp_zip_mul_f32, fp_zip_add_f32
}
```

#### Functions to Implement:
- `fp_pbr_fresnel_schlick()` - Fresnel term using FP library
- `fp_pbr_distribution_ggx()` - GGX normal distribution using FP library
- `fp_pbr_geometry_smith()` - Geometry shadowing using FP library
- `fp_pbr_compute_lighting()` - Full PBR BRDF using FP composition

---

### 4. Framebuffer Management

**Location**: `src/engine_old/renderer_modern.c:198-269` (72 lines)

#### Current Implementation (IMPERATIVE):
```c
Framebuffer* framebuffer_create(int width, int height, FramebufferAttachment attachment) {
    Framebuffer* fb = (Framebuffer*)malloc(sizeof(Framebuffer));  // ❌ ALLOCATION

    fb->width = width;   // ❌ MUTATION
    fb->height = height; // ❌ MUTATION

    glGenFramebuffers(1, &fb->fbo);  // ❌ SIDE EFFECT
    glBindFramebuffer(...);           // ❌ SIDE EFFECT
    glGenTextures(...);               // ❌ SIDE EFFECT
    // ... more OpenGL calls
}
```

#### FP-First Implementation Needed:
```c
/**
 * Framebuffer creation with FP principles
 *
 * FP Approach:
 *   - Return immutable configuration structure
 *   - Separate "description" from "creation" (IO boundary)
 *   - Pure functions to validate configurations
 */

// IMMUTABLE: Framebuffer description
typedef struct {
    const int width;
    const int height;
    const FramebufferAttachment attachment;
    const bool enable_depth;
    const bool enable_color;
} FramebufferSpec;

// PURE FUNCTION: Validate framebuffer spec
bool fp_framebuffer_validate_spec(const FramebufferSpec* spec) {
    return (spec->width > 0 && spec->height > 0 &&
            spec->width <= 16384 && spec->height <= 16384 &&
            (spec->enable_depth || spec->enable_color));
}

// IO BOUNDARY: Create framebuffer from spec
// (This is where side effects happen, but isolated)
Framebuffer* fp_framebuffer_create_from_spec(const FramebufferSpec* spec);
```

#### Functions to Implement:
- `fp_framebuffer_create_spec()` - Pure function returning immutable spec
- `fp_framebuffer_validate_spec()` - Pure validation
- `fp_framebuffer_create_from_spec()` - IO boundary (isolated side effects)

---

### 5. Mesh Generation

**Location**: `src/engine_old/renderer_modern.c:329-457` (129 lines)

#### Current Implementation (IMPERATIVE):
```c
MeshComponent mesh_create_cube(void) {
    Vertex vertices[24];  // ❌ MUTABLE ARRAY

    // ❌ IMPERATIVE LOOP filling vertices
    int idx = 0;
    for (int face = 0; face < 6; face++) {
        vertices[idx++] = ...;  // ❌ MUTATION
        vertices[idx++] = ...;  // ❌ MUTATION
        // ...
    }

    MeshComponent mesh;
    mesh_upload(&mesh, vertices, 24, indices, 36);  // ❌ SIDE EFFECT
    return mesh;
}
```

#### FP-First Implementation Needed:
```c
/**
 * Mesh generation using FP library
 *
 * FP Approach:
 *   - Generate vertex positions using fp_map
 *   - Generate normals using fp_cross (cross product)
 *   - Generate UVs using fp_map
 *   - All data immutable until upload
 */

// PURE FUNCTION: Generate cube vertex positions
void fp_mesh_generate_cube_positions(
    float* positions_out,  // 24 vertices × 3 = 72 floats
    float size
) {
    // Define 8 corner positions
    const float corners[8][3] = {
        {-0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, -0.5f},
        {0.5f, 0.5f, -0.5f}, {-0.5f, 0.5f, -0.5f},
        // ... etc
    };

    // Use FP library to scale all positions by size
    fp_map_scale_f32((const float*)corners, positions_out, 8 * 3, size);
}

// PURE FUNCTION: Generate cube normals
void fp_mesh_generate_cube_normals(
    const float* positions,
    float* normals_out,
    int triangle_count
) {
    // For each triangle:
    //   v1 - v0, v2 - v0
    //   cross product using FP library
    //   normalize using fp_fold_dotp_f32 (for length)
}

// PURE FUNCTION: Generate sphere positions (procedural)
void fp_mesh_generate_sphere_positions(
    float* positions_out,
    float radius,
    int segments,
    int rings
) {
    // Use FP library for sin/cos calculations
    // Generate all vertices in parallel using fp_map
}
```

#### Functions to Implement:
- `fp_mesh_generate_cube_positions()` - Cube vertices using FP library
- `fp_mesh_generate_cube_normals()` - Face normals using cross product
- `fp_mesh_generate_sphere_positions()` - Sphere via parametric surface
- `fp_mesh_generate_plane_positions()` - Plane mesh generation

---

### 6. Shader System (Partial - Uniforms)

**Location**: `src/engine_old/renderer_modern.c:174-192` (19 lines)

#### Current Implementation (SIDE EFFECTS):
```c
void shader_set_vec3(Shader* shader, const char* name, Vec3 value) {
    GLint location = glGetUniformLocation(shader->program, name);  // ❌ SIDE EFFECT
    glUniform3f(location, value.x, value.y, value.z);              // ❌ SIDE EFFECT
}

void shader_set_mat4(Shader* shader, const char* name, const Mat4* value) {
    GLint location = glGetUniformLocation(shader->program, name);
    glUniformMatrix4fv(location, 1, GL_FALSE, value->m);
}
```

#### FP-First Implementation Needed:
```c
/**
 * Shader uniform batching with FP principles
 *
 * FP Approach:
 *   - Build immutable "uniform batch" structure
 *   - Apply all uniforms at once (fewer OpenGL calls)
 *   - Pure functions to construct batches
 */

typedef struct {
    const char* name;
    float value[16];  // Enough for Mat4
    int component_count;  // 1=float, 3=vec3, 16=mat4
} UniformData;

typedef struct {
    const UniformData* uniforms;
    int count;
} UniformBatch;

// PURE FUNCTION: Create uniform batch
UniformBatch fp_shader_create_uniform_batch(
    const char** names,
    const float** values,
    const int* component_counts,
    int uniform_count
) {
    // Allocate and copy (immutable after creation)
    // Return batch
}

// IO BOUNDARY: Apply batch to shader
void fp_shader_apply_uniform_batch(
    Shader* shader,
    const UniformBatch* batch
) {
    // Single function to upload all uniforms
    // Fewer OpenGL calls = better performance
}
```

#### Functions to Implement:
- `fp_shader_create_uniform_batch()` - Build immutable uniform collection
- `fp_shader_apply_uniform_batch()` - Upload all uniforms at once

---

## SUMMARY: ALGORITHMS TO PORT

| Algorithm | Lines (Old) | FP Library Functions Needed | Priority |
|-----------|-------------|----------------------------|----------|
| **SSAO** | 223 | `fp_map`, `fp_reduce_add_f32`, `fp_zip_add_f32` | **HIGH** |
| **Shadow Mapping** | 40 | `fp_fold_dotp_f32`, `fp_reduce_min_f32` | **HIGH** |
| **PBR Shading** | 86 + shaders | `fp_fold_dotp_f32`, `fp_map`, `fp_zip_mul_f32` | **HIGH** |
| **Framebuffer Mgmt** | 72 | N/A (mostly IO boundary design) | MEDIUM |
| **Mesh Generation** | 129 | `fp_map_scale_f32`, cross product | MEDIUM |
| **Shader Uniforms** | 19 | N/A (batching strategy) | LOW |

**Total Old Code**: ~569 lines
**Estimated FP Code**: ~800-1000 lines (more verbose but clearer)

---

## IMPLEMENTATION PLAN

### Phase 1: Core Algorithms (Week 1)
1. ✅ Port SSAO kernel generation to FP-first
2. ✅ Port SSAO occlusion computation to FP-first
3. ✅ Port shadow map generation to FP-first

### Phase 2: PBR & Lighting (Week 2)
4. ✅ Port Fresnel-Schlick to FP-first
5. ✅ Port GGX distribution to FP-first
6. ✅ Port full PBR BRDF to FP-first

### Phase 3: Mesh & Utilities (Week 3)
7. ✅ Port mesh generation to FP-first
8. ✅ Port uniform batching to FP-first
9. ✅ Port framebuffer management to FP-first

### Phase 4: Integration & Demo (Week 4)
10. ✅ Build complete FP-first demo combining all algorithms
11. ✅ Performance comparison (FP vs old imperative)
12. ✅ Documentation and showcase

---

## SUCCESS CRITERIA

For each ported algorithm:

✅ **Zero imperative loops** in hot paths (computational code)
✅ **100% FP library usage** for numerical operations
✅ **All inputs marked `const`** (immutability)
✅ **Pure functions** return new data (no mutations)
✅ **Performance ≥ 90%** of imperative version
✅ **Visually identical** output to old version

---

## REFERENCE IMPLEMENTATIONS

**Correct FP-first examples**:
- `demo_fp_cube_final.c` - Spinning cube (all transforms via FP library)
- `demo_fp_ssao.c` - SSAO using `fp_reduce_add_f32`
- `src/engine/fp_graphics_lighting.c` - Lighting via FP library
- `src/engine/fp_graphics_postprocess.c` - Post-processing via FP library

**Wrong imperative examples** (DO NOT COPY):
- `src/engine_old/renderer_modern.c` - ALL CODE (0 FP library calls!)
- `src/engine_old/ecs.c` - Mutable state everywhere

---

## NOTES

1. **OpenGL calls are side effects** - That's fine! We isolate them at the IO boundary.
2. **ECS is inherently mutable** - That's fine! We treat it as external state.
3. **Focus on COMPUTATIONAL code** - That's where FP library shines!
4. **Shaders are already declarative** - GLSL is fine as-is.

The goal is **not** to make OpenGL functional (impossible). The goal is to make **all numerical computations** FP-first using the library.

---

**Status**: 📋 PLAN COMPLETE - Ready for implementation
**Next**: Start porting SSAO (highest priority)
