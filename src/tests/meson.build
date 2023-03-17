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

if enable_gtk3
  ibus_tests += {
    'name': 'ibus-compose',
    'extra_deps': [ gtk3_dep ],
  }

  ibus_tests += {
    'name': 'ibus-keypress',
    'extra_deps': [ gtk3_dep, x11_dep, xtest_dep ],
  }
endif

ibus_tests_deps = [
  ibus_dep,
  glib_dep,
  gio_dep,
]

ibus_tests_cflags = [
  '-DIBUS_DISABLE_DEPRECATION_WARNINGS',
  '-DX11_DATA_PREFIX="@0@"'.format(x11_prefix),
]

# XXX what to do with test runner, maybe make testsuite?
ibus_tests_env = environment()

foreach _test: ibus_tests
  test_bin = executable(_test['name'],
    '@0@.c'.format(_test['name']),
    dependencies: ibus_tests_deps + _test.get('extra_deps', []),
    c_args: ibus_tests_cflags,
  )

  test(_test['name'], test_bin,
    env: ibus_tests_env,
    suite: 'ibus',
  )
endforeach
