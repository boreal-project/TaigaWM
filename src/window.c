#include <river-window-management-v1-client-protocol.h>
#include <stdlib.h>
#include <wayland-util.h>

#include "config.h"
#include "output.h"
#include "seat.h"
#include "window.h"
#include "wm.h"

// fucking bitwise magic
// split the hex into 4 parts, then multiply by 0x01010101u to make it a 32bit
// value
#define HEX_TO_RGBA(packed, R, G, B, A)                                        \
    do {                                                                       \
        uint32_t _p = (packed);                                                \
        uint32_t r8 = (_p >> 24) & 0xFF;                                       \
        uint32_t g8 = (_p >> 16) & 0xFF;                                       \
        uint32_t b8 = (_p >> 8) & 0xFF;                                        \
        uint32_t a8 = (_p >> 0) & 0xFF;                                        \
        uint32_t r_pm8 = (r8 * a8 + 127) / 255;                                \
        uint32_t g_pm8 = (g8 * a8 + 127) / 255;                                \
        uint32_t b_pm8 = (b8 * a8 + 127) / 255;                                \
        (R) = r_pm8 * 0x01010101u;                                             \
        (G) = g_pm8 * 0x01010101u;                                             \
        (B) = b_pm8 * 0x01010101u;                                             \
        (A) = a8 * 0x01010101u;                                                \
    } while (0)

struct river_window_manager_v1 *window_manager_v1;
const struct river_window_v1_listener river_window_listener = {
    .closed = window_handle_closed,
    .dimensions_hint = window_handle_dimensions_hint,
    .dimensions = window_handle_dimensions,
    .app_id = window_handle_app_id,
    .title = window_handle_title,
    .parent = window_handle_parent,
    .decoration_hint = window_handle_decoration_hint,
    .pointer_move_requested = window_handle_pointer_move_requested,
    .pointer_resize_requested = window_handle_pointer_resize_requested,
    .show_window_menu_requested = window_handle_show_window_menu_requested,
    .maximize_requested = window_handle_maximize_requested,
    .unmaximize_requested = window_handle_unmaximize_requested,
    .fullscreen_requested = window_handle_fullscreen_requested,
    .exit_fullscreen_requested = window_handle_exit_fullscreen_requested,
    .minimize_requested = window_handle_minimize_requested,
    .unreliable_pid = window_handle_unreliable_pid,
    .presentation_hint = window_handle_presentation_hint,
    .identifier = window_handle_identifier,
};

void window_handle_closed(void *data, struct river_window_v1 *obj) {
    (void)obj;

    fprintf(stdout, "window_handle_closed\n");

    struct Window *window = data;
    window->closed = true;
}

void window_handle_dimensions(void *data, struct river_window_v1 *obj,
                              int32_t width, int32_t height) {
    (void)obj;

    fprintf(stdout, "window_handle_dimensions\n");

    struct Window *window = data;
    window->width = width;
    window->height = height;
}

void window_handle_pointer_move_requested(void *data,
                                          struct river_window_v1 *obj,
                                          struct river_seat_v1 *river_seat) {
    (void)obj;

    fprintf(stdout, "window_handle_pointer_move_requested\n");

    struct Window *window = data;
    window->pointer_move_requested = river_seat_v1_get_user_data(river_seat);
}

void window_handle_pointer_resize_requested(void *data,
                                            struct river_window_v1 *obj,
                                            struct river_seat_v1 *river_seat,
                                            uint32_t edges) {
    (void)obj;

    fprintf(stdout, "window_handle_pointer_resize_requested\n");

    struct Window *window = data;
    window->pointer_resize_requested = river_seat_v1_get_user_data(river_seat);
    window->pointer_resize_requested_edges = edges;
}

void seat_enter_fullscreen(struct Window *window, struct Output *output) {
    fprintf(stdout, "seat_enter_fullscreen\n");
    window_handle_fullscreen_requested(window, window->obj, output->obj);
}

void seat_exit_fullscreen(struct Window *window) {
    fprintf(stdout, "seat_exit_fullscreen\n");
    window_handle_exit_fullscreen_requested(window, window->obj);
}

void seat_maximize(struct Window *window) {
    fprintf(stdout, "seat_maximize\n");
    window_handle_maximize_requested(window, window->obj);
}

