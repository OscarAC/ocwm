#ifndef OCWM_H
#define OCWM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/render/pixman.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>
#include <xkbcommon/xkbcommon.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

/* Version */
#define OCWM_VERSION "0.4.0"

/* Forward declarations */
struct ocwm_server;
struct ocwm_output;
struct ocwm_view;
struct ocwm_keyboard;
struct ocwm_keybinding;
struct ocwm_workspace;
struct ocwm_animation;

/* Layout types */
enum ocwm_layout_type {
    LAYOUT_FLOATING,      /* Traditional floating windows (default) */
    LAYOUT_MASTER_STACK,  /* Master on left, stack on right (dwm-style) */
    LAYOUT_GRID,          /* Grid layout */
    LAYOUT_MONOCLE,       /* One window fullscreen at a time */
};

/* Workspace (virtual desktop/tag) */
struct ocwm_workspace {
    struct wl_list link;
    struct ocwm_server *server;
    int id;
    char *name;
    enum ocwm_layout_type layout;
    bool visible;

    /* Layout settings */
    float master_ratio;   /* For master-stack: width ratio of master area */
    int nmaster;          /* Number of windows in master area */
    int gap_size;         /* Gap between windows */
};

/* Easing functions for animations */
enum ocwm_easing {
    EASING_LINEAR,
    EASING_EASE_IN_OUT,
    EASING_EASE_OUT,
    EASING_ELASTIC,
    EASING_BOUNCE,
};

/* Animation types */
enum ocwm_animation_type {
    ANIM_WINDOW_OPEN,
    ANIM_WINDOW_CLOSE,
    ANIM_FADE,
    ANIM_SCALE,
    ANIM_SLIDE,
};

/* Animation structure */
struct ocwm_animation {
    struct wl_list link;
    struct ocwm_view *view;

    enum ocwm_animation_type type;
    enum ocwm_easing easing;

    uint32_t start_time;   /* milliseconds */
    uint32_t duration;     /* milliseconds */

    /* Animation values */
    float start_value;
    float end_value;
    float current_value;

    /* Callback when done */
    void (*on_complete)(struct ocwm_animation *anim);
    void *user_data;
};

/* Main server structure */
struct ocwm_server {
    struct wl_display *wl_display;
    struct wlr_backend *backend;
    struct wlr_renderer *renderer;
    struct wlr_allocator *allocator;
    struct wlr_scene *scene;
    struct wlr_scene_output_layout *scene_layout;

    /* Wayland protocols */
    struct wlr_compositor *compositor;
    struct wlr_xdg_shell *xdg_shell;
    struct wlr_seat *seat;
    struct wlr_cursor *cursor;
    struct wlr_xcursor_manager *cursor_mgr;
    struct wlr_output_layout *output_layout;
    struct wlr_data_device_manager *data_device_mgr;

    /* Lists */
    struct wl_list outputs;      // ocwm_output::link
    struct wl_list views;        // ocwm_view::link
    struct wl_list keyboards;    // ocwm_keyboard::link
    struct wl_list keybindings;  // ocwm_keybinding::link
    struct wl_list workspaces;   // ocwm_workspace::link
    struct wl_list animations;   // ocwm_animation::link

    /* Workspace management */
    struct ocwm_workspace *active_workspace;

    /* Effect settings */
    bool effects_enabled;
    uint32_t anim_duration_open;   /* ms */
    uint32_t anim_duration_close;  /* ms */
    bool blur_enabled;
    int blur_passes;

    /* Lua integration */
    lua_State *lua;
    char *config_path;

    /* Event listeners */
    struct wl_listener new_output;
    struct wl_listener new_xdg_surface;
    struct wl_listener new_input;
    struct wl_listener cursor_motion;
    struct wl_listener cursor_motion_absolute;
    struct wl_listener cursor_button;
    struct wl_listener cursor_axis;
    struct wl_listener cursor_frame;
    struct wl_listener request_cursor;
    struct wl_listener request_set_selection;

    /* State */
    struct ocwm_view *grabbed_view;
    double grab_x, grab_y;
    double grab_geobox_x, grab_geobox_y;
    uint32_t grab_width, grab_height;
    uint32_t resize_edges;

    /* Cursor mode */
    enum ocwm_cursor_mode {
        OCWM_CURSOR_PASSTHROUGH,
        OCWM_CURSOR_MOVE,
        OCWM_CURSOR_RESIZE,
    } cursor_mode;
};

/* Output (monitor) */
struct ocwm_output {
    struct wl_list link;
    struct ocwm_server *server;
    struct wlr_output *wlr_output;
    struct wl_listener frame;
    struct wl_listener destroy;
};

/* View (window) */
struct ocwm_view {
    struct wl_list link;
    struct ocwm_server *server;
    struct wlr_xdg_toplevel *xdg_toplevel;
    struct wlr_scene_tree *scene_tree;

