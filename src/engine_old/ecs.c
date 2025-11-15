/**
 * ecs.c
 *
 * Entity Component System Implementation
 *
 * Uses sparse sets for O(1) component operations:
 * - Sparse array: Entity ID -> dense index (large, mostly empty)
 * - Dense array: Packed component data (small, cache-friendly)
 * - Entity array: Maps dense index back to entity (for iteration)
 */

#include "ecs.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define SPARSE_INVALID 0xFFFFFFFF

//==============================================================================
// World Management
//==============================================================================

ECSWorld* ecs_world_create(void) {
    ECSWorld* world = (ECSWorld*)calloc(1, sizeof(ECSWorld));

    // Allocate entity storage
    world->entity_capacity = 1024;
    world->entities = (Entity*)malloc(world->entity_capacity * sizeof(Entity));
    world->entity_versions = (uint32_t*)calloc(world->entity_capacity, sizeof(uint32_t));
    world->free_entities = (uint32_t*)malloc(world->entity_capacity * sizeof(uint32_t));

    // Initialize free list (all entities start free)
    for (uint32_t i = 0; i < world->entity_capacity; i++) {
        world->free_entities[i] = i;
    }
    world->free_count = world->entity_capacity;

    // Allocate component storage
    uint32_t component_capacity = 1024;

    // Transforms
    world->transforms = (TransformComponent*)malloc(component_capacity * sizeof(TransformComponent));
    world->transform_sparse = (uint32_t*)malloc(world->entity_capacity * sizeof(uint32_t));
    world->transform_entities = (Entity*)malloc(component_capacity * sizeof(Entity));
    for (uint32_t i = 0; i < world->entity_capacity; i++) {
        world->transform_sparse[i] = SPARSE_INVALID;
    }

    // Meshes
    world->meshes = (MeshComponent*)malloc(component_capacity * sizeof(MeshComponent));
    world->mesh_sparse = (uint32_t*)malloc(world->entity_capacity * sizeof(uint32_t));
    world->mesh_entities = (Entity*)malloc(component_capacity * sizeof(Entity));
    for (uint32_t i = 0; i < world->entity_capacity; i++) {
        world->mesh_sparse[i] = SPARSE_INVALID;
    }

    // Materials
    world->materials = (MaterialComponent*)malloc(component_capacity * sizeof(MaterialComponent));
    world->material_sparse = (uint32_t*)malloc(world->entity_capacity * sizeof(uint32_t));
    world->material_entities = (Entity*)malloc(component_capacity * sizeof(Entity));
    for (uint32_t i = 0; i < world->entity_capacity; i++) {
        world->material_sparse[i] = SPARSE_INVALID;
    }

    // Lights
    world->lights = (LightComponent*)malloc(component_capacity * sizeof(LightComponent));
    world->light_sparse = (uint32_t*)malloc(world->entity_capacity * sizeof(uint32_t));
    world->light_entities = (Entity*)malloc(component_capacity * sizeof(Entity));
    for (uint32_t i = 0; i < world->entity_capacity; i++) {
        world->light_sparse[i] = SPARSE_INVALID;
    }

    // Cameras
    world->cameras = (CameraComponent*)malloc(component_capacity * sizeof(CameraComponent));
    world->camera_sparse = (uint32_t*)malloc(world->entity_capacity * sizeof(uint32_t));
    world->camera_entities = (Entity*)malloc(component_capacity * sizeof(Entity));
    for (uint32_t i = 0; i < world->entity_capacity; i++) {
        world->camera_sparse[i] = SPARSE_INVALID;
    }

    world->active_camera = ENTITY_INVALID;

    return world;
}

void ecs_world_destroy(ECSWorld* world) {
    if (!world) return;

    free(world->entities);
    free(world->entity_versions);
    free(world->free_entities);

    free(world->transforms);
    free(world->transform_sparse);
    free(world->transform_entities);

    free(world->meshes);
    free(world->mesh_sparse);
    free(world->mesh_entities);

    free(world->materials);
    free(world->material_sparse);
    free(world->material_entities);

    free(world->lights);
    free(world->light_sparse);
    free(world->light_entities);

    free(world->cameras);
    free(world->camera_sparse);
    free(world->camera_entities);

    free(world);
}

//==============================================================================
// Entity Management
//==============================================================================