void seat_unmaximize(struct Window *window) {
    fprintf(stdout, "seat_unmaximize\n");
    window_handle_unmaximize_requested(window, window->obj);
}

void window_handle_fullscreen_requested(void *data, struct river_window_v1 *obj,
                                        struct river_output_v1 *river_output) {
    (void)obj;
    (void)river_output;

    fprintf(stdout, "window_handle_fullscreen_requested\n");

    struct Window *window = data;
    if (window->maximized) {
        window_handle_unmaximize_requested(window, window->obj);
    }
    window->fullscreen = true;
    window->oldx = window->x;
    window->oldy = window->y;

    // Inform fullscreen
    river_window_v1_inform_fullscreen(window->obj);

    // set window to fullscreen on focused monitor
    struct Output *output = get_focused_output();
    river_window_v1_propose_dimensions(window->obj, output->width,
                                       output->height);
    window_set_position(window, output->posx, output->posy);
}

void window_handle_exit_fullscreen_requested(void *data,
                                             struct river_window_v1 *obj) {
    (void)obj;

    fprintf(stdout, "window_handle_exit_fullscreen_requested\n");

    struct Window *window = data;
    window->fullscreen = false;

    // Inform not fullscreen
    river_window_v1_inform_not_fullscreen(window->obj);

    // Let window choose its dimensions
    river_window_v1_propose_dimensions(window->obj, 0, 0);

    // Set window to old position before fullscreen
    window_set_position(window, window->oldx, window->oldy);
}

void window_handle_maximize_requested(void *data, struct river_window_v1 *obj) {
    (void)obj;

    fprintf(stdout, "window_handle_maximize_requested\n");

    struct Window *window = data;
    if (window->fullscreen) {
        return;
    }
    window->maximized = true;
    window->oldx = window->x;
    window->oldy = window->y;

    // Inform maximized
    river_window_v1_inform_maximized(window->obj);

    // Get the focused output and maximize it on that output
    struct Output *output = get_focused_output();

    // use non exclusive area so we dont cover bars
    river_window_v1_propose_dimensions(window->obj, output->newidth,
                                       output->neheight);
    window_set_position(window, output->neposx, output->neposy);
}

void window_handle_unmaximize_requested(void *data,
                                        struct river_window_v1 *obj) {
    (void)obj;

    fprintf(stdout, "window_handle_unmaximize_requested\n");

    struct Seat *seat = wl_container_of(wm.seats.next, seat, link);
    struct Window *window = data;
    window->maximized = false;

    // Inform unmax and let the window choose it's WxH
    river_window_v1_inform_unmaximized(window->obj);
    river_window_v1_propose_dimensions(window->obj, 0, 0);

    // Set window 0x0 to cursor pos
    window_set_position(window, seat->cur_ptr_posx, seat->cur_ptr_posy);
}

// Ignored events
void window_handle_dimensions_hint(void *data, struct river_window_v1 *obj,
                                   int32_t min_width, int32_t min_height,
                                   int32_t max_width, int32_t max_height) {}
void window_handle_app_id(void *data, struct river_window_v1 *obj,
                          const char *app_id) {}
void window_handle_title(void *data, struct river_window_v1 *obj,
                         const char *title) {}
void window_handle_parent(void *data, struct river_window_v1 *obj,
                          struct river_window_v1 *parent) {}
void window_handle_decoration_hint(void *data, struct river_window_v1 *obj,
                                   uint32_t hint) {}
void window_handle_show_window_menu_requested(void *data,
                                              struct river_window_v1 *obj,
                                              int32_t x, int32_t y) {}
void window_handle_minimize_requested(void *data, struct river_window_v1 *obj) {
}
void window_handle_unreliable_pid(void *data, struct river_window_v1 *obj,
                                  int32_t unreliable_pid) {}
void window_handle_presentation_hint(void *data, struct river_window_v1 *obj,
                                     uint32_t hint) {}
void window_handle_identifier(void *data, struct river_window_v1 *obj,
                              const char *identifier) {}

void window_maybe_destroy(struct Window *window) {
    if (!window->closed) {
        return;
    }

    fprintf(stdout, "window_maybe_destroy\n");

    struct Seat *seat;
    wl_list_for_each(seat, &wm.seats, link) {
        if (seat->focused == window) {
            seat->focused = NULL;
        }
        if (seat->op_window == window) {
            river_seat_v1_op_end(seat->obj);
            seat->op = SEAT_OP_NONE;
            seat->op_window = NULL;
        }
    }

    river_window_v1_destroy(window->obj);
    wl_list_remove(&window->link);
    free(window);
}

