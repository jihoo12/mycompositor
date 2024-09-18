#include <wayland-server.h>
#include <wlr/backend.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_output.h>
#include <wlr/util/log.h>
#include <stdio.h>

struct server {
    struct wl_display *display;
    struct wlr_backend *backend;
    struct wlr_renderer *renderer;
    struct wlr_allocator *allocator;

    struct wl_listener new_output;
};

static void server_new_output(struct wl_listener *listener, void *data) {
    struct server *server = wl_container_of(listener, server, new_output);
    struct wlr_output *output = data;

    wlr_output_init_render(output, server->allocator, server->renderer);

    if (!wl_list_empty(&output->modes)) {
        struct wlr_output_mode *mode = wlr_output_preferred_mode(output);
        wlr_output_set_mode(output, mode);
        wlr_output_enable(output, true);
        if (!wlr_output_commit(output)) {
            return;
        }
    }

    wlr_log(WLR_DEBUG, "New output %s", output->name);
}

int main(int argc, char *argv[]) {
    wlr_log_init(WLR_DEBUG, NULL);

    struct server server = {0};

    server.display = wl_display_create();
    server.backend = wlr_backend_autocreate(server.display);
    server.renderer = wlr_renderer_autocreate(server.backend);
    server.allocator = wlr_allocator_autocreate(server.backend, server.renderer);

    server.new_output.notify = server_new_output;
    wl_signal_add(&server.backend->events.new_output, &server.new_output);

    if (!wlr_backend_start(server.backend)) {
        fprintf(stderr, "Failed to start backend\n");
        wl_display_destroy(server.display);
        return 1;
    }

    const char *socket = wl_display_add_socket_auto(server.display);
    if (!socket) {
        fprintf(stderr, "Failed to add Wayland socket\n");
        return 1;
    }

    printf("Running Wayland display on %s\n", socket);
    wl_display_run(server.display);

    wl_display_destroy(server.display);
    return 0;
}
