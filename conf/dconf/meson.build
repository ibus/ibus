ibus_dconf_sources = files(
  'config.c',
  'main.c',
)

ibus_dconf_deps = [
  glib_dep,
  gio_dep,
  ibus_dep,
  dconf_dep,
]

ibus_dconf_cflags = [
  '-DG_LOG_DOMAIN="@0@"'.format(ibus_log_domain),
]

ibus_dconf_bin = executable('ibus-dconf',
  ibus_dconf_sources,
  dependencies: ibus_dconf_deps,
  c_args: ibus_dconf_cflags,
  include_directories: config_h_inc,
  install: true,
  install_dir: get_option('libexecdir'),
)

configure_file(
  input: 'dconf.xml.in',
  output: '@BASENAME@',
  configuration: {
    'VERSION': meson.project_version(),
    'libexecdir': get_option('prefix') / get_option('libexecdir'),
  },
  install: true,
  install_dir: pkgdatadir / 'component',
)
