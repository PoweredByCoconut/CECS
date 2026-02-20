#include "cecs.h"
#include <ncurses.h>

ComponentType POSITION_TYPE;
typedef struct {
    float x, y;
} Position;

ComponentType VELOCITY_TYPE;
typedef Position Velocity;

ComponentType CHARACTER_TYPE;
typedef char RenderChar;

#define MOVEMENT_COMPONENTS \
    2, POSITION_TYPE, VELOCITY_TYPE
void movement_system(Registry* registry, Entity entity, double delta) {
    Position* position = registry_get_component(registry, entity, POSITION_TYPE);
    Velocity* velocity = registry_get_component(registry, entity, VELOCITY_TYPE);

    position->x += velocity->x * delta;
    position->y += velocity->y * delta;

    velocity->x = 0.0f;
    velocity->y = 0.0f;
}

#define RENDER_COMPONENTS \
    2, POSITION_TYPE, CHARACTER_TYPE
void render_system(Registry* registry, Entity entity, double delta) {
    Position* position = registry_get_component(registry, entity, POSITION_TYPE);
    RenderChar character = *(RenderChar*)registry_get_component(registry, entity, CHARACTER_TYPE);

    clear();
    mvprintw((int)position->y, (int)position->x, "%c", character);
    refresh();
}

#define INPUT_SETUP_COMPONENTS \
    0
void input_setup_system(Registry* registry, Entity entity, double _) {
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
}

#define INPUT_COMPONENTS \
    2, VELOCITY_TYPE, CHARACTER_TYPE
void input_system(Registry* registry, Entity entity, double percentage) {
    Velocity* velocity = registry_get_component(registry, entity, VELOCITY_TYPE);

    switch (getch()) {
        case KEY_UP:
            velocity->y = -5.0f;
            break;
        case KEY_DOWN:
            velocity->y = 5.0f;
            break;
        case KEY_LEFT:
            velocity->x = -5.0f;
            break;
        case KEY_RIGHT:
            velocity->x = 5.0f;
            break;
    }
}

int main(void) {
    Registry* registry = registry_init(NULL);
    
    registry_set_fixed_delta(registry, 0.5);

    POSITION_TYPE = registry_register_component(registry, sizeof(Position));
    VELOCITY_TYPE = registry_register_component(registry, sizeof(Velocity));
    CHARACTER_TYPE = registry_register_component(registry, sizeof(RenderChar));

    registry_register_system(registry, startup, input_setup_system, INPUT_COMPONENTS);
    registry_register_system(registry, update, input_system, INPUT_COMPONENTS);
    registry_register_system(registry, fixed_update, render_system, RENDER_COMPONENTS);
    registry_register_system(registry, fixed_update, movement_system, MOVEMENT_COMPONENTS);

    Position position = {0.0f, 0.0f};
    Velocity velocity = {0.0f, 0.0f};
    RenderChar character = '*';

    Entity player = registry_register_entity(registry);
    registry_add_component(registry, player, POSITION_TYPE, &position);
    registry_add_component(registry, player, VELOCITY_TYPE, &velocity);
    registry_add_component(registry, player, CHARACTER_TYPE, &character);

    registry_start(registry);

    endwin();
}
