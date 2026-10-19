#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <river-layer-shell-v1-client-protocol.h>
#include <river-window-management-v1-client-protocol.h>

#include "actions.hpp"
#include "pointer.hpp"
#include "xkb.hpp"

struct Output;
struct Window;

enum class SeatOp {
    None,
    Move,
    Resize,
};

struct Seat {
    explicit Seat(river_seat_v1 *obj) : obj(obj) {}

    void focus(Window *window);
    void pointerMove(Window *window);
    void pointerResize(Window *window, uint32_t edges);
    void performAction(Action action);
    void manage();
    void render();
    void applyConfig();
    void clearBindings();
    void destroy();

    river_seat_v1 *obj;
    river_layer_shell_seat_v1 *layerShellSeat = nullptr;
    bool isNew = true;
    bool removed = false;

    Window *focused = nullptr;
    Window *hovered = nullptr;
    Window *interacted = nullptr;
    Output *focusedOutput = nullptr;

    std::vector<std::unique_ptr<XkbBinding>> xkbBindings;
    std::vector<std::unique_ptr<PointerBinding>> pointerBindings;

    Action pendingAction = Action::None;
    std::string pendingArg;

    SeatOp op = SeatOp::None;
    Window *opWindow = nullptr;
    int32_t opStartX = 0;
    int32_t opStartY = 0;
    int32_t opDx = 0;
    int32_t opDy = 0;
    bool opRelease = false;
    int32_t opStartWidth = 0;
    int32_t opStartHeight = 0;
    uint32_t opEdges = 0;

    int32_t cursorX = 0;
    int32_t cursorY = 0;
};

extern const river_seat_v1_listener seatListener;
extern const river_layer_shell_seat_v1_listener layerShellSeatListener;
