# SPDX-FileCopyrightText: © 2026 Isaac Freund
# SPDX-License-Identifier: 0BSD

project(
  'taiga',
  ['c', 'cpp'],
  meson_version: '>= 1.8.0',
  version: '0.1',
  default_options: [
    'warning_level=3',
    'buildtype=debugoptimized',
    'cpp_std=c++20',
  ],
)

cxx = meson.get_compiler('cpp')
add_project_arguments(cxx.get_supported_arguments([
  '-Wno-unused-parameter',
]), language: 'cpp')

wayland_client = dependency('wayland-client')
xkbcommon = dependency('xkbcommon')
libinput = dependency('libinput')



lua_dep = disabler()
lua_names = [
  'lua54', 'lua5.4', 'lua-5.4',
  'lua55', 'lua5.5', 'lua-5.5',
  'lua'
]

foreach name : lua_names
  dep = dependency(name, required: false)
  if dep.found()
    lua_dep = dep
    break
  endif
endforeach

if not lua_dep.found()
  error('Lua not found. Tried: ' + ' '.join(lua_names))
endif
message('Using Lua: @0@'.format(lua_dep.name()))

wl_mod = import('wayland')
generated = wl_mod.scan_xml(
  'protocol/river-window-management-v1.xml',
  'protocol/river-xkb-bindings-v1.xml',
  'protocol/river-layer-shell-v1.xml',
  'protocol/river-libinput-config-v1.xml',
  'protocol/river-input-management-v1.xml',
  'protocol/river-xkb-config-v1.xml',
)

inc = include_directories('include')

install_data(
  'TaigaWM.desktop',
  install_dir: '/usr/share/wayland-sessions',
)

install_data(
  'taigarc.lua',
  install_dir: '/etc/taiga'
)

exe = executable(
  'taiga',
  'src/main.cpp',
  'src/util.cpp',
  'src/output.cpp',
  'src/window.cpp',
  'src/xkb.cpp',
  'src/pointer.cpp',
  'src/seat.cpp',
  'src/wm.cpp',
  'src/layer_shell.cpp',
  'src/config.cpp',
  'src/libinput.cpp',
  'src/autostart.cpp',
  'src/input.cpp',
  generated,
  dependencies: [
    wayland_client,
    xkbcommon,
    libinput,
    lua_dep,
  ],
  include_directories: inc,
  install: true,
)