void window_set_position(struct Window *window, int32_t x, int32_t y) {
    fprintf(stdout, "window_set_position\n");

    river_node_v1_set_position(window->node, x, y);
    window->x = x;
    window->y = y;
}

void set_borders(struct Window *window) {
    fprintf(stdout, "set_borders\n");

    uint32_t fr, fg, fb, fa;
    uint32_t ufr, ufg, ufb, ufa;

    if (window->fullscreen || window->maximized) {
        HEX_TO_RGBA(0x00000000, fr, fg, fb, fa);
    } else {
        HEX_TO_RGBA(misc_config.focused_border_color_hex, fr, fg, fb, fa);
    }

    HEX_TO_RGBA(misc_config.unfocused_border_color_hex, ufr, ufg, ufb, ufa);
    struct Seat *seat = wl_container_of(wm.seats.next, seat, link);
    if (seat->focused == window) {
        river_window_v1_set_borders(
            window->obj, 15, (int)misc_config.border_size, fr, fg, fb, fa);
    } else {
        river_window_v1_set_borders(
            window->obj, 15, (int)misc_config.border_size, ufr, ufg, ufb, ufa);
    }
}

void tag_focus_first_window(struct Output *output) {
    fprintf(stdout, "tag_focus_first_window\n");

    struct Window *tmp_window;
    struct Seat *seat = wl_container_of(wm.seats.next, seat, link);
    seat->focused = NULL;
    wl_list_for_each(tmp_window, &wm.windows, link) {
        if (tmp_window->tag_id == output->tag_id) {
            seat_focus(seat, tmp_window);
            break;
        }
    }
}

void output_inc_tag(struct Output *output) {
    fprintf(stdout, "output_inc_tag\n");

    output->tag_id++;
    tag_focus_first_window(output);
}

void output_dec_tag(struct Output *output) {
    if (output->tag_id <= 0) {
        return;
    }

    fprintf(stdout, "output_dec_tag\n");

    output->tag_id--;
    tag_focus_first_window(output);
}

void window_inc_tag(struct Window *window) {
    fprintf(stdout, "window_inc_tag\n");

    window->tag_id++;
}

void window_dec_tag(struct Window *window) {
    if (window->tag_id <= 0) {
        return;
    }

    fprintf(stdout, "window_dec_tag\n");

    window->tag_id--;
}

void output_set_tag(struct Output *output, uint32_t tag) {
    fprintf(stdout, "output_set_tag\n");

    output->tag_id = tag;
    tag_focus_first_window(output);
}

void window_set_tag(struct Window *window, uint32_t tag) {
    fprintf(stdout, "window_set_tag\n");

    window->tag_id = tag;
}

void window_manage(struct Window *window) {
    fprintf(stdout, "window_manage\n");

    if (window->new) {
        window->new = false;
        struct Output *output = get_focused_output();
        window_set_position(window, output->posx + output->width / 3,
                            output->posy + output->height / 5);
        river_window_v1_propose_dimensions(window->obj, 0, 0);
        if (misc_config.client_side_decorations) {
            river_window_v1_use_csd(window->obj);
        } else {
            river_window_v1_use_ssd(window->obj);
        }
        window->output = output;
        window->tag_id = output->tag_id;
    }
    set_borders(window);

    if (window->pointer_move_requested != NULL) {
        if (window->maximized) {
            window_handle_unmaximize_requested(window, window->obj);
        }
        seat_pointer_move(window->pointer_move_requested, window);
        window->pointer_move_requested = NULL;
    }
    if (window->pointer_resize_requested != NULL) {
        if (window->maximized) {
            window_handle_unmaximize_requested(window, window->obj);
        }
        seat_pointer_resize(window->pointer_resize_requested, window,
                            window->pointer_resize_requested_edges);
        window->pointer_resize_requested = NULL;
    }

    // tag magic
    if (window->tag_id != window->output->tag_id) {
        river_window_v1_hide(window->obj);
    } else {
        river_window_v1_show(window->obj);
    }
}
