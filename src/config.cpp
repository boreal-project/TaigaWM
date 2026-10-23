#include "config.hpp"

#include <linux/input-event-codes.h>
#include <unistd.h>

#include <cstdlib>
#include <string_view>
#include <unordered_map>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

#include <river-window-management-v1-client-protocol.h>

#include "util.hpp"

Config config;
std::string configPath;

namespace {

std::vector<std::string_view> splitTokens(std::string_view text, char delim) {
    std::vector<std::string_view> tokens;
    size_t pos = 0;
    while (pos < text.size()) {
        const size_t end = text.find(delim, pos);
        const size_t stop = end == std::string_view::npos ? text.size() : end;
        if (stop > pos) {
            tokens.push_back(text.substr(pos, stop - pos));
        }
        pos = stop + 1;
    }
    return tokens;
}

uint32_t parseModifiers(std::string_view text) {
    uint32_t mods = RIVER_SEAT_V1_MODIFIERS_NONE;
    for (std::string_view mod : splitTokens(text, '+')) {
        if (mod == "super") {
            mods |= RIVER_SEAT_V1_MODIFIERS_MOD4;
        } else if (mod == "ctrl") {
            mods |= RIVER_SEAT_V1_MODIFIERS_CTRL;
        } else if (mod == "alt") {
            mods |= RIVER_SEAT_V1_MODIFIERS_MOD1;
        } else if (mod == "shift") {
            mods |= RIVER_SEAT_V1_MODIFIERS_SHIFT;
        } else if (mod != "none") {
            diag::warn("unknown modifier '%.*s'", static_cast<int>(mod.size()),
                       mod.data());
        }
    }
    return mods;
}

struct KeyActionInfo {
    Action action;
    bool needsArg;
};

const std::unordered_map<std::string_view, KeyActionInfo> &keyActions() {
    static const std::unordered_map<std::string_view, KeyActionInfo> table = {
        {"spawn", {Action::SpawnSh, true}},
        {"kill_active", {Action::Close, false}},
        {"focus_next", {Action::FocusNext, false}},
        {"exit", {Action::Exit, false}},
        {"fullscreen", {Action::Fullscreen, false}},
        {"maximize", {Action::Maximize, false}},
        {"tag_inc", {Action::TagInc, false}},
        {"tag_dec", {Action::TagDec, false}},
        {"win_tag_inc", {Action::WinTagInc, false}},
        {"win_tag_dec", {Action::WinTagDec, false}},
        {"tag", {Action::TagSet, true}},
        {"win_tag", {Action::WinTagSet, true}},
        {"focus_mon_next", {Action::FocusMonNext, false}},
        {"focus_mon_prev", {Action::FocusMonPrev, false}},
    };
    return table;
}

class LuaState {
  public:
    LuaState() : state_(luaL_newstate()) { luaL_openlibs(state_); }
    ~LuaState() { lua_close(state_); }
    LuaState(const LuaState &) = delete;
    LuaState &operator=(const LuaState &) = delete;

    bool runFile(const std::string &path) {
        if (luaL_dofile(state_, path.c_str()) != LUA_OK) {
            diag::error("Lua failed to load '%s': %s", path.c_str(),
                        lua_tostring(state_, -1));
            lua_pop(state_, 1);
            return false;
        }
        return true;
    }

    std::optional<std::vector<std::string>> stringList(const char *table) {
        const int top = lua_gettop(state_);
        lua_getglobal(state_, table);
        if (!lua_istable(state_, -1)) {
            diag::warn("'%s' is not a table in '%s'", table,
                       configPath.c_str());
            lua_settop(state_, top);
            return std::nullopt;
        }

        std::vector<std::string> result;
        const lua_Unsigned len = lua_rawlen(state_, -1);
        for (lua_Unsigned i = 1; i <= len; i++) {
            lua_rawgeti(state_, -1, static_cast<lua_Integer>(i));
            if (!lua_isstring(state_, -1)) {
                diag::error("failed to read '%s' section.", table);
                lua_settop(state_, top);
                return std::nullopt;
            }
            result.emplace_back(lua_tostring(state_, -1));
            lua_pop(state_, 1);
        }
        lua_settop(state_, top);
        return result;
    }

    std::optional<std::string> string(const char *table, const char *field) {
        const int top = lua_gettop(state_);
        std::optional<std::string> result;
        if (pushField(table, field) && lua_type(state_, -1) == LUA_TSTRING) {
            result = lua_tostring(state_, -1);
        }
        lua_settop(state_, top);
        return result;
    }

    bool boolean(const char *table, const char *field) {
        const int top = lua_gettop(state_);
        bool result = false;
        if (pushField(table, field) && lua_type(state_, -1) == LUA_TBOOLEAN) {
            result = lua_toboolean(state_, -1) != 0;
        }
        lua_settop(state_, top);
        return result;
    }

    lua_Integer integer(const char *table, const char *field) {
        const int top = lua_gettop(state_);
        lua_Integer result = 0;
        if (pushField(table, field) && lua_type(state_, -1) == LUA_TNUMBER) {
            int isnum = 0;
            const lua_Integer value = lua_tointegerx(state_, -1, &isnum);
            if (isnum) {
                result = value;
            }
        }
        lua_settop(state_, top);
        return result;
    }

