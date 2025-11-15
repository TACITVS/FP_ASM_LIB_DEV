#ifndef FP_ENGINE_TYPES_H
#define FP_ENGINE_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include "fp_core.h" // For Mat4, Vec3f, Quaternion

// Forward declarations for FP_MeshData (defined in fp_mesh_generation.h)
typedef struct FP_MeshData FP_MeshData;

// FP_Transform: Position, Rotation (Quaternion), Scale
// All operations on this struct should return a new FP_Transform
typedef struct {
    Vec3f position;
    Quaternion rotation; // Use Quaternion from fp_core.h
    Vec3f scale;
    Mat4 local_matrix;   // Cached transform matrix
    Mat4 world_matrix;   // World transform matrix
    bool dirty;          // Needs recalculation
} FP_Transform;

// FP_Material: Albedo (Vec3f), Metallic, Roughness, AO
// All operations on this struct should return a new FP_Material
typedef struct {
    Vec3f albedo;
    float metallic;
    float roughness;
    float ao;
} FP_Material;

// FP_Light: Position (Vec3f), Target (Vec3f), Color (Vec3f), Type, Shadow Matrix (Mat4)
// All operations on this struct should return a new FP_Light
typedef enum {
    FP_LIGHT_DIRECTIONAL,
    FP_LIGHT_POINT,
    FP_LIGHT_SPOT,
} FP_LightType;

typedef struct {
    Vec3f position;
    Vec3f target;
    Vec3f color;
    FP_LightType type;
    Mat4 shadow_matrix; // Pre-calculated light space matrix
} FP_Light;

// FP_Camera: Position (Vec3f), Target (Vec3f), Up (Vec3f), Projection Matrix (Mat4), View Matrix (Mat4)
// All operations on this struct should return a new FP_Camera
typedef struct {
    Vec3f position;
    Vec3f target;
    Vec3f up;
    Mat4 projection_matrix;
    Mat4 view_matrix;
} FP_Camera;

// FP_SceneObject: Contains FP_Transform, FP_Material, and a pointer to FP_MeshData
// All operations on this struct should return a new FP_SceneObject
typedef struct {
    FP_Transform transform;
    FP_Material material;
    const FP_MeshData* mesh; // Pointer to shared mesh data
} FP_SceneObject;

// FP_AppState: Contains arrays of FP_SceneObject, FP_Light, FP_Camera, and other global state.
// This is the main immutable state of our application.
// Functions will take a const FP_AppState* and return a new FP_AppState.
#define MAX_SCENE_OBJECTS 1024
#define MAX_LIGHTS 4

typedef struct {
    FP_SceneObject objects[MAX_SCENE_OBJECTS];
    uint32_t object_count;

    FP_Light lights[MAX_LIGHTS];
    uint32_t light_count;

    FP_Camera camera;

    // Other global state (e.g., time, input, quality settings)
    float current_time;
    // Add more as needed, ensuring they are treated immutably
} FP_AppState;

#endif // FP_ENGINE_TYPES_H
