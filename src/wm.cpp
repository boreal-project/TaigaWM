#include "wm.hpp"

#include <poll.h>
#include <signal.h>
#include <sys/inotify.h>

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "config.hpp"
#include "libinput.hpp"
#include "util.hpp"

WindowManager wm;

namespace {

const wl_registry_listener registryListener = {
    .global =
        [](void *, wl_registry *registry, uint32_t name, const char *interface,
           uint32_t version) {
            Globals &g = wm.globals;
            const std::string_view iface = interface;

            if (iface == river_window_manager_v1_interface.name) {
                if (version >= 4) {
                    g.windowManager =
                        static_cast<river_window_manager_v1 *>(wl_registry_bind(
                            registry, name, &river_window_manager_v1_interface,
                            4));
                }
            } else if (iface == river_xkb_bindings_v1_interface.name) {
                g.xkbBindings =
                    static_cast<river_xkb_bindings_v1 *>(wl_registry_bind(
                        registry, name, &river_xkb_bindings_v1_interface, 1));
            } else if (iface == river_layer_shell_v1_interface.name) {
                g.layerShell =
                    static_cast<river_layer_shell_v1 *>(wl_registry_bind(
                        registry, name, &river_layer_shell_v1_interface, 1));
            } else if (iface == river_libinput_config_v1_interface.name) {
                g.libinputConfig = static_cast<river_libinput_config_v1 *>(
                    wl_registry_bind(registry, name,
                                     &river_libinput_config_v1_interface, 1));
                river_libinput_config_v1_add_listener(
                    g.libinputConfig, &libinputConfigListener, nullptr);
            } else if (iface == river_input_manager_v1_interface.name) {
                g.inputManager =
                    static_cast<river_input_manager_v1 *>(wl_registry_bind(
                        registry, name, &river_input_manager_v1_interface, 1));
                river_input_manager_v1_add_listener(
                    g.inputManager, &inputManagerListener, nullptr);
            } else if (iface == river_xkb_config_v1_interface.name) {
                g.xkbContext = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
                g.xkbConfig =
                    static_cast<river_xkb_config_v1 *>(wl_registry_bind(
                        registry, name, &river_xkb_config_v1_interface, 1));
                river_xkb_config_v1_add_listener(g.xkbConfig,
                                                 &xkbConfigListener, nullptr);
            }
        },
    .global_remove = noop,
};

constexpr uint32_t kConfigWatchMask =
    IN_CLOSE_WRITE | IN_DELETE_SELF | IN_MOVE_SELF;

bool drainConfigEvents(int fd) {
    alignas(inotify_event) char buffer[4096];
    bool modified = false;
    for (;;) {
        const ssize_t len = read(fd, buffer, sizeof(buffer));
        if (len <= 0) {
            break;
        }
        for (ssize_t offset = 0; offset < len;) {
            const auto *event =
                reinterpret_cast<const inotify_event *>(buffer + offset);
            if (event->mask & kConfigWatchMask) {
                modified = true;
            }
            offset += static_cast<ssize_t>(sizeof(inotify_event) + event->len);
        }
    }
    return modified;
}

void watchConfig(int fd) {
    if (fd >= 0 && !configPath.empty()) {
        inotify_add_watch(fd, configPath.c_str(), kConfigWatchMask);
    }
}

} // namespace

const river_window_manager_v1_listener windowManagerListener = {
    .unavailable =
        [](void *, river_window_manager_v1 *) {
            diag::error("Another window manager is already running");
            std::exit(1);
        },
    .finished =
        [](void *, river_window_manager_v1 *) {
            diag::info("window manager finished");
            std::exit(0);
        },
    .manage_start = [](void *, river_window_manager_v1 *) { wm.manageStart(); },
    .render_start = [](void *, river_window_manager_v1 *) { wm.renderStart(); },
    .session_locked = noop,
    .session_unlocked = noop,
    .window = [](void *, river_window_manager_v1 *,
                 river_window_v1 *window) { wm.addWindow(window); },
    .output = [](void *, river_window_manager_v1 *,
                 river_output_v1 *output) { wm.addOutput(output); },
    .seat = [](void *, river_window_manager_v1 *,
               river_seat_v1 *seat) { wm.addSeat(seat); },
};

