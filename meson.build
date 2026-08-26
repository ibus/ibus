project('ibus',
  [ 'c', 'vala' ],
  version: '1.5.35@0@'.format(run_command(
      'sh', '-c',
      '. "@0@" 2>/dev/null; test "x$IBUS_RELEASED" = "x1" && echo -n "" || echo -n "@1@"'.format(
          meson.current_source_dir() / 'meson.config',
          '-beta1'),
      check: true).stdout().strip()),
  # Use default_options because
  # add_project_arguments() affects gnome.generate_gir() but
  # g-ir-scanner does not support '-W' options.
  #
  # Set IBUS_ANALYZER=1 to enable '-fanalyzer' flag.
  # Use `meson setup -Db_sanitize=address,leak` to enable ASAN build
  # and need libasan package.
  default_options: [
    'optimization=2',
    'c_args=-g -O2 -Wall -Wformat -Werror=format-security @0@'.format(
        run_command('sh', '-c',
                    '. "@0@" 2 >/dev/null; test "x$IBUS_ANALYZER" = "x0" && echo -n "" || echo -n "@1@"'.format(
            meson.current_source_dir() / 'meson.config',
            '-O0 -fanalyzer'),
        check: true).stdout().strip()),
  ],
  meson_version: '>= 1.5',
  license: 'LGPL-2.1-or-later'
)

# Modules
#-------------------------------------------------

fs = import('fs')
gnome = import('gnome')
i18n = import('i18n')
pkgconfig = import('pkgconfig')
python = import('python').find_installation('python3')
sed = find_program('sed')

# Versioning
#-------------------------------------------------
_version_split = meson.project_version().split('.')
ibus_major_version = _version_split[0].to_int()
ibus_minor_version = _version_split[1].to_int()
ibus_micro_version = _version_split[2].split('-')[0].to_int()

ibus_api_version = ibus_major_version.to_string() + '.0'

ibus_abi_current = 5
ibus_abi_age = 0
ibus_abi_revision = (100 * ibus_minor_version) + ibus_micro_version
ibus_abi_version = '@0@.@1@.@2@'.format(ibus_abi_current,
                                        ibus_abi_age,
                                        ibus_abi_revision)

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
endian = host_machine.endian()

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
    error('No $UNICODE_EMOJI_DIR/emoji-test.txt found: You can get the emoji ' +
          'files from http://www.unicode.org/Public/emoji/4.0/')
  endif

  emoji_annotation_dir = get_option('emoji-annotation-dir')
  if not fs.is_file(emoji_annotation_dir / 'en.xml')
    error('No $EMOJI_ANNOTATION_DIR/en.xml found: You can get it from ' +
          'https://github.com/fujiwarat/cldr-emoji-annotation')
  endif
endif

if get_option('unicode-dict')
  if not fs.is_file(get_option('ucd-dir') / 'NamesList.txt')
    error('No find $UCD_DIR/NamesList.txt found: You can get UCD files from ' +
          'https://www.unicode.org/Public/UNIDATA/')
  endif
  if not fs.is_file(get_option('ucd-dir') / 'Blocks.txt')
    error('No $UCD_DIR/Blocks.txt found: You can get UCD ' +
          'files from https://www.unicode.org/Public/UNIDATA/')
  endif
endif

# Dependencies
#-------------------------------------------------
# Also used for Vala's --target-glib, so no micro version.
min_glib_version = '2.46'
glib_dep = dependency('glib-2.0', version: '>=' + min_glib_version)
gobject_dep = dependency('gobject-2.0', version: '>=' + min_glib_version)
gio_dep = dependency('gio-2.0', version: '>=' + min_glib_version)
gio_unix_dep = dependency('gio-unix-2.0', version: '>=' + min_glib_version)
dbus_dep = dependency('dbus-1')
posix_dep = valac.find_library('posix')
json_glib_dep = dependency('json-glib-1.0', required: false)

# Check to reconfigure build flags.
# Could be failed due to no RPM_ARCH variable as meson-log.txt explaned.
assert(cc.links('''
int main (int argc, char *argv[]) {
  return 0;
}''',
                name: 'test CFLAGS and LDFLAGS'),
       'Failed to compile and link code with current CFLAGS and LDFLAGS')


# I18N
intl_dep = dependency('intl', required: false)
enable_nls = intl_dep.found()
enable_gnu_gettext = enable_nls

