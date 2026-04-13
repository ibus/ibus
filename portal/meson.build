# Portal service
configure_file(
  input: 'org.freedesktop.portal.IBus.service.in',
  output: '@BASENAME@',
  configuration: {
    'libexecdir': get_option('libexecdir'),
  },
  install: true,
  install_dir: get_option('datadir') / 'dbus-1' / 'services',
)

# Portal executable
ibus_portal_sources = files(
  'portal.c',
)

ibus_portal_gdbus_sources = gnome.gdbus_codegen('ibus-portal-dbus',
  sources: 'org.freedesktop.IBus.Portal.xml',
  interface_prefix: 'org.freedesktop.IBus.',
  namespace: 'IBusDbus',
)

ibus_portal_deps = [
  gobject_dep,
  glib_dep,
  gio_dep,
  gio_unix_dep,
  ibus_dep,
]

ibus_portal_cflags = [
  '-DG_LOG_DOMAIN="@0@"'.format(ibus_log_domain)
]

ibus_portal = executable('ibus-portal',
  [ ibus_portal_sources, ibus_portal_gdbus_sources ],
  dependencies: ibus_portal_deps,
  c_args: ibus_portal_cflags,
  include_directories: config_h_inc,
  install: true,
  install_dir: get_option('libexecdir'),
)
