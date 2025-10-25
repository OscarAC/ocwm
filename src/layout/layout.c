/*
 * Layout engine - Window positioning algorithms
 */

#include "../ocwm.h"
#include <math.h>

/*
 * Get usable area from output
 * TODO: Account for panels/bars when we support layer-shell
 */
static void get_usable_area(struct ocwm_server *server, struct wlr_box *usable) {
    struct ocwm_output *output;

    /* For now, use the first output */
    if (wl_list_empty(&server->outputs)) {
        usable->x = 0;
        usable->y = 0;
        usable->width = 1920;
        usable->height = 1080;
        return;
    }

    output = wl_container_of(server->outputs.next, output, link);

    usable->x = 0;
    usable->y = 0;
    usable->width = output->wlr_output->width;
    usable->height = output->wlr_output->height;
}

/*
 * Get layout name
 */
const char *layout_get_name(enum ocwm_layout_type type) {
    switch (type) {
    case LAYOUT_FLOATING:
        return "floating";
    case LAYOUT_MASTER_STACK:
        return "master-stack";
    case LAYOUT_GRID:
        return "grid";
    case LAYOUT_MONOCLE:
        return "monocle";
    default:
        return "unknown";
    }
}

/*
 * Set layout type for workspace
 */
void layout_set_type(struct ocwm_workspace *workspace, enum ocwm_layout_type type) {
    if (!workspace) {
        return;
    }

    workspace->layout = type;
    wlr_log(WLR_INFO, "Workspace %d: layout set to %s",
        workspace->id, layout_get_name(type));

    /* Apply the new layout */
    layout_apply(workspace);
}

/*
 * FLOATING LAYOUT
 * Windows are not tiled, they float freely
 */
static void layout_floating(struct ocwm_workspace *workspace) {
    /* Nothing to do - windows position themselves */
    (void)workspace;
}

/*
 * MASTER-STACK LAYOUT (dwm/i3 style)
 * Master area on left, stack on right
 */
static void layout_master_stack(struct ocwm_workspace *workspace) {
    struct wlr_box usable;
    get_usable_area(workspace->server, &usable);

    int gap = workspace->gap_size;
    struct ocwm_view *view;

    /* Count tiled views */
    int n_views = 0;
    wl_list_for_each(view, &workspace->server->views, link) {
        if (view->workspace == workspace && view->mapped &&
            !view->floating && !view->fullscreen) {
            n_views++;
        }
    }

    if (n_views == 0) {
        return;
    }

    /* Calculate dimensions */
    int master_width = usable.width * workspace->master_ratio;
    int stack_width = usable.width - master_width - gap;
    int nmaster = workspace->nmaster < n_views ? workspace->nmaster : n_views;
    int nstack = n_views - nmaster;

    int master_height = (usable.height - (nmaster + 1) * gap) / nmaster;
    int stack_height = nstack > 0 ? (usable.height - (nstack + 1) * gap) / nstack : 0;

    /* Position windows */
    int master_idx = 0;
    int stack_idx = 0;

    wl_list_for_each(view, &workspace->server->views, link) {
        if (view->workspace != workspace || !view->mapped ||
            view->floating || view->fullscreen) {
            continue;
        }

        int x, y, width, height;

        if (master_idx < nmaster) {
            /* Master area */
            x = usable.x + gap;
            y = usable.y + gap + master_idx * (master_height + gap);
            width = master_width - gap;
            height = master_height;
            master_idx++;
        } else {
            /* Stack area */
            x = usable.x + master_width + gap;
            y = usable.y + gap + stack_idx * (stack_height + gap);
            width = stack_width - gap;
            height = stack_height;
            stack_idx++;
        }

        /* Set window position and size */
        wlr_scene_node_set_position(&view->scene_tree->node, x, y);
        wlr_xdg_toplevel_set_size(view->xdg_toplevel, width, height);
    }
}

/*
 * GRID LAYOUT
 * Windows arranged in a grid
 */
