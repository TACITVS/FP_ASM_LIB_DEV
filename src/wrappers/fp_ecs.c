#include "../../include/fp_ecs.h"

#include <stdlib.h>
#include <string.h>

struct fp_ecs_component_pool {
    fp_ecs_entity* entities;
    unsigned char* data;
    size_t count;
};

static fp_ecs_world fp_ecs_world_make_empty(void) {
    fp_ecs_world world;
    world.next_entity_id = 1;
    world.entity_count = 0;
    world.entities = NULL;
    world.component_type_count = 0;
    world.component_sizes = NULL;
    world.pools = NULL;
    return world;
}

fp_ecs_world fp_ecs_world_empty(void) {
    return fp_ecs_world_make_empty();
}

void fp_ecs_world_destroy(fp_ecs_world* world) {
    if (!world) return;

    if (world->pools) {
        for (size_t i = 0; i < world->component_type_count; i++) {
            free(world->pools[i].entities);
            free(world->pools[i].data);
        }
    }

    free(world->entities);
    free(world->component_sizes);
    free(world->pools);

    *world = fp_ecs_world_make_empty();
}

static bool fp_ecs_internal_entity_exists(const fp_ecs_world* world, fp_ecs_entity entity) {
    if (!world || world->entity_count == 0) return false;
    for (size_t i = 0; i < world->entity_count; i++) {
        if (world->entities[i] == entity) {
            return true;
        }
    }
    return false;
}

static size_t fp_ecs_internal_component_index(const struct fp_ecs_component_pool* pool,
                                              fp_ecs_entity entity) {
    if (!pool) return (size_t)-1;
    for (size_t i = 0; i < pool->count; i++) {
        if (pool->entities[i] == entity) {
            return i;
        }
    }
    return (size_t)-1;
}

static fp_ecs_world fp_ecs_world_clone_internal(const fp_ecs_world* world, fp_ecs_status* status) {
    if (status) {
        *status = FP_ECS_OK;
    }

    if (!world) {
        if (status) {
            *status = FP_ECS_ERROR_INVALID_ARGUMENT;
        }
        return fp_ecs_world_make_empty();
    }

    fp_ecs_world clone = fp_ecs_world_make_empty();
    clone.next_entity_id = world->next_entity_id;
    clone.entity_count = world->entity_count;
    clone.component_type_count = world->component_type_count;

    if (world->entity_count > 0) {
        clone.entities = (fp_ecs_entity*)malloc(sizeof(fp_ecs_entity) * world->entity_count);
        if (!clone.entities) {
            if (status) *status = FP_ECS_ERROR_OUT_OF_MEMORY;
            goto failure;
        }
        memcpy(clone.entities, world->entities, sizeof(fp_ecs_entity) * world->entity_count);
    }

    if (world->component_type_count > 0) {
        clone.component_sizes = (size_t*)malloc(sizeof(size_t) * world->component_type_count);
        clone.pools = (struct fp_ecs_component_pool*)malloc(
            sizeof(struct fp_ecs_component_pool) * world->component_type_count);
        if (!clone.component_sizes || !clone.pools) {
            if (status) *status = FP_ECS_ERROR_OUT_OF_MEMORY;
            goto failure;
        }
        memcpy(clone.component_sizes, world->component_sizes,
               sizeof(size_t) * world->component_type_count);
        for (size_t i = 0; i < world->component_type_count; i++) {
            clone.pools[i].count = world->pools[i].count;
            clone.pools[i].entities = NULL;
            clone.pools[i].data = NULL;
            if (world->pools[i].count > 0) {
                size_t component_size = world->component_sizes[i];
                clone.pools[i].entities = (fp_ecs_entity*)malloc(
                    sizeof(fp_ecs_entity) * world->pools[i].count);
                clone.pools[i].data = (unsigned char*)malloc(
                    component_size * world->pools[i].count);
                if (!clone.pools[i].entities || !clone.pools[i].data) {
                    if (status) *status = FP_ECS_ERROR_OUT_OF_MEMORY;
                    goto failure;
                }
                memcpy(clone.pools[i].entities, world->pools[i].entities,
                       sizeof(fp_ecs_entity) * world->pools[i].count);
                memcpy(clone.pools[i].data, world->pools[i].data,
                       component_size * world->pools[i].count);
            }
        }
    }

    return clone;

failure:
    fp_ecs_world_destroy(&clone);
    return fp_ecs_world_make_empty();
}

