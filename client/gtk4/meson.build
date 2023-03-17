gtk4_im_sources = files(
  'ibusim.c',
  'ibusimcontext.c',
)

gtk4_im_deps = [
  ibus_dep,
  gtk4_dep,
  dbus_dep,
]

gtk4_im_cflags = [
  '-DG_LOG_DOMAIN="@0@"'.format(ibus_log_domain)
]

gtk4_im_module = shared_module('im-ibus',
  gtk4_im_sources,
  dependencies: gtk4_im_deps,
  c_args: gtk4_im_cflags,
  include_directories: config_h_inc,
  install: true,
  install_dir: gtk4_im_module_dir,
)