Entity ecs_entity_create(ECSWorld* world) {
    if (world->free_count == 0) {
        // Expand capacity (double it)
        uint32_t new_capacity = world->entity_capacity * 2;

        world->entities = (Entity*)realloc(world->entities, new_capacity * sizeof(Entity));
        world->entity_versions = (uint32_t*)realloc(world->entity_versions, new_capacity * sizeof(uint32_t));
        world->free_entities = (uint32_t*)realloc(world->free_entities, new_capacity * sizeof(uint32_t));

        // Expand sparse arrays
        world->transform_sparse = (uint32_t*)realloc(world->transform_sparse, new_capacity * sizeof(uint32_t));
        world->mesh_sparse = (uint32_t*)realloc(world->mesh_sparse, new_capacity * sizeof(uint32_t));
        world->material_sparse = (uint32_t*)realloc(world->material_sparse, new_capacity * sizeof(uint32_t));
        world->light_sparse = (uint32_t*)realloc(world->light_sparse, new_capacity * sizeof(uint32_t));
        world->camera_sparse = (uint32_t*)realloc(world->camera_sparse, new_capacity * sizeof(uint32_t));

        // Initialize new sparse entries
        for (uint32_t i = world->entity_capacity; i < new_capacity; i++) {
            world->entity_versions[i] = 0;
            world->transform_sparse[i] = SPARSE_INVALID;
            world->mesh_sparse[i] = SPARSE_INVALID;
            world->material_sparse[i] = SPARSE_INVALID;
            world->light_sparse[i] = SPARSE_INVALID;
            world->camera_sparse[i] = SPARSE_INVALID;
            world->free_entities[world->free_count++] = i;
        }

        world->entity_capacity = new_capacity;
    }

    // Pop from free list
    uint32_t index = world->free_entities[--world->free_count];
    Entity entity = index;

    world->entities[world->entity_count++] = entity;

    return entity;
}

void ecs_entity_destroy(ECSWorld* world, Entity entity) {
    if (entity >= world->entity_capacity) return;

    // Remove all components
    ecs_remove_transform(world, entity);
    ecs_remove_mesh(world, entity);
    ecs_remove_material(world, entity);
    ecs_remove_light(world, entity);
    ecs_remove_camera(world, entity);

    // Increment version for handle validation
    world->entity_versions[entity]++;

    // Add back to free list
    world->free_entities[world->free_count++] = entity;

    // Remove from entity list (swap with last)
    for (uint32_t i = 0; i < world->entity_count; i++) {
        if (world->entities[i] == entity) {
            world->entities[i] = world->entities[--world->entity_count];
            break;
        }
    }
}

bool ecs_entity_is_alive(ECSWorld* world, Entity entity) {
    if (entity >= world->entity_capacity) return false;

    // Check if entity is not in free list
    for (uint32_t i = 0; i < world->free_count; i++) {
        if (world->free_entities[i] == entity) {
            return false;
        }
    }

    return true;
}

//==============================================================================
// Component Management - Transform
//==============================================================================

void ecs_add_transform(ECSWorld* world, Entity entity, TransformComponent* transform) {
    if (entity >= world->entity_capacity) return;
    if (world->transform_sparse[entity] != SPARSE_INVALID) return; // Already has component

    uint32_t dense_index = world->transform_count++;
    world->transform_sparse[entity] = dense_index;
    world->transform_entities[dense_index] = entity;
    world->transforms[dense_index] = *transform;
    world->transforms[dense_index].dirty = true;
}

TransformComponent* ecs_get_transform(ECSWorld* world, Entity entity) {
    if (entity >= world->entity_capacity) return NULL;

    uint32_t dense_index = world->transform_sparse[entity];
    if (dense_index == SPARSE_INVALID) return NULL;

    return &world->transforms[dense_index];
}

bool ecs_has_transform(ECSWorld* world, Entity entity) {
    if (entity >= world->entity_capacity) return false;
    return world->transform_sparse[entity] != SPARSE_INVALID;
}

void ecs_remove_transform(ECSWorld* world, Entity entity) {
    if (entity >= world->entity_capacity) return;

    uint32_t dense_index = world->transform_sparse[entity];
    if (dense_index == SPARSE_INVALID) return;

    // Swap with last element (swap-and-pop)
    uint32_t last_index = --world->transform_count;
    Entity last_entity = world->transform_entities[last_index];

    world->transforms[dense_index] = world->transforms[last_index];
    world->transform_entities[dense_index] = last_entity;
    world->transform_sparse[last_entity] = dense_index;
    world->transform_sparse[entity] = SPARSE_INVALID;
}

//==============================================================================
// Component Management - Mesh (same pattern as Transform)
//==============================================================================

