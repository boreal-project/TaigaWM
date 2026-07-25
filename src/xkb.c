#define _GNU_SOURCE

#include <fcntl.h>
#include <river-xkb-bindings-v1-client-protocol.h>
#include <river-xkb-config-v1-client-protocol.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <xkbcommon/xkbcommon.h>

#include "config.h"
#include "seat.h"
#include "xkb.h"

struct xkb_context *xkb_context;
struct river_xkb_bindings_v1 *xkb_bindings_v1;
struct river_xkb_config_v1 *river_xkb_config;

const struct river_xkb_config_v1_listener river_xkb_config_listener = {
    .finished = river_xkb_config_handle_finished,
    .xkb_keyboard = river_xkb_config_handle_xkb_keyboard,
};

// credit to https://codeberg.org/auoggi/anvl
static struct river_xkb_keymap_v1 *
create_keymap(struct river_xkb_config_v1 *config) {
    struct xkb_rule_names keymap_rule_names = {0};
    keymap_rule_names.layout = strdup(xkb_config.layout);
    keymap_rule_names.variant = strdup(xkb_config.variant);

    struct xkb_keymap *keymap = xkb_keymap_new_from_names2(
        xkb_context, &keymap_rule_names, XKB_KEYMAP_FORMAT_TEXT_V2,
        XKB_KEYMAP_COMPILE_NO_FLAGS);
    if (keymap == NULL) {
        fprintf(stderr, "Failed to create xkb keymap\n");
        return NULL;
    }

    char *keymap_str = xkb_keymap_get_as_string2(
        keymap, XKB_KEYMAP_FORMAT_TEXT_V2, XKB_KEYMAP_SERIALIZE_NO_FLAGS);
    xkb_keymap_unref(keymap);
    int keymap_str_len = strlen(keymap_str) + 1;
    int keymap_fd =
        memfd_create("taiga-keymap", MFD_CLOEXEC | MFD_ALLOW_SEALING);
    if (keymap_fd == -1 || ftruncate(keymap_fd, keymap_str_len) < 0) {
        fprintf(stderr, "Failed to create or truncate mem fd\n");
        close(keymap_fd);
        free(keymap_str);
        return NULL;
    }

    void *data = mmap(NULL, keymap_str_len, PROT_READ | PROT_WRITE, MAP_SHARED,
                      keymap_fd, 0);
    if (data == MAP_FAILED) {
        fprintf(stderr, "Failed to map data\n");
        close(keymap_fd);
        free(keymap_str);
        return NULL;
    }

    memcpy(data, keymap_str, keymap_str_len);
    free(keymap_str);

    if (munmap(data, keymap_str_len) < 0) {
        fprintf(stderr, "Failed to unmap data\n");
        close(keymap_fd);
        free(keymap_str);
        return NULL;
    }

    if (fcntl(keymap_fd, F_ADD_SEALS,
              F_SEAL_SHRINK | F_SEAL_GROW | F_SEAL_WRITE | F_SEAL_SEAL) < 0) {
        fprintf(stderr, "Failed to seal mem fd\n");
        close(keymap_fd);
        free(keymap_str);
        return NULL;
    }

    return river_xkb_config_v1_create_keymap(config, keymap_fd,
                                             XKB_KEYMAP_FORMAT_TEXT_V2);
}

void river_xkb_config_handle_xkb_keyboard(void *data,
                                          struct river_xkb_config_v1 *config,
                                          struct river_xkb_keyboard_v1 *id) {
    (void)data;

    fprintf(stdout, "INFO: New xkb_keyboard.\n");
    fprintf(stdout, "INFO: Setting keymap.\n");
    struct river_xkb_keymap_v1 *keymap = create_keymap(config);
    if (keymap == NULL) {
        fprintf(stderr, "ERROR: Failed to create keymap.\n");
        return;
    }
    river_xkb_keyboard_v1_set_keymap(id, keymap);
}

void river_xkb_config_handle_finished(void *data,
                                      struct river_xkb_config_v1 *config) {
    (void)data;
    (void)config;

    fprintf(stdout, "INFO: Config finished.\n");
}

void xkb_binding_handle_pressed(void *data, struct river_xkb_binding_v1 *obj) {
    (void)obj;

    struct XkbBinding *binding = data;
    binding->seat->pending_action = binding->action;
    binding->seat->pending_cmd = binding->cmd;
}

void xkb_binding_handle_released(void *data, struct river_xkb_binding_v1 *obj) {
}

const struct river_xkb_binding_v1_listener river_xkb_binding_listener = {
    .pressed = xkb_binding_handle_pressed,
    .released = xkb_binding_handle_released,
};

void xkb_binding_destroy(struct XkbBinding *binding) {
    river_xkb_binding_v1_destroy(binding->obj);
    free(binding->cmd);
    wl_list_remove(&binding->link);
    free(binding);
}

void xkb_binding_create(struct Seat *seat, uint32_t mods, xkb_keysym_t keysym,
                        enum Action action, char *cmd) {
    struct XkbBinding *binding = calloc(1, sizeof(struct XkbBinding));
    binding->obj = river_xkb_bindings_v1_get_xkb_binding(
        xkb_bindings_v1, seat->obj, keysym, mods);
    binding->seat = seat;
    binding->action = action;
    if (cmd != NULL) {
        binding->cmd = strdup(cmd);
    }

    river_xkb_binding_v1_add_listener(binding->obj, &river_xkb_binding_listener,
                                      binding);
    river_xkb_binding_v1_enable(binding->obj);

    wl_list_insert(seat->xkb_bindings.prev, &binding->link);
}