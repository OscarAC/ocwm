/*
 * Lua API implementation for OCWM
 */

#include "../ocwm.h"
#include <unistd.h>
#include <sys/wait.h>

/* Global server pointer for Lua callbacks */
static struct ocwm_server *g_server = NULL;

/*
 * Helper functions
 */

static uint32_t parse_modifiers(const char *mod_str) {
    uint32_t mods = 0;

    if (strstr(mod_str, "Mod") || strstr(mod_str, "Super") || strstr(mod_str, "Win")) {
        mods |= WLR_MODIFIER_LOGO;
    }
    if (strstr(mod_str, "Shift")) {
        mods |= WLR_MODIFIER_SHIFT;
    }
    if (strstr(mod_str, "Ctrl") || strstr(mod_str, "Control")) {
        mods |= WLR_MODIFIER_CTRL;
    }
    if (strstr(mod_str, "Alt")) {
        mods |= WLR_MODIFIER_ALT;
    }

    return mods;
}

static xkb_keysym_t parse_keybind_string(const char *bind_str, uint32_t *modifiers) {
    *modifiers = 0;

    /* Split by '+' and parse modifiers */
    char *str = strdup(bind_str);
    char *token = strtok(str, "+");
    char *key_name = NULL;

    while (token != NULL) {
        /* Trim whitespace */
        while (*token == ' ') token++;
        char *end = token + strlen(token) - 1;
        while (end > token && *end == ' ') end--;
        *(end + 1) = '\0';

        /* Check if it's a modifier */
        if (strcmp(token, "Mod") == 0 || strcmp(token, "Super") == 0 || strcmp(token, "Win") == 0) {
            *modifiers |= WLR_MODIFIER_LOGO;
        } else if (strcmp(token, "Shift") == 0) {
            *modifiers |= WLR_MODIFIER_SHIFT;
        } else if (strcmp(token, "Ctrl") == 0 || strcmp(token, "Control") == 0) {
            *modifiers |= WLR_MODIFIER_CTRL;
        } else if (strcmp(token, "Alt") == 0) {
            *modifiers |= WLR_MODIFIER_ALT;
        } else {
            /* This is the actual key */
            key_name = strdup(token);
        }

        token = strtok(NULL, "+");
    }

    free(str);

    if (key_name == NULL) {
        return XKB_KEY_NoSymbol;
    }

    xkb_keysym_t keysym = xkb_keysym_from_name(key_name, XKB_KEYSYM_CASE_INSENSITIVE);
    free(key_name);

    return keysym;
}

/*
 * Lua API: ocwm.bind(keybind, callback)
 * Registers a keybinding with a Lua callback
 */
static int lua_api_bind(lua_State *L) {
    if (!lua_isstring(L, 1)) {
        return luaL_error(L, "First argument must be a keybind string (e.g., 'Mod+Return')");
    }
    if (!lua_isfunction(L, 2)) {
        return luaL_error(L, "Second argument must be a callback function");
    }

    const char *bind_str = lua_tostring(L, 1);
    uint32_t modifiers = 0;
    xkb_keysym_t keysym = parse_keybind_string(bind_str, &modifiers);

    if (keysym == XKB_KEY_NoSymbol) {
        return luaL_error(L, "Invalid keybind: %s", bind_str);
    }

    /* Store the callback function in the registry */
    lua_pushvalue(L, 2);  /* Copy function to top of stack */
    int callback_ref = luaL_ref(L, LUA_REGISTRYINDEX);

    /* Create keybinding */
    struct ocwm_keybinding *binding = calloc(1, sizeof(struct ocwm_keybinding));
    binding->modifiers = modifiers;
    binding->keysym = keysym;
    binding->lua_callback_ref = callback_ref;

    wl_list_insert(&g_server->keybindings, &binding->link);

    wlr_log(WLR_INFO, "Registered keybind: %s (mods=0x%x, sym=%d)", bind_str, modifiers, keysym);

    return 0;
}

/*
 * Lua API: ocwm.spawn(command)
 * Spawns a process
 */
static int lua_api_spawn(lua_State *L) {
    if (!lua_isstring(L, 1)) {
        return luaL_error(L, "Argument must be a command string");
    }

    const char *cmd = lua_tostring(L, 1);

    pid_t pid = fork();
    if (pid < 0) {
        wlr_log(WLR_ERROR, "Failed to fork for command: %s", cmd);
        return 0;
    }

    if (pid == 0) {
        /* Child process */
        setsid();
        execl("/bin/sh", "/bin/sh", "-c", cmd, (char *)NULL);
        _exit(1);
    }

    wlr_log(WLR_INFO, "Spawned command: %s (pid=%d)", cmd, pid);

    return 0;
}

