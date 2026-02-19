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

#define MOVEMENT_COMPONENTS \
    NAME_TYPE, POSITION_TYPE, VELOCITY_TYPE
void movement(Registry* registry, Entity entity) {
    Name* name = registry_get_component(registry, entity, NAME_TYPE);
    Position* position = registry_get_component(registry, entity, POSITION_TYPE);
    Velocity* velocity = registry_get_component(registry, entity, VELOCITY_TYPE);

    position->x += velocity->x;
    position->y += velocity->y;

    printf("%s is now at %f, %f\n", *name, position->x, position->y);
}

int main(void) {
    Registry registry = {0};
    registry_init(&registry);

    Name name = "Fred";
    Position pos = {1.0f, 3.0f};
    Velocity vel = {10.f, 3.0f};

    NAME_TYPE = registry_register_component(&registry, sizeof(Name));
    POSITION_TYPE = registry_register_component(&registry, sizeof(Position));
    VELOCITY_TYPE = registry_register_component(&registry, sizeof(Velocity));

    Entity person = registry_register_entity(&registry);

    registry_add_component(&registry, person, NAME_TYPE, &name);
    registry_add_component(&registry, person, POSITION_TYPE, &pos);
    registry_add_component(&registry, person, VELOCITY_TYPE, &vel);

    registry_register_system(&registry, movement, 3, MOVEMENT_COMPONENTS);

    registry_execute_systems(&registry);

    registry_clean(&registry);
}
