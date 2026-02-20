#include "cecs.h"
#include "stdio.h"

ComponentID TIMER;
typedef double Timer;

#define POSITION_SYSTEM \
    1, TIMER
void position_system(App* app, Entity entity, double _) {
    Timer* timer = app_get_entity_component(app, entity, TIMER);

    (*timer)++;

    if (*timer >= 5) {
        app_stop(app);
    }

    printf("hello\n");
}

int main(void) {
    App app = {0};
    app_init(&app);

    Entity entity = app_add_entity(&app);

    TIMER = app_register_component(&app, sizeof(Timer));
    Timer timer = 0.0;

    app_set_entity_component(&app, entity, TIMER, &timer);
    
    app_register_system(&app, FIXED, position_system, POSITION_SYSTEM);

    app_start(&app);
}
