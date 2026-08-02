#include <lauxlib.h>
#include <linux/input-event-codes.h>
#include <lua.h>
#include <lualib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wayland-util.h>
#include <xkbcommon/xkbcommon.h>

#include "config.h"
#include "seat.h"
#include "xkb.h"

struct KeybindConfig keybind_config = {0};
struct PointerConfig pointer_config = {0};
struct AutostartConfig autostart_config = {0};
struct XkbConfig xkb_config = {0};
struct InputConfig input_config = {0};
struct MiscConfig misc_config = {0};
const char *config_path = NULL;

// open a lua table, just for slighly cleaner code
int get_lua_table_by_name(lua_State *state, const char *name) {
    fprintf(stdout, "get_lua_table_by_name\n");

    lua_getglobal(state, name);
    if (!lua_istable(state, -1)) {
        return 1;
    }
    return 0;
}

// add home + str to tmp buf
void homeify(char **tmp_buf, char *home, char *after_home) {
    fprintf(stdout, "homeify\n");

    size_t home_sz = strlen(home);
    size_t af_sz = strlen(after_home);

    *tmp_buf = malloc(home_sz + 1 + af_sz + 1);

    snprintf(*tmp_buf, home_sz + 1 + af_sz + 1, "%s/%s", home, after_home);
}

// mods mousebutton action
int parse_and_add_pointerbind(const char *pointer_str, struct Seat *seat) {
    fprintf(stdout, "parse_and_add_pointerbind\n");

    // pre-alloc that array
    size_t len = strlen(pointer_str);

    char buf[len + 1];
    strcpy(buf, pointer_str);

    char *saveptr1, *saveptr2;
    // start tokenizing

    // get mod str
    char *mods = strtok_r(buf, " ", &saveptr1);
    if (!mods) {
        fprintf(stderr, "err: missing modifiers.\n");
        return 1;
    }

    char *mb = strtok_r(NULL, " ", &saveptr1);
    if (!mb) {
        fprintf(stderr, "err: missing mouse button.\n");
        return 1;
    }

    char *action = strtok_r(NULL, " ", &saveptr1);
    if (!action) {
        fprintf(stderr, "err: missing action.\n");
        return 3;
    }

    uint32_t mods_local = RIVER_SEAT_V1_MODIFIERS_NONE;
    char *mod_tok = strtok_r(mods, "+", &saveptr2);

    // add all the mods together if there are any if not, no mod will be used
    while (mod_tok != NULL) {
        if (strcmp(mod_tok, "super") == 0) {
            mods_local |= RIVER_SEAT_V1_MODIFIERS_MOD4;
        } else if (strcmp(mod_tok, "ctrl") == 0) {
            mods_local |= RIVER_SEAT_V1_MODIFIERS_CTRL;
        } else if (strcmp(mod_tok, "alt") == 0) {
            mods_local |= RIVER_SEAT_V1_MODIFIERS_MOD1;
        } else if (strcmp(mod_tok, "shift") == 0) {
            mods_local |= RIVER_SEAT_V1_MODIFIERS_SHIFT;
        } else {
            fprintf(stderr, "warn: no modifer selected\n");
        }
        mod_tok = strtok_r(NULL, "+", &saveptr2);
    }

    uint32_t button = 0;
    if (strcmp(mb, "left_click") == 0) {
        button = BTN_LEFT;
    } else if (strcmp(mb, "right_click") == 0) {
        button = BTN_RIGHT;
    } else {
        fprintf(stderr, "err: invalid button\n");
    }

    if (strcmp(action, "move") == 0) {
        pointer_binding_create(seat, mods_local, button, ACTION_MOVE);
    } else if (strcmp(action, "resize") == 0) {
        pointer_binding_create(seat, mods_local, button, ACTION_RESIZE);
    } else {
        fprintf(stderr, "err: unknown action.\n");
        return 1;
    }

    return 0;
}

