#ifndef CECS_H
#define CECS_H

#include "types.h"
#include "map.h"

#define MAX_ENTITIES 100
#define MAX_COMPONENTS 32
#define MAX_SYSTEMS 100

typedef struct Registry Registry;
typedef u32 Entity;
typedef u32 ComponentType;
typedef void(*System)(Registry* registry, Entity entity);

typedef struct {
    u32 size;
    Map data;
} ComponentPool;

typedef struct {
    System system;
    u32 mask;
} SystemMask;

struct Registry {
    u32 entity_count;
    u32 component_masks[MAX_ENTITIES]; // TODO: make this use a component_mask struct to increase max component count

    u32 component_count;
    Map component_pools;

    u32 system_count;
    //SystemMask system_masks[MAX_SYSTEMS]; // TODO: hashmap here too
    Map system_masks;
};

void registry_init(Registry* registry);

void registry_clean(Registry* registry);

ComponentType registry_register_component(Registry *registry, u32 size);

Entity registry_register_entity(Registry *registry);

void registry_register_system(
        Registry* registry,
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
        u32 mask
    );

void registry_execute_systems(Registry* registry);

#endif
