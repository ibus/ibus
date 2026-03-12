ibus_tests = [
  { 'name': 'ibus-bus' },
  { 'name': 'ibus-config' },
  { 'name': 'ibus-configservice' },
  { 'name': 'ibus-factory' },
  { 'name': 'ibus-inputcontext' },
  { 'name': 'ibus-inputcontext-create' },
  { 'name': 'ibus-keynames' },
  { 'name': 'ibus-registry' },
  { 'name': 'ibus-serializable' },
  {
    'name': 'ibus-share',
    'extra_deps': [ dbus_dep ],
  },
  { 'name': 'ibus-util' },
]

if get_option('engine')
  ibus_tests += { 'name': 'ibus-engine-switch' }
endif

if enable_gtk4 or enable_gtk3
  ibus_tests += {
    'name': 'ibus-compose',
    'extra_deps': [ gtk_dep ],
  }

  if libevdev_dep.found() and libudev_dep.found()
    ibus_tests += {
      'name': 'ibus-keypress',
      'extra_sources': [ 'uinput-replay.c' ],
      'extra_deps': [ libevdev_dep, libudev_dep, gtk_dep ],
    }
  endif
endif

ibus_tests_deps = [
  ibus_dep,
  glib_dep,
  gio_dep,
]

ibus_tests_cflags = [
  '-DIBUS_DISABLE_DEPRECATION_WARNINGS',
  '-DX11_LOCALEDATADIR="@0@"'.format(x11_localedatadir),
]

# XXX what to do with test runner, maybe make testsuite?
ibus_tests_env = environment()

foreach _test: ibus_tests
  test_bin = executable(_test['name'],
    files(['@0@.c'.format(_test['name'])] + _test.get('extra_sources', [])),
    dependencies: ibus_tests_deps + _test.get('extra_deps', []),
    c_args: ibus_tests_cflags,
  )

  if enable_run_tests
  test(_test['name'], test_bin,
    env: ibus_tests_env,
    suite: 'ibus',
  )
  endif
endforeach

if libevdev_dep.found() and libudev_dep.found()
  uinput_replay_bin = executable('uinput-replay',
    sources: files(['uinput-replay.c']),
    c_args: [ '-DUINPUT_REPLAY_MAIN' ],
    include_directories: config_h_inc,
    dependencies: [ libevdev_dep, libudev_dep, glib_dep ],
  )
endif
