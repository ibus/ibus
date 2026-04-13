ibus_bin_sources = files(
  'main.vala',
  'krcfile.vala',
)

ibus_bin_deps = [
  glib_dep,
  gio_dep,
  gobject_dep,
  ibus_dep,
  ibus_vapi,
  posix_dep,
  config_vapi,
  dbus_vapi,
]

ibus_bin_cflags = [
  '-DGETTEXT_PACKAGE="@0@"'.format(gettext_package),
  '-DIBUS_COMPILATION',
]

ibus_bin_valaflags = [
]

if get_option('emoji-dict') and get_option('ui')
  ibus_bin_valaflags += [ '-D', 'EMOJI_DICT' ]
endif

if enable_introspection
  libibusimmodule_sources = files([
    'ibusimmodule.c',
  ])
  libibusimmodule_headers = [
    'ibusimmodule.h',
  ]
  libibusimmodule_cflags = [
    '-DGTK2_IM_MODULEDIR="@0@"'.format(gtk2_im_module_dir),
    '-DGTK3_IM_MODULEDIR="@0@"'.format(gtk3_im_module_dir),
    '-DGTK4_IM_MODULEDIR="@0@"'.format(gtk4_im_module_dir),
  ]
  libibusimmodule_deps = [
    glib_dep,
  ]

  libibusimmodule = library('libibusimmodule',
    libibusimmodule_sources,
    include_directories: [ config_h_inc, ],
    c_args: libibusimmodule_cflags,
    dependencies: libibusimmodule_deps,
  )

  libibusimmodule_dep = declare_dependency(
    link_with: libibusimmodule,
    include_directories: [ config_h_inc, ],
    sources: files(libibusimmodule_headers),
  )

  ibusimmodule_gir = gnome.generate_gir(libibusimmodule,
    sources: files(libibusimmodule_headers),
    namespace: 'IBusIMModule',
    nsversion: ibus_api_version,
    identifier_prefix: 'IBus',
    symbol_prefix: [
      'ibus',
    ],
    header: libibusimmodule_headers,
    includes: [ 'GLib-2.0', ],
    extra_args: ibus_cflags + introspection_args,
  )

   ibusimmodule_vapi = gnome.generate_vapi('ibus-immodule-1.0',
    sources: [
      ibusimmodule_gir.get(0),
    ],
    packages: [ 'gio-2.0', ],
    metadata_dirs: [ '.' ],
    vapi_dirs: [ '.' ],
  )

  ibus_bin_deps += [ ibusimmodule_vapi ]
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
