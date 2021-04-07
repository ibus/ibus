ibus_daemon_common_sources = files(
  'component.c',
  'connection.c',
  'dbusimpl.c',
  'engineproxy.c',
  'factoryproxy.c',
  'global.c',
  'ibusimpl.c',
  'inputcontext.c',
  'matchrule.c',
  'panelproxy.c',
  'server.c',
)

ibus_daemon_sources = files(
  'main.c',
)

ibus_daemon_marshalers = gnome.genmarshal('marshalers',
  prefix: 'bus_marshal',
  sources: 'marshalers.list',
)

ibus_daemon_deps = [
  glib_dep,
  gio_dep,
  ibus_dep,
]

ibus_daemon_cflags = [
  '-DG_LOG_DOMAIN="@0@"'.format(ibus_log_domain),
  '-DBINDIR="@0@"'.format(get_option('prefix') / get_option('bindir')),
]

ibus_daemon = executable('ibus-daemon',
  [ ibus_daemon_common_sources, ibus_daemon_sources, ibus_daemon_marshalers ],
  dependencies: ibus_daemon_deps,
  c_args: ibus_daemon_cflags,
  include_directories: config_h_inc,
  install: true,
)

# Tests
if get_option('tests')

  ibus_daemon_tests = [
    {
      'name': 'test-matchrule',
      'extra_sources': [],
      'extra_deps': [],
    },
    {
      'name': 'test-stress',
      'extra_sources': files('test-client.c'),
      'extra_deps': [ x11_dep ],
    },
  ]

  foreach _test: ibus_daemon_tests
    test_sources = [
      '@0@.c'.format(_test['name']),
      _test['extra_sources'],
      ibus_daemon_common_sources,
      ibus_daemon_marshalers,
    ]

    test_bin = executable(_test['name'],
      test_sources,
      dependencies: ibus_daemon_deps + _test['extra_deps'],
      c_args: ibus_daemon_cflags,
      include_directories: config_h_inc,
    )

    test(_test['name'], test_bin,
      suite: 'ibus-daemon',
    )
  endforeach

endif

# manpages
configure_file(
  input: 'ibus-daemon.1.in',
  output: '@BASENAME@',
  configuration: {
    'VERSION': meson.project_version(),
  },
  install: true,
  install_dir: get_option('mandir') / 'man1',
)

# D-Bus service file
configure_file(
  input: 'org.freedesktop.IBus.service.in',
  output: '@BASENAME@',
  configuration: {
    'bindir': get_option('prefix') / get_option('bindir'),
    'xim_cli_arg': enable_xim? '--xim' : '',
  },
  install: true,
  install_dir: get_option('datadir') / 'dbus-1' / 'services',
)
