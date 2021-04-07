configure_file(
  input: 'gtkpanel.xml.in',
  output: '@BASENAME@',
  configuration: {
    'VERSION': meson.project_version(),
    'libexecdir': get_option('prefix') / get_option('libexecdir'),
  },
  install: true,
  install_dir: pkgdatadir / 'component',
)

ibus_ui_gtk3_xml_files = [
  'notification-item.xml',
  'notification-watcher.xml',
]

ibus_ui_gtk3_xml_sources = []
foreach xml : ibus_ui_gtk3_xml_files
  xml_source = custom_target('gen-@0@.c'.format(xml),
    input: xml,
    output: 'gen-@0@.c'.format(xml),
    command: [
      # Note that we have to use a python script, due to
      # https://github.com/mesonbuild/meson/issues/1564
      find_program('xml-to-c.py'),
      '@INPUT@',
    ],
    capture: true,
  )

  ibus_ui_gtk3_xml_sources += xml_source
endforeach

ibus_ui_gtk3_sources = files(
  'application.vala',
  'bindingcommon.vala',
  'candidatearea.vala',
  'candidatepanel.vala',
  'emojier.vala',
  'handle.vala',
  'iconwidget.vala',
  'indicator.vala',
  'keybindingmanager.vala',
  'panel.vala',
  'pango.vala',
  'property.vala',
  'propertypanel.vala',
  'separator.vala',
  'switcher.vala',
  'xkblayout.vala',
)

ibus_ui_gtk3_deps = [
  glib_dep,
  gobject_dep,
  gio_dep,
  gtk3_dep,
  gdk3_x11_dep,
  posix_dep,
  x11_dep,
  xi_vapi,
  ibus_dep,
  ibus_vapi,
  config_vapi,
]

ibus_ui_gtk3_cflags = [
  '-DGETTEXT_PACKAGE="@0@"'.format(gettext_package),
  '-DG_LOG_DOMAIN="@0@"'.format(ibus_log_domain),
  '-DPKGDATADIR="@0@"'.format(pkgdatadir),
]

ibus_ui_gtk3_valaflags = []

if get_option('emoji-dict')
  ibus_ui_gtk3_valaflags += [ '-D', 'EMOJI_DICT' ]
endif

if gdk3_wayland_dep.found()
  ibus_ui_gtk3_deps += [ gdk3_wayland_vapi ]
  ibus_ui_gtk3_valaflags += [ '-D', 'USE_GDK_WAYLAND' ]
endif

if get_option('appindicator')
  ibus_ui_gtk3_valaflags += [ '-D', 'INDICATOR' ]
endif

ibus_ui_gtk3 = executable('ibus-ui-gtk3',
  [ ibus_ui_gtk3_sources, ibus_ui_gtk3_xml_sources ],
  dependencies: ibus_ui_gtk3_deps,
  c_args: ibus_ui_gtk3_cflags,
  vala_args: ibus_ui_gtk3_valaflags,
  install: true,
  install_dir: get_option('libexecdir'),
)

if get_option('emoji-dict')
  configure_file(
    input: 'gtkextension.xml.in',
    output: '@BASENAME@',
    configuration: {
      'VERSION': meson.project_version(),
      'libexecdir': get_option('prefix') / get_option('libexecdir'),
    },
    install: true,
    install_dir: pkgdatadir / 'component',
  )

  # ibus-ui-emojier
  ibus_ui_emojier_sources = files(
    'emojier.vala',
    'emojierapp.vala',
    'iconwidget.vala',
    'pango.vala',
    'separator.vala',
  )

  ibus_ui_emojier = executable('ibus-ui-emojier',
    ibus_ui_emojier_sources,
    dependencies: ibus_ui_gtk3_deps,
    c_args: ibus_ui_gtk3_cflags,
    vala_args: ibus_ui_gtk3_valaflags,
    install: true,
    install_dir: get_option('libexecdir'),
  )

  # ibus-extension-gtk3
  ibus_extension_gtk3_sources = files(
    'bindingcommon.vala',
    'emojier.vala',
    'extension.vala',
    'iconwidget.vala',
    'keybindingmanager.vala',
    'panelbinding.vala',
    'pango.vala',
  )

  ibus_extension_gtk3 = executable('ibus-extension-emojier',
    ibus_extension_gtk3_sources,
    dependencies: ibus_ui_gtk3_deps,
    c_args: ibus_ui_gtk3_cflags,
    vala_args: ibus_ui_gtk3_valaflags,
    install: true,
    install_dir: get_option('libexecdir'),
  )

  # manpage
  configure_file(
    input: 'ibus-emoji.7.in',
    output: '@BASENAME@',
    configuration: {
      'VERSION': meson.project_version(),
    },
    install: true,
    install_dir: get_option('mandir') / 'man7',
  )

  # Desktop files
  desktop_filenames = [
    [ 'ibus-ui-emojier.desktop.in',     'org.freedesktop.IBus.Panel.Emojier.desktop'        ],
    [ 'ibus-extension-gtk3.desktop.in', 'org.freedesktop.IBus.Panel.Extension.Gtk3.desktop' ],
  ]
  foreach desktop_entry : desktop_filenames
    i18n.merge_file(
      input: configure_file(
        input: desktop_entry[0],
        output: '@BASENAME@',
        configuration: {
          'libexecdir': get_option('libexecdir'),
        },
      ),
      output: desktop_entry[1],
      type: 'desktop',
      po_dir: po_dir,
      install: true,
      install_dir: get_option('datadir') / 'applications'
    )
  endforeach
endif
