#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <xkbcommon/xkbcommon.h>

#include "actions.hpp"

struct InputConfig {
    std::string accelProfile;
    bool tapToClick = false;
    int repeatRate = 0;
    int repeatDelay = 0;
};

struct XkbConfig {
    std::string layout;
    std::string variant;
    std::string options;
};

struct MiscConfig {
    bool tearing = false;
    std::string xcursorTheme;
    uint32_t xcursorSize = 0;
    uint32_t borderSize = 0;
    bool clientSideDecorations = false;
    uint32_t focusedBorderColor = 0;
    uint32_t unfocusedBorderColor = 0;
};

struct Config {
    std::optional<std::vector<std::string>> keybinds;
    std::optional<std::vector<std::string>> pointerBinds;
    std::optional<std::vector<std::string>> autostart;
    InputConfig input;
    XkbConfig xkb;
    MiscConfig misc;
};

struct KeybindSpec {
    uint32_t mods = 0;
    xkb_keysym_t keysym = XKB_KEY_NoSymbol;
    Action action = Action::None;
    std::string arg;
};

struct PointerBindSpec {
    uint32_t mods = 0;
    uint32_t button = 0;
    Action action = Action::None;
};

extern Config config;
extern std::string configPath;

std::optional<std::string> locateConfig();
bool loadConfig();
std::optional<KeybindSpec> parseKeybind(std::string_view line);
std::optional<PointerBindSpec> parsePointerBind(std::string_view line);
