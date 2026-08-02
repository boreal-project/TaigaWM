#include <river-layer-shell-v1-client-protocol.h>
#include <stdio.h>
#include <wayland-util.h>

#include "layer_shell.h"

#include "output.h"
#include "seat.h"
#include "wm.h"

struct river_layer_shell_v1 *layer_shell;

const struct river_layer_shell_seat_v1_listener layer_shell_seat_listener = {
    .focus_exclusive = layer_shell_handle_focus_exclusive,
    .focus_non_exclusive = layer_shell_handle_focus_non_exclusive,
    .focus_none = layer_shell_handle_focus_none,
};

const struct river_layer_shell_output_v1_listener layer_shell_output_listener =
    {
        .non_exclusive_area = layer_shell_handle_non_exclusive_area,
};

void layer_shell_handle_focus_none(void *data,
                                   struct river_layer_shell_seat_v1 *seat) {
    (void)data;
    (void)seat;

    fprintf(stdout, "layer_shell_handle_focus_none\n");

    struct Seat *s, *s_tmp;
    wl_list_for_each_safe(s, s_tmp, &wm.seats, link) { s->focused = NULL; }
}

void layer_shell_handle_non_exclusive_area(
    void *data, struct river_layer_shell_output_v1 *output, int x, int y,
    int width, int height) {
    (void)output;

    fprintf(stdout, "layer_shell_handle_non_exclusive_area\n");

    struct Output *tmp_output = data;

    // set non exclusive w,h,x,y
    tmp_output->neposx = x;
    tmp_output->neposy = y;
    tmp_output->newidth = width;
    tmp_output->neheight = height;
}

void layer_shell_handle_focus_exclusive(
    void *data, struct river_layer_shell_seat_v1 *seat) {}
void layer_shell_handle_focus_non_exclusive(
    void *data, struct river_layer_shell_seat_v1 *seat) {}
