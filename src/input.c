#include <river-input-management-v1-client-protocol.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "input.h"

struct river_input_manager_v1 *river_input_manager;
const struct river_input_manager_v1_listener river_input_manager_listener = {
    .finished = river_input_manager_handle_finished,
    .input_device = river_input_manager_handle_input_device,
};

const struct river_input_device_v1_listener river_input_device_listener = {
    .removed = river_input_device_handle_removed,
    .type = river_input_device_handle_type,
    .name = river_input_device_handle_name,
    .done = river_input_device_handle_done,
};

void river_input_device_handle_removed(void *data,
                                       struct river_input_device_v1 *device) {
    (void)device;

    fprintf(stdout, "river_input_device_handle_removed\n");
    struct RiverInputDevice *rdevice = data;

    river_input_device_v1_destroy(rdevice->device);
    wl_list_remove(&rdevice->link);
    free(rdevice);
}

void river_input_device_handle_type(void *data,
                                    struct river_input_device_v1 *device,
                                    uint32_t type) {
    (void)device;

    fprintf(stdout, "river_input_device_handle_type: %u\n", type);

    struct RiverInputDevice *rdevice = data;
    rdevice->type = type;
}

void river_input_device_handle_name(void *data,
                                    struct river_input_device_v1 *device,
                                    const char *name) {
    (void)device;

    fprintf(stdout, "river_input_device_handle_name: %s\n", name);

    struct RiverInputDevice *rdevice = data;
    rdevice->name = strdup(name);
}

void river_input_device_handle_done(void *data,
                                    struct river_input_device_v1 *device) {
    (void)data;
    (void)device;

    fprintf(stdout, "river_input_device_handle_done\n");
}

void river_input_manager_handle_finished(
    void *data, struct river_input_manager_v1 *input_manager) {
    (void)data;
    (void)input_manager;

    fprintf(stdout, "river_input_manager_handle_finished\n");
}

void river_input_manager_handle_input_device(
    void *data, struct river_input_manager_v1 *input_manager,
    struct river_input_device_v1 *input_device) {
    (void)input_manager;

    fprintf(stdout, "river_input_manager_handle_input_device\n");

    if (input_config.repeat_rate > 0 && input_config.repeat_delay > 0) {
        river_input_device_v1_set_repeat_info(
            input_device, input_config.repeat_rate, input_config.repeat_delay);
    }

    river_input_device_v1_add_listener(input_device,
                                       &river_input_device_listener, data);
}
