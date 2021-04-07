ibus_sources = files(
  'ibusaccelgroup.c',
  'ibusattribute.c',
  'ibusattrlist.c',
  'ibusbus.c',
  'ibuscomponent.c',
  'ibusconfig.c',
  'ibusconfigservice.c',
  'ibusemoji.c',
  'ibusengine.c',
  'ibusenginedesc.c',
  'ibusenginesimple.c',
  'ibuserror.c',
  'ibusfactory.c',
  'ibushotkey.c',
  'ibusinputcontext.c',
  'ibuskeymap.c',
  'ibuskeynames.c',
  'ibuskeyuni.c',
  'ibuslookuptable.c',
  'ibusobject.c',
  'ibusobservedpath.c',
  'ibuspanelservice.c',
  'ibusproperty.c',
  'ibusproplist.c',
  'ibusproxy.c',
  'ibusregistry.c',
  'ibusserializable.c',
  'ibusservice.c',
  'ibusshare.c',
  'ibustext.c',
  'ibusunicode.c',
  'ibusutil.c',
  'ibusxevent.c',
  'ibusxml.c',
)

ibus_headers = files(
  'ibus.h',
  'ibusaccelgroup.h',
  'ibusattribute.h',
  'ibusattrlist.h',
  'ibusbus.h',
  'ibuscomponent.h',
  'ibusconfig.h',
  'ibusconfigservice.h',
  'ibusdebug.h',
  'ibusemoji.h',
  'ibusengine.h',
  'ibusenginedesc.h',
  'ibusenginesimple.h',
  'ibuserror.h',
  'ibusfactory.h',
  'ibushotkey.h',
  'ibusinputcontext.h',
  'ibuskeymap.h',
  'ibuskeys.h',
  'ibuskeysyms-compat.h',
  'ibuskeysyms.h',
  'ibuslookuptable.h',
  'ibusobject.h',
  'ibusobservedpath.h',
  'ibuspanelservice.h',
  'ibusproperty.h',
  'ibusproplist.h',
  'ibusproxy.h',
  'ibusregistry.h',
  'ibusserializable.h',
  'ibusservice.h',
  'ibusshare.h',
  'ibustext.h',
  'ibustypes.h',
  'ibusunicode.h',
  'ibusutil.h',
  'ibusxevent.h',
  'ibusxml.h',
)

ibus_version_h = configure_file(
  input: 'ibusversion.h.in',
  output: '@BASENAME@',
  configuration: {
    'IBUS_MAJOR_VERSION': ibus_major_version,
    'IBUS_MINOR_VERSION': ibus_minor_version,
    'IBUS_MICRO_VERSION': ibus_micro_version,
  },
)
install_headers(ibus_headers, ibus_version_h,
  subdir: package_string,
)

ibus_marshalers = gnome.genmarshal('ibusmarshalers',
  prefix: '_ibus_marshal',
  sources: 'ibusmarshalers.list',
)

ibus_enums = gnome.mkenums('ibusenumtypes',
  sources: ibus_headers,
  identifier_prefix: 'IBus',
  c_template: 'ibusenumtypes.c.template',
  h_template: 'ibusenumtypes.h.template',
  install_header: true,
  install_dir: get_option('includedir') / package_string,
)

ibus_deps = [
  glib_dep,
  gobject_dep,
  gio_dep,
  gio_unix_dep,
]

ibus_cflags = [
  '-DG_LOG_DOMAIN="@0@"'.format(ibus_log_domain),
  '-DIBUS_CACHE_DIR="@0@"'.format(get_option('localstatedir') / 'cache' / 'ibus'),
  '-DIBUS_DATA_DIR="@0@"'.format(pkgdatadir),
  '-DIBUS_DISABLE_DEPRECATION_WARNINGS',
  '-DIBUS_COMPILATION',
  '-DISOCODES_PREFIX="@0@"'.format(isocodes_prefix),
  '-DX11_DATA_PREFIX="@0@"'.format(x11_prefix),
]

ibus_symbolmap = meson.current_source_dir() / 'libibus.map'
ibus_linkflags = cc.get_supported_link_arguments(
  '-Wl,--version-script,@0@'.format(ibus_symbolmap),
)

ibus_lib = library('ibus-@0@'.format(ibus_api_version),
  [
    ibus_sources,
    ibus_version_h,
    ibus_marshalers,
    ibus_enums,
    files('ibuscomposetable.c'),
  ],
  dependencies: ibus_deps,
  c_args: ibus_cflags,
  link_args: ibus_linkflags,
  include_directories: config_h_inc,
  version: ibus_abi_version,
  install: true,
)

