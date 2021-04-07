gtk2_im_sources = files(
  'ibusim.c',
  'ibusimcontext.c',
)

gtk2_im_deps = [
  ibus_dep,
  gtk2_dep,
  dbus_dep,
]

gtk2_im_module = shared_module('im-ibus',
  gtk2_im_sources,
  dependencies: gtk2_im_deps,
  include_directories: config_h_inc,
  install: true,
  install_dir: gtk2_im_module_dir,
)
