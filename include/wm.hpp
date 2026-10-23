#pragma once

#include <list>
#include <memory>

#include <river-input-management-v1-client-protocol.h>
#include <river-layer-shell-v1-client-protocol.h>
#include <river-libinput-config-v1-client-protocol.h>
#include <river-window-management-v1-client-protocol.h>
#include <river-xkb-bindings-v1-client-protocol.h>
#include <river-xkb-config-v1-client-protocol.h>
#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>

#include "input.hpp"
#include "output.hpp"
#include "seat.hpp"
#include "window.hpp"

struct Globals {
    wl_display *display = nullptr;
    river_window_manager_v1 *windowManager = nullptr;
    river_xkb_bindings_v1 *xkbBindings = nullptr;
    river_layer_shell_v1 *layerShell = nullptr;
    river_libinput_config_v1 *libinputConfig = nullptr;
    river_input_manager_v1 *inputManager = nullptr;
    river_xkb_config_v1 *xkbConfig = nullptr;
    xkb_context *xkbContext = nullptr;
};

class WindowManager {
  public:
    bool connect();
    int run();

    Seat *firstSeat();
    Output *firstOutput();
    Output *focusedOutput();
    Output *outputAt(int32_t x, int32_t y);

    void focusFirstWindowWithTag(Output &output);
    void focusMonitor(bool forward);

    Globals globals;
    std::list<std::unique_ptr<Output>> outputs;
    std::list<std::unique_ptr<Window>> windows;
    std::list<std::unique_ptr<Seat>> seats;
    std::list<std::unique_ptr<InputDevice>> inputDevices;
    bool layerShellHasDefaultOutput = false;

    void manageStart();
    void renderStart();
    void addWindow(river_window_v1 *obj);
    void addOutput(river_output_v1 *obj);
    void addSeat(river_seat_v1 *obj);

  private:
    void collectRemoved();
    bool dispatchOnce(int inotifyFd);
};

extern WindowManager wm;
extern const river_window_manager_v1_listener windowManagerListener;