# GTK
gtk2_dep = dependency('gtk+-2.0', required: get_option('gtk2'))
enable_gtk2 = gtk2_dep.found()
if enable_gtk2
  if get_option('gtk2-im-module-dir') != ''
    gtk2_im_module_dir = get_option('gtk2-im-module-dir')
  else
    gtk2_binary_version = gtk2_dep.get_variable(pkgconfig: 'gtk_binary_version')
    gtk2_im_module_dir = get_option('prefix') / get_option('libdir') / \
        'gtk-2.0' / gtk2_binary_version / 'immodules'
  endif
else
  gtk2_im_module_dir = ''
endif

enable_gtk3 = get_option('gtk3')
if enable_gtk3
  gtk3_dep = dependency('gtk+-3.0', version: '>= 3.12.0')
  gtk_dep = gtk3_dep
  gdk3_x11_dep = dependency('gdk-x11-3.0', required: false)
  gdk3_wayland_dep = dependency('gdk-wayland-3.0', required: false)

  if get_option('gtk3-im-module-dir') != ''
    gtk3_im_module_dir = get_option('gtk3-im-module-dir')
  else
    gtk3_binary_version = gtk3_dep.get_variable(pkgconfig: 'gtk_binary_version')
    gtk3_im_module_dir = get_option('prefix') / get_option('libdir') / \
        'gtk-3.0' / gtk3_binary_version / 'immodules'
  endif
else
  if get_option('ui')
    error('UI option requires GTK3')
  endif
  gtk3_im_module_dir = ''
endif

enable_gtk4 = get_option('gtk4')
if enable_gtk4
  gtk4_dep = dependency('gtk4')
  gtk_dep = gtk4_dep
  if get_option('gtk4-im-module-dir') != ''
    gtk4_im_module_dir = get_option('gtk4-im-module-dir')
  else
    gtk4_binary_version = gtk4_dep.get_variable(pkgconfig: 'gtk_binary_version')
    gtk4_im_module_dir = get_option('prefix') / get_option('libdir') / \
        'gtk-4.0' / gtk4_binary_version / 'immodules'
  endif
else
  gtk4_im_module_dir = ''
endif

# X11 / XIM
xkbcommon_dep = dependency('xkbcommon', required: false)
x11_dep = dependency('x11', required: get_option('xim'))
enable_xim = x11_dep.found()
if enable_xim
  have_xkblib = cc.has_header('X11/XKBlib.h')

  xfixes_dep = dependency('xfixes', required: false)
  have_xfixes = xfixes_dep.found()

  x11_prefix = x11_dep.get_variable(pkgconfig: 'prefix')
else
  have_xkblib = false
  have_xfixes = false
  x11_prefix = get_option('prefix')
endif

x11_localedatadir = get_option('x11-localedata-dir')
if x11_localedatadir == ''
  x11_localedatadir = x11_prefix / 'share' / 'X11' / 'locale'
endif

xkbconfig = dependency('xkeyboard-config', required: false)
if xkbconfig.found()
  xkbconfig_base = xkbconfig.get_variable(pkgconfig: 'xkb_base')
else
  xkbconfig_base = get_option('prefix') / get_option('datadir') / 'X11' / 'xkb'
endif

# Wayland
wayland_dep = dependency('wayland-client', version: '>= 1.2.0',
                         required: get_option('wayland'))
enable_wayland = wayland_dep.found() and xkbcommon_dep.found()
if enable_wayland
  if meson.version().version_compare('>= 1.8.0')
    wl_mod = import('wayland')
  else
    wl_mod = import('unstable-wayland')
  endif
endif

# Bindings
introspection_dep = dependency('gobject-introspection-1.0', version: '>= 0.6.8',
                               required: get_option('introspection'))
enable_introspection = introspection_dep.found()
if enable_introspection
  introspection_args = [ '--warn-all', ]
  if introspection_dep.version().version_compare('>=1.83.2')
    introspection_args += [ '--doc-format=gi-docgen', ]
  endif
endif

vapigen_bin = find_program('vapigen', required: get_option('vapi'))
enable_vapi = vapigen_bin.found()
if enable_vapi and not enable_introspection
  error('VAPI support requires GObject introspection support')
endif

# Python
pygobject_dep = dependency('pygobject-3.0', version: '>= 3.0.0',
                           required: false)
enable_pygobject = pygobject_dep.found()
if enable_pygobject
  pyoverridesdir = get_option('python-overrides-dir')
  if pyoverridesdir == ''
    pyoverridesdir = run_command(python.full_path(), '-c',
                                 'import gi; print(gi._overridesdir)',
                                 check: true).stdout().strip()
  endif
endif

# DConf
dconf_dep = dependency('dconf', version: '>= 0.7.5',
                       required: get_option('dconf'))
enable_dconf = dconf_dep.found()

