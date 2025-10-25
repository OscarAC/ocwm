/*
 * Input handling (keyboard, mouse, etc.)
 */

#include "../ocwm.h"
#include <linux/input-event-codes.h>

/* Keyboard handling */
static void keyboard_handle_modifiers(struct wl_listener *listener, void *data) {
    struct ocwm_keyboard *keyboard = wl_container_of(listener, keyboard, modifiers);
    wlr_seat_set_keyboard(keyboard->server->seat, keyboard->wlr_keyboard);
    wlr_seat_keyboard_notify_modifiers(keyboard->server->seat,
        &keyboard->wlr_keyboard->modifiers);
}

/* Moved to header - now called from keyboard_handle_key */

static void keyboard_handle_key(struct wl_listener *listener, void *data) {
    struct ocwm_keyboard *keyboard = wl_container_of(listener, keyboard, key);
    struct ocwm_server *server = keyboard->server;
    struct wlr_keyboard_key_event *event = data;
    struct wlr_seat *seat = server->seat;

    /* Translate libinput keycode to xkbcommon */
    uint32_t keycode = event->keycode + 8;

    /* Get keysyms from the keymap */
    const xkb_keysym_t *syms;
    int nsyms = xkb_state_key_get_syms(
        keyboard->wlr_keyboard->xkb_state, keycode, &syms);

    bool handled = false;
    uint32_t modifiers = wlr_keyboard_get_modifiers(keyboard->wlr_keyboard);

    /* Handle keybindings on key press */
    if (event->state == WL_KEYBOARD_KEY_STATE_PRESSED) {
        for (int i = 0; i < nsyms; i++) {
            handled = handle_keybinding(server, modifiers, syms[i]);
            if (handled) break;
        }
    }

    if (!handled) {
        /* Pass through to the client */
        wlr_seat_set_keyboard(seat, keyboard->wlr_keyboard);
        wlr_seat_keyboard_notify_key(seat, event->time_msec,
            event->keycode, event->state);
    }
}

static void keyboard_handle_destroy(struct wl_listener *listener, void *data) {
    struct ocwm_keyboard *keyboard = wl_container_of(listener, keyboard, destroy);

    wl_list_remove(&keyboard->modifiers.link);
    wl_list_remove(&keyboard->key.link);
    wl_list_remove(&keyboard->destroy.link);
    wl_list_remove(&keyboard->link);
    free(keyboard);
}

static void server_new_keyboard(struct ocwm_server *server,
        struct wlr_input_device *device) {
    struct wlr_keyboard *wlr_keyboard = wlr_keyboard_from_input_device(device);

    struct ocwm_keyboard *keyboard = calloc(1, sizeof(struct ocwm_keyboard));
    keyboard->server = server;
    keyboard->wlr_keyboard = wlr_keyboard;

    /* Set the keymap */
    struct xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    struct xkb_keymap *keymap = xkb_keymap_new_from_names(context, NULL,
        XKB_KEYMAP_COMPILE_NO_FLAGS);

    wlr_keyboard_set_keymap(wlr_keyboard, keymap);
    xkb_keymap_unref(keymap);
    xkb_context_unref(context);

    wlr_keyboard_set_repeat_info(wlr_keyboard, 25, 600);

    /* Setup listeners */
    keyboard->modifiers.notify = keyboard_handle_modifiers;
    wl_signal_add(&wlr_keyboard->events.modifiers, &keyboard->modifiers);

    keyboard->key.notify = keyboard_handle_key;
    wl_signal_add(&wlr_keyboard->events.key, &keyboard->key);

    keyboard->destroy.notify = keyboard_handle_destroy;
    wl_signal_add(&device->events.destroy, &keyboard->destroy);

    wlr_seat_set_keyboard(server->seat, keyboard->wlr_keyboard);

    /* Add to list */
    wl_list_insert(&server->keyboards, &keyboard->link);

    wlr_log(WLR_INFO, "New keyboard: %s", device->name);
}

