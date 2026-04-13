gtk3_im_sources = files(
  'ibusim.c',
  'ibusimcontext.c',
)

gtk3_im_deps = [
  ibus_dep,
  gtk3_dep,
  dbus_dep,
]

gtk3_im_cflags = [
  '-DG_LOG_DOMAIN="@0@"'.format(ibus_log_domain)
]

gtk3_im_module = shared_module('im-ibus',
  gtk3_im_sources,
  dependencies: gtk3_im_deps,
  c_args: gtk3_im_cflags,
  include_directories: config_h_inc,
  install: true,
  install_dir: gtk3_im_module_dir,
)
