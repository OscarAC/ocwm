/*
 * OCWM - Orbital Compositor & Window Manager
 * A minimal, performant Wayland compositor with Lua scripting
 */

#include "ocwm.h"
#include <unistd.h>
#include <signal.h>

static struct ocwm_server server = {0};

static void handle_signal(int sig) {
    (void)sig;
    wl_display_terminate(server.wl_display);
}

void ocwm_server_init(struct ocwm_server *server) {
    wlr_log_init(WLR_DEBUG, NULL);

    /* Create the Wayland display */
    server->wl_display = wl_display_create();
    if (!server->wl_display) {
        wlr_log(WLR_ERROR, "Failed to create Wayland display");
        exit(1);
    }

    /* Initialize the backend (DRM, X11, etc.) */
    server->backend = wlr_backend_autocreate(server->wl_display, NULL);
    if (!server->backend) {
        wlr_log(WLR_ERROR, "Failed to create wlr_backend");
        exit(1);
    }

    /* Create renderer */
    server->renderer = wlr_renderer_autocreate(server->backend);
    if (!server->renderer) {
        wlr_log(WLR_ERROR, "Failed to create wlr_renderer");
        exit(1);
    }

    wlr_renderer_init_wl_display(server->renderer, server->wl_display);

    /* Create allocator */
    server->allocator = wlr_allocator_autocreate(server->backend, server->renderer);
    if (!server->allocator) {
        wlr_log(WLR_ERROR, "Failed to create wlr_allocator");
        exit(1);
    }

    /* Create compositor (handles surfaces) */
    server->compositor = wlr_compositor_create(server->wl_display, 5, server->renderer);
    wlr_subcompositor_create(server->wl_display);

    /* Create data device manager (clipboard) */
    server->data_device_mgr = wlr_data_device_manager_create(server->wl_display);

    /* Create output layout */
    server->output_layout = wlr_output_layout_create();

    /* Initialize lists */
    wl_list_init(&server->outputs);
    wl_list_init(&server->views);
    wl_list_init(&server->keyboards);

    /* Setup output management */
    server->new_output.notify = server_new_output;
    wl_signal_add(&server->backend->events.new_output, &server->new_output);

    /* Create scene graph */
    server->scene = wlr_scene_create();
    server->scene_layout = wlr_scene_attach_output_layout(server->scene, server->output_layout);

    /* Setup XDG shell */
    server->xdg_shell = wlr_xdg_shell_create(server->wl_display, 3);
    server->new_xdg_surface.notify = server_new_xdg_surface;
    wl_signal_add(&server->xdg_shell->events.new_surface, &server->new_xdg_surface);

    /* Create cursor */
    server->cursor = wlr_cursor_create();
    wlr_cursor_attach_output_layout(server->cursor, server->output_layout);

    /* Create cursor manager */
    server->cursor_mgr = wlr_xcursor_manager_create(NULL, 24);

    /* Setup cursor events */
    server->cursor_motion.notify = server_cursor_motion;
    wl_signal_add(&server->cursor->events.motion, &server->cursor_motion);

    server->cursor_motion_absolute.notify = server_cursor_motion_absolute;
    wl_signal_add(&server->cursor->events.motion_absolute, &server->cursor_motion_absolute);

    server->cursor_button.notify = server_cursor_button;
    wl_signal_add(&server->cursor->events.button, &server->cursor_button);

    server->cursor_axis.notify = server_cursor_axis;
    wl_signal_add(&server->cursor->events.axis, &server->cursor_axis);

    server->cursor_frame.notify = server_cursor_frame;
    wl_signal_add(&server->cursor->events.frame, &server->cursor_frame);

    /* Create seat (input management) */
    server->seat = wlr_seat_create(server->wl_display, "seat0");
    server->request_cursor.notify = seat_request_cursor;
    wl_signal_add(&server->seat->events.request_set_cursor, &server->request_cursor);

    server->request_set_selection.notify = seat_request_set_selection;
    wl_signal_add(&server->seat->events.request_set_selection, &server->request_set_selection);

    /* Setup input device handling */
    server->new_input.notify = server_new_input;
    wl_signal_add(&server->backend->events.new_input, &server->new_input);

    /* Initialize cursor mode */
    server->cursor_mode = OCWM_CURSOR_PASSTHROUGH;

    /* Initialize workspaces */
    workspace_init(server);

    /* Initialize effects system */
    effects_init(server);

    /* Initialize Lua */
    lua_init(server);

    wlr_log(WLR_INFO, "OCWM v%s initialized", OCWM_VERSION);
}

