#include "window.h"
#include <stdio.h>
#include <stdlib.h>
#include <wayland-client.h>
#include "types.h"

static void wl_output_handle_resource_destroy(struct wl_resource* resource) {
    struct my_output *client_output = wl_resource_get_user_data(resource);

    // TODO: Clean up resource

    remove_to_list(client_output->state->client_outputs, client_output);
}

static void registry_handle_global(void* data, struct wl_registry *registry, u32 name, const char* interface, u32 version) {
    printf("interface: '%s', version: %d, name: %d\n", interface, version, name);
}

static void registry_handle_global_remove(void* data, struct wl_registry *registry, u32 name) {

}

static void wl_output_handle_bind(struct wl_client* client, void* data, u32 version, u32 id) {
    struct my_state* state = data;
}

static const struct wl_registry_listener registry_listener = {
    .global = registry_handle_global,
    .global_remove = registry_handle_global_remove,
};

void window(void) {
    struct wl_display* display = wl_display_connect(NULL);
    if (!display) {
        fprintf(stderr, "Failed to connect to wayland display\n");
        exit(1);
    }
    printf("Connection established\n");

    struct wl_registry* registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, NULL);
    wl_display_roundtrip(display);

    struct my_state state = {};

    wl_global_create(display, &wl_output_interface, 1, &state, wl_output_handle_bind);

    while (wl_display_dispatch(display) != -1) {}

    wl_display_disconnect(display);
}