// huge function to parse, and .... add keybinds. duh.
// m4rn-progs note here,
// I have decided to rewrite this to use pre-allocated memory. Since we'll
// be calling this function in seat.c for each bit of config we have,
// I don't want to keep calling malloc like 50 times per line and for each " "
// char. Therefore, pre-allocate memory via array and operate in that array, and
// just destroy the array
int parse_and_add_keybind(const char *keybind_str, struct Seat *seat) {
    fprintf(stdout, "parse_and_add_keybind\n");

    // pre-alloc that array
    size_t len = strlen(keybind_str);

    char buf[len + 1];
    strcpy(buf, keybind_str);

    char *saveptr1, *saveptr2;
    // start tokenizing

    // get mod str
    char *mods = strtok_r(buf, " ", &saveptr1);
    if (!mods) {
        fprintf(stderr, "err: missing modifiers.\n");
        return 1;
    }

    // get key str
    char *key = strtok_r(NULL, " ", &saveptr1);
    if (!key) {
        fprintf(stderr, "err: missing key.\n");
        return 2;
    }

    // get action str
    char *action = strtok_r(NULL, " ", &saveptr1);
    if (!action) {
        fprintf(stderr, "err: missing action.\n");
        return 3;
    }

    // prepare a buffer that will soon enough get copied to the heap
    char final_cmd_buf[len + 1];
    size_t final_cmd_len = 0;

    // prepare the cmd_token that will be in that buffer
    char *cmd_tok = strtok_r(NULL, " ", &saveptr1);

    // "loop over all delims and basically add the rest of the cmds to
    // final_cmd" - zoey, 2026
    while (cmd_tok != NULL) {
        size_t tok_len = strlen(cmd_tok);

        if (final_cmd_len > 0) {
            final_cmd_buf[final_cmd_len++] = ' ';
        }

        memcpy(final_cmd_buf + final_cmd_len, cmd_tok, tok_len);
        final_cmd_len += tok_len;

        cmd_tok = strtok_r(NULL, " ", &saveptr1);
    }
    final_cmd_buf[final_cmd_len] = '\0';

    uint32_t mods_local = RIVER_SEAT_V1_MODIFIERS_NONE;
    char *mod_tok = strtok_r(mods, "+", &saveptr2);

    // add all the mods together if there are any if not, no mod will be used
    while (mod_tok != NULL) {
        if (strcmp(mod_tok, "super") == 0) {
            mods_local |= RIVER_SEAT_V1_MODIFIERS_MOD4;
        } else if (strcmp(mod_tok, "ctrl") == 0) {
            mods_local |= RIVER_SEAT_V1_MODIFIERS_CTRL;
        } else if (strcmp(mod_tok, "alt") == 0) {
            mods_local |= RIVER_SEAT_V1_MODIFIERS_MOD1;
        } else if (strcmp(mod_tok, "shift") == 0) {
            mods_local |= RIVER_SEAT_V1_MODIFIERS_SHIFT;
        } else {
            fprintf(stderr, "warn: no modifer selected\n");
        }
        mod_tok = strtok_r(NULL, "+", &saveptr2);
    }

    // check the action to decide what to do
    if (strcmp(action, "spawn") == 0) {
        if (final_cmd_len == 0) {
            fprintf(stderr, "err: missing command.\n");
            return 4;
        }

        // look mom the buffer got copied to heap
        xkb_binding_create(
            seat, mods_local,
            xkb_keysym_from_name(key, XKB_KEYSYM_CASE_INSENSITIVE),
            ACTION_SPAWN_SH, strdup(final_cmd_buf));
    } else if (strcmp(action, "kill_active") == 0) {
        xkb_binding_create(
            seat, mods_local,
            xkb_keysym_from_name(key, XKB_KEYSYM_CASE_INSENSITIVE),
            ACTION_CLOSE, NULL);
    } else if (strcmp(action, "focus_next") == 0) {
        xkb_binding_create(
            seat, mods_local,
            xkb_keysym_from_name(key, XKB_KEYSYM_CASE_INSENSITIVE),
            ACTION_FOCUS_NEXT, NULL);
    } else if (strcmp(action, "exit") == 0) {
        xkb_binding_create(
            seat, mods_local,
            xkb_keysym_from_name(key, XKB_KEYSYM_CASE_INSENSITIVE), ACTION_EXIT,
            NULL);
    } else if (strcmp(action, "fullscreen") == 0) {
        xkb_binding_create(
            seat, mods_local,
            xkb_keysym_from_name(key, XKB_KEYSYM_CASE_INSENSITIVE),
            ACTION_FULLSCREEN, NULL);
    } else if (strcmp(action, "maximize") == 0) {
        xkb_binding_create(
            seat, mods_local,
            xkb_keysym_from_name(key, XKB_KEYSYM_CASE_INSENSITIVE),
            ACTION_MAXIMIZE, NULL);
    } else if (strcmp(action, "tag_inc") == 0) {
        xkb_binding_create(
            seat, mods_local,
            xkb_keysym_from_name(key, XKB_KEYSYM_CASE_INSENSITIVE),
            ACTION_TAG_INC, NULL);
    } else if (strcmp(action, "tag_dec") == 0) {
        xkb_binding_create(
            seat, mods_local,
            xkb_keysym_from_name(key, XKB_KEYSYM_CASE_INSENSITIVE),
            ACTION_TAG_DEC, NULL);
    } else if (strcmp(action, "win_tag_inc") == 0) {
        xkb_binding_create(
            seat, mods_local,
            xkb_keysym_from_name(key, XKB_KEYSYM_CASE_INSENSITIVE),
            ACTION_WIN_TAG_INC, NULL);
    } else if (strcmp(action, "win_tag_dec") == 0) {
        xkb_binding_create(
            seat, mods_local,
            xkb_keysym_from_name(key, XKB_KEYSYM_CASE_INSENSITIVE),
            ACTION_WIN_TAG_DEC, NULL);
    } else if (strcmp(action, "tag") == 0) {
        if (final_cmd_len == 0) {
            fprintf(stderr, "err: Missing tag id.\n");
            return 4;
        }

        xkb_binding_create(
            seat, mods_local,
            xkb_keysym_from_name(key, XKB_KEYSYM_CASE_INSENSITIVE),
            ACTION_TAG_SET, strdup(final_cmd_buf));
    } else if (strcmp(action, "win_tag") == 0) {
        if (final_cmd_len == 0) {
            fprintf(stderr, "err: Missing tag id.\n");
            return 4;
        }

        xkb_binding_create(
            seat, mods_local,
            xkb_keysym_from_name(key, XKB_KEYSYM_CASE_INSENSITIVE),
            ACTION_WIN_TAG_SET, strdup(final_cmd_buf));
    } else if (strcmp(action, "focus_mon_next") == 0) {
        xkb_binding_create(
            seat, mods_local,
            xkb_keysym_from_name(key, XKB_KEYSYM_CASE_INSENSITIVE),
            ACTION_FOCUS_MON_NEXT, NULL);
    } else if (strcmp(action, "focus_mon_prev") == 0) {
        xkb_binding_create(
            seat, mods_local,
            xkb_keysym_from_name(key, XKB_KEYSYM_CASE_INSENSITIVE),
            ACTION_FOCUS_MON_PREV, NULL);
    } else {
        fprintf(stderr, "err: unknown action.\n");
        return 1;
    }

    return 0;
}

