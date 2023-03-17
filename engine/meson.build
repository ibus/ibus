# ibus-engine-simple
ibus_engine_simple_sources = files(
  'main.vala',
)

ibus_engine_simple_deps = [
  glib_dep,
  gio_dep,
  ibus_dep,
  ibus_vapi,
]

ibus_engine_simple_cflags = [
  '-DG_LOG_DOMAIN="@0@"'.format(ibus_log_domain),
]

ibus_engine_simple = executable('ibus-engine-simple',
  ibus_engine_simple_sources,
  dependencies: ibus_engine_simple_deps,
  c_args: ibus_engine_simple_cflags,
  install: true,
  install_dir: get_option('libexecdir'),
)

# simple.xml
simple_xml = custom_target('simple.xml',
  output: 'simple.xml',
  command: [
    find_program('gensimple.py'),
    '--input', xkbconfig_base / 'rules' / 'evdev.xml',
    '--output=@OUTPUT@',
    '--version', meson.project_version(),
    '--exec-path', get_option('prefix') / get_option('libexecdir') / 'ibus-engine-simple',
    '--iso-path', isocodes_prefix / 'share' / 'xml' / 'iso-codes' / 'iso_639.xml',
    '--first-language',
  ],
  install: true,
  install_dir: pkgdatadir / 'component',
)