void ecs_add_mesh(ECSWorld* world, Entity entity, MeshComponent* mesh) {
    if (entity >= world->entity_capacity) return;
    if (world->mesh_sparse[entity] != SPARSE_INVALID) return;

    uint32_t dense_index = world->mesh_count++;
    world->mesh_sparse[entity] = dense_index;
    world->mesh_entities[dense_index] = entity;
    world->meshes[dense_index] = *mesh;
}

MeshComponent* ecs_get_mesh(ECSWorld* world, Entity entity) {
    if (entity >= world->entity_capacity) return NULL;
    uint32_t dense_index = world->mesh_sparse[entity];
    if (dense_index == SPARSE_INVALID) return NULL;
    return &world->meshes[dense_index];
}

bool ecs_has_mesh(ECSWorld* world, Entity entity) {
    if (entity >= world->entity_capacity) return false;
    return world->mesh_sparse[entity] != SPARSE_INVALID;
}

void ecs_remove_mesh(ECSWorld* world, Entity entity) {
    if (entity >= world->entity_capacity) return;
    uint32_t dense_index = world->mesh_sparse[entity];
    if (dense_index == SPARSE_INVALID) return;

    uint32_t last_index = --world->mesh_count;
    Entity last_entity = world->mesh_entities[last_index];

    world->meshes[dense_index] = world->meshes[last_index];
    world->mesh_entities[dense_index] = last_entity;
    world->mesh_sparse[last_entity] = dense_index;
    world->mesh_sparse[entity] = SPARSE_INVALID;
}

//==============================================================================
// Component Management - Material
//==============================================================================

void ecs_add_material(ECSWorld* world, Entity entity, MaterialComponent* material) {
    if (entity >= world->entity_capacity) return;
    if (world->material_sparse[entity] != SPARSE_INVALID) return;

    uint32_t dense_index = world->material_count++;
    world->material_sparse[entity] = dense_index;
    world->material_entities[dense_index] = entity;
    world->materials[dense_index] = *material;
}

MaterialComponent* ecs_get_material(ECSWorld* world, Entity entity) {
    if (entity >= world->entity_capacity) return NULL;
    uint32_t dense_index = world->material_sparse[entity];
    if (dense_index == SPARSE_INVALID) return NULL;
    return &world->materials[dense_index];
}

bool ecs_has_material(ECSWorld* world, Entity entity) {
    if (entity >= world->entity_capacity) return false;
    return world->material_sparse[entity] != SPARSE_INVALID;
}

void ecs_remove_material(ECSWorld* world, Entity entity) {
    if (entity >= world->entity_capacity) return;
    uint32_t dense_index = world->material_sparse[entity];
    if (dense_index == SPARSE_INVALID) return;

    uint32_t last_index = --world->material_count;
    Entity last_entity = world->material_entities[last_index];

    world->materials[dense_index] = world->materials[last_index];
    world->material_entities[dense_index] = last_entity;
    world->material_sparse[last_entity] = dense_index;
    world->material_sparse[entity] = SPARSE_INVALID;
}

//==============================================================================
// Component Management - Light
//==============================================================================

void ecs_add_light(ECSWorld* world, Entity entity, LightComponent* light) {
    if (entity >= world->entity_capacity) return;
    if (world->light_sparse[entity] != SPARSE_INVALID) return;

    uint32_t dense_index = world->light_count++;
    world->light_sparse[entity] = dense_index;
    world->light_entities[dense_index] = entity;
    world->lights[dense_index] = *light;
}

LightComponent* ecs_get_light(ECSWorld* world, Entity entity) {
    if (entity >= world->entity_capacity) return NULL;
    uint32_t dense_index = world->light_sparse[entity];
    if (dense_index == SPARSE_INVALID) return NULL;
    return &world->lights[dense_index];
}

bool ecs_has_light(ECSWorld* world, Entity entity) {
    if (entity >= world->entity_capacity) return false;
    return world->light_sparse[entity] != SPARSE_INVALID;
}

void ecs_remove_light(ECSWorld* world, Entity entity) {
    if (entity >= world->entity_capacity) return;
    uint32_t dense_index = world->light_sparse[entity];
    if (dense_index == SPARSE_INVALID) return;

    uint32_t last_index = --world->light_count;
    Entity last_entity = world->light_entities[last_index];

    world->lights[dense_index] = world->lights[last_index];
    world->light_entities[dense_index] = last_entity;
    world->light_sparse[last_entity] = dense_index;
    world->light_sparse[entity] = SPARSE_INVALID;
}

//==============================================================================
// Component Management - Camera
//==============================================================================