lua_State *lua_open_table(const char *config_path, const char *table_name) {
    fprintf(stdout, "lua_open_table\n");

    lua_State *L = luaL_newstate();
    luaL_openlibs(L);

    if (luaL_dofile(L, config_path) != LUA_OK) {
        fprintf(stderr, "err: Lua failed to load '%s': %s\n", config_path,
                lua_tostring(L, -1));
        lua_close(L);
        return NULL;
    }

    lua_getglobal(L, table_name);
    if (!lua_istable(L, -1)) {
        fprintf(stderr, "err: '%s' is not a table in '%s'\n", table_name,
                config_path);
        lua_close(L);
        return NULL;
    }

    return L;
}

int get_int_from_var_from_table(const char *config_path, const char *table_name,
                                const char *var_name) {
    fprintf(stdout, "get_int_from_var_from_table\n");

    lua_State *L;
    if ((L = lua_open_table(config_path, table_name)) == NULL) {
        return 0;
    }

    lua_getfield(L, -1, var_name);
    int luatype = lua_type(L, -1);

    if (luatype == LUA_INT_DEFAULT) {
        int b = lua_tointeger(L, -1);

        lua_close(L);
        return b;
    } else {
        lua_close(L);
        return 0;
    }
}

bool get_bool_from_var_from_table(const char *config_path,
                                  const char *table_name,
                                  const char *var_name) {
    fprintf(stdout, "get_bool_from_var_from_table\n");

    lua_State *L;
    if ((L = lua_open_table(config_path, table_name)) == NULL) {
        return NULL;
    }

    lua_getfield(L, -1, var_name);
    int luatype = lua_type(L, -1);

    if (luatype == LUA_TBOOLEAN) {
        bool b = lua_toboolean(L, -1);

        lua_close(L);
        return b;
    } else {
        lua_close(L);
        return NULL;
    }
}

