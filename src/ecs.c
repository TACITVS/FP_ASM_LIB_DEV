#include "../include/ecs.h"
#include "../include/fp_core.h"
#include <string.h> // For memcpy
#include <stdlib.h> // For calloc, free

// Placeholder implementations for ECS core functions
// These would typically be in a full ECS implementation
ECSWorld* ecs_world_create(void) {
    ECSWorld* world = (ECSWorld*)calloc(1, sizeof(ECSWorld));
    // Initialize sparse sets and other ECS data structures
    // For a simple demo, we'll just allocate space for components
    world->transforms = (TransformComponent*)calloc(ECS_MAX_ENTITIES, sizeof(TransformComponent));
    world->transform_sparse = (uint32_t*)calloc(ECS_MAX_ENTITIES, sizeof(uint32_t));
    world->transform_entities = (Entity*)calloc(ECS_MAX_ENTITIES, sizeof(Entity));
    for (uint32_t i = 0; i < ECS_MAX_ENTITIES; ++i) world->transform_sparse[i] = ENTITY_INVALID;

    world->meshes = (MeshComponent*)calloc(ECS_MAX_ENTITIES, sizeof(MeshComponent));
    world->mesh_sparse = (uint32_t*)calloc(ECS_MAX_ENTITIES, sizeof(uint32_t));
    world->mesh_entities = (Entity*)calloc(ECS_MAX_ENTITIES, sizeof(Entity));
    for (uint32_t i = 0; i < ECS_MAX_ENTITIES; ++i) world->mesh_sparse[i] = ENTITY_INVALID;

    world->materials = (MaterialComponent*)calloc(ECS_MAX_ENTITIES, sizeof(MaterialComponent));
    world->material_sparse = (uint32_t*)calloc(ECS_MAX_ENTITIES, sizeof(uint32_t));
    world->material_entities = (Entity*)calloc(ECS_MAX_ENTITIES, sizeof(Entity));
    for (uint32_t i = 0; i < ECS_MAX_ENTITIES; ++i) world->material_sparse[i] = ENTITY_INVALID;

    world->lights = (LightComponent*)calloc(ECS_MAX_ENTITIES, sizeof(LightComponent));
    world->light_sparse = (uint32_t*)calloc(ECS_MAX_ENTITIES, sizeof(uint32_t));
    world->light_entities = (Entity*)calloc(ECS_MAX_ENTITIES, sizeof(Entity));
    for (uint32_t i = 0; i < ECS_MAX_ENTITIES; ++i) world->light_sparse[i] = ENTITY_INVALID;

    world->cameras = (CameraComponent*)calloc(ECS_MAX_ENTITIES, sizeof(CameraComponent));
    world->camera_sparse = (uint32_t*)calloc(ECS_MAX_ENTITIES, sizeof(uint32_t));
    world->camera_entities = (Entity*)calloc(ECS_MAX_ENTITIES, sizeof(Entity));
    for (uint32_t i = 0; i < ECS_MAX_ENTITIES; ++i) world->camera_sparse[i] = ENTITY_INVALID;

    return world;
}