/*
 * Lua API: ocwm.log(message)
 * Logs a message
 */
static int lua_api_log(lua_State *L) {
    const char *msg = luaL_checkstring(L, 1);
    wlr_log(WLR_INFO, "[Lua] %s", msg);
    return 0;
}

/*
 * Lua API: ocwm.window.focused()
 * Returns the focused window
 */
static int lua_api_window_focused(lua_State *L) {
    struct wlr_surface *focused = g_server->seat->keyboard_state.focused_surface;

    if (focused == NULL) {
        lua_pushnil(L);
        return 1;
    }

    /* Find the view for this surface */
    struct ocwm_view *view;
    wl_list_for_each(view, &g_server->views, link) {
        if (view->xdg_toplevel->base->surface == focused) {
            lua_pushlightuserdata(L, view);
            return 1;
        }
    }

    lua_pushnil(L);
    return 1;
}

/*
 * Lua API: ocwm.window.close(window)
 * Closes a window
 */
static int lua_api_window_close(lua_State *L) {
    if (!lua_isuserdata(L, 1) && !lua_islightuserdata(L, 1)) {
        return luaL_error(L, "Argument must be a window object");
    }

    struct ocwm_view *view = lua_touserdata(L, 1);
    if (view && view->xdg_toplevel) {
        wlr_xdg_toplevel_send_close(view->xdg_toplevel);
    }

    return 0;
}

/*
 * Lua API: ocwm.window.list()
 * Returns a list of all windows
 */
static int lua_api_window_list(lua_State *L) {
    lua_newtable(L);

    int i = 1;
    struct ocwm_view *view;
    wl_list_for_each(view, &g_server->views, link) {
        if (view->mapped) {
            lua_pushlightuserdata(L, view);
            lua_rawseti(L, -2, i++);
        }
    }

    return 1;
}

/*
 * Lua API: ocwm.window.get_title(window)
 * Gets window title
 */
static int lua_api_window_get_title(lua_State *L) {
    if (!lua_isuserdata(L, 1) && !lua_islightuserdata(L, 1)) {
        return luaL_error(L, "Argument must be a window object");
    }

    struct ocwm_view *view = lua_touserdata(L, 1);
    if (view && view->xdg_toplevel && view->xdg_toplevel->title) {
        lua_pushstring(L, view->xdg_toplevel->title);
    } else {
        lua_pushstring(L, "");
    }

    return 1;
}

/*
 * Lua API: ocwm.window.get_app_id(window)
 * Gets window app_id
 */
static int lua_api_window_get_app_id(lua_State *L) {
    if (!lua_isuserdata(L, 1) && !lua_islightuserdata(L, 1)) {
        return luaL_error(L, "Argument must be a window object");
    }

    struct ocwm_view *view = lua_touserdata(L, 1);
    if (view && view->xdg_toplevel && view->xdg_toplevel->app_id) {
        lua_pushstring(L, view->xdg_toplevel->app_id);
    } else {
        lua_pushstring(L, "");
    }

    return 1;
}

/*
 * Lua API: ocwm.reload()
 * Reloads the configuration
 */
static int lua_api_reload(lua_State *L) {
    wlr_log(WLR_INFO, "Reloading configuration...");
    lua_reload_config(g_server);
    return 0;
}

/*
 * Lua API: ocwm.quit()
 * Exits the compositor
 */
static int lua_api_quit(lua_State *L) {
    wlr_log(WLR_INFO, "Quitting compositor...");
    wl_display_terminate(g_server->wl_display);
    return 0;
}

/*
 * Event hook storage
 */
static int event_hooks[16] = {0}; /* Lua registry refs for event callbacks */

/*
 * Lua API: ocwm.on(event_name, callback)
 * Registers an event hook
 */
