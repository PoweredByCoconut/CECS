#include <stdio.h>
#include "cecs.h"

ComponentType NAME_TYPE;
typedef char Name[8];

ComponentType POSITION_TYPE;
typedef struct {
    float x, y;
} Position;

ComponentType VELOCITY_TYPE;
typedef Position Velocity;

ComponentType COUNTER_TYPE;
typedef u32 Counter;

#define MOVEMENT_COMPONENTS \
    2, POSITION_TYPE, VELOCITY_TYPE
void movement(Registry* registry, Entity entity, double delta) {
    Position* position = registry_get_component(registry, entity, POSITION_TYPE);
    Velocity* velocity = registry_get_component(registry, entity, VELOCITY_TYPE);

    position->x += velocity->x * delta;
    position->y += velocity->y * delta;
}

#define COUNT_COMPONENTS \
    2, COUNTER_TYPE, POSITION_TYPE
void count_system(Registry* registry, Entity entity, double delta) {
    Counter* counter = registry_get_component(registry, entity, COUNTER_TYPE);
    Position* position = registry_get_component(registry, entity, POSITION_TYPE);

    (*counter)++;

    printf("%d\n", *counter);
    printf("%f, %f\n", position->x, position->y);

    if (*counter >= 10) {
        registry_stop(registry);
    }
}

#define WELCOME_COMPONENTS \
    1, NAME_TYPE
void welcome(Registry* registry, Entity entity, double delta) {
    Name* name = registry_get_component(registry, entity, NAME_TYPE);

    printf("my name is %s\n", *name);
}

int main(void) {
    Registry registry = {0};
    registry_init(&registry);

    Name name = "Fred";
    Position pos = {0.0f, 0.0f};
    Velocity vel = {1.f, 1.0f};
    Counter count = 0;

    NAME_TYPE = registry_register_component(&registry, sizeof(Name));
    POSITION_TYPE = registry_register_component(&registry, sizeof(Position));
    VELOCITY_TYPE = registry_register_component(&registry, sizeof(Velocity));
    COUNTER_TYPE = registry_register_component(&registry, sizeof(Counter));

    Entity person = registry_register_entity(&registry);

    registry_add_component(&registry, person, NAME_TYPE, &name);
    registry_add_component(&registry, person, POSITION_TYPE, &pos);
    registry_add_component(&registry, person, VELOCITY_TYPE, &vel);
    registry_add_component(&registry, person, COUNTER_TYPE, &count);

    registry_register_system(&registry, fixed_update, movement, MOVEMENT_COMPONENTS);
    registry_register_system(&registry, fixed_update, count_system, COUNT_COMPONENTS);
    registry_register_system(&registry, startup, welcome, WELCOME_COMPONENTS);

    registry_start(&registry);
}
