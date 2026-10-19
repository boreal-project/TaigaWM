#include "seat.hpp"

#include <linux/input-event-codes.h>

#include <algorithm>
#include <cstdlib>

#include "autostart.hpp"
#include "config.hpp"
#include "output.hpp"
#include "util.hpp"
#include "window.hpp"
#include "wm.hpp"

namespace {

Seat *self(void *data) { return static_cast<Seat *>(data); }

Window *windowOf(river_window_v1 *window) {
    return static_cast<Window *>(river_window_v1_get_user_data(window));
}

void fallbackKeybinds(Seat &seat) {
    constexpr uint32_t super = RIVER_SEAT_V1_MODIFIERS_MOD4;
    auto add = [&](xkb_keysym_t key, Action action, const char *arg) {
        seat.xkbBindings.push_back(
            createXkbBinding(seat, super, key, action, arg));
    };
    add(XKB_KEY_Return, Action::SpawnSh, "foot");
    add(XKB_KEY_d, Action::SpawnSh, "rofi -show drun");
    add(XKB_KEY_q, Action::Close, "");
    add(XKB_KEY_n, Action::FocusNext, "");
    add(XKB_KEY_Escape, Action::Exit, "");
}

void fallbackPointerBinds(Seat &seat) {
    constexpr uint32_t super = RIVER_SEAT_V1_MODIFIERS_MOD4;
    seat.pointerBindings.push_back(
        createPointerBinding(seat, super, BTN_LEFT, Action::Move));
    seat.pointerBindings.push_back(
        createPointerBinding(seat, super, BTN_RIGHT, Action::Resize));
}

} // namespace

const river_seat_v1_listener seatListener = {
    .removed = [](void *data, river_seat_v1 *) { self(data)->removed = true; },
    .wl_seat = noop,
    .pointer_enter =
        [](void *data, river_seat_v1 *, river_window_v1 *window) {
            self(data)->hovered = windowOf(window);
        },
    .pointer_leave = [](void *data,
                        river_seat_v1 *) { self(data)->hovered = nullptr; },
    .window_interaction =
        [](void *data, river_seat_v1 *, river_window_v1 *window) {
            self(data)->interacted = windowOf(window);
        },
    .shell_surface_interaction = noop,
    .op_delta =
        [](void *data, river_seat_v1 *, int32_t dx, int32_t dy) {
            self(data)->opDx = dx;
            self(data)->opDy = dy;
        },
    .op_release = [](void *data,
                     river_seat_v1 *) { self(data)->opRelease = true; },
    .pointer_position =
        [](void *data, river_seat_v1 *, int32_t x, int32_t y) {
            Seat *seat = self(data);
            seat->cursorX = x;
            seat->cursorY = y;

            Output *current = wm.focusedOutput();
            if (current != seat->focusedOutput) {
                seat->focusedOutput = current;
                if (current != nullptr) {
                    river_layer_shell_output_v1_set_default(
                        current->layerShellOutput);
                }
            }
        },
};

void Seat::clearBindings() {
    for (auto &binding : xkbBindings) {
        binding->destroy();
    }
    xkbBindings.clear();
    for (auto &binding : pointerBindings) {
        binding->destroy();
    }
    pointerBindings.clear();
}

void Seat::destroy() {
    clearBindings();
    if (layerShellSeat != nullptr) {
        river_layer_shell_seat_v1_destroy(layerShellSeat);
    }
    river_seat_v1_destroy(obj);
}

void Seat::focus(Window *window) {
    if (window == nullptr && !wm.windows.empty()) {
        if (Output *output = wm.focusedOutput()) {
            for (auto it = wm.windows.rbegin(); it != wm.windows.rend(); ++it) {
                if ((*it)->tag == output->tag && (*it)->output == output) {
                    window = it->get();
                    break;
                }
            }
        }
        if (window == nullptr) {
            window = wm.windows.back().get();
        }
    }

    if (focused == window) {
        return;
    }

    if (window != nullptr) {
        river_seat_v1_focus_window(obj, window->obj);
        river_node_v1_place_top(window->node);
        auto it = std::find_if(
            wm.windows.begin(), wm.windows.end(),
            [window](const auto &entry) { return entry.get() == window; });
        if (it != wm.windows.end()) {
            wm.windows.splice(wm.windows.end(), wm.windows, it);
        }
    } else {
        river_seat_v1_clear_focus(obj);
    }

    focused = window;
}

void Seat::pointerMove(Window *window) {
    focus(window);
    river_seat_v1_op_start_pointer(obj);
    op = SeatOp::Move;
    opWindow = window;
    opStartX = window->x;
    opStartY = window->y;
    opDx = 0;
    opDy = 0;
}

void Seat::pointerResize(Window *window, uint32_t edges) {
    focus(window);
    river_window_v1_inform_resize_start(window->obj);
    river_seat_v1_op_start_pointer(obj);
    op = SeatOp::Resize;
    opWindow = window;
    opEdges = edges;
    opStartX = window->x;
    opStartY = window->y;
    opStartWidth = window->width;
    opStartHeight = window->height;
    opDx = 0;
    opDy = 0;
}

