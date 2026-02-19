#include "cecs.h"
#include "map.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

void registry_init(Registry* registry) {
    registry->entity_count = 0;
    registry->component_count = 0;
    registry->running = false;
    registry->fixed_delta = 1.0;
    registry->startup_system_count = 0;
    registry->update_system_count = 0;
    registry->fixed_system_count = 0;

    map_init(&registry->component_pools, sizeof(ComponentPool), 4);
    map_init(&registry->startup_system_masks, sizeof(SystemMask), 4);
    map_init(&registry->update_system_masks, sizeof(SystemMask), 4);
    map_init(&registry->fixed_system_masks, sizeof(SystemMask), 4);

    map_init(&registry->component_masks, sizeof(Map), 4);
}

void registry_clean(Registry * registry) {
    for (int i = 0; i < registry->component_count; i++) {
        map_clean(&((ComponentPool*)map_get(&registry->component_pools, i))->data);
    }
    for (int i = 0; i < registry->entity_count; i++) {
        map_clean(map_get(&registry->component_masks, i));
    }

    for (int i = 0; i < registry->startup_system_count; i++) {
        free(((SystemMask*)map_get(&registry->startup_system_masks, i))->components);
    }
    for (int i = 0; i < registry->update_system_count; i++) {
        free(((SystemMask*)map_get(&registry->update_system_masks, i))->components);
    }
    for (int i = 0; i < registry->fixed_system_count; i++) {
        free(((SystemMask*)map_get(&registry->fixed_system_masks, i))->components);
    }
    map_clean(&registry->component_pools);
    map_clean(&registry->component_masks);
    map_clean(&registry->startup_system_masks);
    map_clean(&registry->update_system_masks);
    map_clean(&registry->fixed_system_masks);
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
    
    map_add(&registry->component_masks, id);
    map_init(map_get(&registry->component_masks, id), sizeof(bool), 4);

    return id;
}

void registry_register_system(Registry* registry, ProcessMode process_mode, System system, int component_count, ...) {
    u32* system_count;
    Map* system_masks;

    switch (process_mode) {
        case update:
            system_count = &registry->update_system_count;
            system_masks = &registry->update_system_masks;
            break;
        case fixed_update:
            system_count = &registry->fixed_system_count;
            system_masks = &registry->fixed_system_masks;
            break;
        default:
            system_count = &registry->startup_system_count;
            system_masks = &registry->startup_system_masks;
            break;
    }

    u32 system_id = (*system_count)++;

    SystemMask system_mask = {0};
    system_mask.system = system;
    system_mask.component_count = component_count;
    system_mask.components = calloc(sizeof(ComponentType), component_count);

    va_list args;
    va_start(args, component_count);

    for (int i = 0; i < component_count; i++) {
        system_mask.components[i] = va_arg(args, ComponentType);
    }

    va_end(args);

    map_set(system_masks, system_id, &system_mask);
}


void registry_add_component(
        Registry* registry,
        Entity entity,
        ComponentType component,
        void* data
    ) {
    ComponentPool* pool = map_get(&registry->component_pools, component);

    map_set(&pool->data, entity, data);

    bool set = true;
    map_set(map_get(&registry->component_masks, entity), component, &set);
}

void* registry_get_component(
        Registry* registry,
        Entity entity,
        ComponentType component
    ) {
    if (!*(bool*)map_get(map_get(&registry->component_masks, entity), component)) {
        return NULL;
    }

    ComponentPool* pool = map_get(&registry->component_pools, component);
    return map_get(&pool->data, entity);
}

bool registry_has_components(
        Registry* registry,
        Entity entity,
        u32 num_components,
        ComponentType* components
    ) {
    for (int i = 0; i < num_components; i++) {
        bool* has = map_get(map_get(&registry->component_masks, entity), components[i]);
        if (!has || !*has) {
            return false;
        }
    }

    return true;
}

void registry_startup_systems(Registry* registry) {
    for (int i = 0; i < registry->startup_system_count; i++) {
        SystemMask* system_mask = map_get(&registry->startup_system_masks, i);

        for (Entity entity = 0; entity < registry->entity_count; entity++) {
            if (registry_has_components(registry, entity, system_mask->component_count, system_mask->components)) {
                system_mask->system(registry, entity, 0.0);
            }
        }
    }
}

void registry_update_systems(Registry* registry, double delta) {
    for (int i = 0; i < registry->update_system_count; i++) {
        SystemMask* system_mask = map_get(&registry->update_system_masks, i);

        for (Entity entity = 0; entity < registry->entity_count; entity++) {
            if (registry_has_components(registry, entity, system_mask->component_count, system_mask->components)) {
                system_mask->system(registry, entity, delta);
            }
        }
    }
}

void registry_fixed_systems(Registry* registry, double delta) {
    for (int i = 0; i < registry->fixed_system_count; i++) {
        SystemMask* system_mask = map_get(&registry->fixed_system_masks, i);

        for (Entity entity = 0; entity < registry->entity_count; entity++) {
            if (registry_has_components(registry, entity, system_mask->component_count, system_mask->components)) {
                system_mask->system(registry, entity, delta);
            }
        }
    }
}

void registry_set_fixed_delta(Registry* registry, double fixed_delta) {
    registry->fixed_delta = fixed_delta;
}

void registry_start(Registry* registry) {
    registry->running = true;

    registry_startup_systems(registry);

    double last_time = current_time_secs();
    double accumulator = 0.0;

    while (registry->running) {
        double current_time = current_time_secs();
        double frame_time = current_time - last_time;
        last_time = current_time;

        accumulator += frame_time;

        while (accumulator >= registry->fixed_delta) {
            registry_fixed_systems(registry, registry->fixed_delta);
            accumulator -= registry->fixed_delta;
        }

        registry_update_systems(registry, accumulator / registry->fixed_delta); 
    }

    registry_clean(registry);
}

void registry_stop(Registry* registry) {
    registry->running = false;
}

double current_time_secs(void) {
    struct timeval now;
    gettimeofday(&now, NULL);

    return now.tv_sec + now.tv_usec / 1000000.0;
}