bool ocwm_server_start(struct ocwm_server *server) {
    /* Get a Wayland socket */
    const char *socket = wl_display_add_socket_auto(server->wl_display);
    if (!socket) {
        wlr_log(WLR_ERROR, "Failed to open Wayland socket");
        wlr_backend_destroy(server->backend);
        return false;
    }

    /* Start the backend */
    if (!wlr_backend_start(server->backend)) {
        wlr_log(WLR_ERROR, "Failed to start backend");
        wlr_backend_destroy(server->backend);
        wl_display_destroy(server->wl_display);
        return false;
    }

    /* Set the WAYLAND_DISPLAY environment variable */
    setenv("WAYLAND_DISPLAY", socket, true);

    wlr_log(WLR_INFO, "OCWM running on WAYLAND_DISPLAY=%s", socket);

    return true;
}

void ocwm_server_run(struct ocwm_server *server) {
    wl_display_run(server->wl_display);
}

void ocwm_server_finish(struct ocwm_server *server) {
    /* Cleanup Lua */
    lua_finish(server);

    /* Cleanup effects */
    animation_finish(server);

    /* Cleanup workspaces */
    workspace_finish(server);

    wl_display_destroy_clients(server->wl_display);
    wlr_scene_node_destroy(&server->scene->tree.node);
    wlr_xcursor_manager_destroy(server->cursor_mgr);
    wlr_cursor_destroy(server->cursor);
    wlr_allocator_destroy(server->allocator);
    wlr_renderer_destroy(server->renderer);
    wlr_backend_destroy(server->backend);
    wl_display_destroy(server->wl_display);

    wlr_log(WLR_INFO, "OCWM shutdown complete");
}

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    /* Handle signals */
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    printf("╔═══════════════════════════════════════════════╗\n");
    printf("║  OCWM - Orbital Compositor & Window Manager  ║\n");
    printf("║  Version %s - Starting up...              ║\n", OCWM_VERSION);
    printf("╚═══════════════════════════════════════════════╝\n\n");

    /* Initialize the server */
    ocwm_server_init(&server);

    /* Start the server */
    if (!ocwm_server_start(&server)) {
        return 1;
    }

    printf("✓ Compositor initialized successfully\n");
    printf("✓ Wayland display ready\n");

    /* Load Lua configuration */
    const char *config_paths[] = {
        getenv("OCWM_CONFIG"),
        NULL, /* Will be filled with $HOME/.config/ocwm/init.lua */
        "/etc/ocwm/init.lua",
        "config/init.lua",
        NULL
    };

    /* Build home config path */
    char home_config[512] = {0};
    const char *home = getenv("HOME");
    if (home) {
        snprintf(home_config, sizeof(home_config), "%s/.config/ocwm/init.lua", home);
        config_paths[1] = home_config;
    }

    bool config_loaded = false;
    for (int i = 0; config_paths[i] != NULL; i++) {
        if (config_paths[i][0] != '\0' && lua_load_config(&server, config_paths[i])) {
            printf("✓ Loaded config: %s\n", config_paths[i]);
            config_loaded = true;
            break;
        }
    }

    if (!config_loaded) {
        printf("⚠ No config file found, using defaults\n");
        printf("  Create ~/.config/ocwm/init.lua to customize\n");
    }

    printf("✓ Ready to accept clients\n\n");
    printf("Press Ctrl+C to exit\n\n");

    /* Run the Wayland event loop */
    ocwm_server_run(&server);

    /* Cleanup */
    printf("\nShutting down OCWM...\n");
    ocwm_server_finish(&server);

    return 0;
}
