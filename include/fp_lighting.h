/**
 * fp_lighting.h
 *
 * FP-First Lighting Module for the 3D Engine.
 *
 * All functions are pure and compose primitives from the fp_asm library
 * to calculate lighting for batches of vertices.
 */

#ifndef FP_LIGHTING_H
#define FP_LIGHTING_H

#include "fp_core.h"
#include <stddef.h>

//==============================================================================
// Lighting Data Structures
//==============================================================================

typedef struct {
    float direction[3];
    float intensity;
    float color[3];
} DirectionalLight;

typedef struct {
    float position[3];
    float intensity;
    float radius;
    float color[3];
} PointLight;


//==============================================================================
// Core Lighting Functions
//==============================================================================

/**
 * @brief Calculates diffuse lighting for a batch of vertices from a single light source.
 *
 * @param normals A flat array of normal vectors (size = vertex_count * 3).
 * @param intensities An output array to store the calculated light intensity for each vertex.
 * @param light_dir The normalized direction of the light.
 * @param light_intensity The intensity of the light.
 * @param vertex_count The number of vertices to process.
 */
void fp_lighting_diffuse_batch(
    const Vec3f* normals,
    float* intensities,
    const Vec3f* light_dir,
    float light_intensity,
    size_t vertex_count
);

/**
 * @brief Calculates the contribution of a directional light for a batch of vertices.
 *
 * @param normals A flat array of normal vectors.
 * @param intensities An output array to store the calculated light intensity.
 * @param light A pointer to the directional light source.
 * @param vertex_count The number of vertices.
 */
void fp_lighting_directional(
    const Vec3f* normals,
    float* intensities,
    const DirectionalLight* light,
    size_t vertex_count
);

/**
 * @brief Combines two light contributions into one.
 *
 * @param light1 Input array of the first light's intensities.
 * @param light2 Input array of the second light's intensities.
 * @param combined Output array for the combined intensities.
 * @param vertex_count The number of vertices.
 */
void fp_lighting_combine(
    const float* light1,
    const float* light2,
    float* combined,
    size_t vertex_count
);

/**
 * @brief Adds an ambient light component to existing light intensities.
 *
 * @param intensities Input array of current light intensities.
 * @param output Output array for the final intensities.
 * @param ambient_intensity The intensity of the ambient light.
 * @param vertex_count The number of vertices.
 */
void fp_lighting_add_ambient(
    const float* intensities,
    float* output,
    float ambient_intensity,
    size_t vertex_count
);


#endif // FP_LIGHTING_H