bool WindowManager::connect() {
    globals.display = wl_display_connect(nullptr);
    if (globals.display == nullptr) {
        diag::error("failed to connect to Wayland server.");
        return false;
    }

    unsetenv("WAYLAND_DEBUG");
    unsetenv("TAIGA_WRAPPED");
    signal(SIGCHLD, SIG_IGN);

    wl_registry *registry = wl_display_get_registry(globals.display);
    wl_registry_add_listener(registry, &registryListener, nullptr);
    if (wl_display_roundtrip(globals.display) < 0) {
        diag::error("roundtrip failed.");
        return false;
    }

    if (globals.windowManager == nullptr || globals.xkbBindings == nullptr ||
        globals.layerShell == nullptr) {
        diag::error("compositor is not river compatible.");
        return false;
    }

    river_window_manager_v1_add_listener(globals.windowManager,
                                         &windowManagerListener, nullptr);
    return true;
}

int WindowManager::run() {
    UniqueFd inotifyFd(inotify_init1(IN_CLOEXEC | IN_NONBLOCK));
    watchConfig(inotifyFd.get());

    wl_display *display = globals.display;
    bool needWrite = false;

    for (;;) {
        while (wl_display_prepare_read(display) != 0) {
            if (wl_display_dispatch_pending(display) < 0) {
                diag::error("dispatch failed");
                return 1;
            }
        }

        needWrite = false;
        if (wl_display_flush(display) < 0) {
            if (errno != EAGAIN) {
                wl_display_cancel_read(display);
                diag::error("flush failed");
                return 1;
            }
            needWrite = true;
        }

        pollfd fds[2] = {
            {wl_display_get_fd(display),
             static_cast<short>(POLLIN | (needWrite ? POLLOUT : 0)), 0},
            {inotifyFd.get(), POLLIN, 0},
        };
        if (poll(fds, inotifyFd.valid() ? 2 : 1, -1) < 0) {
            wl_display_cancel_read(display);
            if (errno == EINTR) {
                continue;
            }
            diag::error("poll failed");
            return 1;
        }

        if (fds[0].revents & POLLIN) {
            if (wl_display_read_events(display) < 0) {
                diag::error("reading events failed");
                return 1;
            }
        } else {
            wl_display_cancel_read(display);
        }

        if (wl_display_dispatch_pending(display) < 0) {
            diag::error("dispatch failed");
            return 1;
        }

        if (inotifyFd.valid() && (fds[1].revents & POLLIN) &&
            drainConfigEvents(inotifyFd.get())) {
            diag::info("config changed");
            if (loadConfig()) {
                for (auto &seat : seats) {
                    seat->isNew = true;
                }
            }
            watchConfig(inotifyFd.get());
        }
    }
}

Seat *WindowManager::firstSeat() {
    return seats.empty() ? nullptr : seats.front().get();
}

Output *WindowManager::firstOutput() {
    return outputs.empty() ? nullptr : outputs.front().get();
}

Output *WindowManager::outputAt(int32_t x, int32_t y) {
    for (auto &output : outputs) {
        if (output->contains(x, y)) {
            return output.get();
        }
    }
    return nullptr;
}

Output *WindowManager::focusedOutput() {
    const Seat *seat = firstSeat();
    return seat == nullptr ? nullptr : outputAt(seat->cursorX, seat->cursorY);
}

void WindowManager::focusFirstWindowWithTag(Output &output) {
    Seat *seat = firstSeat();
    if (seat == nullptr) {
        return;
    }
    seat->focused = nullptr;
    for (auto &window : windows) {
        if (window->tag == output.tag) {
            seat->focus(window.get());
            break;
        }
    }
}

void WindowManager::focusMonitor(bool forward) {
    Seat *seat = firstSeat();
    if (seat == nullptr) {
        return;
    }

    Output *current = focusedOutput();
    auto pick = [current](auto begin, auto end) -> Output * {
        for (auto it = begin; it != end; ++it) {
            if (it->get() != current) {
                return it->get();
            }
        }
        return nullptr;
    };
    Output *target = forward ? pick(outputs.begin(), outputs.end())
                             : pick(outputs.rbegin(), outputs.rend());
    if (target == nullptr) {
        return;
    }

    river_seat_v1_pointer_warp(seat->obj,
                               target->area.x + target->area.width / 2,
                               target->area.y + target->area.height / 2);
    for (auto &window : windows) {
        if (window->output == target && window->tag == target->tag) {
            seat->focus(window.get());
            break;
        }
    }
}

