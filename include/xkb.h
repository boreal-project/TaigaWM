#ifndef XKB_H
#define XKB_H

#include <wayland-util.h>

#include <xkbcommon/xkbcommon-keysyms.h>
#include <xkbcommon/xkbcommon.h>

#include "actions.h"

struct XkbBinding {
    struct river_xkb_binding_v1 *obj;
    struct Seat *seat;
    enum Action action;
    char *cmd;
    struct wl_list link;
};

struct RiverXkbDevice {
    struct river_input_device_v1 *obj;
    char *keymap_name;
    uint32_t keymap_index;
};

extern struct xkb_context *xkb_context;
extern struct river_xkb_config_v1 *river_xkb_config;
extern const struct river_xkb_config_v1_listener river_xkb_config_listener;

void river_xkb_config_handle_finished(void *data, struct river_xkb_config_v1 *config);
void river_xkb_config_handle_xkb_keyboard(void * data, struct river_xkb_config_v1 *config,
                                          struct river_xkb_keyboard_v1 *id);

extern struct river_xkb_bindings_v1 *xkb_bindings_v1;
void xkb_binding_handle_pressed(void *data, struct river_xkb_binding_v1 *obj);
void xkb_binding_handle_released(void *data, struct river_xkb_binding_v1 *obj);
void xkb_binding_destroy(struct XkbBinding *binding);
void xkb_binding_create(struct Seat *seat, uint32_t mods, xkb_keysym_t keysym,
                        enum Action action, char *cmd);

#endif
