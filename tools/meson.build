ibus_bin_sources = files(
  'main.vala',
)

ibus_bin_deps = [
  glib_dep,
  gio_dep,
  gobject_dep,
  ibus_dep,
  ibus_vapi,
  posix_dep,
  config_vapi,
]

ibus_bin_cflags = [
  '-DGETTEXT_PACKAGE="@0@"'.format(gettext_package),
]

ibus_bin_valaflags = [
]

if get_option('emoji-dict') and get_option('ui')
  ibus_bin_valaflags += [ '-D', 'EMOJI_DICT' ]
endif

ibus_bin = executable('ibus',
  ibus_bin_sources,
  dependencies: ibus_bin_deps,
  c_args: ibus_bin_cflags,
  vala_args: ibus_bin_valaflags,
  install: true,
)

# manpage
configure_file(
  input: 'ibus.1.in',
  output: '@BASENAME@',
  configuration: {
    'VERSION': meson.project_version(),
  },
  install: true,
  install_dir: get_option('mandir') / 'man1',
)

# bash completion
install_data('ibus.bash',
  install_dir: get_option('datadir') / 'bash-completion' / 'completions',
)