char *get_string_from_var_from_table(const char *config_path,
                                     const char *table_name,
                                     const char *var_name) {
    fprintf(stdout, "get_string_from_var_from_table\n");

    lua_State *L;
    if ((L = lua_open_table(config_path, table_name)) == NULL) {
        return NULL;
    }

    lua_getfield(L, -1, var_name);
    int luatype = lua_type(L, -1);

    if (luatype == LUA_TSTRING) {
        // we strdup it cuz if we dont, lua_close will take the string with it
        char *result = NULL;
        if (lua_isstring(L, -1)) {
            result = strdup(lua_tostring(L, -1));
        }
        lua_close(L);
        return result;

    } else {
        lua_close(L);
        return NULL;
    }
}

// this function is just crazy honestly
// allocate a buffer to hold strings, then loop over the table adding strings to
// the buffer probably leaks memory
char **get_list_of_strings_from_lua_table(const char *config_path,
                                          size_t *len_return,
                                          const char *table_name) {
    fprintf(stdout, "get_list_of_strings_from_lua_table\n");

    lua_State *L;
    if ((L = lua_open_table(config_path, table_name)) == NULL) {
        return NULL;
    }

    size_t len = lua_rawlen(L, -1);
    size_t used = 0;
    char **strings_buf = malloc(len * sizeof(*strings_buf));
    for (size_t i = 0; i < len; i++)
        strings_buf[i] = NULL;

    for (size_t i = 1; i <= len; i++) {
        lua_rawgeti(L, -1, i);

        if (!lua_isstring(L, -1)) {
            fprintf(stderr, "err: failed to read '%s' section.\n",
                    table_name);
            lua_pop(L, 1);
            for (size_t j = 0; j < used; j++)
                free(strings_buf[j]);
            free(strings_buf);
            lua_close(L);
            return NULL;
        }

        strings_buf[used] = strdup(lua_tostring(L, -1));
        used++;
        lua_pop(L, 1);
    }

    *len_return = len;
    lua_close(L);
    return strings_buf;
}

// locate the config file
char *locate_config(void) {
    fprintf(stdout, "locate_config\n");

    // we will add home later.
    char *config_locations[] = {
        ".config/taiga/taigarc.lua",
        ".taigarc.lua",
        "/etc/taiga/taigarc.lua",
    };

    // get len
    size_t config_locations_len =
        sizeof(config_locations) / sizeof(config_locations[0]);

    // for every possible config location
    for (size_t i = 0; i < config_locations_len; i++) {
        // if it doesnt start with / we want to add home to it
        if (config_locations[i][0] != '/') {
            // proper string
            char *home = getenv("HOME");
            if (home == NULL) {
                fprintf(
                    stderr,
                    "WARNING: HOME environment variable not set. Skipping %s\n",
                    config_locations[i]);
                continue;
            }

            char *tmp_buf = NULL;
            homeify(&tmp_buf, home, config_locations[i]);

            // try open it
            FILE *tmp_file = fopen(tmp_buf, "r");
            if (tmp_file != NULL) {
                fclose(tmp_file);
                return tmp_buf;
            }

            // clean
            free(tmp_buf);
        } else {
            // try open it
            FILE *tmp_file = fopen(config_locations[i], "r");
            if (tmp_file != NULL) {
                fclose(tmp_file);
                return config_locations[i];
            }
        }
    }

    fprintf(stdout, "loading fallback config: ./taigarc.lua\n");

    // get cwd and prepend it to file name
    char cwd[1024];
    char full_path[2048];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        return NULL;
    }
    snprintf(full_path, sizeof(full_path), "%s/%s", cwd, "taigarc.lua");

    // try open it
    FILE *tmp_file = fopen(full_path, "r");
    if (tmp_file != NULL) {
        fclose(tmp_file);
        return strdup(full_path);
    }

    return NULL;
}

