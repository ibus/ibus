ibus_gresources_data_file = 'sequences-@0@-endian'.format(endian)

ibus_gresources_data_gen = custom_target(ibus_gresources_data_file,
  input: ibus_gresources_xml,
  output: ibus_gresources_data_file,
  command: [ gen_internal_compose_table ],
  build_by_default: true,
)
