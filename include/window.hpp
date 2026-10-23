#pragma once

#include <cstdint>

#include <river-window-management-v1-client-protocol.h>

struct Output;
struct Seat;

struct Window {
    explicit Window(river_window_v1 *obj);

    bool isFocused() const;
    void setPosition(int32_t x, int32_t y);
    void setBorders();
    void enterFullscreen();
    void exitFullscreen();
    void maximize();
    void unmaximize();
    void manage();

    river_window_v1 *obj;
    river_node_v1 *node;
    Output *output = nullptr;

    bool isNew = true;
    bool closed = false;
    bool fullscreen = false;
    bool maximized = false;

    int32_t x = 0;
    int32_t y = 0;
    int32_t oldX = 0;
    int32_t oldY = 0;
    int32_t width = 0;
    int32_t height = 0;

    Seat *pointerMoveRequested = nullptr;
    Seat *pointerResizeRequested = nullptr;
    uint32_t pointerResizeEdges = 0;

    uint32_t tag = 0;
};

extern const river_window_v1_listener windowListener;
