#include "output.hpp"
#include "seat.hpp"
#include "util.hpp"
#include "wm.hpp"

const river_layer_shell_output_v1_listener layerShellOutputListener = {
    .non_exclusive_area =
        [](void *data, river_layer_shell_output_v1 *, int32_t x, int32_t y,
           int32_t width, int32_t height) {
            static_cast<Output *>(data)->usable = {x, y, width, height};
        },
};

const river_layer_shell_seat_v1_listener layerShellSeatListener = {
    .focus_exclusive = noop,
    .focus_non_exclusive = noop,
    .focus_none =
        [](void *, river_layer_shell_seat_v1 *) {
            for (auto &seat : wm.seats) {
                seat->focused = nullptr;
            }
        },
};
