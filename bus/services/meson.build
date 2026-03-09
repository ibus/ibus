# D-Bus service file
configure_file(
  input: 'org.freedesktop.IBus.service.in',
  output: '@BASENAME@',
  configuration: {
    'bindir': get_option('prefix') / get_option('bindir'),
    'xim_cli_arg': enable_xim ? '--xim' : '',
  },
  install: true,
  install_dir: get_option('datadir') / 'dbus-1' / 'services',
)

if enable_systemd
  configure_file(
    input: 'org.freedesktop.IBus.session.GNOME.service.in',
    output: '@BASENAME@',
    configuration: {
      'bindir': get_option('prefix') / get_option('bindir'),
      'xim_cli_arg': enable_xim ? '--xim' : '',
    },
    install: true,
    install_dir: systemduserunitdir,
  )

  configure_file(
    input: 'org.freedesktop.IBus.session.generic.service.in',
    output: '@BASENAME@',
    configuration: {
      'bindir': get_option('prefix') / get_option('bindir'),
      'xim_cli_arg': enable_xim ? '--xim' : '',
    },
    install: true,
    install_dir: systemduserunitdir,
  )
endif
