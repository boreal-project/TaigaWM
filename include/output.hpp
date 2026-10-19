#pragma once

#include <cstdint>

#include <river-layer-shell-v1-client-protocol.h>
#include <river-window-management-v1-client-protocol.h>

struct Rect {
    int32_t x = 0;
    int32_t y = 0;
    int32_t width = 0;
    int32_t height = 0;
};

struct Output {
    explicit Output(river_output_v1 *obj) : obj(obj) {}

    bool contains(int32_t px, int32_t py) const {
        return px >= area.x && px < area.x + area.width && py >= area.y &&
               py < area.y + area.height;
    }

    river_output_v1 *obj;
    river_layer_shell_output_v1 *layerShellOutput = nullptr;
    bool removed = false;
    Rect area;
    Rect usable;
    uint32_t tag = 0;
};

extern const river_output_v1_listener outputListener;
extern const river_layer_shell_output_v1_listener layerShellOutputListener;