    struct wl_listener map;
    struct wl_listener unmap;
    struct wl_listener destroy;
    struct wl_listener request_move;
    struct wl_listener request_resize;
    struct wl_listener request_maximize;
    struct wl_listener request_fullscreen;

    int x, y;
    bool mapped;

    /* Workspace and layout */
    struct ocwm_workspace *workspace;
    bool floating;     /* If true, don't tile this window */
    bool fullscreen;   /* Fullscreen state */

    /* Effects */
    float opacity;          /* 0.0 to 1.0 */
    bool blur;             /* Enable blur for this window */
    float scale;           /* Current scale (for animations) */
    bool animating;        /* Currently animating */
};

/* Keyboard */
struct ocwm_keyboard {
    struct wl_list link;
    struct ocwm_server *server;
    struct wlr_keyboard *wlr_keyboard;

    struct wl_listener modifiers;
    struct wl_listener key;
    struct wl_listener destroy;
};

/* Keybinding */
struct ocwm_keybinding {
    struct wl_list link;
    uint32_t modifiers;      // Modifier mask (Mod, Shift, Ctrl, Alt)
    xkb_keysym_t keysym;     // Key symbol
    int lua_callback_ref;    // Lua registry reference to callback function
};

/* Event hooks */
enum ocwm_event_type {
    OCWM_EVENT_WINDOW_OPEN,
    OCWM_EVENT_WINDOW_CLOSE,
    OCWM_EVENT_WINDOW_FOCUS,
};

/* Function declarations */

/* main.c */
void ocwm_server_init(struct ocwm_server *server);
bool ocwm_server_start(struct ocwm_server *server);
void ocwm_server_run(struct ocwm_server *server);
void ocwm_server_finish(struct ocwm_server *server);

/* wayland/output.c */
void server_new_output(struct wl_listener *listener, void *data);

/* wm/view.c */
void server_new_xdg_surface(struct wl_listener *listener, void *data);
void focus_view(struct ocwm_view *view, struct wlr_surface *surface);
struct ocwm_view *desktop_view_at(struct ocwm_server *server,
    double lx, double ly, struct wlr_surface **surface, double *sx, double *sy);

/* wayland/input.c */
void server_new_input(struct wl_listener *listener, void *data);
void server_cursor_motion(struct wl_listener *listener, void *data);
void server_cursor_motion_absolute(struct wl_listener *listener, void *data);
void server_cursor_button(struct wl_listener *listener, void *data);
void server_cursor_axis(struct wl_listener *listener, void *data);
void server_cursor_frame(struct wl_listener *listener, void *data);
void seat_request_cursor(struct wl_listener *listener, void *data);
void seat_request_set_selection(struct wl_listener *listener, void *data);
void process_cursor_motion(struct ocwm_server *server, uint32_t time);
bool handle_keybinding(struct ocwm_server *server, uint32_t modifiers, xkb_keysym_t sym);

/* lua/lua_api.c */
void lua_init(struct ocwm_server *server);
void lua_finish(struct ocwm_server *server);
bool lua_load_config(struct ocwm_server *server, const char *path);
void lua_reload_config(struct ocwm_server *server);
void lua_fire_event(struct ocwm_server *server, enum ocwm_event_type event, void *data);

/* layout/workspace.c */
void workspace_init(struct ocwm_server *server);
void workspace_finish(struct ocwm_server *server);
struct ocwm_workspace *workspace_create(struct ocwm_server *server, int id, const char *name);
void workspace_destroy(struct ocwm_workspace *workspace);
void workspace_switch(struct ocwm_server *server, struct ocwm_workspace *workspace);
struct ocwm_workspace *workspace_get_by_id(struct ocwm_server *server, int id);

/* layout/layout.c */
void layout_apply(struct ocwm_workspace *workspace);
void layout_set_type(struct ocwm_workspace *workspace, enum ocwm_layout_type type);
const char *layout_get_name(enum ocwm_layout_type type);

/* effects/animation.c */
void animation_init(struct ocwm_server *server);
void animation_finish(struct ocwm_server *server);
void animation_update(struct ocwm_server *server, uint32_t time_msec);
struct ocwm_animation *animation_create(struct ocwm_view *view,
    enum ocwm_animation_type type, uint32_t duration);
void animation_destroy(struct ocwm_animation *anim);
float easing_function(enum ocwm_easing easing, float t);

/* effects/effects.c */
void effects_init(struct ocwm_server *server);
void view_set_opacity(struct ocwm_view *view, float opacity);
void view_set_blur(struct ocwm_view *view, bool enabled);
void view_animate_open(struct ocwm_view *view);
void view_animate_close(struct ocwm_view *view);

#endif /* OCWM_H */