fp_ecs_world fp_ecs_world_clone(const fp_ecs_world* world, fp_ecs_status* status) {
    return fp_ecs_world_clone_internal(world, status);
}

static fp_ecs_world_update fp_ecs_world_update_from_clone(fp_ecs_world clone, fp_ecs_status status) {
    fp_ecs_world_update update;
    update.world = clone;
    update.status = status;
    return update;
}

fp_ecs_component_registration
fp_ecs_register_component(const fp_ecs_world* world, size_t component_size) {
    fp_ecs_component_registration result;
    result.world = fp_ecs_world_make_empty();
    result.type = 0;
    result.status = FP_ECS_ERROR_INVALID_ARGUMENT;

    if (!world || component_size == 0) {
        return result;
    }

    fp_ecs_status status = FP_ECS_OK;
    fp_ecs_world clone = fp_ecs_world_clone_internal(world, &status);
    if (status != FP_ECS_OK) {
        fp_ecs_world_destroy(&clone);
        result.status = status;
        return result;
    }

    size_t new_count = clone.component_type_count + 1;
    size_t* new_sizes = (size_t*)malloc(sizeof(size_t) * new_count);
    struct fp_ecs_component_pool* new_pools =
        (struct fp_ecs_component_pool*)malloc(sizeof(struct fp_ecs_component_pool) * new_count);

    if (!new_sizes || !new_pools) {
        free(new_sizes);
        free(new_pools);
        fp_ecs_world_destroy(&clone);
        result.status = FP_ECS_ERROR_OUT_OF_MEMORY;
        return result;
    }

    if (clone.component_type_count > 0) {
        memcpy(new_sizes, clone.component_sizes, sizeof(size_t) * clone.component_type_count);
        memcpy(new_pools, clone.pools,
               sizeof(struct fp_ecs_component_pool) * clone.component_type_count);
    }

    new_sizes[new_count - 1] = component_size;
    new_pools[new_count - 1].entities = NULL;
    new_pools[new_count - 1].data = NULL;
    new_pools[new_count - 1].count = 0;

    free(clone.component_sizes);
    free(clone.pools);

    clone.component_sizes = new_sizes;
    clone.pools = new_pools;
    clone.component_type_count = new_count;

    result.world = clone;
    result.type = (fp_ecs_component_type)(new_count - 1);
    result.status = FP_ECS_OK;
    return result;
}

fp_ecs_entity_result fp_ecs_create_entity(const fp_ecs_world* world) {
    fp_ecs_entity_result result;
    result.world = fp_ecs_world_make_empty();
    result.status = FP_ECS_ERROR_INVALID_ARGUMENT;
    result.entity = 0;

    if (!world) {
        return result;
    }

    fp_ecs_status status = FP_ECS_OK;
    fp_ecs_world clone = fp_ecs_world_clone_internal(world, &status);
    if (status != FP_ECS_OK) {
        fp_ecs_world_destroy(&clone);
        result.status = status;
        return result;
    }

    size_t old_count = clone.entity_count;
    size_t new_count = old_count + 1;
    fp_ecs_entity* new_entities = (fp_ecs_entity*)malloc(sizeof(fp_ecs_entity) * new_count);
    if (!new_entities) {
        fp_ecs_world_destroy(&clone);
        result.status = FP_ECS_ERROR_OUT_OF_MEMORY;
        return result;
    }

    if (old_count > 0) {
        memcpy(new_entities, clone.entities, sizeof(fp_ecs_entity) * old_count);
    }

    fp_ecs_entity new_entity = clone.next_entity_id;
    new_entities[new_count - 1] = new_entity;

    free(clone.entities);
    clone.entities = new_entities;
    clone.entity_count = new_count;
    clone.next_entity_id = new_entity + 1;

    result.world = clone;
    result.status = FP_ECS_OK;
    result.entity = new_entity;
    return result;
}