static int lua_api_on(lua_State *L) {
    if (!lua_isstring(L, 1)) {
        return luaL_error(L, "First argument must be an event name string");
    }
    if (!lua_isfunction(L, 2)) {
        return luaL_error(L, "Second argument must be a callback function");
    }

    const char *event_name = lua_tostring(L, 1);
    enum ocwm_event_type event_type;

    if (strcmp(event_name, "window_open") == 0) {
        event_type = OCWM_EVENT_WINDOW_OPEN;
    } else if (strcmp(event_name, "window_close") == 0) {
        event_type = OCWM_EVENT_WINDOW_CLOSE;
    } else if (strcmp(event_name, "window_focus") == 0) {
        event_type = OCWM_EVENT_WINDOW_FOCUS;
    } else {
        return luaL_error(L, "Unknown event: %s", event_name);
    }

    /* Store the callback */
    lua_pushvalue(L, 2);
    int ref = luaL_ref(L, LUA_REGISTRYINDEX);

    /* Unref old callback if exists */
    if (event_hooks[event_type] != 0) {
        luaL_unref(L, LUA_REGISTRYINDEX, event_hooks[event_type]);
    }

    event_hooks[event_type] = ref;

    wlr_log(WLR_INFO, "Registered event hook: %s", event_name);

    return 0;
}

/*
 * Lua API: ocwm.workspace.switch(id)
 * Switch to a workspace by ID
 */
static int lua_api_workspace_switch(lua_State *L) {
    int id = luaL_checkinteger(L, 1);

    struct ocwm_workspace *workspace = workspace_get_by_id(g_server, id);
    if (!workspace) {
        return luaL_error(L, "Workspace %d does not exist", id);
    }

    workspace_switch(g_server, workspace);
    return 0;
}

/*
 * Lua API: ocwm.workspace.get_active()
 * Get the active workspace ID
 */
static int lua_api_workspace_get_active(lua_State *L) {
    if (g_server->active_workspace) {
        lua_pushinteger(L, g_server->active_workspace->id);
    } else {
        lua_pushnil(L);
    }
    return 1;
}

/*
 * Lua API: ocwm.layout.set(layout_name)
 * Set the layout for the active workspace
 */
static int lua_api_layout_set(lua_State *L) {
    const char *layout_name = luaL_checkstring(L, 1);

    if (!g_server->active_workspace) {
        return luaL_error(L, "No active workspace");
    }

    enum ocwm_layout_type type;
    if (strcmp(layout_name, "floating") == 0) {
        type = LAYOUT_FLOATING;
    } else if (strcmp(layout_name, "master-stack") == 0 || strcmp(layout_name, "tile") == 0) {
        type = LAYOUT_MASTER_STACK;
    } else if (strcmp(layout_name, "grid") == 0) {
        type = LAYOUT_GRID;
    } else if (strcmp(layout_name, "monocle") == 0 || strcmp(layout_name, "max") == 0) {
        type = LAYOUT_MONOCLE;
    } else {
        return luaL_error(L, "Unknown layout: %s", layout_name);
    }

    layout_set_type(g_server->active_workspace, type);
    return 0;
}

/*
 * Lua API: ocwm.layout.get()
 * Get the current layout name
 */
static int lua_api_layout_get(lua_State *L) {
    if (!g_server->active_workspace) {
        lua_pushnil(L);
        return 1;
    }

    const char *name = layout_get_name(g_server->active_workspace->layout);
    lua_pushstring(L, name);
    return 1;
}

/*
 * Lua API: ocwm.layout.set_master_ratio(ratio)
 * Set master area ratio for master-stack layout
 */
static int lua_api_layout_set_master_ratio(lua_State *L) {
    double ratio = luaL_checknumber(L, 1);

    if (ratio <= 0.0 || ratio >= 1.0) {
        return luaL_error(L, "Master ratio must be between 0 and 1");
    }

    if (g_server->active_workspace) {
        g_server->active_workspace->master_ratio = ratio;
        layout_apply(g_server->active_workspace);
    }

    return 0;
}

/*
 * Lua API: ocwm.layout.set_gap(size)
 * Set gap size between windows
 */
static int lua_api_layout_set_gap(lua_State *L) {
    int gap = luaL_checkinteger(L, 1);

    if (gap < 0) {
        return luaL_error(L, "Gap size must be non-negative");
    }

    if (g_server->active_workspace) {
        g_server->active_workspace->gap_size = gap;
        layout_apply(g_server->active_workspace);
    }

    return 0;
}

/*
 * Lua API: ocwm.window.set_floating(window, floating)
 * Set window floating state
 */
static int lua_api_window_set_floating(lua_State *L) {
    if (!lua_isuserdata(L, 1) && !lua_islightuserdata(L, 1)) {
        return luaL_error(L, "First argument must be a window object");
    }

    struct ocwm_view *view = lua_touserdata(L, 1);
    bool floating = lua_toboolean(L, 2);

    if (view && view->workspace) {
        view->floating = floating;
        layout_apply(view->workspace);
    }

    return 0;
}

/*
 * Lua API: ocwm.effects.enable(bool)
 * Enable or disable effects globally
 */
