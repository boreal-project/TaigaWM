#include <river-window-management-v1-client-protocol.h>
#include <stdio.h>
#include <stdlib.h>

#include "output.h"
#include "seat.h"
#include "window.h"
#include "wm.h"

const struct river_output_v1_listener river_output_listener = {
    .removed = output_handle_removed,
    .wl_output = output_handle_wl_output,
    .position = output_handle_position,
    .dimensions = output_handle_dimensions,
};

void output_handle_removed(void *data, struct river_output_v1 *obj) {
    (void)obj;

    fprintf(stdout, "output_handle_removed\n");
    struct Output *output = data;
    output->removed = true;
}

void output_maybe_destroy(struct Output *output) {
    if (!output->removed) {
        return;
    }

    fprintf(stdout, "output_maybe_destroy\n");
    river_output_v1_destroy(output->obj);
    wl_list_remove(&output->link);
    free(output);
}

void output_handle_dimensions(void *data, struct river_output_v1 *obj,
                              int32_t width, int32_t height) {
    (void)obj;

    fprintf(stdout, "output_handle_dimensions: %dx%d.\n", width, height);

    struct Output *output = data;
    output->width = width;
    output->height = height;
}

void output_handle_wl_output(void *data, struct river_output_v1 *obj,
                             uint32_t name) {
    (void)data;
    (void)obj;
    (void)name;

    fprintf(stdout, "output_handle_wl_output\n");
}

void output_handle_position(void *data, struct river_output_v1 *obj, int32_t x,
                            int32_t y) {
    (void)obj;

    fprintf(stdout, "output_handle_position: %dx%d.\n", x, y);
    struct Output *output = data;
    output->posx = x;
    output->posy = y;
}

struct Output *get_focused_output(void) {
    fprintf(stdout, "get_focused_output\n");

    if (wl_list_empty(&wm.seats)) {
        return NULL;
    }

    // This just gets the first seat
    struct Seat *seat = wl_container_of(wm.seats.next, seat, link);

    // Maths magic for detecting focus
    struct Output *output;
    wl_list_for_each(output, &wm.outputs, link) {
        int32_t within_x = seat->cur_ptr_posx >= output->posx &&
                           seat->cur_ptr_posx < output->posx + output->width;
        int32_t within_y = seat->cur_ptr_posy >= output->posy &&
                           seat->cur_ptr_posy < output->posy + output->height;

        if (within_x && within_y) {
            return output;
        }
    }

    return NULL;
}

struct Output *get_output_at_position(int x, int y) {
    fprintf(stdout, "get_output_at_position\n");

    struct Output *output;
    wl_list_for_each(output, &wm.outputs, link) {
        if (x >= output->posx && x < output->posx + output->width &&
            y >= output->posy && y < output->posy + output->height) {
            return output;
        }
    }
    return NULL;
}

void focus_first_window_on_output(struct Output *output) {
    fprintf(stdout, "focus_first_window_on_output\n");

    struct Window *tmp_window;
    struct Seat *seat = wl_container_of(wm.seats.next, seat, link);

    wl_list_for_each(tmp_window, &wm.windows, link) {
        if (tmp_window->output == output &&
            tmp_window->tag_id == output->tag_id) {
            seat_focus(seat, tmp_window);
            break;
        }
    }
}

void focus_mon_next(void) {
    fprintf(stdout, "focus_mon_next\n");

    struct Seat *seat = wl_container_of(wm.seats.next, seat, link);
    struct Output *tmp_output;
    struct Output *focused_output = get_focused_output();
    wl_list_for_each(tmp_output, &wm.outputs, link) {
        if (tmp_output != focused_output) {
            river_seat_v1_pointer_warp(
                seat->obj, tmp_output->posx + tmp_output->width / 2,
                tmp_output->posy + tmp_output->height / 2);
            focus_first_window_on_output(tmp_output);
            break;
        }
    }
}

void focus_mon_prev(void) {
    fprintf(stdout, "focus_mon_prev\n");

    struct Seat *seat = wl_container_of(wm.seats.next, seat, link);
    struct Output *tmp_output;
    struct Output *focused_output = get_focused_output();
    wl_list_for_each_reverse(tmp_output, &wm.outputs, link) {
        if (tmp_output != focused_output) {
            river_seat_v1_pointer_warp(
                seat->obj, tmp_output->posx + tmp_output->width / 2,
                tmp_output->posy + tmp_output->height / 2);
            focus_first_window_on_output(tmp_output);
            break;
        }
    }
}
