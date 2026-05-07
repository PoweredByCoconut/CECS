#include "cecs.h"
#include "map.h"
#include <bits/time.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

App* app_init(App* app) {
    if (!app) {
        app = calloc(sizeof(App), 1);
        if (!app) {
            fprintf(stderr, "failed to allocate app\n");
            return NULL;
        }
    }

    app->entity_count = 0;
    app->running = false;
    app->fixed_delta = 1.0;

    list_init(&app->entity_components, sizeof(Map), 4);
    list_init(&app->startup_systems, sizeof(System), 4);
    list_init(&app->update_systems, sizeof(System), 4);
    list_init(&app->fixed_systems, sizeof(System), 4);

    return app;
}

void app_cleanup(App* app) {
    // clean up maps
    for (int i = 0; i < app->entity_components.length; i++) {
        map_clean(list_get(&app->entity_components, i));
    }

    // clean up systems
    for (int i = 0; i < app->startup_systems.length; i++) {
        free(((System*)list_get(&app->startup_systems, i))->mask);
    }
    for (int i = 0; i < app->update_systems.length; i++) {
        free(((System*)list_get(&app->update_systems, i))->mask);
    }
    for (int i = 0; i < app->fixed_systems.length; i++) {
        free(((System*)list_get(&app->fixed_systems, i))->mask);
    }

    // clean up lists
    list_clean(&app->entity_components);
    list_clean(&app->startup_systems);
    list_clean(&app->update_systems);
    list_clean(&app->fixed_systems);
}

Entity app_add_entity(App* app) {
    return app->entity_count++;
}

ComponentID app_register_component(App* app, u32 component_size) {
    ComponentID id = app->entity_components.length;

    Map* component_map = list_add(&app->entity_components, NULL);
    map_init(component_map, component_size, 4);

    return id;
}

void app_set_entity_component(App* app, Entity entity, ComponentID component_id, void* data) {
    Map* component_map = list_get(&app->entity_components, component_id);
    map_set(component_map, entity, data);
}

void* app_get_entity_component(App* app, Entity entity, ComponentID component_id) {
    return map_get(list_get(&app->entity_components, component_id), entity);
}

bool app_entity_has_components(App* app, Entity entity, u32 component_count, ComponentID* components) {
    for (int i = 0; i < component_count; i++) {
        Map* component_map = list_get(&app->entity_components, i);
        if (!map_get(component_map, entity)) {
            return false;
        }
    }

    return true;
}

void app_register_system(App* app, SystemType type, SystemFunction function, u32 component_count, ...) {
    System system = {0};
    system.mask = calloc(component_count, sizeof(ComponentID));
    if (!system.mask) {
        fprintf(stderr, "Could not allocate memory for system mask\n");
        return;
    }

    va_list args;
    va_start(args, component_count);

    for (int i = 0; i < component_count; i++) {
        system.mask[i] = va_arg(args, ComponentID);
    }

    va_end(args);

    system.mask_length = component_count;
    system.function = function;

    List *systems;

    switch (type) {
        case STARTUP:
            systems = &app->startup_systems;
            break;
        case UPDATE:
            systems = &app->update_systems;
            break;
        case FIXED:
            systems = &app->fixed_systems;
            break;
    }

    list_add(systems, &system);
}

void app_execute_systems(App* app, SystemType type, double d) {
    List* systems;

    switch (type) {
        case STARTUP:
            systems = &app->startup_systems;
            break;
        case UPDATE:
            systems = &app->update_systems;
            break;
        case FIXED:
            systems = &app->fixed_systems;
            break;
    }

    for (int i = 0; i < systems->length; i++) {
        System* system = list_get(systems, i);

        for (Entity e = 0; e < app->entity_count; e++) {
            if (app_entity_has_components(app, e, system->mask_length, system->mask)) {
                system->function(app, e, d);
            }
        }
    }
}

void app_set_fixed_delta(App* app, double fixed_delta) {
    app->fixed_delta = fixed_delta;
}

double current_time_secs(void) {
    struct timeval now;
    gettimeofday(&now, NULL);

    return now.tv_sec + now.tv_usec / 1000000.0;
}

void app_start(App* app) {
    app->running = true;

    app_execute_systems(app, STARTUP, 0.0);

    double last_time = current_time_secs();
    double accumulator = 0.0;

    while (app->running) {
        double current_time = current_time_secs();
        double frame_time = current_time - last_time;
        last_time = current_time;

        accumulator += frame_time;

        while (accumulator >= app->fixed_delta) {
            app_execute_systems(app, FIXED, app->fixed_delta);
            accumulator -= app->fixed_delta;
        }

        app_execute_systems(app, UPDATE, accumulator / app->fixed_delta); 
    }

    app_cleanup(app);
}

void app_stop(App* app) {
    app->running = false;
}

char get_char_clear(void) {
    int character = getchar();

    int temp;
    if (character != '\n' && character != EOF) {
        while ((temp = getchar()) != '\n' && temp != EOF);
    }

    return character;
}
