ibus_memconf_sources = files(
  'config.c',
  'main.c',
)

ibus_memconf_deps = [
  glib_dep,
  gio_dep,
  ibus_dep,
]

ibus_memconf_cflags = [
  '-DG_LOG_DOMAIN="@0@"'.format(ibus_log_domain),
]

ibus_memconf_bin = executable('ibus-memconf',
  ibus_memconf_sources,
  dependencies: ibus_memconf_deps,
  c_args: ibus_memconf_cflags,
  include_directories: config_h_inc,
  install: true,
  install_dir: get_option('libexecdir'),
)

configure_file(
  input: 'memconf.xml.in',
  output: '@BASENAME@',
  configuration: {
    'VERSION': meson.project_version(),
    'libexecdir': get_option('prefix') / get_option('libexecdir'),
  },
  install: true,
  install_dir: pkgdatadir / 'component',
)