static void server_new_pointer(struct ocwm_server *server,
        struct wlr_input_device *device) {
    /* Attach pointer to cursor */
    wlr_cursor_attach_input_device(server->cursor, device);

    wlr_log(WLR_INFO, "New pointer: %s", device->name);
}

void server_new_input(struct wl_listener *listener, void *data) {
    struct ocwm_server *server = wl_container_of(listener, server, new_input);
    struct wlr_input_device *device = data;

    switch (device->type) {
    case WLR_INPUT_DEVICE_KEYBOARD:
        server_new_keyboard(server, device);
        break;
    case WLR_INPUT_DEVICE_POINTER:
        server_new_pointer(server, device);
        break;
    default:
        break;
    }

    /* Set capabilities on the seat */
    uint32_t caps = WL_SEAT_CAPABILITY_POINTER;
    if (!wl_list_empty(&server->keyboards)) {
        caps |= WL_SEAT_CAPABILITY_KEYBOARD;
    }
    wlr_seat_set_capabilities(server->seat, caps);
}

/* Cursor handling */
void process_cursor_motion(struct ocwm_server *server, uint32_t time) {
    /* If in move/resize mode, handle that */
    if (server->cursor_mode == OCWM_CURSOR_MOVE) {
        struct ocwm_view *view = server->grabbed_view;
        wlr_scene_node_set_position(&view->scene_tree->node,
            server->cursor->x - server->grab_x,
            server->cursor->y - server->grab_y);
        return;
    } else if (server->cursor_mode == OCWM_CURSOR_RESIZE) {
        struct ocwm_view *view = server->grabbed_view;
        double border_x = server->cursor->x - server->grab_x;
        double border_y = server->cursor->y - server->grab_y;

        int new_left = server->grab_geobox_x;
        int new_right = server->grab_geobox_x + server->grab_width;
        int new_top = server->grab_geobox_y;
        int new_bottom = server->grab_geobox_y + server->grab_height;

        if (server->resize_edges & WLR_EDGE_TOP) {
            new_top = border_y;
            if (new_top >= new_bottom) {
                new_top = new_bottom - 1;
            }
        } else if (server->resize_edges & WLR_EDGE_BOTTOM) {
            new_bottom = border_y;
            if (new_bottom <= new_top) {
                new_bottom = new_top + 1;
            }
        }

        if (server->resize_edges & WLR_EDGE_LEFT) {
            new_left = border_x;
            if (new_left >= new_right) {
                new_left = new_right - 1;
            }
        } else if (server->resize_edges & WLR_EDGE_RIGHT) {
            new_right = border_x;
            if (new_right <= new_left) {
                new_right = new_left + 1;
            }
        }

        struct wlr_box geo_box;
        wlr_xdg_surface_get_geometry(view->xdg_toplevel->base, &geo_box);

        wlr_scene_node_set_position(&view->scene_tree->node,
            new_left - geo_box.x, new_top - geo_box.y);

        int new_width = new_right - new_left;
        int new_height = new_bottom - new_top;
        wlr_xdg_toplevel_set_size(view->xdg_toplevel, new_width, new_height);

        return;
    }

    /* Find the view under the cursor */
    double sx, sy;
    struct wlr_surface *surface = NULL;
    struct ocwm_view *view = desktop_view_at(server,
        server->cursor->x, server->cursor->y, &surface, &sx, &sy);

    if (!view) {
        /* No view under cursor, set default cursor */
        wlr_cursor_set_xcursor(server->cursor, server->cursor_mgr, "default");
    }

    if (surface) {
        /* Send pointer enter and motion events */
        wlr_seat_pointer_notify_enter(server->seat, surface, sx, sy);
        wlr_seat_pointer_notify_motion(server->seat, time, sx, sy);
    } else {
        /* Clear pointer focus */
        wlr_seat_pointer_clear_focus(server->seat);
    }
}

