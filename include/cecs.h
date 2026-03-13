#ifndef CECS_H
#define CECS_H

#include "types.h"
#include "list.h"

typedef struct App App;

typedef u32 Entity;
typedef u32 ComponentID;
typedef void(*SystemFunction)(App* app, Entity entity, double d);

typedef struct {
    ComponentID* mask;
    u32 mask_length;
    SystemFunction function;
} System;

typedef enum {
    STARTUP,
    UPDATE,
    FIXED,
} SystemType;

struct App {
    u32 entity_count;
    // List<Map<Component>>
    List entity_components;
    u32 component_count;
    // List<System>
    List startup_systems;
    // List<System>
    List update_systems;
    // List<System>
    List fixed_systems;

    bool running;
    double fixed_delta;
};

App* app_init(App* app);

void app_cleanup(App* app);

Entity app_add_entity(App* app);

ComponentID app_register_component(App* app, u32 component_size);

void app_set_entity_component(App* app, Entity entity, ComponentID component_id, void* data);

void* app_get_entity_component(App* app, Entity entity, ComponentID component_id);

bool app_entity_has_components(App* app, Entity entity, u32 component_count, ComponentID* components);

void app_register_system(App* app, SystemType type, SystemFunction function, u32 component_count, ...);

void app_execute_systems(App* app, SystemType type, double d);

void app_set_fixed_delta(App* app, double fixed_delta);

void app_start(App* app);

void app_stop(App* app);

#endif
