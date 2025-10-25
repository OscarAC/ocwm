/*
 * View (window) management
 */

#include "../ocwm.h"

/* Focus a view and bring it to the front */
void focus_view(struct ocwm_view *view, struct wlr_surface *surface) {
    if (view == NULL) {
        return;
    }

    struct ocwm_server *server = view->server;
    struct wlr_seat *seat = server->seat;
    struct wlr_surface *prev_surface = seat->keyboard_state.focused_surface;

    if (prev_surface == surface) {
        /* Already focused */
        return;
    }

    if (prev_surface != NULL) {
        struct wlr_xdg_toplevel *prev_toplevel =
            wlr_xdg_toplevel_try_from_wlr_surface(prev_surface);
        if (prev_toplevel != NULL) {
            wlr_xdg_toplevel_set_activated(prev_toplevel, false);
        }
    }

    /* Raise the view to the top */
    wlr_scene_node_raise_to_top(&view->scene_tree->node);

    /* Activate the new surface */
    wlr_xdg_toplevel_set_activated(view->xdg_toplevel, true);

    /* Set keyboard focus */
    if (surface != NULL) {
        struct wlr_keyboard *keyboard = wlr_seat_get_keyboard(seat);
        wlr_seat_keyboard_notify_enter(seat, surface,
            keyboard->keycodes, keyboard->num_keycodes, &keyboard->modifiers);
    }

    /* Fire Lua event */
    lua_fire_event(server, OCWM_EVENT_WINDOW_FOCUS, view);
}

/* Find view at coordinates */
struct ocwm_view *desktop_view_at(struct ocwm_server *server,
        double lx, double ly, struct wlr_surface **surface, double *sx, double *sy) {
    struct wlr_scene_node *node = wlr_scene_node_at(
        &server->scene->tree.node, lx, ly, sx, sy);

    if (node == NULL || node->type != WLR_SCENE_NODE_BUFFER) {
        return NULL;
    }

    struct wlr_scene_buffer *scene_buffer = wlr_scene_buffer_from_node(node);
    struct wlr_scene_surface *scene_surface =
        wlr_scene_surface_try_from_buffer(scene_buffer);

    if (scene_surface == NULL) {
        return NULL;
    }

    *surface = scene_surface->surface;

    /* Find the view */
    struct wlr_scene_tree *tree = node->parent;
    while (tree != NULL && tree->node.data == NULL) {
        tree = tree->node.parent;
    }

    return tree->node.data;
}

/* View event handlers */
static void xdg_toplevel_map(struct wl_listener *listener, void *data) {
    struct ocwm_view *view = wl_container_of(listener, view, map);

    view->mapped = true;

    /* Show only if in active workspace */
    if (view->workspace != view->server->active_workspace) {
        wlr_scene_node_set_enabled(&view->scene_tree->node, false);
    }

    focus_view(view, view->xdg_toplevel->base->surface);

    /* Apply layout to workspace */
    if (view->workspace) {
        layout_apply(view->workspace);
    }

    /* Animate window opening */
    view_animate_open(view);

    /* Fire Lua event */
    lua_fire_event(view->server, OCWM_EVENT_WINDOW_OPEN, view);

    wlr_log(WLR_INFO, "Window mapped: %s", view->xdg_toplevel->title ?: "(no title)");
}

static void xdg_toplevel_unmap(struct wl_listener *listener, void *data) {
    struct ocwm_view *view = wl_container_of(listener, view, unmap);

    view->mapped = false;

    /* Fire Lua event */
    lua_fire_event(view->server, OCWM_EVENT_WINDOW_CLOSE, view);

    /* Re-apply layout to workspace */
    if (view->workspace) {
        layout_apply(view->workspace);
    }

    wlr_log(WLR_INFO, "Window unmapped");
}

static void xdg_toplevel_destroy(struct wl_listener *listener, void *data) {
    struct ocwm_view *view = wl_container_of(listener, view, destroy);

    wl_list_remove(&view->map.link);
    wl_list_remove(&view->unmap.link);
    wl_list_remove(&view->destroy.link);
    wl_list_remove(&view->request_move.link);
    wl_list_remove(&view->request_resize.link);
    wl_list_remove(&view->request_maximize.link);
    wl_list_remove(&view->request_fullscreen.link);
    wl_list_remove(&view->link);

    free(view);

    wlr_log(WLR_INFO, "Window destroyed");
}