void WindowManager::collectRemoved() {
    for (auto it = outputs.begin(); it != outputs.end();) {
        Output *output = it->get();
        if (!output->removed) {
            ++it;
            continue;
        }
        for (auto &window : windows) {
            if (window->output == output) {
                window->output = nullptr;
            }
        }
        for (auto &seat : seats) {
            if (seat->focusedOutput == output) {
                seat->focusedOutput = nullptr;
            }
        }
        if (output->layerShellOutput != nullptr) {
            river_layer_shell_output_v1_destroy(output->layerShellOutput);
        }
        river_output_v1_destroy(output->obj);
        it = outputs.erase(it);
    }

    for (auto it = windows.begin(); it != windows.end();) {
        Window *window = it->get();
        if (!window->closed) {
            ++it;
            continue;
        }
        for (auto &seat : seats) {
            if (seat->focused == window) {
                seat->focused = nullptr;
            }
            if (seat->hovered == window) {
                seat->hovered = nullptr;
            }
            if (seat->interacted == window) {
                seat->interacted = nullptr;
            }
            if (seat->opWindow == window) {
                river_seat_v1_op_end(seat->obj);
                seat->op = SeatOp::None;
                seat->opWindow = nullptr;
            }
        }
        river_window_v1_destroy(window->obj);
        it = windows.erase(it);
    }

    for (auto it = seats.begin(); it != seats.end();) {
        Seat *seat = it->get();
        if (!seat->removed) {
            ++it;
            continue;
        }
        for (auto &window : windows) {
            if (window->pointerMoveRequested == seat) {
                window->pointerMoveRequested = nullptr;
            }
            if (window->pointerResizeRequested == seat) {
                window->pointerResizeRequested = nullptr;
            }
        }
        seat->destroy();
        it = seats.erase(it);
    }
}

void WindowManager::manageStart() {
    collectRemoved();

    std::vector<Window *> snapshot;
    snapshot.reserve(windows.size());
    for (auto &window : windows) {
        snapshot.push_back(window.get());
    }
    for (Window *window : snapshot) {
        window->manage();
    }
    for (auto &seat : seats) {
        seat->manage();
    }
    river_window_manager_v1_manage_finish(globals.windowManager);
}

void WindowManager::renderStart() {
    for (auto &seat : seats) {
        seat->render();
    }
    river_window_manager_v1_render_finish(globals.windowManager);
}

void WindowManager::addWindow(river_window_v1 *obj) {
    auto window = std::make_unique<Window>(obj);
    river_window_v1_add_listener(obj, &windowListener, window.get());
    windows.push_back(std::move(window));
}

void WindowManager::addOutput(river_output_v1 *obj) {
    auto output = std::make_unique<Output>(obj);
    output->layerShellOutput =
        river_layer_shell_v1_get_output(globals.layerShell, obj);
    river_layer_shell_output_v1_add_listener(
        output->layerShellOutput, &layerShellOutputListener, output.get());

    if (!layerShellHasDefaultOutput) {
        river_layer_shell_output_v1_set_default(output->layerShellOutput);
        layerShellHasDefaultOutput = true;
    }

    river_output_v1_set_presentation_mode(
        obj, config.misc.tearing ? RIVER_OUTPUT_V1_PRESENTATION_MODE_ASYNC
                                 : RIVER_OUTPUT_V1_PRESENTATION_MODE_VSYNC);

    river_output_v1_add_listener(obj, &outputListener, output.get());
    outputs.push_back(std::move(output));
}

void WindowManager::addSeat(river_seat_v1 *obj) {
    auto seat = std::make_unique<Seat>(obj);
    seat->layerShellSeat =
        river_layer_shell_v1_get_seat(globals.layerShell, obj);

    river_seat_v1_add_listener(obj, &seatListener, seat.get());
    river_layer_shell_seat_v1_add_listener(seat->layerShellSeat,
                                           &layerShellSeatListener, seat.get());
    seats.push_back(std::move(seat));
}