static void layout_grid(struct ocwm_workspace *workspace) {
    struct wlr_box usable;
    get_usable_area(workspace->server, &usable);

    int gap = workspace->gap_size;
    struct ocwm_view *view;

    /* Count tiled views */
    int n_views = 0;
    wl_list_for_each(view, &workspace->server->views, link) {
        if (view->workspace == workspace && view->mapped &&
            !view->floating && !view->fullscreen) {
            n_views++;
        }
    }

    if (n_views == 0) {
        return;
    }

    /* Calculate grid dimensions */
    int cols = (int)ceil(sqrt(n_views));
    int rows = (n_views + cols - 1) / cols;  /* Ceiling division */

    int cell_width = (usable.width - (cols + 1) * gap) / cols;
    int cell_height = (usable.height - (rows + 1) * gap) / rows;

    /* Position windows */
    int idx = 0;

    wl_list_for_each(view, &workspace->server->views, link) {
        if (view->workspace != workspace || !view->mapped ||
            view->floating || view->fullscreen) {
            continue;
        }

        int col = idx % cols;
        int row = idx / cols;

        int x = usable.x + gap + col * (cell_width + gap);
        int y = usable.y + gap + row * (cell_height + gap);

        /* Set window position and size */
        wlr_scene_node_set_position(&view->scene_tree->node, x, y);
        wlr_xdg_toplevel_set_size(view->xdg_toplevel, cell_width, cell_height);

        idx++;
    }
}

/*
 * MONOCLE LAYOUT
 * One window fullscreen, others hidden
 */
static void layout_monocle(struct ocwm_workspace *workspace) {
    struct wlr_box usable;
    get_usable_area(workspace->server, &usable);

    int gap = workspace->gap_size;
    struct ocwm_view *view;
    struct ocwm_view *focused = NULL;

    /* Find focused view in this workspace */
    struct wlr_surface *focused_surface =
        workspace->server->seat->keyboard_state.focused_surface;

    wl_list_for_each(view, &workspace->server->views, link) {
        if (view->workspace == workspace && view->mapped &&
            !view->floating && !view->fullscreen) {
            if (focused_surface &&
                view->xdg_toplevel->base->surface == focused_surface) {
                focused = view;
                break;
            }
        }
    }

    /* If no focused view, use the first one */
    if (!focused) {
        wl_list_for_each(view, &workspace->server->views, link) {
            if (view->workspace == workspace && view->mapped &&
                !view->floating && !view->fullscreen) {
                focused = view;
                break;
            }
        }
    }

    /* Make focused window fullscreen, hide others */
    wl_list_for_each(view, &workspace->server->views, link) {
        if (view->workspace != workspace || !view->mapped ||
            view->floating || view->fullscreen) {
            continue;
        }

        if (view == focused) {
            /* Make this window fullscreen */
            wlr_scene_node_set_position(&view->scene_tree->node,
                usable.x + gap, usable.y + gap);
            wlr_xdg_toplevel_set_size(view->xdg_toplevel,
                usable.width - 2 * gap, usable.height - 2 * gap);
            wlr_scene_node_set_enabled(&view->scene_tree->node, true);
        } else {
            /* Hide other windows */
            wlr_scene_node_set_enabled(&view->scene_tree->node, false);
        }
    }
}

/*
 * Apply layout to workspace
 * This repositions all windows according to the active layout
 */
void layout_apply(struct ocwm_workspace *workspace) {
    if (!workspace || !workspace->visible) {
        return;
    }

    switch (workspace->layout) {
    case LAYOUT_FLOATING:
        layout_floating(workspace);
        break;
    case LAYOUT_MASTER_STACK:
        layout_master_stack(workspace);
        break;
    case LAYOUT_GRID:
        layout_grid(workspace);
        break;
    case LAYOUT_MONOCLE:
        layout_monocle(workspace);
        break;
    default:
        wlr_log(WLR_ERROR, "Unknown layout type: %d", workspace->layout);
        break;
    }
}
