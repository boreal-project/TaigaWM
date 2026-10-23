#pragma once

#include <cstdint>
#include <string>

#include <river-input-management-v1-client-protocol.h>

struct InputDevice {
    explicit InputDevice(river_input_device_v1 *obj) : obj(obj) {}

    river_input_device_v1 *obj;
    std::string name;
    uint32_t type = 0;
    bool removed = false;
};

extern const river_input_manager_v1_listener inputManagerListener;
extern const river_input_device_v1_listener inputDeviceListener;
