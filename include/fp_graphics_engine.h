/**
 * fp_graphics_engine.h
 *
 * Core immutable data structures for the FP-First 3D Engine.
 *
 * Principles:
 * - All structs are immutable. Functions that "modify" them return a new copy.
 * - Data-oriented design where possible.
 * - Follows the blueprint from FP_ENGINE_ARCHITECTURE.md.
 */

#ifndef FP_GRAPHICS_ENGINE_H
#define FP_GRAPHICS_ENGINE_H

#include <stdint.h>
#include <stdbool.h>

// Use the existing, aligned vector/matrix types from the core library
#include "fp_core.h"

//==============================================================================
// Core Immutable Data Structures
//==============================================================================

typedef struct {
    Vec3f position;
    Vec3f scale;
    Vec3f euler_rotation;
} Transform;

typedef struct {
    float fov_radians;
    float aspect_ratio;
    float near_plane;
    float far_plane;
} CameraProjection;

typedef struct {
    Transform transform;
    CameraProjection projection;
} Camera;

typedef enum {
    FP_LIGHT_DIRECTIONAL,
    FP_LIGHT_POINT
} FPLightType;

typedef struct {
    FPLightType type;
    Transform transform;
    Vec3f color;
    float intensity;
} FPLight;

//==============================================================================
// Transformation Functions (Pure)
//==============================================================================

/**
 * @brief Multiplies two 4x4 matrices (a * b).
 * This is a pure function that uses the fp_asm library for its core computation.
 *
 * @param out The resulting 4x4 matrix.
 * @param a The first 4x4 matrix.
 * @param b The second 4x4 matrix.
 */
void fp_mat4_mul_pure(Mat4* out, const Mat4* a, const Mat4* b);

/**
 * @brief Creates a model matrix from a Transform struct.
 *
 * @param out The resulting 4x4 model matrix.
 * @param t The transform to use.
 */
void fp_transform_to_matrix(Mat4* out, const Transform* t);

/**
 * @brief Creates a view matrix from a Camera struct.
 *
 * @param out The resulting 4x4 view matrix.
 * @param cam The camera to use.
 */
void fp_view_matrix(Mat4* out, const Camera* cam);

/**
 * @brief Creates a projection matrix from a CameraProjection struct.
 *
 * @param out The resulting 4x4 projection matrix.
 * @param proj The camera projection to use.
 */
void fp_projection_matrix(Mat4* out, const CameraProjection* proj);

/**
 * @brief Creates a combined Model-View-Projection (MVP) matrix.
 *
 * @param out_mvp The resulting 4x4 MVP matrix.
 * @param model The model matrix.
 * @param view The view matrix.
 * @param proj The projection matrix.
 */
void fp_get_mvp_matrix(Mat4* out_mvp, const Mat4* model, const Mat4* view, const Mat4* proj);


#endif // FP_GRAPHICS_ENGINE_H
