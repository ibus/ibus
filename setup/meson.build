ibus_setup = configure_file(
  input: 'ibus-setup.in',
  output: '@BASENAME@',
  configuration: {
    'prefix': get_option('prefix'),
    'datarootdir': get_option('prefix') / get_option('datadir'),
    'localedir': get_option('prefix') / get_option('localedir'),
    'libexecdir': get_option('prefix') / get_option('libexecdir'),
    'PYTHON': python.find_installation().path(),
  },
  install_dir: get_option('bindir'),
  install_mode: 'rwxr-xr-x',
)

ibus_setup_desktop = i18n.merge_file(
  input: 'ibus-setup.desktop',
  output: 'org.freedesktop.IBus.Setup.desktop',
  type: 'desktop',
  po_dir: po_dir,
  install: true,
  install_dir: get_option('datadir') / 'applications'
)

# Python files
install_data(
  'emojilang.py',
  'engineabout.py',
  'enginecombobox.py',
  'enginedialog.py',
  'enginetreeview.py',
  'i18n.py',
  'icon.py',
  'keyboardshortcut.py',
  'main.py',

  'setup.ui',

  install_dir: get_option('datadir') / 'ibus' / 'setup',
)

# manpage
configure_file(
  input: 'ibus-setup.1.in',
  output: '@BASENAME@',
  configuration: {
    'VERSION': meson.project_version(),
  },
  install: true,
  install_dir: get_option('mandir') / 'man1',
)