fp_ecs_world_update fp_ecs_destroy_entity(const fp_ecs_world* world, fp_ecs_entity entity) {
    if (!world || !fp_ecs_internal_entity_exists(world, entity)) {
        return fp_ecs_world_update_from_clone(fp_ecs_world_make_empty(), FP_ECS_ERROR_ENTITY_NOT_FOUND);
    }

    fp_ecs_status status = FP_ECS_OK;
    fp_ecs_world clone = fp_ecs_world_clone_internal(world, &status);
    if (status != FP_ECS_OK) {
        fp_ecs_world_destroy(&clone);
        return fp_ecs_world_update_from_clone(fp_ecs_world_make_empty(), status);
    }

    size_t old_count = clone.entity_count;
    size_t new_count = old_count - 1;
    fp_ecs_entity* new_entities = NULL;

    if (new_count > 0) {
        new_entities = (fp_ecs_entity*)malloc(sizeof(fp_ecs_entity) * new_count);
        if (!new_entities) {
            fp_ecs_world_destroy(&clone);
            return fp_ecs_world_update_from_clone(fp_ecs_world_make_empty(), FP_ECS_ERROR_OUT_OF_MEMORY);
        }
    }

    size_t write_index = 0;
    for (size_t i = 0; i < old_count; i++) {
        if (clone.entities[i] != entity) {
            if (new_entities) {
                new_entities[write_index] = clone.entities[i];
            }
            write_index++;
        }
    }

    free(clone.entities);
    clone.entities = new_entities;
    clone.entity_count = new_count;

    for (size_t type_index = 0; type_index < clone.component_type_count; type_index++) {
        struct fp_ecs_component_pool* pool = &clone.pools[type_index];
        size_t comp_size = clone.component_sizes[type_index];
        size_t idx = fp_ecs_internal_component_index(pool, entity);
        if (idx == (size_t)-1) {
            continue;
        }

        if (pool->count == 1) {
            free(pool->entities);
            free(pool->data);
            pool->entities = NULL;
            pool->data = NULL;
            pool->count = 0;
            continue;
        }

        size_t new_pool_count = pool->count - 1;
        fp_ecs_entity* new_pool_entities = (fp_ecs_entity*)malloc(sizeof(fp_ecs_entity) * new_pool_count);
        unsigned char* new_pool_data = (unsigned char*)malloc(comp_size * new_pool_count);
        if (!new_pool_entities || !new_pool_data) {
            free(new_pool_entities);
            free(new_pool_data);
            fp_ecs_world_destroy(&clone);
            return fp_ecs_world_update_from_clone(fp_ecs_world_make_empty(), FP_ECS_ERROR_OUT_OF_MEMORY);
        }

        if (idx > 0) {
            memcpy(new_pool_entities, pool->entities, sizeof(fp_ecs_entity) * idx);
            memcpy(new_pool_data, pool->data, comp_size * idx);
        }
        if (idx + 1 < pool->count) {
            size_t tail_count = pool->count - idx - 1;
            memcpy(new_pool_entities + idx, pool->entities + idx + 1, sizeof(fp_ecs_entity) * tail_count);
            memcpy(new_pool_data + idx * comp_size, pool->data + (idx + 1) * comp_size,
                   comp_size * tail_count);
        }

        free(pool->entities);
        free(pool->data);
        pool->entities = new_pool_entities;
        pool->data = new_pool_data;
        pool->count = new_pool_count;
    }

    return fp_ecs_world_update_from_clone(clone, FP_ECS_OK);
}

