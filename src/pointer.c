#include <river-window-management-v1-client-protocol.h>
#include <stdlib.h>
#include <stdio.h>

#include "pointer.h"
#include "seat.h"

void pointer_binding_handle_pressed(void *data,
                                    struct river_pointer_binding_v1 *obj) {
    (void)obj;

    fprintf(stdout, "pointer_binding_handle_pressed\n");

    struct PointerBinding *binding = data;
    binding->seat->pending_action = binding->action;
}

void pointer_binding_handle_released(void *data,
                                     struct river_pointer_binding_v1 *obj) {}

const struct river_pointer_binding_v1_listener river_pointer_binding_listener =
    {
        .pressed = pointer_binding_handle_pressed,
        .released = pointer_binding_handle_released,
};

void pointer_binding_destroy(struct PointerBinding *binding) {
    fprintf(stdout, "pointer_binding_destroy\n");

    river_pointer_binding_v1_destroy(binding->obj);
    wl_list_remove(&binding->link);
    free(binding);
}

void pointer_binding_create(struct Seat *seat, uint32_t mods, uint32_t button,
                            enum Action action) {
    fprintf(stdout, "pointer_binding_create\n");

    struct PointerBinding *binding = calloc(1, sizeof(struct PointerBinding));
    binding->obj = river_seat_v1_get_pointer_binding(seat->obj, button, mods);
    binding->seat = seat;
    binding->action = action;

    river_pointer_binding_v1_add_listener(
        binding->obj, &river_pointer_binding_listener, binding);
    river_pointer_binding_v1_enable(binding->obj);

    wl_list_insert(seat->pointer_bindings.prev, &binding->link);
}
