/*
 * Visual effects - Opacity, blur, and animation triggers
 */

#include "../ocwm.h"

/*
 * Initialize effects system
 */
void effects_init(struct ocwm_server *server) {
    /* Initialize animation system */
    animation_init(server);

    wlr_log(WLR_INFO, "Effects system initialized");
}

/*
 * Set window opacity
 */
void view_set_opacity(struct ocwm_view *view, float opacity) {
    if (!view) {
        return;
    }

    /* Clamp to 0.0 - 1.0 */
    if (opacity < 0.0f) opacity = 0.0f;
    if (opacity > 1.0f) opacity = 1.0f;

    view->opacity = opacity;

    /* wlroots scene graph will handle actual rendering with opacity */
    /* We set the alpha through scene node properties */
    /* Note: Full opacity support requires additional wlroots integration */

    wlr_log(WLR_DEBUG, "Set window opacity: %.2f", opacity);
}

/*
 * Set window blur
 */
void view_set_blur(struct ocwm_view *view, bool enabled) {
    if (!view) {
        return;
    }

    view->blur = enabled;

    /* Blur shader would be applied during rendering */
    /* For now we just track the state */

    wlr_log(WLR_DEBUG, "Set window blur: %s", enabled ? "enabled" : "disabled");
}

/*
 * Animate window opening
 */
void view_animate_open(struct ocwm_view *view) {
    if (!view || !view->server->effects_enabled) {
        /* No animation, just show immediately */
        if (view) {
            view->opacity = 1.0f;
            view->scale = 1.0f;
        }
        return;
    }

    /* Create open animation */
    struct ocwm_animation *anim = animation_create(view,
        ANIM_WINDOW_OPEN, view->server->anim_duration_open);

    if (anim) {
        anim->easing = EASING_EASE_OUT;
    }

    wlr_log(WLR_DEBUG, "Started window open animation");
}

/*
 * Animate window closing
 */
void view_animate_close(struct ocwm_view *view) {
    if (!view || !view->server->effects_enabled) {
        /* No animation, just hide immediately */
        if (view) {
            wlr_scene_node_set_enabled(&view->scene_tree->node, false);
        }
        return;
    }

    /* Create close animation */
    struct ocwm_animation *anim = animation_create(view,
        ANIM_WINDOW_CLOSE, view->server->anim_duration_close);

    if (anim) {
        anim->easing = EASING_EASE_IN_OUT;
    }

    wlr_log(WLR_DEBUG, "Started window close animation");
}
