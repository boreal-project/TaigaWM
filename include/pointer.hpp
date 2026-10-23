#pragma once

#include <memory>

#include <river-window-management-v1-client-protocol.h>

#include "actions.hpp"

struct Seat;

struct PointerBinding {
    river_pointer_binding_v1 *obj = nullptr;
    Seat *seat = nullptr;
    Action action = Action::None;

    void destroy();
};

extern const river_pointer_binding_v1_listener pointerBindingListener;

std::unique_ptr<PointerBinding>
createPointerBinding(Seat &seat, uint32_t mods, uint32_t button, Action action);
