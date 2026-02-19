#include "cecs.h"
#include "map.h"
#include <stdarg.h>
#include <string.h>

void registry_init(Registry* registry) {
    registry->entity_count = 0;
    registry->component_count = 0;
    map_init(&registry->component_pools, sizeof(ComponentPool), 4);
    map_init(&registry->system_masks, sizeof(SystemMask), 4);
}

void registry_clean(Registry * registry) {
    for (int i = 0; i < registry->component_count; i++) {
        map_clean(&((ComponentPool*)map_get(&registry->component_pools, i))->data);
    }
    map_clean(&registry->component_pools);
    map_clean(&registry->system_masks);
}

ComponentType registry_register_component(Registry *registry, u32 size) {
    ComponentType id = registry->component_count++;

    map_add(&registry->component_pools, id);
    ComponentPool* pool = map_get(&registry->component_pools, id);
    pool->size = size;
    map_init(&pool->data, size, 4);

    return id;
}

Entity registry_register_entity(Registry *registry) {
    Entity id = registry->entity_count++;
    
    registry->component_masks[id] = 0;

    return id;
}

void registry_register_system(Registry* registry, System system, int component_count, ...) {
    u32 system_id = registry->system_count++;

    SystemMask system_mask = {0};
    system_mask.system = system;

    u32 mask = 0;
    va_list args;
    va_start(args, component_count);

    for (int i = 0; i < component_count; i++) {
        mask |= (1 << va_arg(args, u32));
    }

    va_end(args);

    system_mask.mask = mask;
    map_set(&registry->system_masks, system_id, &system_mask);
}


void registry_add_component(
        Registry* registry,
        Entity entity,
        ComponentType component,
        void* data
    ) {
    ComponentPool* pool = map_get(&registry->component_pools, component);

    map_set(&pool->data, entity, data);

    registry->component_masks[entity] |= 1 << component;
}

void* registry_get_component(
        Registry* registry,
        Entity entity,
        ComponentType component
    ) {
    if (!(registry->component_masks[entity] & (1 << component))) {
        return NULL;
    }

    ComponentPool* pool = map_get(&registry->component_pools, component);
    return map_get(&pool->data, entity);
}

bool registry_has_components(
        Registry* registry,
        Entity entity,
        u32 mask
    ) {
    return (registry->component_masks[entity] & mask) == mask;
}

void registry_execute_systems(Registry* registry) {
    for (int i = 0; i < registry->system_count; i++) {
        SystemMask* system_mask = map_get(&registry->system_masks, i);

        for (Entity entity = 0; entity < registry->entity_count; entity++) {
            if (registry_has_components(registry, entity, system_mask->mask)) {
                system_mask->system(registry, entity);
            }
        }
    }
}
