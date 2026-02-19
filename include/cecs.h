#ifndef CECS_H
#define CECS_H

#include "types.h"
#include "map.h"

#define MAX_ENTITIES 100

typedef struct Registry Registry;
typedef u32 Entity;
typedef u32 ComponentType;
typedef void(*System)(Registry* registry, Entity entity, double delta);

typedef enum {
    startup,
    fixed_update,
    update
} ProcessMode;

typedef struct {
    u32 size;
    Map data;
} ComponentPool;

typedef struct {
    System system;
    u32 component_count;
    ComponentType* components;
} SystemMask;

struct Registry {
    u32 entity_count;
    Map component_masks;

    u32 component_count;
    Map component_pools;

    u32 startup_system_count;
    Map startup_system_masks;

    u32 update_system_count;
    Map update_system_masks;

    u32 fixed_system_count;
    Map fixed_system_masks;

    bool running;
    double fixed_delta;
};

void registry_init(Registry* registry);

void registry_clean(Registry* registry);

ComponentType registry_register_component(Registry *registry, u32 size);

Entity registry_register_entity(Registry *registry);

void registry_register_system(
        Registry* registry,
        ProcessMode process_mode,
        System system,
        int component_count, 
        ...
    );

void registry_add_component(
        Registry* registry,
        Entity entity,
        ComponentType component,
        void* data
    );

void* registry_get_component(
        Registry* registry,
        Entity entity,
        ComponentType component
    );

bool registry_has_components(
        Registry* registry,
        Entity entity,
        u32 num_components,
        ComponentType* components
    );

void registry_startup_systems(Registry* registry);

void registry_update_systems(Registry* registry, double delta);

void registry_fixed_systems(Registry* registry, double percentage);

void registry_start(Registry* registry);

void registry_stop(Registry* registry);

double current_time_secs(void);

#endif
