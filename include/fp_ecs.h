#ifndef FP_ECS_H
#define FP_ECS_H

/**
 * FP-ASM Library - Functional Entity Component System (ECS)
 *
 * This module provides a fully immutable, persistent ECS designed for
 * functional-style workloads. Every mutating operation returns a new world
 * value instead of modifying the input instance in-place, allowing callers to
 * treat the ECS state as a value that can be passed, cloned, and versioned
 * without hidden side effects. All allocations are deterministic and owned by
 * the world structure returned from each API call.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Unique identifier assigned to each entity. */
typedef uint64_t fp_ecs_entity;

/** Component type handle returned from registration. */
typedef uint32_t fp_ecs_component_type;

/** Possible error/status codes returned by ECS operations. */
typedef enum {
    FP_ECS_OK = 0,
    FP_ECS_ERROR_INVALID_ARGUMENT,
    FP_ECS_ERROR_OUT_OF_MEMORY,
    FP_ECS_ERROR_ENTITY_NOT_FOUND,
    FP_ECS_ERROR_COMPONENT_NOT_FOUND,
    FP_ECS_ERROR_COMPONENT_ALREADY_PRESENT,
    FP_ECS_ERROR_TYPE_NOT_REGISTERED
} fp_ecs_status;

struct fp_ecs_component_pool;

/**
 * Immutable world value. All internal memory is owned by this structure and
 * must be released with fp_ecs_world_destroy when no longer needed.
 */
typedef struct {
    fp_ecs_entity next_entity_id;
    size_t entity_count;
    fp_ecs_entity* entities;

    size_t component_type_count;
    size_t* component_sizes;
    struct fp_ecs_component_pool* pools;
} fp_ecs_world;

/** Result when creating a new entity. */
typedef struct {
    fp_ecs_world world;
    fp_ecs_status status;
    fp_ecs_entity entity;
} fp_ecs_entity_result;

/** Result when registering a new component type. */
typedef struct {
    fp_ecs_world world;
    fp_ecs_status status;
    fp_ecs_component_type type;
} fp_ecs_component_registration;

/** Result for operations that only update the world value. */
typedef struct {
    fp_ecs_world world;
    fp_ecs_status status;
} fp_ecs_world_update;

/** Read-only lookup result for an entity-component pair. */
typedef struct {
    fp_ecs_status status;
    const void* component;
    size_t component_size;
} fp_ecs_component_lookup;

/** Read-only snapshot of an entire component pool. */
typedef struct {
    fp_ecs_status status;
    const fp_ecs_entity* entities;
    const void* components;
    size_t count;
    size_t component_size;
} fp_ecs_component_span;

/** Obtain an empty world value with no entities or components. */
fp_ecs_world fp_ecs_world_empty(void);

/** Deeply release all memory owned by the given world value. */
void fp_ecs_world_destroy(fp_ecs_world* world);

/** Create an exact, deep clone of the provided world value. */
fp_ecs_world fp_ecs_world_clone(const fp_ecs_world* world, fp_ecs_status* status);

/**
 * Register a new component type with the ECS.
 *
 * @param world          Existing world value.
 * @param component_size Size in bytes for each component instance.
 * @return A new world value plus the assigned component type identifier.
 */
fp_ecs_component_registration
fp_ecs_register_component(const fp_ecs_world* world, size_t component_size);

/**
 * Create a brand-new entity identifier.
 *
 * @param world Existing world value.
 * @return A new world value containing the newly created entity.
 */
fp_ecs_entity_result fp_ecs_create_entity(const fp_ecs_world* world);

/**
 * Destroy an entity and remove all of its components.
 *
 * @param world  Existing world value.
 * @param entity Entity to remove.
 */
fp_ecs_world_update fp_ecs_destroy_entity(const fp_ecs_world* world, fp_ecs_entity entity);

/**
 * Attach a component instance to an entity.
 *
 * @param world          Existing world value.
 * @param type           Component type previously registered.
 * @param entity         Target entity.
 * @param component_data Pointer to the component payload (can be NULL to zero-initialize).
 */
fp_ecs_world_update fp_ecs_add_component(const fp_ecs_world* world,
                                         fp_ecs_component_type type,
                                         fp_ecs_entity entity,
                                         const void* component_data);

/**
 * Remove a component instance from an entity.
 */
fp_ecs_world_update fp_ecs_remove_component(const fp_ecs_world* world,
                                            fp_ecs_component_type type,
                                            fp_ecs_entity entity);

/**
 * Retrieve a const pointer to a component instance stored on an entity.
 */
fp_ecs_component_lookup fp_ecs_get_component(const fp_ecs_world* world,
                                             fp_ecs_component_type type,
                                             fp_ecs_entity entity);

/**
 * Obtain a read-only span over all components of the specified type.
 */
fp_ecs_component_span fp_ecs_view_components(const fp_ecs_world* world,
                                             fp_ecs_component_type type);

/** Determine whether the given entity exists inside the world. */
bool fp_ecs_entity_exists(const fp_ecs_world* world, fp_ecs_entity entity);

#ifdef __cplusplus
}
#endif

#endif /* FP_ECS_H */
