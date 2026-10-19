#include "input.hpp"

#include <algorithm>

#include "config.hpp"
#include "util.hpp"
#include "wm.hpp"

namespace {

InputDevice *findDevice(void *data) { return static_cast<InputDevice *>(data); }

} // namespace

const river_input_device_v1_listener inputDeviceListener = {
    .removed =
        [](void *data, river_input_device_v1 *) {
            InputDevice *device = findDevice(data);
            river_input_device_v1_destroy(device->obj);
            wm.inputDevices.remove_if(
                [device](const auto &entry) { return entry.get() == device; });
        },
    .type = [](void *data, river_input_device_v1 *,
               uint32_t type) { findDevice(data)->type = type; },
    .name = [](void *data, river_input_device_v1 *,
               const char *name) { findDevice(data)->name = name; },
    .done = noop,
};

const river_input_manager_v1_listener inputManagerListener = {
    .finished = noop,
    .input_device =
        [](void *, river_input_manager_v1 *, river_input_device_v1 *obj) {
            if (config.input.repeatRate > 0 && config.input.repeatDelay > 0) {
                river_input_device_v1_set_repeat_info(
                    obj, config.input.repeatRate, config.input.repeatDelay);
            }
            auto device = std::make_unique<InputDevice>(obj);
            river_input_device_v1_add_listener(obj, &inputDeviceListener,
                                               device.get());
            wm.inputDevices.push_back(std::move(device));
        },
};
