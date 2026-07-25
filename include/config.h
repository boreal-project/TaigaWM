#ifndef CONFIG_H
#define CONFIG_H

#include <lua.h>
#include <stdio.h>

#include "seat.h"

struct KeybindConfig {
    char **keybinds;
    size_t keybinds_len;
};

struct PointerConfig {
    char **pointerbinds;
    size_t pointerbinds_len;
};

struct AutostartConfig {
    char **autostarts;
    size_t autostarts_len;
};

struct InputConfig {
    char *accel_profile;
    bool tap_to_click;
    int repeat_rate;
    int repeat_delay;
};

struct XkbConfig {
    char *layout;
    char *variant;
    char *options;
};

struct MiscConfig {
    bool tearing;
    char *xcursor_theme;
    uint32_t xcursor_size;
    uint32_t border_size;
    bool client_side_decorations;
    uint32_t focused_border_color_hex;
    uint32_t unfocused_border_color_hex;
};

extern struct KeybindConfig keybind_config;
extern struct PointerConfig pointer_config;
extern struct AutostartConfig autostart_config;
extern struct XkbConfig xkb_config;
extern struct InputConfig input_config;
extern struct MiscConfig misc_config;
extern const char *config_path;

int get_lua_table_by_name(lua_State *state, const char *name);
void homeify(char **tmp_buf, char *home, char *after_home);
void parse_get(char **save, char **str, const char *delim);
int parse_and_add_pointerbind(const char *pointer_str, struct Seat *seat);
int parse_and_add_keybind(const char *keybind_str, struct Seat *seat);
lua_State *lua_open_table(const char *config_path, const char *table_name);
bool get_bool_from_var_from_table(const char *config_path,
                                  const char *table_name, const char *var_name);
int get_int_from_var_from_table(const char *config_path, const char *table_name,
                                const char *var_name);
char *get_string_from_var_from_table(const char *config_path,
                                     const char *table_name,
                                     const char *var_name);
char **get_list_of_strings_from_lua_table(const char *config_path,
                                          size_t *len_return,
                                          const char *table_name);
char *locate_config(void);
int load_config(void);

#endif
