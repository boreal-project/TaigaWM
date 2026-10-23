# TaigaWM

A small, floating window manager written in C++20 for river.

Used primarily in the borealOS project.

## Project status

Very early, expect bugs, missing features, etc.. 
However, it's very usable.

## Installation

The following deps are needed

- meson
- C++20 compiler
- cmake
- pkg-config
- wayland-dev
- libxkbcommon-dev
- libinput-dev
- lua-dev >= 5.4
- river compositor >= 4.0

### Compiling
```bash
git clone https://github.com/m4rn-progs/TaigaWM
cd TaigaWM
meson setup build
ninja -C build
```

### Installing
```bash
sudo meson install -C build
```

### Running
```bash
./build/taiga
OR
taiga
```

## Configuration
Taiga is configured using lua, with hot reload support.

Copy the default config file to one of the supported locations:
- ~/.taigarc.lua
- ~/.config/taiga/taigarc.lua

The default config has examples in it.