void Seat::performAction(Action action) {
    Output *output = wm.focusedOutput();
    Window *window = focused;

    switch (action) {
    case Action::None:
        break;
    case Action::SpawnSh:
        spawnShell(pendingArg);
        break;
    case Action::Close:
        if (window != nullptr) {
            river_window_v1_close(window->obj);
        }
        break;
    case Action::FocusNext:
        if (!wm.windows.empty()) {
            focus(wm.windows.front().get());
        }
        break;
    case Action::Move:
        if (op == SeatOp::None && hovered != nullptr) {
            pointerMove(hovered);
        }
        break;
    case Action::Resize:
        if (op == SeatOp::None && hovered != nullptr) {
            pointerResize(hovered, RIVER_WINDOW_V1_EDGES_BOTTOM |
                                       RIVER_WINDOW_V1_EDGES_RIGHT);
        }
        break;
    case Action::Exit:
        river_window_manager_v1_exit_session(wm.globals.windowManager);
        break;
    case Action::Fullscreen:
        if (window != nullptr && output != nullptr) {
            if (window->fullscreen) {
                window->exitFullscreen();
            } else {
                window->enterFullscreen();
            }
        }
        break;
    case Action::Maximize:
        if (window != nullptr) {
            if (window->maximized) {
                window->unmaximize();
            } else {
                window->maximize();
            }
        }
        break;
    case Action::TagInc:
        if (output != nullptr) {
            output->tag++;
            wm.focusFirstWindowWithTag(*output);
        }
        break;
    case Action::TagDec:
        if (output != nullptr && output->tag > 0) {
            output->tag--;
            wm.focusFirstWindowWithTag(*output);
        }
        break;
    case Action::WinTagInc:
        if (window != nullptr) {
            window->tag++;
        }
        break;
    case Action::WinTagDec:
        if (window != nullptr && window->tag > 0) {
            window->tag--;
        }
        break;
    case Action::TagSet:
        if (output != nullptr) {
            output->tag = static_cast<uint32_t>(std::atoi(pendingArg.c_str()));
            wm.focusFirstWindowWithTag(*output);
        }
        break;
    case Action::WinTagSet:
        if (window != nullptr) {
            window->tag = static_cast<uint32_t>(std::atoi(pendingArg.c_str()));
        }
        break;
    case Action::FocusMonNext:
        wm.focusMonitor(true);
        break;
    case Action::FocusMonPrev:
        wm.focusMonitor(false);
        break;
    }
}

void Seat::applyConfig() {
    isNew = false;
    clearBindings();

    if (config.keybinds) {
        for (const std::string &line : *config.keybinds) {
            if (auto spec = parseKeybind(line)) {
                xkbBindings.push_back(createXkbBinding(
                    *this, spec->mods, spec->keysym, spec->action, spec->arg));
            }
        }
    } else {
        fallbackKeybinds(*this);
    }

    if (config.pointerBinds) {
        for (const std::string &line : *config.pointerBinds) {
            if (auto spec = parsePointerBind(line)) {
                pointerBindings.push_back(createPointerBinding(
                    *this, spec->mods, spec->button, spec->action));
            }
        }
    } else {
        fallbackPointerBinds(*this);
    }

    if (!config.misc.xcursorTheme.empty()) {
        if (config.misc.xcursorSize == 0) {
            diag::warn("falling back to xcursor_size = 24");
            config.misc.xcursorSize = 24;
        }
        river_seat_v1_set_xcursor_theme(obj, config.misc.xcursorTheme.c_str(),
                                        config.misc.xcursorSize);
    }

    for (auto &window : wm.windows) {
        window->setBorders();
    }
}

void Seat::manage() {
    if (isNew) {
        applyConfig();
    }

    focus(interacted);
    interacted = nullptr;

    performAction(pendingAction);
    pendingAction = Action::None;

    switch (op) {
    case SeatOp::None:
        break;
    case SeatOp::Move:
        if (opRelease) {
            const int32_t finalX = opStartX + opDx;
            const int32_t finalY = opStartY + opDy;

            Output *target = wm.outputAt(finalX, finalY);
            if (target != nullptr && target != opWindow->output) {
                opWindow->output = target;
                opWindow->tag = target->tag;
            }

            river_seat_v1_op_end(obj);
            op = SeatOp::None;
            opWindow = nullptr;
        }
        break;
    case SeatOp::Resize: {
        if (opRelease) {
            river_window_v1_inform_resize_end(opWindow->obj);
            river_seat_v1_op_end(obj);
            op = SeatOp::None;
            opWindow = nullptr;
            break;
        }
        int32_t width = opStartWidth;
        int32_t height = opStartHeight;
        if (opEdges & RIVER_WINDOW_V1_EDGES_LEFT) {
            width -= opDx;
        }
        if (opEdges & RIVER_WINDOW_V1_EDGES_RIGHT) {
            width += opDx;
        }
        if (opEdges & RIVER_WINDOW_V1_EDGES_TOP) {
            height -= opDy;
        }
        if (opEdges & RIVER_WINDOW_V1_EDGES_BOTTOM) {
            height += opDy;
        }
        river_window_v1_propose_dimensions(opWindow->obj, std::max(width, 1),
                                           std::max(height, 1));
        break;
    }
    }
    opRelease = false;
}

void Seat::render() {
    switch (op) {
    case SeatOp::None:
        break;
    case SeatOp::Move:
        opWindow->setPosition(opStartX + opDx, opStartY + opDy);
        break;
    case SeatOp::Resize: {
        int32_t x = opStartX;
        int32_t y = opStartY;
        if (opEdges & RIVER_WINDOW_V1_EDGES_LEFT) {
            x += opStartWidth - opWindow->width;
        }
        if (opEdges & RIVER_WINDOW_V1_EDGES_TOP) {
            y += opStartHeight - opWindow->height;
        }
        opWindow->setPosition(x, y);
        break;
    }
    }
}