# Easy-to-reuse dependency
ibus_dep = declare_dependency(
  link_with: ibus_lib,
  sources: [
    ibus_enums,
    ibus_marshalers,
  ],
  include_directories: include_directories('.'),
)

# pkgconfig
pkgconfig.generate(ibus_lib,
  filebase: 'ibus-@0@'.format(ibus_api_version),
  name: 'IBus',
  description: 'IBus Library',
  requires: [ 'gobject-2.0', 'gio-2.0' ],
  subdirs: 'ibus-@0@'.format(ibus_api_version),
  variables: [
    'exec_prefix=${prefix}',
    'datadir=${prefix}/share',
    'pkgdatadir=${prefix}/share/ibus',
  ],
)

# GObject Introspection (GIR) file
ibus_gir = gnome.generate_gir(ibus_lib,
  sources: [
    ibus_headers,
    ibus_sources,
    ibus_enums,
    ibus_version_h,
  ],
  namespace: 'IBus',
  nsversion: ibus_api_version,
  identifier_prefix: 'IBus',
  symbol_prefix: [
    'ibus',
  ],
  header: 'ibus.h',
  includes: [ 'GLib-2.0', 'GObject-2.0', 'Gio-2.0', ],
  extra_args: ibus_cflags,
  export_packages: 'ibus-1.0',
  install: true,
)

if get_option('emoji-dict')
  emoji_parser_deps = [
    glib_dep,
    gobject_dep,
    ibus_dep,
  ]

  if json_glib_dep.found()
    emoji_parser_deps += json_glib_dep
  endif

  emoji_parser = executable('emoji-parser',
    'emoji-parser.c',
    dependencies: emoji_parser_deps,
    c_args: ibus_cflags,
    include_directories: config_h_inc,
  )

  emoji_langs = run_command(
    find_program('get_system_langs.py'),
    emoji_annotation_dir,
    check: true,
  ).stdout().split('\n')
  message('Found emoji annotations for @0@'.format(emoji_langs))

  foreach lang : emoji_langs
    gen_emoji_dict = 'emoji-@0@.dict'.format(lang)
    emoji_parser_extra_opts = []

    emoji_xml = emoji_annotation_dir / lang + '.xml'
    derived_xml = emoji_annotation_dir / '..' / 'annotationsDerived' / lang + '.xml'
    if fs.is_file(derived_xml)
      emoji_parser_extra_opts += [ '--xml-derived', derived_xml ]
    endif

    if lang == 'en'
      emoji_parser_extra_opts += [
        '--xml-ascii', files(source_root / 'data' / 'annotations' / 'en_ascii.xml'),
        '--out-category', 'ibusemojigen.h',
      ]
    endif

    emoji_parser_gen = custom_target(gen_emoji_dict,
      input: emoji_xml,
      output: gen_emoji_dict,
      command: [
        emoji_parser,
        '--unicode-emoji-dir', get_option('unicode-emoji-dir'),
        '--xml', emoji_xml,
        '--out', '@OUTPUT@',
        emoji_parser_extra_opts,
      ],
      build_by_default: true,
    )

    # There are cases where there was no .dict file to output
    # so don't try to install them (or Meson complains)
    if fs.exists(meson.current_build_dir() / gen_emoji_dict)
      install_data(meson.current_build_dir() / gen_emoji_dict,
        install_dir: pkgdatadir / 'dicts',
      )
    endif
  endforeach
endif

if get_option('unicode-dict')
  unicode_parser_deps = [
    glib_dep,
    gobject_dep,
    ibus_dep,
  ]

  unicode_parser = executable('unicode-parser',
    'unicode-parser.c',
    dependencies: unicode_parser_deps,
    c_args: ibus_cflags,
    include_directories: config_h_inc,
  )

  unicode_names_dict = custom_target('unicode-names.dict',
    input: get_option('ucd-dir') / 'NamesList.txt',
    output: 'unicode-names.dict',
    command: [
      unicode_parser,
      '--input-names-list', '@INPUT@',
      '--output-names-list', '@OUTPUT@',
    ],
    build_by_default: true,
    install: true,
    install_dir: pkgdatadir / 'dicts',
  )

  unicode_blocks_dict = custom_target('unicode-blocks.dict',
    input: get_option('ucd-dir') / 'Blocks.txt',
    output: 'unicode-blocks.dict',
    command: [
      unicode_parser,
      '--input-blocks', '@INPUT@',
      '--output-blocks-trans', 'ibusunicodegen.h',
      '--output-blocks', '@OUTPUT@',
    ],
    build_by_default: true,
    install: true,
    install_dir: pkgdatadir / 'dicts',
  )
endif

# Tests
if get_option('tests')
  subdir('tests')
endif
