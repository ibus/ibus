project('ibus',
  [ 'c', 'vala' ],
  version: '1.5.25',
  meson_version: '>= 0.53',
)

# Modules
#-------------------------------------------------

fs = import('fs')
gnome = import('gnome')
i18n = import('i18n')
pkgconfig = import('pkgconfig')
python = import('python')

# Versioning
#-------------------------------------------------
_version_split = meson.project_version().split('.')
ibus_major_version = _version_split[0].to_int()
ibus_minor_version = _version_split[1].to_int()
ibus_micro_version = _version_split[2].to_int()

ibus_api_version = ibus_major_version.to_string() + '.0'

ibus_abi_current = 5
ibus_abi_age = 0
ibus_abi_revision = (100 * ibus_minor_version) + ibus_micro_version
ibus_abi_version = '@0@.@1@.@2@'.format(ibus_abi_current, ibus_abi_age, ibus_abi_revision)

# Some variables
#-------------------------------------------------
cc = meson.get_compiler('c')
valac = meson.get_compiler('vala')
source_root = meson.current_source_dir()
build_root = meson.current_build_dir()
po_dir = source_root / 'po'
package_string = meson.project_name() + '-' + ibus_api_version
pkgdatadir = get_option('prefix') / get_option('datadir') / meson.project_name()
ibus_log_domain = 'IBUS'
gettext_package = 'ibus10'

if get_option('socket-dir') == ''
  if target_machine.system() == 'linux'
    ibus_socket_dir = 'unix:tmpdir=$XDG_CACHE_HOME/ibus'
  else
    ibus_socket_dir = 'unix:tmpdir=/tmp'
  endif
else
  ibus_socket_dir = get_option('socket-dir')
endif

if get_option('emoji-dict')
  if not fs.is_file(get_option('unicode-emoji-dir') / 'emoji-test.txt')
    error('Couldn\'t find $UNICODE_EMOJI_DIR/emoji-test.txt. You can get the emoji files from http://www.unicode.org/Public/emoji/4.0/')
  endif

  emoji_annotation_dir = get_option('emoji-annotation-dir')
  if not fs.is_file(emoji_annotation_dir / 'en.xml')
    error('Couldn\'t find $EMOJI_ANNOTATION_DIR/en.xml. You can get it from https://github.com/fujiwarat/cldr-emoji-annotation')
  endif
endif

if get_option('unicode-dict')
  if not fs.is_file(get_option('ucd-dir') / 'NamesList.txt')
    error('Couldn\'t find $UCD_DIR/NamesList.txt. You can get UCD files from https://www.unicode.org/Public/UNIDATA/')
  endif
  if not fs.is_file(get_option('ucd-dir') / 'Blocks.txt')
    error('Couldn\'t find $UCD_DIR/Blocks.txt. You can get UCD files from https://www.unicode.org/Public/UNIDATA/')
  endif
endif

# Dependencies
#-------------------------------------------------
min_glib_version = '2.46' # Also used for Vala's --target-glib, so no micro version.
glib_dep = dependency('glib-2.0', version: '>=' + min_glib_version)
gobject_dep = dependency('gobject-2.0', version: '>=' + min_glib_version)
gio_dep = dependency('gio-2.0', version: '>=' + min_glib_version)
gio_unix_dep = dependency('gio-unix-2.0', version: '>=' + min_glib_version)
dbus_dep = dependency('dbus-1')
posix_dep = valac.find_library('posix')
json_glib_dep = dependency('json-glib-1.0', required: false)

# GTK
gtk2_dep = dependency('gtk+-2.0', required: get_option('gtk2'))
enable_gtk2 = gtk2_dep.found()
if enable_gtk2
  if get_option('gtk2-im-module-dir') != ''
    gtk2_im_module_dir = get_option('gtk2-im-module-dir')
  else
    gtk2_binary_version = gtk2_dep.get_variable(pkgconfig: 'gtk_binary_version')
    gtk2_im_module_dir = get_option('prefix') / get_option('libdir') / 'gtk-2.0' / gtk2_binary_version / 'immodules'
  endif
endif

gtk3_dep = dependency('gtk+-3.0', version: '>= 3.12.0', required: get_option('gtk3'))
enable_gtk3 = gtk3_dep.found()
if enable_gtk3
  gdk3_x11_dep = dependency('gdk-x11-3.0')
  gdk3_wayland_dep = dependency('gdk-wayland-3.0', required: false)

  if get_option('gtk3-im-module-dir') != ''
    gtk3_im_module_dir = get_option('gtk3-im-module-dir')
  else
    gtk3_binary_version = gtk3_dep.get_variable(pkgconfig: 'gtk_binary_version')
    gtk3_im_module_dir = get_option('prefix') / get_option('libdir') / 'gtk-3.0' / gtk3_binary_version / 'immodules'
  endif
endif

