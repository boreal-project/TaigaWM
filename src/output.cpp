#include "output.hpp"

#include "util.hpp"

const river_output_v1_listener outputListener = {
    .removed =
        [](void *data, river_output_v1 *) {
            static_cast<Output *>(data)->removed = true;
        },
    .wl_output = noop,
    .position =
        [](void *data, river_output_v1 *, int32_t x, int32_t y) {
            auto *output = static_cast<Output *>(data);
            output->area.x = x;
            output->area.y = y;
        },
    .dimensions =
        [](void *data, river_output_v1 *, int32_t width, int32_t height) {
            auto *output = static_cast<Output *>(data);
            output->area.width = width;
            output->area.height = height;
        },
};