static void begin_interactive(struct ocwm_view *view,
        enum ocwm_cursor_mode mode, uint32_t edges) {
    struct ocwm_server *server = view->server;
    struct wlr_surface *focused_surface =
        server->seat->pointer_state.focused_surface;

    if (view->xdg_toplevel->base->surface !=
            wlr_surface_get_root_surface(focused_surface)) {
        return;
    }

    server->grabbed_view = view;
    server->cursor_mode = mode;

    if (mode == OCWM_CURSOR_MOVE) {
        server->grab_x = server->cursor->x - view->scene_tree->node.x;
        server->grab_y = server->cursor->y - view->scene_tree->node.y;
    } else {
        struct wlr_box geo_box;
        wlr_xdg_surface_get_geometry(view->xdg_toplevel->base, &geo_box);

        double border_x = (view->scene_tree->node.x + geo_box.x) +
            ((edges & WLR_EDGE_RIGHT) ? geo_box.width : 0);
        double border_y = (view->scene_tree->node.y + geo_box.y) +
            ((edges & WLR_EDGE_BOTTOM) ? geo_box.height : 0);

        server->grab_x = server->cursor->x - border_x;
        server->grab_y = server->cursor->y - border_y;

        server->grab_geobox_x = geo_box.x;
        server->grab_geobox_y = geo_box.y;
        server->grab_width = geo_box.width;
        server->grab_height = geo_box.height;

        server->resize_edges = edges;
    }
}

static void xdg_toplevel_request_move(struct wl_listener *listener, void *data) {
    struct ocwm_view *view = wl_container_of(listener, view, request_move);
    begin_interactive(view, OCWM_CURSOR_MOVE, 0);
}

static void xdg_toplevel_request_resize(struct wl_listener *listener, void *data) {
    struct wlr_xdg_toplevel_resize_event *event = data;
    struct ocwm_view *view = wl_container_of(listener, view, request_resize);
    begin_interactive(view, OCWM_CURSOR_RESIZE, event->edges);
}

static void xdg_toplevel_request_maximize(struct wl_listener *listener, void *data) {
    struct ocwm_view *view = wl_container_of(listener, view, request_maximize);
    wlr_xdg_surface_schedule_configure(view->xdg_toplevel->base);
}

static void xdg_toplevel_request_fullscreen(struct wl_listener *listener, void *data) {
    struct ocwm_view *view = wl_container_of(listener, view, request_fullscreen);
    wlr_xdg_toplevel_set_fullscreen(view->xdg_toplevel,
        view->xdg_toplevel->requested.fullscreen);
}

void server_new_xdg_surface(struct wl_listener *listener, void *data) {
    struct ocwm_server *server = wl_container_of(listener, server, new_xdg_surface);
    struct wlr_xdg_surface *xdg_surface = data;

    /* Only handle toplevels (actual windows) */
    if (xdg_surface->role != WLR_XDG_SURFACE_ROLE_TOPLEVEL) {
        return;
    }

    /* Create our view */
    struct ocwm_view *view = calloc(1, sizeof(struct ocwm_view));
    view->server = server;
    view->xdg_toplevel = xdg_surface->toplevel;
    view->mapped = false;

    /* Assign to active workspace */
    view->workspace = server->active_workspace;
    view->floating = false;  /* New windows are tiled by default */
    view->fullscreen = false;

    /* Initialize effects */
    view->opacity = 1.0f;
    view->blur = false;
    view->scale = 1.0f;
    view->animating = false;

    /* Create scene tree for this view */
    view->scene_tree = wlr_scene_xdg_surface_create(
        &server->scene->tree, xdg_surface);
    view->scene_tree->node.data = view;
    xdg_surface->data = view->scene_tree;

    /* Setup event listeners */
    view->map.notify = xdg_toplevel_map;
    wl_signal_add(&xdg_surface->surface->events.map, &view->map);

    view->unmap.notify = xdg_toplevel_unmap;
    wl_signal_add(&xdg_surface->surface->events.unmap, &view->unmap);

    view->destroy.notify = xdg_toplevel_destroy;
    wl_signal_add(&xdg_surface->events.destroy, &view->destroy);

    struct wlr_xdg_toplevel *toplevel = xdg_surface->toplevel;
    view->request_move.notify = xdg_toplevel_request_move;
    wl_signal_add(&toplevel->events.request_move, &view->request_move);

    view->request_resize.notify = xdg_toplevel_request_resize;
    wl_signal_add(&toplevel->events.request_resize, &view->request_resize);

    view->request_maximize.notify = xdg_toplevel_request_maximize;
    wl_signal_add(&toplevel->events.request_maximize, &view->request_maximize);

    view->request_fullscreen.notify = xdg_toplevel_request_fullscreen;
    wl_signal_add(&toplevel->events.request_fullscreen, &view->request_fullscreen);

    /* Add to list */
    wl_list_insert(&server->views, &view->link);

    wlr_log(WLR_DEBUG, "New XDG surface");
}
