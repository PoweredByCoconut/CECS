#include <stdio.h>
#include <stdlib.h>
#include "cecs.h"

ComponentType POSITION_TYPE;
typedef struct {
    float x, y;
} Position;

ComponentType VELOCITY_TYPE;
typedef Position Velocity;

ComponentType COUNTER_TYPE;
typedef double Counter;

#define MOVEMENT_COMPONENTS \
    2, POSITION_TYPE, VELOCITY_TYPE
void movement_system(Registry* registry, Entity entity, double delta) {
    Position* position = registry_get_component(registry, entity, POSITION_TYPE);
    Velocity* velocity = registry_get_component(registry, entity, VELOCITY_TYPE);

    position->x += velocity->x * delta;
    position->y += velocity->y * delta;
}

#define RENDER_COMPONENTS \
    2, POSITION_TYPE, VELOCITY_TYPE
void render_system(Registry* registry, Entity entity, double progress) {
    Position* real_position = registry_get_component(registry, entity, POSITION_TYPE);
    Velocity* velocity = registry_get_component(registry, entity, VELOCITY_TYPE);

    Position interpolate = {0};
    interpolate.x = real_position->x + progress * velocity->x;
    interpolate.y = real_position->y + progress * velocity->y;

    printf("%f, %f\n", interpolate.x, interpolate.y);
}

#define COUNT_COMPONENTS \
    1, COUNTER_TYPE
void count_system(Registry* registry, Entity entity, double delta) {
    Counter* counter = registry_get_component(registry, entity, COUNTER_TYPE);
    (*counter) += delta;
    if (*counter >= 10) {
        registry_stop(registry);
    }
}

void spawn_object(Registry* registry, Position position, Velocity velocity) {
    Entity object = registry_register_entity(registry);

    registry_add_component(registry, object, POSITION_TYPE, &position);
    registry_add_component(registry, object, VELOCITY_TYPE, &velocity);
}

int main(void) {
    Registry* registry = registry_init(NULL);

    POSITION_TYPE = registry_register_component(registry, sizeof(Position));
    VELOCITY_TYPE = registry_register_component(registry, sizeof(Velocity));
    COUNTER_TYPE = registry_register_component(registry, sizeof(Counter));

    registry_register_system(registry, fixed_update, movement_system, MOVEMENT_COMPONENTS);
    registry_register_system(registry, update, render_system, RENDER_COMPONENTS);
    registry_register_system(registry, fixed_update, count_system, COUNT_COMPONENTS);

    spawn_object(registry, (Position){0.0f, 0.0f}, (Velocity){1.0f, 1.0f});

    Entity counter = registry_register_entity(registry);
    Counter counter_data = 0;
    registry_add_component(registry, counter, COUNTER_TYPE, &counter_data);

    registry_start(registry);
    free(registry);
}