  private:
    bool pushField(const char *table, const char *field) {
        lua_getglobal(state_, table);
        if (!lua_istable(state_, -1)) {
            return false;
        }
        lua_getfield(state_, -1, field);
        return true;
    }

    lua_State *state_;
};

bool readable(const std::string &path) {
    return access(path.c_str(), R_OK) == 0;
}

} // namespace

std::optional<std::string> locateConfig() {
    if (const char *home = std::getenv("HOME")) {
        for (const char *relative :
             {".config/taiga/taigarc.lua", ".taigarc.lua"}) {
            std::string path = std::string(home) + "/" + relative;
            if (readable(path)) {
                return path;
            }
        }
    } else {
        diag::warn("HOME environment variable not set. Skipping user configs");
    }

    const std::string system = "/etc/taiga/taigarc.lua";
    if (readable(system)) {
        return system;
    }

    diag::info("loading fallback config: ./taigarc.lua");
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == nullptr) {
        return std::nullopt;
    }
    std::string local = std::string(cwd) + "/taigarc.lua";
    if (readable(local)) {
        return local;
    }
    return std::nullopt;
}

bool loadConfig() {
    const std::optional<std::string> path = locateConfig();
    if (!path) {
        diag::error("failed to open a config file.");
        return false;
    }
    configPath = *path;

    LuaState lua;
    if (!lua.runFile(configPath)) {
        return false;
    }

    Config next;
    next.keybinds = lua.stringList("Keybinds");
    next.pointerBinds = lua.stringList("Pointerbinds");
    next.autostart = lua.stringList("Autostart");

    next.input.accelProfile = lua.string("Input", "accel_profile").value_or("");
    next.input.tapToClick = lua.boolean("Input", "tap_to_click");
    next.input.repeatRate =
        static_cast<int>(lua.integer("Input", "repeat_rate"));
    next.input.repeatDelay =
        static_cast<int>(lua.integer("Input", "repeat_delay"));

    next.xkb.layout = lua.string("Xkb", "layout").value_or("");
    next.xkb.variant = lua.string("Xkb", "variant").value_or("");
    next.xkb.options = lua.string("Xkb", "options").value_or("");

    next.misc.tearing = lua.boolean("Misc", "tearing");
    next.misc.xcursorTheme = lua.string("Misc", "xcursor_theme").value_or("");
    next.misc.xcursorSize =
        static_cast<uint32_t>(lua.integer("Misc", "xcursor_size"));
    next.misc.borderSize =
        static_cast<uint32_t>(lua.integer("Misc", "border_size"));
    next.misc.clientSideDecorations =
        lua.boolean("Misc", "client_side_decorations");
    next.misc.focusedBorderColor =
        static_cast<uint32_t>(lua.integer("Misc", "focused_border_color_hex"));
    next.misc.unfocusedBorderColor = static_cast<uint32_t>(
        lua.integer("Misc", "unfocused_border_color_hex"));

    config = std::move(next);
    return true;
}

std::optional<KeybindSpec> parseKeybind(std::string_view line) {
    const std::vector<std::string_view> tokens = splitTokens(line, ' ');
    if (tokens.empty()) {
        diag::error("missing modifiers.");
        return std::nullopt;
    }
    if (tokens.size() < 2) {
        diag::error("missing key.");
        return std::nullopt;
    }
    if (tokens.size() < 3) {
        diag::error("missing action.");
        return std::nullopt;
    }

    const auto &table = keyActions();
    const auto found = table.find(tokens[2]);
    if (found == table.end()) {
        diag::error("unknown action.");
        return std::nullopt;
    }

    KeybindSpec spec;
    spec.mods = parseModifiers(tokens[0]);
    spec.action = found->second.action;

    for (size_t i = 3; i < tokens.size(); i++) {
        if (i > 3) {
            spec.arg += ' ';
        }
        spec.arg += tokens[i];
    }
    if (found->second.needsArg && spec.arg.empty()) {
        diag::error("missing argument for '%.*s'",
                    static_cast<int>(tokens[2].size()), tokens[2].data());
        return std::nullopt;
    }

    const std::string key(tokens[1]);
    spec.keysym =
        xkb_keysym_from_name(key.c_str(), XKB_KEYSYM_CASE_INSENSITIVE);
    if (spec.keysym == XKB_KEY_NoSymbol) {
        diag::error("unknown key '%s'", key.c_str());
        return std::nullopt;
    }
    return spec;
}

std::optional<PointerBindSpec> parsePointerBind(std::string_view line) {
    const std::vector<std::string_view> tokens = splitTokens(line, ' ');
    if (tokens.empty()) {
        diag::error("missing modifiers.");
        return std::nullopt;
    }
    if (tokens.size() < 2) {
        diag::error("missing mouse button.");
        return std::nullopt;
    }
    if (tokens.size() < 3) {
        diag::error("missing action.");
        return std::nullopt;
    }

    PointerBindSpec spec;
    spec.mods = parseModifiers(tokens[0]);

    if (tokens[1] == "left_click") {
        spec.button = BTN_LEFT;
    } else if (tokens[1] == "right_click") {
        spec.button = BTN_RIGHT;
    } else {
        diag::error("invalid button");
        return std::nullopt;
    }

    if (tokens[2] == "move") {
        spec.action = Action::Move;
    } else if (tokens[2] == "resize") {
        spec.action = Action::Resize;
    } else {
        diag::error("unknown action.");
        return std::nullopt;
    }
    return spec;
}
