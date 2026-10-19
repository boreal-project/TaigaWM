#pragma once

#include <memory>
#include <string>

#include <river-xkb-bindings-v1-client-protocol.h>
#include <river-xkb-config-v1-client-protocol.h>
#include <xkbcommon/xkbcommon.h>

#include "actions.hpp"

struct Seat;

struct XkbBinding {
    river_xkb_binding_v1 *obj = nullptr;
    Seat *seat = nullptr;
    Action action = Action::None;
    std::string arg;

    void destroy();
};

extern const river_xkb_binding_v1_listener xkbBindingListener;
extern const river_xkb_config_v1_listener xkbConfigListener;

std::unique_ptr<XkbBinding> createXkbBinding(Seat &seat, uint32_t mods,
                                             xkb_keysym_t keysym, Action action,
                                             const std::string &arg);
