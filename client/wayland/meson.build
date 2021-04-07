ibus_wayland_sources = files(
  # See also README
  'input-method-unstable-v1-protocol.c',
  'main.c',
)

ibus_wayland_deps = [
  ibus_dep,
  gio_dep,
  wayland_dep,
  xkbcommon_dep,
]

executable('ibus-wayland',
  ibus_wayland_sources,
  dependencies: ibus_wayland_deps,
  include_directories: config_h_inc,
  install: true,
  install_dir: get_option('libexecdir'),
)
