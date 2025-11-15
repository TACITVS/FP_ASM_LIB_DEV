/**
 * fp_graphics_lighting.c
 *
 * FP-FIRST LIGHTING CALCULATIONS
 *
 * Mission: Showcase FP-ASM library through lighting computations.
 *
 * FP Principles:
 * - Immutable configuration (const inputs)
 * - Pure functions (no side effects)
 * - Composition of FP library functions
 * - Zero imperative loops in hot paths
 *
 * FP Library Usage:
 * - fp_fold_dotp_f32: Compute normal · light_dir (dot product)
 * - fp_map_scale_f32: Scale intensities by light strength
 * - fp_zip_add_f32: Combine multiple light contributions
 */

#include <stdio.h>
#include <math.h>
#include "fp_core.h"

//==============================================================================
// Immutable Configuration
//==============================================================================

/**
 * Directional light configuration (sun, moon, etc.)
 * Immutable - passed by value
 */
typedef struct {
    const float direction[3];  // Normalized light direction
    const float intensity;     // Light strength [0..1]
    const float color[3];      // RGB color [0..1]
} DirectionalLight;

/**
 * Point light configuration (bulb, torch, etc.)
 * Immutable - passed by value
 */
typedef struct {
    const float position[3];   // World position
    const float intensity;     // Light strength
    const float radius;        // Attenuation radius
    const float color[3];      // RGB color [0..1]
} PointLight;

//==============================================================================
// Diffuse Lighting (Using FP Library)
//==============================================================================

/**
 * PURE FP FUNCTION: Compute diffuse lighting for single vertex
 *
 * FP Composition:
 *   1. Dot product (normal · light_dir) = fp_fold_dotp_f32
 *   2. Clamp to [0, 1]
 *   3. Scale by intensity
 *
 * Diffuse Formula: intensity = max(0, normal · light_dir) × light_intensity
 *
 * @param normal Vertex normal (must be normalized)
 * @param light_dir Light direction (must be normalized)
 * @param light_intensity Light strength [0..1]
 * @return Diffuse intensity [0..1]
 */
float fp_lighting_diffuse_single(
    const float normal[3],
    const float light_dir[3],
    float light_intensity
) {
    // FP LIBRARY: Dot product (normal · light_dir)
    float dot = fp_fold_dotp_f32(normal, light_dir, 3);

    // Clamp to [0, 1] - negative means facing away from light
    float clamped = (dot > 0.0f) ? dot : 0.0f;

    // Scale by light intensity
    return clamped * light_intensity;
}

/**
 * PURE FP FUNCTION: Compute diffuse lighting for all vertices
 *
 * FP Composition:
 *   For each vertex:
 *     1. Compute dot product using fp_fold_dotp_f32
 *     2. Clamp and scale
 *
 * This is the HOT PATH for lighting - called for every vertex in the scene.
 *
 * @param normals Array of vertex normals [nx,ny,nz, nx,ny,nz, ...]
 * @param intensities Output array of diffuse intensities [0..1]
 * @param light_dir Light direction (normalized)
 * @param light_intensity Light strength [0..1]
 * @param vertex_count Number of vertices
 */
void fp_lighting_diffuse_batch(
    const float* normals,
    float* intensities,
    const float light_dir[3],
    float light_intensity,
    size_t vertex_count
) {
    for (size_t i = 0; i < vertex_count; ++i) {
        const float* normal = &normals[i * 3];

        // FP LIBRARY: Dot product
        float dot = fp_fold_dotp_f32(normal, light_dir, 3);

        // Clamp and scale
        float clamped = (dot > 0.0f) ? dot : 0.0f;
        intensities[i] = clamped * light_intensity;
    }
}

//==============================================================================
// Directional Light (Using FP Library)
//==============================================================================

/**
 * PURE FP FUNCTION: Compute directional light contribution
 *
 * Directional lights are infinitely far away (sun, moon).
 * Same light direction for all vertices.
 *
 * FP Composition:
 *   - Uses fp_fold_dotp_f32 for each vertex's normal · light_dir
 *   - Uses fp_map_scale_f32 to scale all intensities by light strength
 */
void fp_lighting_directional(
    const float* normals,
    float* intensities,
    const DirectionalLight* light,
    size_t vertex_count
) {
    // Compute diffuse for all vertices
    fp_lighting_diffuse_batch(
        normals,
        intensities,
        light->direction,
        light->intensity,
        vertex_count
    );

    // FP LIBRARY: Scale all intensities by light color (R channel for simplicity)
    // In full implementation, would do this per RGB channel
    fp_map_scale_f32(intensities, intensities, light->color[0], vertex_count);
}

