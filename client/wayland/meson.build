ibus_wayland_sources = files([
  'main.c',
])

ibus_wayland_common_sources = files([
  'ibuswaylandim.c',
])

ibus_wayland_common_headers = [
  'ibuswaylandim.h',
]

ibus_wayland_vapi_includes = [
  '.',
  '../../bindings/vala',
]

ibus_wayland_includes = [
  include_directories(ibus_wayland_vapi_includes),
  config_h_inc,
]

ibus_wayland_cflags = [
  '-DIBUS_COMPILATION',
]

wayland_built_sources = []

# Format:
#  - protocol name
#  - protocol type ('stable', 'staging', 'unstable', 'private')
#    * 'stable', 'staging' and 'unstable' are from wayland-protocols
#    * 'private' are shipped in this repo
#  - protocol version
#  - protocol code
#  - protocol client header
wayland_protocols = [
  ['input-method', 'unstable', 1, true, true],
  ['input-method', 'private', 2, true, true],
  ['text-input', 'unstable', 1, false, true],
  ['text-input', 'unstable', 3, false, true],
  ['virtual-keyboard', 'private', 1, true, true],
]

foreach p: wayland_protocols
  protocol_name = p.get(0)
  protocol_type = p.get(1)
  protocol_version = p.get(2)
  if protocol_type == 'private'
    protocol_xml = '@0@/@1@-@2@-v@3@.xml'.format(meson.current_source_dir(),
                                                protocol_name,
                                                'unstable',
                                                protocol_version)
  else
    protocol_xml = wl_mod.find_protocol(protocol_name,
                                        state: protocol_type,
                                        version: protocol_version)
  endif
  generated = wl_mod.scan_xml(protocol_xml,
                              client: p.get(4),
                              server: false)

  if p.get(3)
    wayland_built_sources += generated[0]
  endif
  if p.get(4)
    wayland_built_sources += generated[1]
  endif
endforeach

ibus_wayland_deps = [
  ibus_dep,
  gio_dep,
  wayland_dep,
  xkbcommon_dep,
]

executable('ibus-wayland',
  ibus_wayland_sources,
  ibus_wayland_common_sources,
  wayland_built_sources,
  include_directories: config_h_inc,
  c_args: ibus_wayland_cflags,
  dependencies: ibus_wayland_deps,
  install: true,
  install_dir: get_option('libexecdir'),
)

if enable_introspection
  libibuswaylandim = library('ibuswaylandim',
    ibus_wayland_common_sources,
    wayland_built_sources,
    include_directories: ibus_wayland_includes,
    c_args: ibus_wayland_cflags,
    dependencies: ibus_wayland_deps,
  )

  libibuswaylandim_dep = declare_dependency(
    link_with: libibuswaylandim,
    include_directories: ibus_wayland_includes,
    sources: files(ibus_wayland_common_headers),
  )

  ibuswaylandim_gir = gnome.generate_gir(libibuswaylandim,
    sources: files(ibus_wayland_common_headers),
    namespace: 'IBusWaylandIM',
    nsversion: ibus_api_version,
    identifier_prefix: 'IBus',
    symbol_prefix: [
      'ibus',
    ],
    header: ibus_wayland_common_headers,
    includes: [ 'GLib-2.0', 'GObject-2.0', 'Gio-2.0', ],
    extra_args: ibus_cflags + introspection_args,
    export_packages: 'ibus-1.0',
  )

  ibuswaylandim_vapi = gnome.generate_vapi('ibus-wayland-im-1.0',
    sources: [
      ibuswaylandim_gir.get(0),
      'ibus-wayland-im-custom.vala',
    ],
    packages: [ 'gio-2.0', ibus_vapi ],
    metadata_dirs: [ '.' ],
    vapi_dirs: ibus_wayland_vapi_includes,
  )
endif