static fp_ecs_status fp_ecs_pool_append(struct fp_ecs_component_pool* pool,
                                        size_t component_size,
                                        fp_ecs_entity entity,
                                        const void* component_data) {
    size_t new_count = pool->count + 1;
    fp_ecs_entity* new_entities = (fp_ecs_entity*)malloc(sizeof(fp_ecs_entity) * new_count);
    unsigned char* new_data = (unsigned char*)malloc(component_size * new_count);
    if (!new_entities || !new_data) {
        free(new_entities);
        free(new_data);
        return FP_ECS_ERROR_OUT_OF_MEMORY;
    }

    if (pool->count > 0) {
        memcpy(new_entities, pool->entities, sizeof(fp_ecs_entity) * pool->count);
        memcpy(new_data, pool->data, component_size * pool->count);
    }

    new_entities[new_count - 1] = entity;
    if (component_data) {
        memcpy(new_data + (new_count - 1) * component_size, component_data, component_size);
    } else {
        memset(new_data + (new_count - 1) * component_size, 0, component_size);
    }

    free(pool->entities);
    free(pool->data);

    pool->entities = new_entities;
    pool->data = new_data;
    pool->count = new_count;
    return FP_ECS_OK;
}

fp_ecs_world_update fp_ecs_add_component(const fp_ecs_world* world,
                                         fp_ecs_component_type type,
                                         fp_ecs_entity entity,
                                         const void* component_data) {
    if (!world) {
        return fp_ecs_world_update_from_clone(fp_ecs_world_make_empty(), FP_ECS_ERROR_INVALID_ARGUMENT);
    }

    if (type >= world->component_type_count) {
        return fp_ecs_world_update_from_clone(fp_ecs_world_make_empty(), FP_ECS_ERROR_TYPE_NOT_REGISTERED);
    }

    if (!fp_ecs_internal_entity_exists(world, entity)) {
        return fp_ecs_world_update_from_clone(fp_ecs_world_make_empty(), FP_ECS_ERROR_ENTITY_NOT_FOUND);
    }

    const struct fp_ecs_component_pool* original_pool = &world->pools[type];
    if (fp_ecs_internal_component_index(original_pool, entity) != (size_t)-1) {
        return fp_ecs_world_update_from_clone(fp_ecs_world_make_empty(), FP_ECS_ERROR_COMPONENT_ALREADY_PRESENT);
    }

    fp_ecs_status status = FP_ECS_OK;
    fp_ecs_world clone = fp_ecs_world_clone_internal(world, &status);
    if (status != FP_ECS_OK) {
        fp_ecs_world_destroy(&clone);
        return fp_ecs_world_update_from_clone(fp_ecs_world_make_empty(), status);
    }

    struct fp_ecs_component_pool* pool = &clone.pools[type];
    size_t component_size = clone.component_sizes[type];
    status = fp_ecs_pool_append(pool, component_size, entity, component_data);
    if (status != FP_ECS_OK) {
        fp_ecs_world_destroy(&clone);
        return fp_ecs_world_update_from_clone(fp_ecs_world_make_empty(), status);
    }

    return fp_ecs_world_update_from_clone(clone, FP_ECS_OK);
}

static fp_ecs_status fp_ecs_pool_remove(struct fp_ecs_component_pool* pool,
                                        size_t component_size,
                                        size_t index) {
    if (index >= pool->count) {
        return FP_ECS_ERROR_COMPONENT_NOT_FOUND;
    }

    if (pool->count == 1) {
        free(pool->entities);
        free(pool->data);
        pool->entities = NULL;
        pool->data = NULL;
        pool->count = 0;
        return FP_ECS_OK;
    }

    size_t new_count = pool->count - 1;
    fp_ecs_entity* new_entities = (fp_ecs_entity*)malloc(sizeof(fp_ecs_entity) * new_count);
    unsigned char* new_data = (unsigned char*)malloc(component_size * new_count);
    if (!new_entities || !new_data) {
        free(new_entities);
        free(new_data);
        return FP_ECS_ERROR_OUT_OF_MEMORY;
    }

    if (index > 0) {
        memcpy(new_entities, pool->entities, sizeof(fp_ecs_entity) * index);
        memcpy(new_data, pool->data, component_size * index);
    }
    if (index + 1 < pool->count) {
        size_t tail_count = pool->count - index - 1;
        memcpy(new_entities + index, pool->entities + index + 1,
               sizeof(fp_ecs_entity) * tail_count);
        memcpy(new_data + index * component_size, pool->data + (index + 1) * component_size,
               component_size * tail_count);
    }

    free(pool->entities);
    free(pool->data);

    pool->entities = new_entities;
    pool->data = new_data;
    pool->count = new_count;
    return FP_ECS_OK;
}

