#include "cecs.h"
#include <ncurses.h>

ComponentID POSITION_TYPE;
typedef struct {
    float x, y;
} Position;

ComponentID VELOCITY_TYPE;
typedef Position Velocity;

ComponentID CHARACTER_TYPE;
typedef char RenderChar;

#define MOVEMENT_COMPONENTS \
    2, POSITION_TYPE, VELOCITY_TYPE
void movement_system(App* app, Entity entity, double delta) {
    Position* position = app_get_entity_component(app, entity, POSITION_TYPE);
    Velocity* velocity = app_get_entity_component(app, entity, VELOCITY_TYPE);

    position->x += velocity->x * delta;
    position->y += velocity->y * delta;

    velocity->x = 0.0f;
    velocity->y = 0.0f;
}

#define RENDER_COMPONENTS \
    2, POSITION_TYPE, CHARACTER_TYPE
void render_system(App* app, Entity entity, double delta) {
    Position* position = app_get_entity_component(app, entity, POSITION_TYPE);
    RenderChar character = *(RenderChar*)app_get_entity_component(app, entity, CHARACTER_TYPE);

    clear();
    mvprintw((int)position->y, (int)position->x, "%c", character);
    refresh();
}

#define INPUT_SETUP_COMPONENTS \
    0
void input_setup_system(App* app, Entity entity, double _) {
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
}

#define INPUT_COMPONENTS \
    2, VELOCITY_TYPE, CHARACTER_TYPE
void input_system(App* app, Entity entity, double percentage) {
    Velocity* velocity = app_get_entity_component(app, entity, VELOCITY_TYPE);

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
    App* app = app_init(NULL);
    
    app_set_fixed_delta(app, 0.5);

    POSITION_TYPE = app_register_component(app, sizeof(Position));
    VELOCITY_TYPE = app_register_component(app, sizeof(Velocity));
    CHARACTER_TYPE = app_register_component(app, sizeof(RenderChar));

    app_register_system(app, STARTUP, input_setup_system, INPUT_COMPONENTS);
    app_register_system(app, UPDATE, input_system, INPUT_COMPONENTS);
    app_register_system(app, FIXED, render_system, RENDER_COMPONENTS);
    app_register_system(app, FIXED, movement_system, MOVEMENT_COMPONENTS);

    Position position = {0.0f, 0.0f};
    Velocity velocity = {0.0f, 0.0f};
    RenderChar character = '*';

    Entity player = app_add_entity(app);
    app_set_entity_component(app, player, POSITION_TYPE, &position);
    app_set_entity_component(app, player, VELOCITY_TYPE, &velocity);
    app_set_entity_component(app, player, CHARACTER_TYPE, &character);

    app_start(app);

    endwin();
}