void ecs_world_destroy(ECSWorld* world) {
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

Entity ecs_entity_create(ECSWorld* world) {
    // Simple entity creation
    if (world->entity_count >= ECS_MAX_ENTITIES) return ENTITY_INVALID;
    return world->entity_count++;
}

void ecs_entity_destroy(ECSWorld* world, Entity entity) {
    // Mark entity as invalid
}

bool ecs_entity_is_alive(ECSWorld* world, Entity entity) {
    return entity < world->entity_count;
}

void ecs_add_transform(ECSWorld* world, Entity entity, TransformComponent* transform) {
    if (world->transform_count >= ECS_MAX_ENTITIES) return;
    world->transforms[world->transform_count] = *transform;
    world->transform_entities[world->transform_count] = entity;
    world->transform_sparse[entity] = world->transform_count;
    world->transform_count++;
}

void ecs_add_mesh(ECSWorld* world, Entity entity, MeshComponent* mesh) {
    if (world->mesh_count >= ECS_MAX_ENTITIES) return;
    world->meshes[world->mesh_count] = *mesh;
    world->mesh_entities[world->mesh_count] = entity;
    world->mesh_sparse[entity] = world->mesh_count;
    world->mesh_count++;
}

void ecs_add_material(ECSWorld* world, Entity entity, MaterialComponent* material) {
    if (world->material_count >= ECS_MAX_ENTITIES) return;
    world->materials[world->material_count] = *material;
    world->material_entities[world->material_count] = entity;
    world->material_sparse[entity] = world->material_count;
    world->material_count++;
}

void ecs_add_light(ECSWorld* world, Entity entity, LightComponent* light) {
    if (world->light_count >= ECS_MAX_ENTITIES) return;
    world->lights[world->light_count] = *light;
    world->light_entities[world->light_count] = entity;
    world->light_sparse[entity] = world->light_count;
    world->light_count++;
}

void ecs_add_camera(ECSWorld* world, Entity entity, CameraComponent* camera) {
    if (world->camera_count >= ECS_MAX_ENTITIES) return;
    world->cameras[world->camera_count] = *camera;
    world->camera_entities[world->camera_count] = entity;
    world->camera_sparse[entity] = world->camera_count;
    world->camera_count++;
}

TransformComponent* ecs_get_transform(ECSWorld* world, Entity entity) {
    if (entity >= ECS_MAX_ENTITIES || world->transform_sparse[entity] == ENTITY_INVALID) return NULL;
    return &world->transforms[world->transform_sparse[entity]];
}

MeshComponent* ecs_get_mesh(ECSWorld* world, Entity entity) {
    if (entity >= ECS_MAX_ENTITIES || world->mesh_sparse[entity] == ENTITY_INVALID) return NULL;
    return &world->meshes[world->mesh_sparse[entity]];
}

MaterialComponent* ecs_get_material(ECSWorld* world, Entity entity) {
    if (entity >= ECS_MAX_ENTITIES || world->material_sparse[entity] == ENTITY_INVALID) return NULL;
    return &world->materials[world->material_sparse[entity]];
}

LightComponent* ecs_get_light(ECSWorld* world, Entity entity) {
    if (entity >= ECS_MAX_ENTITIES || world->light_sparse[entity] == ENTITY_INVALID) return NULL;
    return &world->lights[world->light_sparse[entity]];
}

CameraComponent* ecs_get_camera(ECSWorld* world, Entity entity) {
    if (entity >= ECS_MAX_ENTITIES || world->camera_sparse[entity] == ENTITY_INVALID) return NULL;
    return &world->cameras[world->camera_sparse[entity]];
}

bool ecs_has_transform(ECSWorld* world, Entity entity) {
    return entity < ECS_MAX_ENTITIES && world->transform_sparse[entity] != ENTITY_INVALID;
}

bool ecs_has_mesh(ECSWorld* world, Entity entity) {
    return entity < ECS_MAX_ENTITIES && world->mesh_sparse[entity] != ENTITY_INVALID;
}

bool ecs_has_material(ECSWorld* world, Entity entity) {
    return entity < ECS_MAX_ENTITIES && world->material_sparse[entity] != ENTITY_INVALID;
}

bool ecs_has_light(ECSWorld* world, Entity entity) {
    return entity < ECS_MAX_ENTITIES && world->light_sparse[entity] != ENTITY_INVALID;
}

bool ecs_has_camera(ECSWorld* world, Entity entity) {
    return entity < ECS_MAX_ENTITIES && world->camera_sparse[entity] != ENTITY_INVALID;
}

void ecs_remove_transform(ECSWorld* world, Entity entity) {
    // Implement removal logic
}

void ecs_remove_mesh(ECSWorld* world, Entity entity) {
    // Implement removal logic
}

void ecs_remove_material(ECSWorld* world, Entity entity) {
    // Implement removal logic
}

void ecs_remove_light(ECSWorld* world, Entity entity) {
    // Implement removal logic
}

void ecs_remove_camera(ECSWorld* world, Entity entity) {
    // Implement removal logic
}

void ecs_iterate_transforms(ECSWorld* world, ComponentIteratorFn fn, void* user_data) {
    for (uint32_t i = 0; i < world->transform_count; ++i) {
        fn(world->transform_entities[i], &world->transforms[i], user_data);
    }
}

void ecs_iterate_meshes(ECSWorld* world, ComponentIteratorFn fn, void* user_data) {
    for (uint32_t i = 0; i < world->mesh_count; ++i) {
        fn(world->mesh_entities[i], &world->meshes[i], user_data);
    }
}

void ecs_iterate_lights(ECSWorld* world, ComponentIteratorFn fn, void* user_data) {
    for (uint32_t i = 0; i < world->light_count; ++i) {
        fn(world->light_entities[i], &world->lights[i], user_data);
    }
}

EntityQuery ecs_query_entities(ECSWorld* world, uint32_t component_mask) {
    EntityQuery query = {0};
    // Simple query for now, can be optimized
    for (uint32_t i = 0; i < world->entity_count; ++i) {
        // Check if entity has all components in mask
        // This is a very basic implementation and needs proper component bitmasking
        // For now, just return all entities
        if (query.count < ECS_MAX_ENTITIES) {
            query.entities[query.count++] = i;
        }
    }
    return query;
}

void ecs_query_free(EntityQuery* query) {
    // Nothing to free for this simple implementation
}