void ecs_add_camera(ECSWorld* world, Entity entity, CameraComponent* camera) {
    if (entity >= world->entity_capacity) return;
    if (world->camera_sparse[entity] != SPARSE_INVALID) return;

    uint32_t dense_index = world->camera_count++;
    world->camera_sparse[entity] = dense_index;
    world->camera_entities[dense_index] = entity;
    world->cameras[dense_index] = *camera;
    world->cameras[dense_index].dirty = true;
}

CameraComponent* ecs_get_camera(ECSWorld* world, Entity entity) {
    if (entity >= world->entity_capacity) return NULL;
    uint32_t dense_index = world->camera_sparse[entity];
    if (dense_index == SPARSE_INVALID) return NULL;
    return &world->cameras[dense_index];
}

bool ecs_has_camera(ECSWorld* world, Entity entity) {
    if (entity >= world->entity_capacity) return false;
    return world->camera_sparse[entity] != SPARSE_INVALID;
}

void ecs_remove_camera(ECSWorld* world, Entity entity) {
    if (entity >= world->entity_capacity) return;
    uint32_t dense_index = world->camera_sparse[entity];
    if (dense_index == SPARSE_INVALID) return;

    uint32_t last_index = --world->camera_count;
    Entity last_entity = world->camera_entities[last_index];

    world->cameras[dense_index] = world->cameras[last_index];
    world->camera_entities[dense_index] = last_entity;
    world->camera_sparse[last_entity] = dense_index;
    world->camera_sparse[entity] = SPARSE_INVALID;

    if (world->active_camera == entity) {
        world->active_camera = ENTITY_INVALID;
    }
}

//==============================================================================
// Component Iteration
//==============================================================================

void ecs_iterate_transforms(ECSWorld* world, ComponentIteratorFn fn, void* user_data) {
    for (uint32_t i = 0; i < world->transform_count; i++) {
        fn(world->transform_entities[i], &world->transforms[i], user_data);
    }
}

void ecs_iterate_meshes(ECSWorld* world, ComponentIteratorFn fn, void* user_data) {
    for (uint32_t i = 0; i < world->mesh_count; i++) {
        fn(world->mesh_entities[i], &world->meshes[i], user_data);
    }
}

void ecs_iterate_lights(ECSWorld* world, ComponentIteratorFn fn, void* user_data) {
    for (uint32_t i = 0; i < world->light_count; i++) {
        fn(world->light_entities[i], &world->lights[i], user_data);
    }
}

//==============================================================================
// Entity Queries
//==============================================================================

EntityQuery ecs_query_entities(ECSWorld* world, uint32_t component_mask) {
    EntityQuery query = {0};

    // Allocate temporary array
    Entity* temp_entities = (Entity*)malloc(world->entity_count * sizeof(Entity));
    uint32_t count = 0;

    // Check each entity
    for (uint32_t i = 0; i < world->entity_count; i++) {
        Entity entity = world->entities[i];
        bool matches = true;

        if ((component_mask & COMPONENT_TRANSFORM) && !ecs_has_transform(world, entity)) matches = false;
        if ((component_mask & COMPONENT_MESH) && !ecs_has_mesh(world, entity)) matches = false;
        if ((component_mask & COMPONENT_MATERIAL) && !ecs_has_material(world, entity)) matches = false;
        if ((component_mask & COMPONENT_LIGHT) && !ecs_has_light(world, entity)) matches = false;
        if ((component_mask & COMPONENT_CAMERA) && !ecs_has_camera(world, entity)) matches = false;

        if (matches) {
            temp_entities[count++] = entity;
        }
    }

    // Copy to final array
    query.entities = (Entity*)malloc(count * sizeof(Entity));
    memcpy(query.entities, temp_entities, count * sizeof(Entity));
    query.count = count;

    free(temp_entities);

    return query;
}

void ecs_query_free(EntityQuery* query) {
    if (query && query->entities) {
        free(query->entities);
        query->entities = NULL;
        query->count = 0;
    }
}

// Transform helpers
void transform_update_matrix(TransformComponent* transform) {
    if (!transform->dirty) return;

    Mat4 t, r, s, temp_mul;
    fp_mat4_translation(&t, transform->position.x, transform->position.y, transform->position.z);
    fp_mat4_rotation_quat(&r, transform->rotation.x, transform->rotation.y, transform->rotation.z, transform->rotation.w);
    fp_mat4_scale(&s, transform->scale.x, transform->scale.y, transform->scale.z);

    fp_mat4_mul(&temp_mul, &t, &r);
    fp_mat4_mul(&transform->local_matrix, &temp_mul, &s);
    transform->world_matrix = transform->local_matrix; // No hierarchy yet
    transform->dirty = false;
}

Mat4 transform_get_matrix(const TransformComponent* transform) {
    return transform->world_matrix;
}