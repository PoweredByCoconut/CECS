#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ENTITIES 100
#define MAX_COMPONENTS 32

typedef u32 Entity;
typedef u32 ComponentType;

typedef struct {
    u32 size;
    void* data;
} ComponentPool;

typedef struct {
    u32 entity_count;

    u32 component_masks[MAX_ENTITIES];

    u32 component_count;
    
    ComponentPool component_pools[MAX_COMPONENTS];
} Registry;

void registry_init(Registry* registry) {
    registry->entity_count = 0;
    registry->component_count = 0;
}

ComponentType registry_register_component(Registry *registry, u32 size) {
    ComponentType id = registry->component_count++;

    registry->component_pools[id].size = size;
    registry->component_pools[id].data = calloc(MAX_ENTITIES, size);

    return id;
}

Entity registry_register_entity(Registry *registry) {
    Entity id = registry->entity_count++;
    
    registry->component_masks[id] = 0;

    return id;
}

void registry_add_component(
        Registry* registry,
        Entity entity,
        ComponentType component,
        void* data
    ) {
    ComponentPool* pool = &registry->component_pools[component];

    void* dest = (u8*)pool->data + (entity * pool->size);
    memcpy(dest, data, pool->size);

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

    ComponentPool* pool = &registry->component_pools[component];
    return (u8*)pool->data + (entity * pool->size);
}

bool registry_has_components(
        Registry* registry,
        Entity entity,
        u32 mask
    ) {
    return (registry->component_masks[entity] & mask) == mask;
}

typedef struct {
    char name[8];
} Name;

typedef struct {
    float x, y;
} Position;

int main(void) {
    Registry registry;
    registry_init(&registry);

    Name name = {"Fred"};
    Position pos = {1.0f, 3.0f};

    ComponentType name_component = registry_register_component(&registry, sizeof(Name));
    ComponentType position_component = registry_register_component(&registry, sizeof(Position));

    Entity person = registry_register_entity(&registry);

    registry_add_component(&registry, person, name_component, &name);
    registry_add_component(&registry, person, position_component, &pos);

    u32 movement_mask = 1 << name_component | 1 << position_component;

    for (Entity entity = 0; entity < registry.entity_count; entity++) {
        if (registry_has_components(&registry, entity, movement_mask)) {
            Position* position = registry_get_component(&registry, entity, position_component);
            Name* named = registry_get_component(&registry, entity, name_component);
            position->x += 10;
            position->y += 13;

            printf("%s is at %f, %f\n", named->name, position->x, position->y);
        }
    }

    // cleanup allocs
}
