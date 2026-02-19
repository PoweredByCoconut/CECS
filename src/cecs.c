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

    map_init(&registry->component_pools, sizeof(ComponentPool), 4);
    map_init(&registry->system_masks, sizeof(SystemMask), 4);

    map_init(&registry->component_masks, sizeof(Map), 4);
}

void registry_clean(Registry * registry) {
    for (int i = 0; i < registry->component_count; i++) {
        map_clean(&((ComponentPool*)map_get(&registry->component_pools, i))->data);
    }
    for (int i = 0; i < registry->entity_count; i++) {
        map_clean(map_get(&registry->component_masks, i));
    }
    for (int i = 0; i < registry->system_count; i++) {
        free(((SystemMask*)map_get(&registry->system_masks, i))->components);
    }
    map_clean(&registry->component_pools);
    map_clean(&registry->system_masks);
    map_clean(&registry->component_masks);
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
    u32 system_id = registry->system_count++;

    SystemMask system_mask = {0};
    system_mask.system = system;
    system_mask.component_count = component_count;
    system_mask.components = calloc(sizeof(ComponentType), component_count);
    system_mask.process_mode = process_mode;

    va_list args;
    va_start(args, component_count);

    for (int i = 0; i < component_count; i++) {
        system_mask.components[i] = va_arg(args, ComponentType);
    }

    va_end(args);

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

void registry_execute_systems(Registry* registry, ProcessMode process_mode, double delta) {
    for (int i = 0; i < registry->system_count; i++) {
        SystemMask* system_mask = map_get(&registry->system_masks, i);

        for (Entity entity = 0; entity < registry->entity_count; entity++) {
            if (
                    registry_has_components(registry, entity, system_mask->component_count, system_mask->components) &&
                    system_mask->process_mode == process_mode
                ) {
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

    registry_execute_systems(registry, startup, 0.0);

    double last_time = current_time_secs();
    double accumulator = 0.0;

    while (registry->running) {
        double current_time = current_time_secs();
        double frame_time = current_time - last_time;
        last_time = current_time;

        accumulator += frame_time;

        while (accumulator >= registry->fixed_delta) {
            registry_execute_systems(registry, fixed_update, registry->fixed_delta);
            accumulator -= registry->fixed_delta;
        }

        // registry_execute_systems(registry, update, accumulator / registry->fixed_delta); // use this for rendering in the future
        registry_execute_systems(registry, update, frame_time);
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