static int lua_api_effects_enable(lua_State *L) {
    bool enabled = lua_toboolean(L, 1);
    g_server->effects_enabled = enabled;
    wlr_log(WLR_INFO, "Effects %s", enabled ? "enabled" : "disabled");
    return 0;
}

/*
 * Lua API: ocwm.effects.set_duration(open_ms, close_ms)
 * Set animation durations
 */
static int lua_api_effects_set_duration(lua_State *L) {
    int open = luaL_checkinteger(L, 1);
    int close = luaL_optinteger(L, 2, open);

    if (open < 0 || close < 0) {
        return luaL_error(L, "Duration must be non-negative");
    }

    g_server->anim_duration_open = open;
    g_server->anim_duration_close = close;

    return 0;
}

/*
 * Lua API: ocwm.window.set_opacity(window, opacity)
 * Set window opacity (0.0 to 1.0)
 */
static int lua_api_window_set_opacity(lua_State *L) {
    if (!lua_isuserdata(L, 1) && !lua_islightuserdata(L, 1)) {
        return luaL_error(L, "First argument must be a window object");
    }

    struct ocwm_view *view = lua_touserdata(L, 1);
    double opacity = luaL_checknumber(L, 2);

    if (view) {
        view_set_opacity(view, (float)opacity);
    }

    return 0;
}

/*
 * Lua API: ocwm.window.set_blur(window, enabled)
 * Enable blur for a window
 */
static int lua_api_window_set_blur(lua_State *L) {
    if (!lua_isuserdata(L, 1) && !lua_islightuserdata(L, 1)) {
        return luaL_error(L, "First argument must be a window object");
    }

    struct ocwm_view *view = lua_touserdata(L, 1);
    bool enabled = lua_toboolean(L, 2);

    if (view) {
        view_set_blur(view, enabled);
    }

    return 0;
}

/*
 * Register all Lua API functions
 */
static void register_lua_api(lua_State *L) {
    /* Create ocwm table */
    lua_newtable(L);

    /* Core functions */
    lua_pushcfunction(L, lua_api_bind);
    lua_setfield(L, -2, "bind");

    lua_pushcfunction(L, lua_api_spawn);
    lua_setfield(L, -2, "spawn");

    lua_pushcfunction(L, lua_api_log);
    lua_setfield(L, -2, "log");

    lua_pushcfunction(L, lua_api_reload);
    lua_setfield(L, -2, "reload");

    lua_pushcfunction(L, lua_api_quit);
    lua_setfield(L, -2, "quit");

    lua_pushcfunction(L, lua_api_on);
    lua_setfield(L, -2, "on");

    /* Window API table */
    lua_newtable(L);
    lua_pushcfunction(L, lua_api_window_focused);
    lua_setfield(L, -2, "focused");
    lua_pushcfunction(L, lua_api_window_close);
    lua_setfield(L, -2, "close");
    lua_pushcfunction(L, lua_api_window_list);
    lua_setfield(L, -2, "list");
    lua_pushcfunction(L, lua_api_window_get_title);
    lua_setfield(L, -2, "get_title");
    lua_pushcfunction(L, lua_api_window_get_app_id);
    lua_setfield(L, -2, "get_app_id");
    lua_pushcfunction(L, lua_api_window_set_floating);
    lua_setfield(L, -2, "set_floating");
    lua_pushcfunction(L, lua_api_window_set_opacity);
    lua_setfield(L, -2, "set_opacity");
    lua_pushcfunction(L, lua_api_window_set_blur);
    lua_setfield(L, -2, "set_blur");
    lua_setfield(L, -2, "window");

    /* Workspace API table */
    lua_newtable(L);
    lua_pushcfunction(L, lua_api_workspace_switch);
    lua_setfield(L, -2, "switch");
    lua_pushcfunction(L, lua_api_workspace_get_active);
    lua_setfield(L, -2, "get_active");
    lua_setfield(L, -2, "workspace");

    /* Layout API table */
    lua_newtable(L);
    lua_pushcfunction(L, lua_api_layout_set);
    lua_setfield(L, -2, "set");
    lua_pushcfunction(L, lua_api_layout_get);
    lua_setfield(L, -2, "get");
    lua_pushcfunction(L, lua_api_layout_set_master_ratio);
    lua_setfield(L, -2, "set_master_ratio");
    lua_pushcfunction(L, lua_api_layout_set_gap);
    lua_setfield(L, -2, "set_gap");
    lua_setfield(L, -2, "layout");

    /* Effects API table */
    lua_newtable(L);
    lua_pushcfunction(L, lua_api_effects_enable);
    lua_setfield(L, -2, "enable");
    lua_pushcfunction(L, lua_api_effects_set_duration);
    lua_setfield(L, -2, "set_duration");
    lua_setfield(L, -2, "effects");

    /* Set as global */
    lua_setglobal(L, "ocwm");
}

