/*
 * Output (monitor) management
 */

#include "../ocwm.h"

static void output_frame(struct wl_listener *listener, void *data) {
    struct ocwm_output *output = wl_container_of(listener, output, frame);
    struct wlr_scene *scene = output->server->scene;
    struct wlr_scene_output *scene_output = wlr_scene_get_scene_output(
        scene, output->wlr_output);

    /* Get current time for animations */
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    uint32_t time_msec = now.tv_sec * 1000 + now.tv_nsec / 1000000;

    /* Update all animations */
    animation_update(output->server, time_msec);

    /* Render the scene */
    wlr_scene_output_commit(scene_output, NULL);

    wlr_scene_output_send_frame_done(scene_output, &now);
}

static void output_destroy(struct wl_listener *listener, void *data) {
    struct ocwm_output *output = wl_container_of(listener, output, destroy);

    wl_list_remove(&output->frame.link);
    wl_list_remove(&output->destroy.link);
    wl_list_remove(&output->link);
    free(output);
}

void server_new_output(struct wl_listener *listener, void *data) {
    struct ocwm_server *server = wl_container_of(listener, server, new_output);
    struct wlr_output *wlr_output = data;

    /* Initialize the output with a preferred mode if available */
    wlr_output_init_render(wlr_output, server->allocator, server->renderer);

    struct wlr_output_state state;
    wlr_output_state_init(&state);
    wlr_output_state_set_enabled(&state, true);

    struct wlr_output_mode *mode = wlr_output_preferred_mode(wlr_output);
    if (mode != NULL) {
        wlr_output_state_set_mode(&state, mode);
    }

    wlr_output_commit_state(wlr_output, &state);
    wlr_output_state_finish(&state);

    /* Create our output structure */
    struct ocwm_output *output = calloc(1, sizeof(struct ocwm_output));
    output->server = server;
    output->wlr_output = wlr_output;

    /* Setup listeners */
    output->frame.notify = output_frame;
    wl_signal_add(&wlr_output->events.frame, &output->frame);

    output->destroy.notify = output_destroy;
    wl_signal_add(&wlr_output->events.destroy, &output->destroy);

    /* Add to our list */
    wl_list_insert(&server->outputs, &output->link);

    /* Add to output layout (auto-position) */
    struct wlr_output_layout_output *l_output =
        wlr_output_layout_add_auto(server->output_layout, wlr_output);

    struct wlr_scene_output *scene_output =
        wlr_scene_output_create(server->scene, wlr_output);
    wlr_scene_output_layout_add_output(server->scene_layout, l_output, scene_output);

    wlr_log(WLR_INFO, "New output: %s", wlr_output->name);
}
