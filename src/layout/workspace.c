/*
 * Workspace (virtual desktop) management
 */

#include "../ocwm.h"

#define DEFAULT_NUM_WORKSPACES 9
#define DEFAULT_MASTER_RATIO 0.55f
#define DEFAULT_NMASTER 1
#define DEFAULT_GAP_SIZE 10

/*
 * Initialize workspace system
 * Creates default workspaces (1-9)
 */
void workspace_init(struct ocwm_server *server) {
    wl_list_init(&server->workspaces);

    /* Create 9 workspaces by default */
    for (int i = 1; i <= DEFAULT_NUM_WORKSPACES; i++) {
        char name[32];
        snprintf(name, sizeof(name), "%d", i);
        workspace_create(server, i, name);
    }

    /* Set workspace 1 as active */
    server->active_workspace = workspace_get_by_id(server, 1);
    if (server->active_workspace) {
        server->active_workspace->visible = true;
    }

    wlr_log(WLR_INFO, "Initialized %d workspaces", DEFAULT_NUM_WORKSPACES);
}

/*
 * Cleanup workspace system
 */
void workspace_finish(struct ocwm_server *server) {
    struct ocwm_workspace *workspace, *tmp;
    wl_list_for_each_safe(workspace, tmp, &server->workspaces, link) {
        workspace_destroy(workspace);
    }

    wlr_log(WLR_INFO, "Workspaces cleaned up");
}

/*
 * Create a new workspace
 */
struct ocwm_workspace *workspace_create(struct ocwm_server *server, int id, const char *name) {
    struct ocwm_workspace *workspace = calloc(1, sizeof(struct ocwm_workspace));
    if (!workspace) {
        wlr_log(WLR_ERROR, "Failed to allocate workspace");
        return NULL;
    }

    workspace->server = server;
    workspace->id = id;
    workspace->name = name ? strdup(name) : NULL;
    workspace->layout = LAYOUT_FLOATING;  /* Default to floating */
    workspace->visible = false;

    /* Default layout settings */
    workspace->master_ratio = DEFAULT_MASTER_RATIO;
    workspace->nmaster = DEFAULT_NMASTER;
    workspace->gap_size = DEFAULT_GAP_SIZE;

    wl_list_insert(&server->workspaces, &workspace->link);

    wlr_log(WLR_DEBUG, "Created workspace %d: %s", id, name ?: "(unnamed)");

    return workspace;
}

/*
 * Destroy a workspace
 */
void workspace_destroy(struct ocwm_workspace *workspace) {
    if (!workspace) {
        return;
    }

    /* Note: We don't destroy views here, they're managed separately */

    if (workspace->name) {
        free(workspace->name);
    }

    wl_list_remove(&workspace->link);
    free(workspace);
}

/*
 * Get workspace by ID
 */
struct ocwm_workspace *workspace_get_by_id(struct ocwm_server *server, int id) {
    struct ocwm_workspace *workspace;
    wl_list_for_each(workspace, &server->workspaces, link) {
        if (workspace->id == id) {
            return workspace;
        }
    }
    return NULL;
}

/*
 * Count visible (tiled) windows in a workspace
 */
static int workspace_count_tiled_views(struct ocwm_workspace *workspace) {
    int count = 0;
    struct ocwm_view *view;

    wl_list_for_each(view, &workspace->server->views, link) {
        if (view->workspace == workspace && view->mapped && !view->floating && !view->fullscreen) {
            count++;
        }
    }

    return count;
}

/*
 * Show all windows in a workspace
 */
static void workspace_show(struct ocwm_workspace *workspace) {
    struct ocwm_view *view;

    wl_list_for_each(view, &workspace->server->views, link) {
        if (view->workspace == workspace) {
            wlr_scene_node_set_enabled(&view->scene_tree->node, true);
        }
    }

    workspace->visible = true;
}

/*
 * Hide all windows in a workspace
 */
static void workspace_hide(struct ocwm_workspace *workspace) {
    struct ocwm_view *view;

    wl_list_for_each(view, &workspace->server->views, link) {
        if (view->workspace == workspace) {
            wlr_scene_node_set_enabled(&view->scene_tree->node, false);
        }
    }

    workspace->visible = false;
}

/*
 * Switch to a different workspace
 */
void workspace_switch(struct ocwm_server *server, struct ocwm_workspace *workspace) {
    if (!workspace || workspace == server->active_workspace) {
        return;
    }

    /* Hide current workspace */
    if (server->active_workspace) {
        workspace_hide(server->active_workspace);
    }

    /* Show new workspace */
    workspace_show(workspace);
    server->active_workspace = workspace;

    /* Apply layout to new workspace */
    layout_apply(workspace);

    wlr_log(WLR_INFO, "Switched to workspace %d: %s",
        workspace->id, workspace->name ?: "(unnamed)");
}