void server_cursor_motion(struct wl_listener *listener, void *data) {
    struct ocwm_server *server = wl_container_of(listener, server, cursor_motion);
    struct wlr_pointer_motion_event *event = data;

    wlr_cursor_move(server->cursor, &event->pointer->base, event->delta_x, event->delta_y);
    process_cursor_motion(server, event->time_msec);
}

void server_cursor_motion_absolute(struct wl_listener *listener, void *data) {
    struct ocwm_server *server = wl_container_of(listener, server, cursor_motion_absolute);
    struct wlr_pointer_motion_absolute_event *event = data;

    wlr_cursor_warp_absolute(server->cursor, &event->pointer->base, event->x, event->y);
    process_cursor_motion(server, event->time_msec);
}

void server_cursor_button(struct wl_listener *listener, void *data) {
    struct ocwm_server *server = wl_container_of(listener, server, cursor_button);
    struct wlr_pointer_button_event *event = data;

    /* Notify the client with pointer focus */
    wlr_seat_pointer_notify_button(server->seat,
        event->time_msec, event->button, event->state);

    double sx, sy;
    struct wlr_surface *surface = NULL;
    struct ocwm_view *view = desktop_view_at(server,
        server->cursor->x, server->cursor->y, &surface, &sx, &sy);

    if (event->state == WLR_BUTTON_RELEASED) {
        /* Exit move/resize mode */
        server->cursor_mode = OCWM_CURSOR_PASSTHROUGH;
    } else {
        /* Focus the view under cursor */
        if (view) {
            focus_view(view, surface);
        }
    }
}

void server_cursor_axis(struct wl_listener *listener, void *data) {
    struct ocwm_server *server = wl_container_of(listener, server, cursor_axis);
    struct wlr_pointer_axis_event *event = data;

    /* Notify the client with pointer focus */
    wlr_seat_pointer_notify_axis(server->seat,
        event->time_msec, event->orientation, event->delta,
        event->delta_discrete, event->source, event->relative_direction);
}

void server_cursor_frame(struct wl_listener *listener, void *data) {
    struct ocwm_server *server = wl_container_of(listener, server, cursor_frame);

    /* Notify the client with pointer focus */
    wlr_seat_pointer_notify_frame(server->seat);
}

void seat_request_cursor(struct wl_listener *listener, void *data) {
    struct ocwm_server *server = wl_container_of(listener, server, request_cursor);
    struct wlr_seat_pointer_request_set_cursor_event *event = data;
    struct wlr_seat_client *focused_client =
        server->seat->pointer_state.focused_client;

    /* Only honor request from focused client */
    if (focused_client == event->seat_client) {
        wlr_cursor_set_surface(server->cursor, event->surface,
            event->hotspot_x, event->hotspot_y);
    }
}

void seat_request_set_selection(struct wl_listener *listener, void *data) {
    struct ocwm_server *server = wl_container_of(listener, server, request_set_selection);
    struct wlr_seat_request_set_selection_event *event = data;
    wlr_seat_set_selection(server->seat, event->source, event->serial);
}

bool handle_keybinding(struct ocwm_server *server, uint32_t modifiers, xkb_keysym_t sym) {
    /* Check Lua keybindings */
    if (server->lua) {
        struct ocwm_keybinding *binding;
        wl_list_for_each(binding, &server->keybindings, link) {
            if (binding->modifiers == modifiers && binding->keysym == sym) {
                /* Execute Lua callback */
                lua_rawgeti(server->lua, LUA_REGISTRYINDEX, binding->lua_callback_ref);
                if (lua_pcall(server->lua, 0, 0, 0) != LUA_OK) {
                    const char *error = lua_tostring(server->lua, -1);
                    wlr_log(WLR_ERROR, "Keybinding callback error: %s", error);
                    lua_pop(server->lua, 1);
                }
                return true;
            }
        }
    }

    /* Built-in fallback keybindings (if no Lua config) */
    if (modifiers == WLR_MODIFIER_ALT && sym == XKB_KEY_Escape) {
        wl_display_terminate(server->wl_display);
        return true;
    }

    return false;
}
