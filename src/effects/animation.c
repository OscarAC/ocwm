/*
 * Animation system - Smooth transitions with easing functions
 */

#include "../ocwm.h"
#include <math.h>

#define PI 3.14159265359

/*
 * Easing functions for smooth animations
 * All take t from 0.0 to 1.0 and return adjusted value 0.0 to 1.0
 */
float easing_function(enum ocwm_easing easing, float t) {
    switch (easing) {
    case EASING_LINEAR:
        return t;

    case EASING_EASE_IN_OUT:
        /* Smooth cubic ease in-out */
        return t < 0.5f
            ? 4.0f * t * t * t
            : 1.0f - powf(-2.0f * t + 2.0f, 3.0f) / 2.0f;

    case EASING_EASE_OUT:
        /* Cubic ease out */
        return 1.0f - powf(1.0f - t, 3.0f);

    case EASING_ELASTIC:
        /* Elastic ease out - bouncy! */
        if (t == 0.0f || t == 1.0f) return t;
        return powf(2.0f, -10.0f * t) * sinf((t * 10.0f - 0.75f) * (2.0f * PI) / 3.0f) + 1.0f;

    case EASING_BOUNCE:
        /* Bounce ease out */
        if (t < (1.0f / 2.75f)) {
            return 7.5625f * t * t;
        } else if (t < (2.0f / 2.75f)) {
            t -= (1.5f / 2.75f);
            return 7.5625f * t * t + 0.75f;
        } else if (t < (2.5f / 2.75f)) {
            t -= (2.25f / 2.75f);
            return 7.5625f * t * t + 0.9375f;
        } else {
            t -= (2.625f / 2.75f);
            return 7.5625f * t * t + 0.984375f;
        }

    default:
        return t;
    }
}

/*
 * Initialize animation system
 */
void animation_init(struct ocwm_server *server) {
    wl_list_init(&server->animations);

    /* Set default effect settings */
    server->effects_enabled = true;
    server->anim_duration_open = 250;   /* 250ms */
    server->anim_duration_close = 200;  /* 200ms */
    server->blur_enabled = false;       /* Disable blur by default (expensive) */
    server->blur_passes = 2;

    wlr_log(WLR_INFO, "Animation system initialized");
}

/*
 * Cleanup animation system
 */
void animation_finish(struct ocwm_server *server) {
    struct ocwm_animation *anim, *tmp;
    wl_list_for_each_safe(anim, tmp, &server->animations, link) {
        animation_destroy(anim);
    }

    wlr_log(WLR_INFO, "Animation system cleaned up");
}

/*
 * Create a new animation
 */
struct ocwm_animation *animation_create(struct ocwm_view *view,
        enum ocwm_animation_type type, uint32_t duration) {
    struct ocwm_animation *anim = calloc(1, sizeof(struct ocwm_animation));
    if (!anim) {
        wlr_log(WLR_ERROR, "Failed to allocate animation");
        return NULL;
    }

    anim->view = view;
    anim->type = type;
    anim->easing = EASING_EASE_OUT;  /* Default easing */
    anim->duration = duration;
    anim->start_time = 0;  /* Will be set on first update */

    /* Set animation parameters based on type */
    switch (type) {
    case ANIM_WINDOW_OPEN:
        anim->start_value = 0.0f;
        anim->end_value = 1.0f;
        anim->current_value = 0.0f;
        break;

    case ANIM_WINDOW_CLOSE:
        anim->start_value = 1.0f;
        anim->end_value = 0.0f;
        anim->current_value = 1.0f;
        break;

    case ANIM_FADE:
    case ANIM_SCALE:
    case ANIM_SLIDE:
        anim->start_value = 0.0f;
        anim->end_value = 1.0f;
        anim->current_value = 0.0f;
        break;
    }

    wl_list_insert(&view->server->animations, &anim->link);

    if (view) {
        view->animating = true;
    }

    return anim;
}

/*
 * Destroy an animation
 */
void animation_destroy(struct ocwm_animation *anim) {
    if (!anim) {
        return;
    }

    if (anim->view) {
        anim->view->animating = false;
    }

    wl_list_remove(&anim->link);
    free(anim);
}

/*
 * Update all animations
 * Called every frame
 */
void animation_update(struct ocwm_server *server, uint32_t time_msec) {
    if (!server->effects_enabled) {
        return;
    }

    struct ocwm_animation *anim, *tmp;
    wl_list_for_each_safe(anim, tmp, &server->animations, link) {
        /* Initialize start time on first update */
        if (anim->start_time == 0) {
            anim->start_time = time_msec;
        }

        /* Calculate progress (0.0 to 1.0) */
        uint32_t elapsed = time_msec - anim->start_time;
        float progress = (float)elapsed / (float)anim->duration;

        if (progress >= 1.0f) {
            /* Animation complete */
            progress = 1.0f;
            anim->current_value = anim->end_value;

            /* Apply final value */
            if (anim->view) {
                switch (anim->type) {
                case ANIM_WINDOW_OPEN:
                    anim->view->opacity = 1.0f;
                    anim->view->scale = 1.0f;
                    break;

                case ANIM_WINDOW_CLOSE:
                    anim->view->opacity = 0.0f;
                    anim->view->scale = 0.8f;
                    /* Hide the window */
                    wlr_scene_node_set_enabled(&anim->view->scene_tree->node, false);
                    break;

                case ANIM_FADE:
                    anim->view->opacity = anim->end_value;
                    break;

                case ANIM_SCALE:
                    anim->view->scale = anim->end_value;
                    break;

                case ANIM_SLIDE:
                    /* Slide animation would adjust position */
                    break;
                }
            }

            /* Call completion callback if set */
            if (anim->on_complete) {
                anim->on_complete(anim);
            }

            /* Destroy the animation */
            animation_destroy(anim);
        } else {
            /* Apply easing function */
            float eased = easing_function(anim->easing, progress);

            /* Interpolate value */
            anim->current_value = anim->start_value +
                (anim->end_value - anim->start_value) * eased;

            /* Apply animation to view */
            if (anim->view) {
                switch (anim->type) {
                case ANIM_WINDOW_OPEN:
                    anim->view->opacity = anim->current_value;
                    /* Scale from 0.8 to 1.0 */
                    anim->view->scale = 0.8f + (anim->current_value * 0.2f);
                    break;

                case ANIM_WINDOW_CLOSE:
                    anim->view->opacity = anim->current_value;
                    /* Scale from 1.0 to 0.8 */
                    anim->view->scale = 1.0f - (anim->current_value * 0.2f);
                    break;

                case ANIM_FADE:
                    anim->view->opacity = anim->current_value;
                    break;

                case ANIM_SCALE:
                    anim->view->scale = anim->current_value;
                    break;

                case ANIM_SLIDE:
                    /* Slide animation would adjust position */
                    break;
                }
            }
        }
    }
}
