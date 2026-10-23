#include "window.hpp"

#include "config.hpp"
#include "output.hpp"
#include "seat.hpp"
#include "util.hpp"
#include "wm.hpp"

namespace {

constexpr uint32_t kAllEdges =
    RIVER_WINDOW_V1_EDGES_TOP | RIVER_WINDOW_V1_EDGES_BOTTOM |
    RIVER_WINDOW_V1_EDGES_LEFT | RIVER_WINDOW_V1_EDGES_RIGHT;

struct RiverColor {
    uint32_t r;
    uint32_t g;
    uint32_t b;
    uint32_t a;
};

constexpr RiverColor toRiverColor(uint32_t packed) {
    const uint32_t r8 = (packed >> 24) & 0xFF;
    const uint32_t g8 = (packed >> 16) & 0xFF;
    const uint32_t b8 = (packed >> 8) & 0xFF;
    const uint32_t a8 = packed & 0xFF;
    return {
        ((r8 * a8 + 127) / 255) * 0x01010101u,
        ((g8 * a8 + 127) / 255) * 0x01010101u,
        ((b8 * a8 + 127) / 255) * 0x01010101u,
        a8 * 0x01010101u,
    };
}

Output *targetOutput(Window &window) {
    if (Output *output = wm.focusedOutput()) {
        return output;
    }
    if (window.output != nullptr) {
        return window.output;
    }
    return wm.firstOutput();
}

Window *self(void *data) { return static_cast<Window *>(data); }

Seat *seatOf(river_seat_v1 *seat) {
    return static_cast<Seat *>(river_seat_v1_get_user_data(seat));
}

} // namespace

const river_window_v1_listener windowListener = {
    .closed = [](void *data, river_window_v1 *) { self(data)->closed = true; },
    .dimensions_hint = noop,
    .dimensions =
        [](void *data, river_window_v1 *, int32_t width, int32_t height) {
            self(data)->width = width;
            self(data)->height = height;
        },
    .app_id = noop,
    .title = noop,
    .parent = noop,
    .decoration_hint = noop,
    .pointer_move_requested =
        [](void *data, river_window_v1 *, river_seat_v1 *seat) {
            self(data)->pointerMoveRequested = seatOf(seat);
        },
    .pointer_resize_requested =
        [](void *data, river_window_v1 *, river_seat_v1 *seat, uint32_t edges) {
            self(data)->pointerResizeRequested = seatOf(seat);
            self(data)->pointerResizeEdges = edges;
        },
    .show_window_menu_requested = noop,
    .maximize_requested = [](void *data,
                             river_window_v1 *) { self(data)->maximize(); },
    .unmaximize_requested = [](void *data,
                               river_window_v1 *) { self(data)->unmaximize(); },
    .fullscreen_requested =
        [](void *data, river_window_v1 *, river_output_v1 *) {
            self(data)->enterFullscreen();
        },
    .exit_fullscreen_requested =
        [](void *data, river_window_v1 *) { self(data)->exitFullscreen(); },
    .minimize_requested = noop,
    .unreliable_pid = noop,
    .presentation_hint = noop,
    .identifier = noop,
};

Window::Window(river_window_v1 *obj)
    : obj(obj), node(river_window_v1_get_node(obj)) {}

bool Window::isFocused() const {
    const Seat *seat = wm.firstSeat();
    return seat != nullptr && seat->focused == this;
}

void Window::setPosition(int32_t newX, int32_t newY) {
    river_node_v1_set_position(node, newX, newY);
    x = newX;
    y = newY;
}

void Window::setBorders() {
    uint32_t packed = config.misc.unfocusedBorderColor;
    if (isFocused()) {
        packed = (fullscreen || maximized) ? 0 : config.misc.focusedBorderColor;
    }
    const RiverColor color = toRiverColor(packed);
    river_window_v1_set_borders(obj, kAllEdges,
                                static_cast<int32_t>(config.misc.borderSize),
                                color.r, color.g, color.b, color.a);
}

void Window::enterFullscreen() {
    if (maximized) {
        unmaximize();
    }
    fullscreen = true;
    oldX = x;
    oldY = y;

    river_window_v1_inform_fullscreen(obj);

    Output *target = targetOutput(*this);
    if (target == nullptr) {
        return;
    }
    river_window_v1_propose_dimensions(obj, target->area.width,
                                       target->area.height);
    setPosition(target->area.x, target->area.y);
}

void Window::exitFullscreen() {
    fullscreen = false;

    river_window_v1_inform_not_fullscreen(obj);
    river_window_v1_propose_dimensions(obj, 0, 0);
    setPosition(oldX, oldY);
}

void Window::maximize() {
    if (fullscreen) {
        return;
    }
    maximized = true;
    oldX = x;
    oldY = y;

    river_window_v1_inform_maximized(obj);

    Output *target = targetOutput(*this);
    if (target == nullptr) {
        return;
    }
    river_window_v1_propose_dimensions(obj, target->usable.width,
                                       target->usable.height);
    setPosition(target->usable.x, target->usable.y);
}

void Window::unmaximize() {
    maximized = false;

    river_window_v1_inform_unmaximized(obj);
    river_window_v1_propose_dimensions(obj, 0, 0);

    if (const Seat *seat = wm.firstSeat()) {
        setPosition(seat->cursorX, seat->cursorY);
    }
}

void Window::manage() {
    if (output == nullptr) {
        output = targetOutput(*this);
    }
    if (output == nullptr) {
        return;
    }

    if (isNew) {
        isNew = false;
        setPosition(output->area.x + output->area.width / 3,
                    output->area.y + output->area.height / 5);
        river_window_v1_propose_dimensions(obj, 0, 0);
        if (config.misc.clientSideDecorations) {
            river_window_v1_use_csd(obj);
        } else {
            river_window_v1_use_ssd(obj);
        }
        tag = output->tag;
    }
    setBorders();

    if (pointerMoveRequested != nullptr) {
        if (maximized) {
            unmaximize();
        }
        pointerMoveRequested->pointerMove(this);
        pointerMoveRequested = nullptr;
    }
    if (pointerResizeRequested != nullptr) {
        if (maximized) {
            unmaximize();
        }
        pointerResizeRequested->pointerResize(this, pointerResizeEdges);
        pointerResizeRequested = nullptr;
    }

    if (tag != output->tag) {
        river_window_v1_hide(obj);
    } else {
        river_window_v1_show(obj);
    }
}