/*
 * Initialize Lua subsystem
 */
void lua_init(struct ocwm_server *server) {
    g_server = server;

    /* Create Lua state */
    server->lua = luaL_newstate();
    if (!server->lua) {
        wlr_log(WLR_ERROR, "Failed to create Lua state");
        return;
    }

    /* Load Lua standard libraries */
    luaL_openlibs(server->lua);

    /* Register our API */
    register_lua_api(server->lua);

    /* Initialize keybindings list */
    wl_list_init(&server->keybindings);

    wlr_log(WLR_INFO, "Lua subsystem initialized");
}

/*
 * Cleanup Lua subsystem
 */
void lua_finish(struct ocwm_server *server) {
    if (!server->lua) {
        return;
    }

    /* Free all keybindings */
    struct ocwm_keybinding *binding, *tmp;
    wl_list_for_each_safe(binding, tmp, &server->keybindings, link) {
        luaL_unref(server->lua, LUA_REGISTRYINDEX, binding->lua_callback_ref);
        wl_list_remove(&binding->link);
        free(binding);
    }

    /* Close Lua state */
    lua_close(server->lua);
    server->lua = NULL;

    if (server->config_path) {
        free(server->config_path);
        server->config_path = NULL;
    }

    wlr_log(WLR_INFO, "Lua subsystem shutdown");
}

/*
 * Load configuration file
 */
bool lua_load_config(struct ocwm_server *server, const char *path) {
    if (!server->lua) {
        wlr_log(WLR_ERROR, "Lua not initialized");
        return false;
    }

    /* Check if file exists */
    if (access(path, R_OK) != 0) {
        wlr_log(WLR_ERROR, "Config file not found or not readable: %s", path);
        return false;
    }

    /* Load and execute the config file */
    if (luaL_dofile(server->lua, path) != LUA_OK) {
        const char *error = lua_tostring(server->lua, -1);
        wlr_log(WLR_ERROR, "Lua config error: %s", error);
        lua_pop(server->lua, 1);
        return false;
    }

    /* Save config path for reloading */
    if (server->config_path) {
        free(server->config_path);
    }
    server->config_path = strdup(path);

    wlr_log(WLR_INFO, "Loaded config: %s", path);

    return true;
}

/*
 * Reload configuration
 */
void lua_reload_config(struct ocwm_server *server) {
    if (!server->lua || !server->config_path) {
        return;
    }

    /* Clear all keybindings */
    struct ocwm_keybinding *binding, *tmp;
    wl_list_for_each_safe(binding, tmp, &server->keybindings, link) {
        luaL_unref(server->lua, LUA_REGISTRYINDEX, binding->lua_callback_ref);
        wl_list_remove(&binding->link);
        free(binding);
    }

    /* Clear event hooks */
    for (int i = 0; i < 16; i++) {
        if (event_hooks[i] != 0) {
            luaL_unref(server->lua, LUA_REGISTRYINDEX, event_hooks[i]);
            event_hooks[i] = 0;
        }
    }

    /* Reload config */
    lua_load_config(server, server->config_path);
}

/*
 * Fire an event to Lua hooks
 */
void lua_fire_event(struct ocwm_server *server, enum ocwm_event_type event, void *data) {
    if (!server->lua) {
        return;
    }

    int hook_ref = event_hooks[event];
    if (hook_ref == 0) {
        return; /* No hook registered */
    }

    /* Get the callback function */
    lua_rawgeti(server->lua, LUA_REGISTRYINDEX, hook_ref);

    /* Push event data */
    switch (event) {
    case OCWM_EVENT_WINDOW_OPEN:
    case OCWM_EVENT_WINDOW_CLOSE:
    case OCWM_EVENT_WINDOW_FOCUS:
        lua_pushlightuserdata(server->lua, data); /* data is ocwm_view* */
        break;
    default:
        lua_pushnil(server->lua);
        break;
    }

    /* Call the function */
    if (lua_pcall(server->lua, 1, 0, 0) != LUA_OK) {
        const char *error = lua_tostring(server->lua, -1);
        wlr_log(WLR_ERROR, "Event hook error: %s", error);
        lua_pop(server->lua, 1);
    }
}
