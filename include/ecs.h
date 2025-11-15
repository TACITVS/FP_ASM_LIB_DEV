/**
 * ecs.h
 *
 * Entity Component System (ECS) Architecture
 *
 * Design:
 * - Entities: Unique 32-bit IDs
 * - Components: Pure data structures stored in sparse sets
 * - Systems: Logic that processes entities with specific components
 *
 * Performance:
 * - Cache-friendly packed arrays for fast iteration
 * - Sparse sets for O(1) component add/remove/get
 * - Data-oriented design for CPU optimization
 */

#define _USE_MATH_DEFINES // For M_PI
#ifndef ECS_H
#define ECS_H

#include <stdint.h>
#include <stdbool.h>
#include <math.h> // For sinf, cosf, asinf, atan2f, fabsf, copysignf

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// Use the existing, aligned vector/matrix types from the core library
#include "fp_core.h"

// Maximum number of entities (expandable)
#define ECS_MAX_ENTITIES 10000
#define ECS_MAX_COMPONENTS 64

// Entity type
typedef uint32_t Entity;
#define ENTITY_INVALID 0xFFFFFFFF

// Component types (bitflags for fast queries)
typedef enum {
    COMPONENT_TRANSFORM  = 1 << 0,
    COMPONENT_MESH       = 1 << 1,
    COMPONENT_MATERIAL   = 1 << 2,
    COMPONENT_LIGHT      = 1 << 3,
    COMPONENT_CAMERA     = 1 << 4,
    COMPONENT_PHYSICS    = 1 << 5,  // Future
} ComponentType;

//==============================================================================
// Components (Pure Data)
//==============================================================================

typedef struct {
    Vec3f position; // Changed from Vec3 to Vec3f
    Quaternion rotation;
    Vec3f scale;    // Changed from Vec3 to Vec3f
    Mat4 local_matrix;   // Cached transform matrix
    Mat4 world_matrix;   // If part of hierarchy
    bool dirty;          // Needs recalculation
    float animation_offset; // Unique offset for animation timing
} TransformComponent;

typedef struct {
    uint32_t vao;        // Vertex Array Object
    uint32_t vbo;        // Vertex Buffer Object
    uint32_t ebo;        // Element Buffer Object
    uint32_t vertex_count;
    uint32_t index_count;
    bool indexed;        // Use indices or not
} MeshComponent;

typedef struct {
    // PBR material properties
    Vec3f albedo;         // Base color (Changed from Vec3 to Vec3f)
    float metallic;      // 0 = dielectric, 1 = metal
    float roughness;     // 0 = smooth, 1 = rough
    float ao;            // Ambient occlusion

    // Texture maps (0 = no texture)
    uint32_t albedo_map;
    uint32_t normal_map;
    uint32_t metallic_roughness_map;
    uint32_t ao_map;

    // Emission
    Vec3f emissive;       // Changed from Vec3 to Vec3f
    float emissive_strength;
} MaterialComponent;

typedef enum {
    LIGHT_DIRECTIONAL,
    LIGHT_POINT,
    LIGHT_SPOT,
} LightType;

typedef struct {
    LightType type;
    Vec3f color;          // Changed from Vec3 to Vec3f
    float intensity;

    // Point/Spot light properties
    float range;
    float attenuation;

    // Spot light properties
    float inner_cone;    // Inner cone angle (radians)
    float outer_cone;    // Outer cone angle (radians)

    // Shadow properties
    bool cast_shadows;
    uint32_t shadow_map; // Framebuffer texture
    Mat4 shadow_matrix;  // Light space projection
} LightComponent;

typedef enum {
    CAMERA_PERSPECTIVE,
    CAMERA_ORTHOGRAPHIC,
} CameraType;

typedef struct {
    float fov;           // Field of view (radians)
    float aspect;        // Aspect ratio
    float near_plane;
    float far_plane;

    // Orthographic properties
    float ortho_size;

    // Matrices
    Mat4 view_matrix;
    Mat4 projection_matrix;
    Mat4 view_projection_matrix;

    bool dirty;          // Needs recalculation
} CameraComponent;