# memconf
enable_memconf = get_option('memconf')
build_memconf = enable_memconf or get_option('tests')

# iso-codes
isocodes_prefix = dependency('iso-codes').get_variable(pkgconfig: 'prefix')

# systemd
systemd_dep = dependency('systemd',
                         version: '>= 0.7.5',
                         required: get_option('systemd-services'))
enable_systemd = systemd_dep.found()
if enable_systemd
  # systemd supports the '/usr/local' prefix
  # https://www.freedesktop.org/software/systemd/man/latest/systemd.unit.html
  systemduserunitdir = systemd_dep.get_variable(
      pkgconfig: 'systemduserunitdir',
      pkgconfig_define: ['prefix', get_option('prefix')])
endif

# libnotify
enable_libnotify = get_option('libnotify')
if enable_libnotify
  libnotify_dep = dependency('libnotify', version: '>= 0.7')
endif

# AppIndicator
if get_option('appindicator')
  # dependency('dbusmenu-glib-0.4', method: 'pkg-config') searches pkg-config
  # files but it's expanded to '--pkg dbusmenu-glib-0.4' with
  # executable('ibus-ui-gtk3'). However dbusmenu-glib does not provide VAPI
  # files but Typelib files only.
  # My understanding is, currently dependency('Dbusmenu-0.4') does not support
  # Typelib files so I cannot use dependency() but use declare_dependency().
  dependency('dbusmenu-glib-0.4', method: 'pkg-config')
  dependency('dbusmenu-gtk3-0.4', method: 'pkg-config')
  dbusmenu_glib_dep = declare_dependency(
    compile_args: run_command('pkg-config', '--cflags', 'dbusmenu-glib-0.4',
                              check: true).stdout().strip().split(),
    link_args: run_command('pkg-config', '--libs', 'dbusmenu-glib-0.4',
                           check: true).stdout().strip().split(),
    variables: { 'vala': '--pkg Dbusmenu-0.4' },
  )
  dbusmenu_gtk3_dep = declare_dependency(
    compile_args: run_command('pkg-config', '--cflags', 'dbusmenu-gtk3-0.4',
                              check: true).stdout().strip().split(),
    link_args: run_command('pkg-config', '--libs', 'dbusmenu-gtk3-0.4',
                           check: true).stdout().strip().split(),
    variables: { 'vala': '--pkg DbusmenuGtk3-0.4' },
  )
endif

libevdev_dep = dependency(['libevdev'], required: false)
libudev_dep = dependency(['libudev'], required: false)

# Default tests
exclude_suites_csv = get_option('tests-exclude-suites')
if get_option('tests')
  if exclude_suites_csv != ''
    add_test_setup('exclude_suites',
                   exclude_suites: exclude_suites_csv.split(','),
                   is_default: true,
                  )
  else
    add_test_setup('exclude_suites', is_default: false)
  endif
endif

_ibus_cflags = [ '-DHAVE_CONFIG_H' ]
_ibus_tests_cflags = []
_ibus_link_args = []

buildtype = get_option('buildtype')
if buildtype != 'release'
  if _version_split[2].contains('rc')
    buildtype = 'release'
  elif not _version_split[2].contains('-')
    buildtype = 'release'
  endif
endif
if buildtype == 'release'
  _ibus_cflags += [ '-DG_DISABLE_CAST_CHECKS', '-Wl,-z,relro', '-Wl,-z,now' ]
  _ibus_cflags += [ '-DG_DISABLE_ASSERT' ]
  # GLib testing utilities do not work with `ibus_cflags`
  _ibus_tests_cflags += [ '-UG_DISABLE_ASSERT' ]
  _ibus_link_args += [ '-Wl,-Bsymbolic', '-fno-plt' ]
endif

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
conf.set_quoted('DATADIR', get_option('prefix') / get_option('datadir'))
conf.set10('ENABLE_SNOOPER', get_option('snooper'))
conf.set_quoted('NO_SNOOPER_APPS', ','.join(get_option('no-snooper-apps')))
conf.set_quoted('IBUS_SOCKET_DIR', ibus_socket_dir)
conf.set_quoted('UI_WAYLAND_DESKTOP',
                'org.freedesktop.IBus.Panel.Wayland.Gtk3.desktop')
