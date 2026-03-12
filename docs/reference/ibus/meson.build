ibus_docs_sgml = configure_file(
  input: 'ibus-docs.sgml.in',
  output: '@BASENAME@',
  configuration: {
    'VERSION': meson.project_version(),
  },
)

ibus_docs_ignore_headers = [
  'ibuscomposetable.h',
  'ibusconfigprivate.h',
  'ibusinternal.h',
  'keyname-table.h',
  'keysymdef.h',
  'ibusenumtypes.h',
  'stamp-ibusenumtypes.h',
  'ibusmarshalers.h',
  'stamp-ibusmarshalers.h',
]

gnome.gtkdoc('ibus',
  # XXX
  main_sgml: 'ibus-docs.sgml',
  src_dir: [
    source_root / 'src',
  ],
  ignore_headers: ibus_docs_ignore_headers,
  dependencies: ibus_dep,
  gobject_typesfile: 'ibus.types',
  scan_args: [
    '--rebuild-sections',
  ],
  install: true,
)