gtk4_dep = dependency('gtk4', required: get_option('gtk4'))
enable_gtk4 = gtk4_dep.found()
if enable_gtk4
  if get_option('gtk4-im-module-dir') != ''
    gtk4_im_module_dir = get_option('gtk4-im-module-dir')
  else
    gtk4_binary_version = gtk4_dep.get_variable(pkgconfig: 'gtk_binary_version')
    gtk4_im_module_dir = get_option('prefix') / get_option('libdir') / 'gtk-4.0' / gtk4_binary_version / 'immodules'
  endif
endif

# X11 / XIM
xkbcommon_dep = dependency('xkbcommon', required: false)
x11_dep = dependency('x11', required: get_option('xim'))
enable_xim = x11_dep.found()
if enable_xim
  have_xkblib = cc.has_header('X11/XKBlib.h')

  xfixes_dep = dependency('xfixes', required: false)
  have_xfixes = xfixes_dep.found()

  xtest_dep = dependency('xtst', required: false)
  have_xtest = xfixes_dep.found()

  x11_prefix = x11_dep.get_variable(pkgconfig: 'prefix')
else
  have_xkblib = false
  have_xfixes = false
  have_xtest = false
  x11_prefix = get_option('prefix')
endif

xkbconfig = dependency('xkeyboard-config', required: false)
if xkbconfig.found()
  xkbconfig_base = xkbconfig.get_variable(pkgconfig: 'xkb_base')
else
  xkbconfig_base = get_option('datadir') / 'X11' / 'xkb'
endif

# Wayland
wayland_dep = dependency('wayland-client', version: '>= 1.2.0', required: get_option('wayland'))
enable_wayland = wayland_dep.found() and xkbcommon_dep.found()

# Bindings
vapigen = find_program('vapigen', required: get_option('vapigen'))
enable_vapigen = vapigen.found()

pygobject_dep = dependency('pygobject', version: '>= 3.0.0', required: get_option('pygobject'))
enable_pygobject = pygobject_dep.found()

# DConf
dconf_dep = dependency('dconf', version: '>= 0.7.5', required: get_option('dconf'))
enable_dconf = dconf_dep.found()

# iso-codes
isocodes_prefix = dependency('iso-codes').get_variable(pkgconfig: 'prefix')

# config.h
#-------------------------------------------------
conf = configuration_data()
conf.set_quoted('PACKAGE_NAME', meson.project_name())
conf.set_quoted('PACKAGE_VERSION', meson.project_version())
conf.set_quoted('VERSION', meson.project_version())
conf.set_quoted('GETTEXT_PACKAGE', gettext_package)
conf.set_quoted('BINDIR', get_option('prefix') / get_option('bindir'))
conf.set_quoted('LIBEXECDIR', get_option('prefix') / get_option('libexecdir'))
conf.set_quoted('LOCALEDIR', get_option('prefix') / get_option('localedir'))
conf.set_quoted('PACKAGE_DATADIR', pkgdatadir)
conf.set10('ENABLE_SNOOPER', get_option('snooper'))
conf.set_quoted('NO_SNOOPER_APPS', ','.join(get_option('no-snooper-apps')))
conf.set_quoted('IBUS_SOCKET_DIR', ibus_socket_dir)
conf.set('HAVE_DAEMON', cc.has_function('daemon'))
conf.set('HAVE_LOCALE_H', cc.has_header_symbol('locale.h', 'LC_ALL'))
conf.set('HAVE_SYS_PRCTL_H', cc.has_header_symbol('sys/prctl.h', 'prctl'))
conf.set('HAVE_JSON_GLIB1', json_glib_dep.found())
conf.set('HAVE_XFIXES', have_xfixes)
conf.set('HAVE_X11_XKBLIB_H', have_xkblib)
conf.set('DCONF_0_13_4', dconf_dep.found() and dconf_dep.version().version_compare('>= 0.13.4'))

configure_file(output: 'config.h', configuration: conf)

config_h_inc = include_directories('.')

add_project_arguments('-DHAVE_CONFIG_H', language: 'c')

# Subdirectories
#-------------------------------------------------
subdir('src')
subdir('bindings')
subdir('util')
subdir('conf')
subdir('client')
subdir('portal')
subdir('data')
subdir('po')
# XXX
# subdir('docs')
if get_option('engine')
  subdir('engine')
endif
if get_option('ui')
  subdir('ui')
endif
subdir('tools')
subdir('bus')
if get_option('python2_lib')
  subdir('ibus')
endif
if get_option('setup')
  subdir('setup')
endif

# RPM Spec file
#-------------------------------------------------
ibus_rpm_spec = configure_file(
  input: 'ibus.spec.in',
  output: '@BASENAME@',
  configuration: {
    'IBUS_VERSION': meson.project_version(),
    'DATE_DISPLAY': run_command('date', '+%a %b %d %Y').stdout().strip(),
  },
)
