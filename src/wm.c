#include <complex.h>
#include <river-input-management-v1-client-protocol.h>
#include <river-layer-shell-v1-client-protocol.h>
#include <river-libinput-config-v1-client-protocol.h>
#include <river-window-management-v1-client-protocol.h>
#include <river-xkb-config-v1-client-protocol.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client-protocol.h>
#include <wayland-util.h>

#include "config.h"
#include "input.h"
#include "layer_shell.h"
#include "libinput.h"
#include "output.h"
#include "seat.h"
#include "window.h"
#include "wm.h"
#include "xkb.h"

struct WindowManager wm;

const struct river_window_manager_v1_listener wm_listener = {
    .unavailable = wm_handle_unavailable,
    .finished = wm_handle_finished,
    .manage_start = wm_handle_manage_start,
    .render_start = wm_handle_render_start,
    .session_locked = wm_handle_session_locked,
    .session_unlocked = wm_handle_session_unlocked,
    .window = wm_handle_window,
    .output = wm_handle_output,
    .seat = wm_handle_seat,
};

const struct wl_registry_listener registry_listener = {
    .global = handle_global,
    .global_remove = handle_global_remove,
};

void wm_handle_unavailable(void *data, struct river_window_manager_v1 *obj) {
    (void)data;
    (void)obj;

    fprintf(stderr, "ERROR: Another window manager is already running\n");
    exit(1);
}

void wm_handle_finished(void *data, struct river_window_manager_v1 *obj) {
    (void)data;
    (void)obj;

    exit(0);
}

void wm_handle_manage_start(void *data, struct river_window_manager_v1 *obj) {
    (void)data;
    (void)obj;

    // Destroy closed windows and removed outputs/seats
    struct Output *output, *output_tmp;
    wl_list_for_each_safe(output, output_tmp, &wm.outputs, link) {
        output_maybe_destroy(output);
    }
    struct Window *window, *window_tmp;
    wl_list_for_each_safe(window, window_tmp, &wm.windows, link) {
        window_maybe_destroy(window);
    }
    struct Seat *seat, *seat_tmp;
    wl_list_for_each_safe(seat, seat_tmp, &wm.seats, link) {
        seat_maybe_destroy(seat);
    }

    // Carry out window management policy
    wl_list_for_each(window, &wm.windows, link) { window_manage(window); }
    wl_list_for_each(seat, &wm.seats, link) { seat_manage(seat); }
    river_window_manager_v1_manage_finish(window_manager_v1);
}

void wm_handle_render_start(void *data, struct river_window_manager_v1 *obj) {
    (void)data;
    (void)obj;

    struct Seat *seat;
    wl_list_for_each(seat, &wm.seats, link) { seat_render(seat); }

    river_window_manager_v1_render_finish(window_manager_v1);
}

void wm_handle_window(void *data, struct river_window_manager_v1 *obj,
                      struct river_window_v1 *river_window) {
    (void)data;
    (void)obj;

    struct Window *window = calloc(1, sizeof(struct Window));
    window->obj = river_window;
    window->node = river_window_v1_get_node(window->obj);
    window->new = true;

    river_window_v1_add_listener(window->obj, &river_window_listener, window);

    wl_list_insert(wm.windows.prev, &window->link);
}

void wm_handle_output(void *data, struct river_window_manager_v1 *obj,
                      struct river_output_v1 *river_output) {
    (void)data;
    (void)obj;

    struct Output *output = calloc(1, sizeof(struct Output));
    output->obj = river_output;
    output->layer_shell_output =
        river_layer_shell_v1_get_output(layer_shell, river_output);

    // attach listener to output, pass output itself to it as the data so we can
    // modify it
    river_layer_shell_output_v1_add_listener(
        output->layer_shell_output, &layer_shell_output_listener, output);

    if (!wm.layer_shell_has_default_output) {
        river_layer_shell_output_v1_set_default(output->layer_shell_output);
        wm.layer_shell_has_default_output = true;
    }

    if (misc_config.tearing) {
        fprintf(stdout, "INFO: tearing enabled.\n");
        river_output_v1_set_presentation_mode(output->obj, 1);
    } else {
        fprintf(stdout, "INFO: tearing disabled.\n");
        river_output_v1_set_presentation_mode(output->obj, 0);
    }

    river_output_v1_add_listener(output->obj, &river_output_listener, output);
    wl_list_insert(wm.outputs.prev, &output->link);
}

void wm_handle_seat(void *data, struct river_window_manager_v1 *obj,
                    struct river_seat_v1 *river_seat) {
    (void)data;
    (void)obj;

    struct Seat *seat = calloc(1, sizeof(struct Seat));
    seat->obj = river_seat;
    seat->new = true;
    seat->layer_shell_seat =
        river_layer_shell_v1_get_seat(layer_shell, seat->obj);
    wl_list_init(&seat->xkb_bindings);
    wl_list_init(&seat->pointer_bindings);

    river_seat_v1_add_listener(seat->obj, &river_seat_listener, seat);
    river_layer_shell_seat_v1_add_listener(seat->layer_shell_seat,
                                           &layer_shell_seat_listener, seat);
    wl_list_insert(wm.seats.prev, &seat->link);
}

// Ignored events
void wm_handle_session_locked(void *data, struct river_window_manager_v1 *obj) {
}
void wm_handle_session_unlocked(void *data,
                                struct river_window_manager_v1 *obj) {}

void wm_init(void) {
    wl_list_init(&wm.outputs);
    wl_list_init(&wm.windows);
    wl_list_init(&wm.seats);
    wl_list_init(&wm.input_devices);
}

void handle_global(void *data, struct wl_registry *registry, uint32_t name,
                   const char *interface, uint32_t version) {
    if (strcmp(interface, river_window_manager_v1_interface.name) == 0) {
        if (version >= 4) {
            window_manager_v1 = wl_registry_bind(
                registry, name, &river_window_manager_v1_interface, 4);
        }
    } else if (strcmp(interface, river_xkb_bindings_v1_interface.name) == 0) {
        xkb_bindings_v1 = wl_registry_bind(registry, name,
                                           &river_xkb_bindings_v1_interface, 1);
    } else if (strcmp(interface, river_layer_shell_v1_interface.name) == 0) {
        layer_shell = wl_registry_bind(registry, name,
                                       &river_layer_shell_v1_interface, 1);
    } else if (strcmp(interface, river_libinput_config_v1_interface.name) ==
               0) {
        river_libinput_config = wl_registry_bind(
            registry, name, &river_libinput_config_v1_interface, 1);
        river_libinput_config_v1_add_listener(
            river_libinput_config, &river_libinput_config_listener, NULL);
    } else if (strcmp(interface, river_input_manager_v1_interface.name) == 0) {
        river_input_manager = wl_registry_bind(
            registry, name, &river_input_manager_v1_interface, 1);

        struct RiverInputDevice *rdevice =
            calloc(1, sizeof(struct RiverInputDevice));
        river_input_manager_v1_add_listener(
            river_input_manager, &river_input_manager_listener, rdevice);
    } else if (strcmp(interface, river_xkb_config_v1_interface.name) == 0) {
        xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
        river_xkb_config =
            wl_registry_bind(registry, name, &river_xkb_config_v1_interface, 1);
        river_xkb_config_v1_add_listener(river_xkb_config,
                                         &river_xkb_config_listener, NULL);
    }
}

void handle_global_remove(void *data, struct wl_registry *registry,
                          uint32_t name) {}
