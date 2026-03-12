# GSettings
install_data('org.freedesktop.ibus.gschema.xml',
  install_dir: get_option('datadir') / 'glib-2.0' / 'schemas',
)

# Dconf
install_data('profile/ibus',
  install_dir: get_option('sysconfdir') / 'dconf' / 'profile',
)

custom_target('00-upstream-settings',
  output: '00-upstream-settings',
  command: [
    find_program('make-dconf-override-db.sh'),
    '@OUTDIR@',
    '@CURRENT_SOURCE_DIR@',
  ],
  capture: true,
  install: true,
  install_dir: get_option('sysconfdir') / 'dconf' / 'db' / 'ibus.d',
)

# Man
manpages = [ '00-upstream-settings.5.in', 'ibus.5.in' ]
foreach manpage : manpages
  configure_file(
    input: manpage,
    output: '@BASENAME@',
    configuration: {
      'VERSION': meson.project_version(),
    },
    install: true,
    install_dir: get_option('mandir') / 'man5',
  )
endforeach