conf.set('ENABLE_NLS', enable_nls)
conf.set('HAVE_DAEMON', cc.has_function('daemon'))
conf.set('HAVE_GETGRGID_R', cc.has_header_symbol('grp.h', 'getgrgid_r'))
conf.set('HAVE_GETTEXT', enable_gnu_gettext)
conf.set('HAVE_LOCALE_H', cc.has_header_symbol('locale.h', 'LC_ALL'))
conf.set('HAVE_SYS_PRCTL_H', cc.has_header_symbol('sys/prctl.h', 'prctl'))
conf.set('HAVE_JSON_GLIB1', json_glib_dep.found())
conf.set('HAVE_X11_XKBLIB_H', have_xkblib)
conf.set('HAVE_XFIXES', have_xfixes)
conf.set('HAVE_XIM', enable_xim)
conf.set('GLIB2_EMBED_ERRNO_H', glib_dep.found() and
         glib_dep.version().version_compare('>= 2.65.0'))
conf.set('DCONF_0_13_4', dconf_dep.found() and
                         dconf_dep.version().version_compare('>= 0.13.4'))
conf.set('ENABLE_SURROUNDING', 'TRUE')
if buildtype == 'release'
    conf.set('HAVE_PRODUCT_BUILD', 'TRUE')
endif

configure_file(
  input: 'config.h.meson',
  output: '@BASENAME@',
  configuration: conf
)

config_h_inc = include_directories('.')

ibus_cflags =  cc.get_supported_arguments(_ibus_cflags)
ibus_tests_cflags = cc.get_supported_arguments(_ibus_tests_cflags)
ibus_link_args = cc.get_supported_link_arguments(_ibus_link_args)

add_project_arguments(ibus_cflags, language: 'c')
add_project_link_arguments(ibus_link_args, language: 'c')

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
if get_option('engine')
  subdir('engine')
endif
if get_option('ui')
  subdir('ui')
endif
subdir('tools')
subdir('bus')
if get_option('setup')
  subdir('setup')
endif
if get_option('gtk-doc')
  subdir('docs')
endif


# RPM Spec file
#-------------------------------------------------
ibus_rpm_spec = configure_file(
  input: 'ibus.spec.in',
  output: '@BASENAME@',
  configuration: {
    'IBUS_VERSION': meson.project_version(),
    'DATE_DISPLAY': run_command('date', '+%a %b %d %Y',
                                check: true).stdout().strip(),
  },
)

install_emptydir(get_option('datadir') / 'ibus' / 'engine')

summary({
  'CFLAGS':                        get_option('c_args') + ibus_cflags,
  'LDFLAGS':                       get_option('c_link_args') + ibus_link_args,
  'Build type':                    get_option('buildtype'),
  'Optimization':                  get_option('optimization'),
  'Install prefix':                get_option('prefix'),
  'Build endianness':              endian,
}, section: 'Build')

summary({
  'PYTHON':                        python.full_path(),
  'Build PyGObject':               enable_pygobject,
  'Python overrides dir':          pyoverridesdir,
}, section: 'Python')

summary({
  'Gtk2 immodule dir':             gtk2_im_module_dir,
  'Gtk3 immodule dir':             gtk3_im_module_dir,
  'Gtk4 immodule dir':             gtk4_im_module_dir,
  'Build gtk2 immodule':           enable_gtk2,
  'Build gtk3 immodule':           enable_gtk3,
  'Build gtk4 immodule':           enable_gtk4,
  'Build XIM agent server':        enable_xim,
  'Build wayland support':         enable_wayland,
  'Build gdk3 wayland support':    enable_gtk3 ?
                                     gdk3_wayland_dep.found() : false,
  'Build appindicator support':    get_option('appindicator'),
  'Build memconf modules':         enable_memconf,
  'Build dconf modules':           enable_dconf,
  'Build introspection':           enable_introspection,
  'IBus-1.0.gir scannerflags':     introspection_args,
  'Build vala binding':            true,
  'Build document':                get_option('gtk-doc'),
  'Build UI':                      get_option('ui'),
  'Build engine':                  get_option('engine'),
  'Enable key snooper':            get_option('snooper'),
  'No snooper regexes':            get_option('no-snooper-apps'),
  'Enable libnotify':              enable_libnotify,
  'Enable Emoji dict':             get_option('emoji-dict'),
  'Unicode Emoji directory':       get_option('unicode-emoji-dir'),
  'CLDR annotation directory':     get_option('emoji-annotation-dir'),
  'Enable Unicode dict':           get_option('unicode-dict'),
  'UCD directory':                 get_option('ucd-dir'),
  'Socket directory':              ibus_socket_dir,
  'XFixes client disconnect':      have_xfixes,
  'Install systemd service':       enable_systemd,
}, section: 'Components')

summary({
  'Run test cases':                get_option('tests'),
  'Install tests':                 get_option('install-tests'),
}, section: 'Tests')

meson.add_dist_script(python.full_path(), source_root / 'meson-dist.py')