int load_config(void) {
    fprintf(stdout, "load_config\n");

    config_path = locate_config();
    if (config_path == NULL) {
        fprintf(stderr, "err: failed to open a config file.");
        return 1;
    }

    // keybinds
    size_t binds_len;
    char **binds =
        get_list_of_strings_from_lua_table(config_path, &binds_len, "Keybinds");
    keybind_config.keybinds = binds;
    keybind_config.keybinds_len = binds_len;

    // Pointer binds
    size_t pbinds_len;
    char **pbinds = get_list_of_strings_from_lua_table(config_path, &pbinds_len,
                                                       "Pointerbinds");
    pointer_config.pointerbinds = pbinds;
    pointer_config.pointerbinds_len = pbinds_len;

    // autostart
    size_t autostart_len;
    char **autostart = get_list_of_strings_from_lua_table(
        config_path, &autostart_len, "Autostart");
    autostart_config.autostarts = autostart;
    autostart_config.autostarts_len = autostart_len;

    // libinput
    char *accel_profile =
        get_string_from_var_from_table(config_path, "Input", "accel_profile");
    bool tap_to_click =
        get_bool_from_var_from_table(config_path, "Input", "tap_to_click");
    int repeat_rate =
        get_int_from_var_from_table(config_path, "Input", "repeat_rate");
    int repeat_delay =
        get_int_from_var_from_table(config_path, "Input", "repeat_delay");

    // free the string if it exists
    if (input_config.accel_profile) {
        free(input_config.accel_profile);
    }
    input_config.accel_profile = accel_profile;
    input_config.tap_to_click = tap_to_click;
    input_config.repeat_rate = repeat_rate;
    input_config.repeat_delay = repeat_delay;

    // xkb
    char *layout = get_string_from_var_from_table(config_path, "Xkb", "layout");
    char *variant =
        get_string_from_var_from_table(config_path, "Xkb", "variant");
    char *options =
        get_string_from_var_from_table(config_path, "Xkb", "options");

    if (xkb_config.layout) {
        free(xkb_config.layout);
    }
    if (xkb_config.variant) {
        free(xkb_config.variant);
    }
    if (xkb_config.options) {
        free(xkb_config.options);
    }
    xkb_config.layout = layout;
    xkb_config.variant = variant;
    xkb_config.options = options;

    // misc
    bool tearing = get_bool_from_var_from_table(config_path, "Misc", "tearing");
    char *xcursor_theme =
        get_string_from_var_from_table(config_path, "Misc", "xcursor_theme");
    uint32_t xcursor_size =
        get_int_from_var_from_table(config_path, "Misc", "xcursor_size");
    uint32_t border_size =
        get_int_from_var_from_table(config_path, "Misc", "border_size");
    bool client_side_decorations = get_bool_from_var_from_table(
        config_path, "Misc", "client_side_decorations");
    uint32_t focused_border_color_hex = get_int_from_var_from_table(
        config_path, "Misc", "focused_border_color_hex");
    uint32_t unfocused_border_color_hex = get_int_from_var_from_table(
        config_path, "Misc", "unfocused_border_color_hex");

    if (misc_config.xcursor_theme) {
        free(misc_config.xcursor_theme);
    }
    misc_config.tearing = tearing;
    misc_config.xcursor_theme = xcursor_theme;
    misc_config.xcursor_size = xcursor_size;
    misc_config.border_size = border_size;
    misc_config.client_side_decorations = client_side_decorations;
    misc_config.focused_border_color_hex = focused_border_color_hex;
    misc_config.unfocused_border_color_hex = unfocused_border_color_hex;

    return 0;
}