//==============================================================================
// ECS World
//==============================================================================

typedef struct {
    // Entity management
    Entity* entities;               // All entity IDs
    uint32_t entity_count;
    uint32_t entity_capacity;
    uint32_t* entity_versions;      // For handle validation
    uint32_t* free_entities;        // Free list
    uint32_t free_count;

    // Component storage (sparse sets)
    TransformComponent* transforms;
    uint32_t transform_count;
    uint32_t* transform_sparse;     // Entity ID -> dense index
    Entity* transform_entities;     // Dense array of entities

    MeshComponent* meshes;
    uint32_t mesh_count;
    uint32_t* mesh_sparse;
    Entity* mesh_entities;

    MaterialComponent* materials;
    uint32_t material_count;
    uint32_t* material_sparse;
    Entity* material_entities;

    LightComponent* lights;
    uint32_t light_count;
    uint32_t* light_sparse;
    Entity* light_entities;

    CameraComponent* cameras;
    uint32_t camera_count;
    uint32_t* camera_sparse;
    Entity* camera_entities;

    // Active camera
    Entity active_camera;
} ECSWorld;

//==============================================================================
// ECS Core API
//==============================================================================

// World management
ECSWorld* ecs_world_create(void);
void ecs_world_destroy(ECSWorld* world);

// Entity management
Entity ecs_entity_create(ECSWorld* world);
void ecs_entity_destroy(ECSWorld* world, Entity entity);
bool ecs_entity_is_alive(ECSWorld* world, Entity entity);

// Component management
void ecs_add_transform(ECSWorld* world, Entity entity, TransformComponent* transform);
void ecs_add_mesh(ECSWorld* world, Entity entity, MeshComponent* mesh);
void ecs_add_material(ECSWorld* world, Entity entity, MaterialComponent* material);
void ecs_add_light(ECSWorld* world, Entity entity, LightComponent* light);
void ecs_add_camera(ECSWorld* world, Entity entity, CameraComponent* camera);

TransformComponent* ecs_get_transform(ECSWorld* world, Entity entity);
MeshComponent* ecs_get_mesh(ECSWorld* world, Entity entity);
MaterialComponent* ecs_get_material(ECSWorld* world, Entity entity);
LightComponent* ecs_get_light(ECSWorld* world, Entity entity);
CameraComponent* ecs_get_camera(ECSWorld* world, Entity entity);

bool ecs_has_transform(ECSWorld* world, Entity entity);
bool ecs_has_mesh(ECSWorld* world, Entity entity);
bool ecs_has_material(ECSWorld* world, Entity entity);
bool ecs_has_light(ECSWorld* world, Entity entity);
bool ecs_has_camera(ECSWorld* world, Entity entity);

void ecs_remove_transform(ECSWorld* world, Entity entity);
void ecs_remove_mesh(ECSWorld* world, Entity entity);
void ecs_remove_material(ECSWorld* world, Entity entity);
void ecs_remove_light(ECSWorld* world, Entity entity);
void ecs_remove_camera(ECSWorld* world, Entity entity);

// Helper functions for iteration
typedef void (*ComponentIteratorFn)(Entity entity, void* component, void* user_data);

void ecs_iterate_transforms(ECSWorld* world, ComponentIteratorFn fn, void* user_data);
void ecs_iterate_meshes(ECSWorld* world, ComponentIteratorFn fn, void* user_data);
void ecs_iterate_lights(ECSWorld* world, ComponentIteratorFn fn, void* user_data);

// Query entities with specific components
typedef struct {
    Entity* entities;
    uint32_t count;
} EntityQuery;

EntityQuery ecs_query_entities(ECSWorld* world, uint32_t component_mask);
void ecs_query_free(EntityQuery* query);

//==============================================================================
// Math Utilities (for convenience)
//==============================================================================

// Transform helpers
void transform_update_matrix(TransformComponent* transform);
Mat4 transform_get_matrix(const TransformComponent* transform);

// Transform helpers
void transform_update_matrix(TransformComponent* transform);
Mat4 transform_get_matrix(const TransformComponent* transform);

#endif // ECS_H