ibus_vapi = gnome.generate_vapi(package_string,
  sources: [
    ibus_gir.get(0),
    'IBus-1.0-custom.vala',
  ],
  packages: [ 'gio-2.0' ],
  install: true,
)

config_vapi = declare_dependency(
  dependencies: valac.find_library('config',
    dirs: meson.current_source_dir(),
  ),
  include_directories: config_h_inc,
)

gdk3_wayland_vapi = valac.find_library('gdk-wayland',
  dirs: meson.current_source_dir(),
)

xi_vapi = declare_dependency(
  dependencies: [
    valac.find_library('xi',
      dirs: meson.current_source_dir(),
    ),
    cc.find_library('Xi', required: false),
  ],
)

if get_option('emoji-dict')
  # Lordy lordy
  libibus_emoji_dialog_sources = files(
    meson.current_source_dir() / '../../ui/gtk3/candidatearea.vala',
    meson.current_source_dir() / '../../ui/gtk3/emojier.vala',
    meson.current_source_dir() / '../../ui/gtk3/iconwidget.vala',
    meson.current_source_dir() / '../../ui/gtk3/pango.vala',
    meson.current_source_dir() / '../../ui/gtk3/separator.vala',
  )

  libibus_emoji_dialog_deps = [
    glib_dep,
    gio_dep,
    gtk3_dep,
    ibus_dep,
    ibus_vapi,
    config_vapi,
  ]

  libibus_emoji_dialog_cflags = [
    '-DGETTEXT_PACKAGE="@0@"'.format(gettext_package),
    '-DG_LOG_DOMAIN="@0@"'.format(ibus_log_domain),
    '-DPKGDATADIR="@0@"'.format(pkgdatadir),
  ]

  libibus_emoji_dialog_valaflags = []
  if get_option('emoji-dict')
    libibus_emoji_dialog_valaflags += [ '-D', 'EMOJI_DICT' ]
  endif

  libibus_emoji_dialog = library('ibus-emoji-dialog',
    libibus_emoji_dialog_sources, # Defined in ui/gtk3/meson.build
    dependencies: libibus_emoji_dialog_deps,
    c_args: libibus_emoji_dialog_cflags,
    vala_args: libibus_emoji_dialog_valaflags,
  )
endif