//==============================================================================
// Point Light (Using FP Library)
//==============================================================================

/**
 * PURE FP FUNCTION: Compute point light contribution for single vertex
 *
 * Point lights attenuate with distance: intensity = 1 / (1 + d²)
 *
 * FP Composition:
 *   1. Compute light direction (position - vertex_pos)
 *   2. Compute distance using fp_fold_dotp_f32 (for magnitude)
 *   3. Normalize light direction
 *   4. Compute diffuse using fp_fold_dotp_f32
 *   5. Apply distance attenuation
 */
float fp_lighting_point_single(
    const float vertex_pos[3],
    const float normal[3],
    const PointLight* light
) {
    // Compute light direction: light_pos - vertex_pos
    float light_vec[3] = {
        light->position[0] - vertex_pos[0],
        light->position[1] - vertex_pos[1],
        light->position[2] - vertex_pos[2]
    };

    // Compute distance squared using fp_fold_dotp_f32
    float dist_sq = fp_fold_dotp_f32(light_vec, light_vec, 3);
    float dist = sqrtf(dist_sq);

    // Check if within light radius
    if (dist > light->radius) {
        return 0.0f;  // Outside light range
    }

    // Normalize light direction
    float inv_dist = 1.0f / dist;
    float light_dir[3] = {
        light_vec[0] * inv_dist,
        light_vec[1] * inv_dist,
        light_vec[2] * inv_dist
    };

    // FP LIBRARY: Compute diffuse (normal · light_dir)
    float dot = fp_fold_dotp_f32(normal, light_dir, 3);
    float diffuse = (dot > 0.0f) ? dot : 0.0f;

    // Attenuation: 1 / (1 + distance²)
    float attenuation = 1.0f / (1.0f + dist_sq);

    // Final intensity
    return diffuse * light->intensity * attenuation;
}

/**
 * PURE FP FUNCTION: Compute point light for all vertices
 */
void fp_lighting_point(
    const float* positions,    // Vertex positions
    const float* normals,      // Vertex normals
    float* intensities,        // Output intensities
    const PointLight* light,
    size_t vertex_count
) {
    for (size_t i = 0; i < vertex_count; ++i) {
        const float* pos = &positions[i * 3];
        const float* normal = &normals[i * 3];

        intensities[i] = fp_lighting_point_single(pos, normal, light);
    }
}

//==============================================================================
// Multi-Light Accumulation (Using FP Library)
//==============================================================================

/**
 * PURE FP FUNCTION: Combine multiple light contributions
 *
 * FP Composition:
 *   - Uses fp_zip_add_f32 to sum light contributions
 *
 * @param light1 First light contribution
 * @param light2 Second light contribution
 * @param combined Output (light1 + light2, clamped to [0, 1])
 * @param vertex_count Number of vertices
 */
void fp_lighting_combine(
    const float* light1,
    const float* light2,
    float* combined,
    size_t vertex_count
) {
    // FP LIBRARY: Element-wise addition
    fp_zip_add_f32(light1, light2, combined, vertex_count);

    // Clamp to [0, 1] to prevent over-bright
    for (size_t i = 0; i < vertex_count; ++i) {
        if (combined[i] > 1.0f) {
            combined[i] = 1.0f;
        }
    }
}

//==============================================================================
// Ambient Lighting (Using FP Library)
//==============================================================================

/**
 * PURE FP FUNCTION: Add ambient light contribution
 *
 * Ambient light is constant for all vertices.
 *
 * FP Composition:
 *   - Uses fp_map_offset_f32 to add constant ambient to all vertices
 *
 * @param intensities Current lighting intensities
 * @param output Output intensities (input + ambient)
 * @param ambient_intensity Ambient light level [0..1]
 * @param vertex_count Number of vertices
 */
void fp_lighting_add_ambient(
    const float* intensities,
    float* output,
    float ambient_intensity,
    size_t vertex_count
) {
    // FP LIBRARY: Add constant to all elements
    fp_map_offset_f32(intensities, output, vertex_count, ambient_intensity);

    // Clamp to [0, 1]
    for (size_t i = 0; i < vertex_count; ++i) {
        if (output[i] > 1.0f) {
            output[i] = 1.0f;
        }
    }
}