fp_ecs_world_update fp_ecs_remove_component(const fp_ecs_world* world,
                                            fp_ecs_component_type type,
                                            fp_ecs_entity entity) {
    if (!world) {
        return fp_ecs_world_update_from_clone(fp_ecs_world_make_empty(), FP_ECS_ERROR_INVALID_ARGUMENT);
    }

    if (type >= world->component_type_count) {
        return fp_ecs_world_update_from_clone(fp_ecs_world_make_empty(), FP_ECS_ERROR_TYPE_NOT_REGISTERED);
    }

    const struct fp_ecs_component_pool* pool = &world->pools[type];
    size_t index = fp_ecs_internal_component_index(pool, entity);
    if (index == (size_t)-1) {
        return fp_ecs_world_update_from_clone(fp_ecs_world_make_empty(), FP_ECS_ERROR_COMPONENT_NOT_FOUND);
    }

    fp_ecs_status status = FP_ECS_OK;
    fp_ecs_world clone = fp_ecs_world_clone_internal(world, &status);
    if (status != FP_ECS_OK) {
        fp_ecs_world_destroy(&clone);
        return fp_ecs_world_update_from_clone(fp_ecs_world_make_empty(), status);
    }

    struct fp_ecs_component_pool* mutable_pool = &clone.pools[type];
    status = fp_ecs_pool_remove(mutable_pool, clone.component_sizes[type], index);
    if (status != FP_ECS_OK) {
        fp_ecs_world_destroy(&clone);
        return fp_ecs_world_update_from_clone(fp_ecs_world_make_empty(), status);
    }

    return fp_ecs_world_update_from_clone(clone, FP_ECS_OK);
}

fp_ecs_component_lookup fp_ecs_get_component(const fp_ecs_world* world,
                                             fp_ecs_component_type type,
                                             fp_ecs_entity entity) {
    fp_ecs_component_lookup result;
    result.component = NULL;
    result.component_size = 0;
    result.status = FP_ECS_ERROR_INVALID_ARGUMENT;

    if (!world) {
        return result;
    }

    if (type >= world->component_type_count) {
        result.status = FP_ECS_ERROR_TYPE_NOT_REGISTERED;
        return result;
    }

    const struct fp_ecs_component_pool* pool = &world->pools[type];
    size_t index = fp_ecs_internal_component_index(pool, entity);
    if (index == (size_t)-1) {
        result.status = FP_ECS_ERROR_COMPONENT_NOT_FOUND;
        return result;
    }

    size_t component_size = world->component_sizes[type];
    result.component = pool->data + index * component_size;
    result.component_size = component_size;
    result.status = FP_ECS_OK;
    return result;
}

fp_ecs_component_span fp_ecs_view_components(const fp_ecs_world* world,
                                             fp_ecs_component_type type) {
    fp_ecs_component_span span;
    span.entities = NULL;
    span.components = NULL;
    span.count = 0;
    span.component_size = 0;
    span.status = FP_ECS_ERROR_INVALID_ARGUMENT;

    if (!world) {
        return span;
    }

    if (type >= world->component_type_count) {
        span.status = FP_ECS_ERROR_TYPE_NOT_REGISTERED;
        return span;
    }

    const struct fp_ecs_component_pool* pool = &world->pools[type];
    span.entities = pool->entities;
    span.components = pool->data;
    span.count = pool->count;
    span.component_size = world->component_sizes[type];
    span.status = FP_ECS_OK;
    return span;
}

bool fp_ecs_entity_exists(const fp_ecs_world* world, fp_ecs_entity entity) {
    return fp_ecs_internal_entity_exists(world, entity);
}
