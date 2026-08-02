#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-util.h>

#include "autostart.h"
#include "config.h"
#include "layer_shell.h"
#include "window.h"
#include "wm.h"
#include "xkb.h"

static volatile sig_atomic_t config_changed = 0;
static void setup_inotify(void) {
    fprintf(stdout, "setup_inotify\n");
    // Fork and wait for config changed
    // On config change send sig handler and exit

    // if the watcher pid changes kill the old one
    static pid_t watcher_pid;
    if (watcher_pid > 0) {
        kill(watcher_pid, SIGKILL);
        watcher_pid = 0;
    }

    const pid_t pid = fork();
    if (pid == 0) {
        const int fd = inotify_init();

        // exit if fd is bad
        if (fd < 0)
            _exit(0);

        char buf[1024];
        inotify_add_watch(fd, config_path, IN_MODIFY);

        while (read(fd, buf, sizeof(buf)) > 0) {
            kill(getppid(), SIGUSR1);
        }

        // close fd and exit
        close(fd);
        _exit(0);
    }

    if (pid > 0)
        watcher_pid = pid;
}

// We do this so the main loop can check if the config was changed, in order to
// setup inotify again
static void handle_config_change(int sig) { config_changed = 1; }

static int compositor_main(void) {
    fprintf(stdout, "compositor_main\n");
    // Sig magic
    struct sigaction sa = {0};
    sa.sa_handler = handle_config_change;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGUSR1, &sa, NULL);

    // Connect to display
    struct wl_display *display = wl_display_connect(NULL);
    if (display == NULL) {
        fprintf(stderr, "err: failed to connect to Wayland server.\n");
        return 1;
    }

    // Remove WAYLAND_DEBUG from our children
    unsetenv("WAYLAND_DEBUG");
    unsetenv("TAIGA_WRAPPED");
    signal(SIGCHLD, SIG_IGN);

    // Registry setup
    struct wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, NULL);
    if (wl_display_roundtrip(display) < 0) {
        fprintf(stderr, "err: roundtrip failed.\n");
        return 1;
    }

    // Check that the compositor is river compatible / is river
    if (window_manager_v1 == NULL || xkb_bindings_v1 == NULL ||
        layer_shell == NULL) {
        fprintf(stderr, "err: compositor is not river compatible.\n");
        return 1;
    }

    // Setup the wm struct, inotify, then setup the wm listener
    wm_init();
    setup_inotify();
    river_window_manager_v1_add_listener(window_manager_v1, &wm_listener, NULL);

    // Execute the autostarts in the main function, cuz it only runs once
    if (autostart_config.autostarts != NULL) {
        autostart(autostart_config.autostarts, autostart_config.autostarts_len);
    }

    while (true) {
        // Dispatch and error handle
        if (wl_display_dispatch(display) < 0) {
            if (errno == EINTR) {
                continue;
            }
            fprintf(stderr, "err: dispatch failed\n");
            return 1;
        }

        // If the config changes set seat_new for all, then setup inotify again
        if (config_changed) {
            fprintf(stdout, "config_changed\n");
            config_changed = 0;
            struct Seat *seat;
            wl_list_for_each(seat, &wm.seats, link) { seat->new = true; }
            setup_inotify();
        }
    }
}

// main is now a wrapper function
int main(int argc, char **argv) {
    // Load the config as the first thing we do
    load_config();

    // if the env var TAIGA_WRAPPED is set, we are running inside river
    if (getenv("TAIGA_WRAPPED") != NULL) {
        compositor_main();
    } else {
        // if we arent running inside river, do that and set taiga wrapped
        char exe_path[PATH_MAX];
        size_t len = 0;

        // try to read our name
        len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
        // if we fail, abort
        if (len <= 0) {
            fprintf(stderr, "err: failed to read /proc/self/exe.\n");
            return 1;
        }
        exe_path[len] = '\0';

        setenv("TAIGA_WRAPPED", "1", 1);
        char *const args[] = {"river", "-c", exe_path, NULL};
        execvp("river", args);

        // should only exec if execvp fails
        perror("execvp");
        return 1;
    }
}
