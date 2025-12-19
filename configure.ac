# vim:set et ts=4:
#
# ibus - The Input Bus
#
# Copyright (c) 2007-2016 Peng Huang <shawn.p.huang@gmail.com>
# Copyright (c) 2015-2025 Takao Fujiwara <takao.fujiwara1@gmail.com>
# Copyright (c) 2007-2025 Red Hat, Inc.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
# USA


# If ibus_released is 0, append beta to the version number.
m4_define([ibus_released], [0])

m4_define([ibus_major_version], [1])
m4_define([ibus_minor_version], [5])
m4_define([ibus_micro_version], [34])
m4_define([ibus_beta_version], [alpha1])
m4_define([ibus_abi_current], [5])
m4_define([ibus_abi_revision],
          [m4_eval(100 * ibus_minor_version + ibus_micro_version)])
m4_define([ibus_abi_age], [0])
m4_define([ibus_maybe_datestamp],
    m4_esyscmd([test x]ibus_released[ != x1 && date +.%Y%m%d | tr -d '\n\r']))
m4_define([ibus_beta_suffix],
          m4_if(ibus_released, 0, -ibus_beta_version))
m4_define([ibus_version],
    ibus_major_version.ibus_minor_version.ibus_micro_version[]ibus_beta_suffix)
m4_define([ibus_api_version], [ibus_major_version.0])

AC_PREFEQ([2.62])
AC_INIT([ibus], [ibus_version],
        [https://github.com/ibus/ibus/issues],
        [ibus])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

m4_define([ibus_abi_current_minus_age],
          [m4_eval(ibus_abi_current - ibus_abi_age)])
m4_define([ibus_binary_version],
          [ibus_major_version.ibus_abi_current_minus_age.ibus_abi_age.ibus_abi_revision])

# Required versions of other packages.
m4_define([glib_required_version], [2.46.0])
m4_define([gtk3_required_version], [3.12.0])

# VALA_TARGET_GLIB_VERSION is used by valac --ccode --target-glib .
# VALA_TARGET_GLIB_VERSION and glib_required_version will be different
# in the future.
VALA_TARGET_GLIB_VERSION=2.46
AC_SUBST(VALA_TARGET_GLIB_VERSION)

# Init automake.
AM_INIT_AUTOMAKE([1.11.1 foreign parallel-tests])
AM_MAINTAINER_MODE([enable])
AC_USE_SYSTEM_EXTENSIONS

# Support silent build rules. Disable
# by either passing --disable-silent-rules to configure or passing V=1
# to make
AM_SILENT_RULES([yes])

# Define sustituted variables:
IBUS_MAJOR_VERSION=ibus_major_version
IBUS_MINOR_VERSION=ibus_minor_version
IBUS_MICRO_VERSION=ibus_micro_version
IBUS_ABI_CURRENT=ibus_abi_current
IBUS_ABI_REVISION=ibus_abi_revision
IBUS_ABI_AGE=ibus_abi_age
IBUS_VERSION=ibus_version
IBUS_API_VERSION=ibus_api_version
IBUS_BINARY_VERSION=ibus_binary_version
AC_SUBST(IBUS_MAJOR_VERSION)
AC_SUBST(IBUS_MINOR_VERSION)
AC_SUBST(IBUS_MICRO_VERSION)
AC_SUBST(IBUS_ABI_CURRENT)
AC_SUBST(IBUS_ABI_REVISION)
AC_SUBST(IBUS_ABI_AGE)
AC_SUBST(IBUS_API_VERSION)
AC_SUBST(IBUS_VERSION)
AC_SUBST(IBUS_BINARY_VERSION)

# libtool versioning
#
# If LT_VERSION_INFO="lt_current:lt_revision:lt_age", libibus is
# libibus-ibus_api_version.so.(lt_current - lt_age).lt_age.lt_revision
# If the abi is changed, but it is compatible with last version,
# ibus_abi_current++, ibus_abi_age++;
# If the abi is not compatible with last version,
# ibus_abi_current++, ibus_abi_age = 0;
m4_define([lt_current], [ibus_abi_current])
m4_define([lt_revision], [ibus_abi_revision])
m4_define([lt_age], [ibus_abi_age])
LT_VERSION_INFO="lt_current:lt_revision:lt_age"
LT_CURRENT_MINUS_AGE=m4_eval(lt_current - lt_age)
AC_SUBST(LT_VERSION_INFO)
AC_SUBST(LT_CURRENT_MINUS_AGE)

# Define GETTEXT_* variables.
GETTEXT_PACKAGE=ibus10
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE",
                   [The prefix for out gettext translation domains.])
DEFINE_IBUS_LOCALEDIR
AC_DEFINE_UNQUOTED(LOCALEDIR, "$ibus_localedir",
                   [Define the location where the catalogs will be installed])

# For dislpay date.
m4_define(ibus_datedisplay,
    m4_esyscmd(date '+%a %b %d %Y' | tr -d '\n\r'))
DATE_DISPLAY="ibus_datedisplay"
AC_SUBST(DATE_DISPLAY)


# Check for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AM_PROG_VALAC([0.20])
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AX_PROG_CC_FOR_BUILD

# i18n stuff
AM_GNU_GETTEXT_VERSION([0.19.8])
AM_GNU_GETTEXT([external])

# Define PACKAGE_VERSION_* variables.
AC_DISABLE_STATIC([])
AC_SEARCH_LIBS([strerror],[cposix])

LT_INIT

AC_MSG_CHECKING([for product build])
# --disable-product-build option.
AC_ARG_ENABLE(product-build,
    AS_HELP_STRING([--disable-product-build],
                   [Disable CFLAGS for the product release]),
    [enable_product_build="$enableval"],
    [enable_product_build="auto"]
)
if test x"$enable_product_build" = xauto; then
    if echo ibus_beta_version | grep -q "rc"; then
        enable_product_build="yes"
    else
        enable_product_build="no"
    fi
fi
TEST_CFLAGS="$CFLAGS"
PRODUCT_LIB_CFLAGS=''
if test x"$enable_product_build" = xyes; then
    if test $ac_compiler_gnu = yes; then
        _CFLAGS="-DG_DISABLE_CAST_CHECKS -Wl,-z,relro -Wl,-z,now $CFLAGS"
        TEST_CFLAGS="$_CFLAGS"
        CFLAGS="-DG_DISABLE_ASSERT $_CFLAGS"
        PRODUCT_LIB_CFLAGS="-Wl,-Bsymbolic -fno-plt"
    fi
    AC_DEFINE(HAVE_PRODUCT_BUILD, 1, [Enable product build])
fi
AC_SUBST(PRODUCT_LIB_CFLAGS)
AC_SUBST(TEST_CFLAGS)
AC_MSG_RESULT([$enable_product_build])

# Check header filess.
AC_CHECK_HEADERS([sys/prctl.h])

# Check functions.
AC_CHECK_FUNCS(daemon getgrgid_r)

# Check dlclose() in libc.so.
AC_CHECK_LIB(c, dlclose, LIBDL="", [AC_CHECK_LIB(dl, dlclose, LIBDL="-ldl")])
AC_SUBST(LIBDL)

# Check if static build only
AC_MSG_CHECKING([for IBUS_STATIC_COMPILATION])
ibus_build_static_only=0
if test "x$enable_static" = "xyes" -a "x$enable_shared" != "xyes" ; then
    ibus_build_static_only=1
    AC_DEFINE(IBUS_STATIC_COMPILATION, $ibus_build_static_only,
              [Define to build static])
fi
AC_MSG_RESULT([$ibus_build_static_only])

# Check if cross compiling.
AM_CONDITIONAL(CROSS_COMPILING, test "x$cross_compiling" = xyes)

# Check endianness.
AC_C_BIGENDIAN([ENDIAN=big], [ENDIAN=little], [ENDIAN=unknown], [ENDIAN=big])
if test x"$ENDIAN" != xlittle  -a x"$ENDIAN" != xbig; then
    AC_MSG_ERROR([Cannot determine endianness. Use ac_cv_c_bigendian to yes or no])
fi
AC_SUBST(ENDIAN)
ENDIAN_MSG="$ENDIAN (ac_cv_c_bigendian=$ac_cv_c_bigendian)"

# Check packages.
# Check glib2.
AM_PATH_GLIB_2_0
PKG_CHECK_MODULES(GLIB2, [
    glib-2.0 >= glib_required_version
])
PKG_CHECK_MODULES(GOBJECT2, [
    gobject-2.0 >= glib_required_version
])
PKG_CHECK_MODULES(GIO2, [
    gio-2.0 gio-unix-2.0 >= glib_required_version
])
PKG_CHECK_MODULES(GTHREAD2, [
    gthread-2.0 >= glib_required_version
])
PKG_CHECK_EXISTS(
    [glib-2.0 >= 2.65.0],
    [AC_DEFINE(GLIB2_EMBED_ERRNO_H, TRUE, [glib2 include errno.h])]
)
AH_BOTTOM([
#ifndef GLIB2_EMBED_ERRNO_H
#  include <errno.h>
#endif
])

if test "x$cross_compiling" = "xyes"; then
PKG_PROG_PKG_CONFIG_FOR_BUILD
GLIB_CFLAGS_FOR_BUILD=`$PKG_CONFIG_FOR_BUILD --cflags glib-2.0 gobject-2.0 gio-2.0 gio-unix-2.0 gthread-2.0`
GLIB_LIBS_FOR_BUILD=`$PKG_CONFIG_FOR_BUILD --libs glib-2.0 gobject-2.0 gio-2.0 gio-unix-2.0 gthread-2.0`
AC_SUBST(GLIB_CFLAGS_FOR_BUILD)
AC_SUBST(GLIB_LIBS_FOR_BUILD)
fi

# --disable-tests option.
AC_ARG_ENABLE(tests,
    AS_HELP_STRING([--disable-tests],
                   [Do not run tests]),
    [enable_tests=$enableval],
    [enable_tests=yes]
)
AM_CONDITIONAL([ENABLE_TESTS], [test x"$enable_tests" = x"yes"])
if test x"$enable_tests" != x"yes"; then
    enable_tests="no (disabled, use --enable-tests to enable)"
fi

# --disable-gtk2 option.
AC_ARG_ENABLE(gtk2,
    AS_HELP_STRING([--disable-gtk2],
                   [Do not build gtk2 im module]),
    [enable_gtk2=$enableval],
    [enable_gtk2=yes]
)
AM_CONDITIONAL([ENABLE_GTK2], [test x"$enable_gtk2" = x"yes"])

# --disable-gtk3 option.
AC_ARG_ENABLE(gtk3,
    AS_HELP_STRING([--disable-gtk3],
                   [Do not build gtk3 im module]),
    [enable_gtk3=$enableval],
    [enable_gtk3=yes]
)
AM_CONDITIONAL([ENABLE_GTK3], [test x"$enable_gtk3" = x"yes"])

# --enable-gtk4 option.
AC_ARG_ENABLE(gtk4,
    AS_HELP_STRING([--enable-gtk4],
                   [Build gtk4 im module]),
    [enable_gtk4=$enableval],
    [enable_gtk4=yes]
)
AM_CONDITIONAL([ENABLE_GTK4], [test x"$enable_gtk4" = x"yes"])

AM_CONDITIONAL([ENABLE_GTK3_OR_4],
               [test x"$enable_gtk3" = x"yes" || test x"$enable_gtk4" = x"yes"])

# --disable-xim option.
AC_ARG_ENABLE(xim,
    AS_HELP_STRING([--disable-xim],
                   [Do not build xim server]),
    [enable_xim=$enableval],
    [enable_xim=yes]
)
AM_CONDITIONAL([ENABLE_XIM], [test x"$enable_xim" = x"yes"])

# --enable-wayland option.
AC_ARG_ENABLE(wayland,
    AS_HELP_STRING([--enable-wayland],
                   [Build wayland support]),
    [enable_wayland=$enableval],
    [enable_wayland=yes]
)
AM_CONDITIONAL([ENABLE_WAYLAND], [test x"$enable_wayland" = x"yes"])

# --enable-appindicator option.
AC_ARG_ENABLE(appindicator,
    AS_HELP_STRING([--enable-appindicator],
                   [Build appindicator support]),
    [enable_appindicator=$enableval],
    [enable_appindicator=yes]
)
AM_CONDITIONAL([ENABLE_APPINDICATOR], [test x"$enable_appindicator" = x"yes"])

if test x"$enable_gtk2" = x"yes"; then
    # check for gtk2
    PKG_CHECK_MODULES(GTK2, [
        gtk+-2.0
    ])

    gtk2_binary_version=`$PKG_CONFIG --variable=gtk_binary_version gtk+-2.0`
    GTK2_IM_MODULEDIR="$libdir"/gtk-2.0/$gtk2_binary_version/immodules
    enable_gtk2="yes (enabled, use --disable-gtk2 to disable)"
fi

if test x"$enable_gtk3" = x"yes"; then
    # check for gtk3
    PKG_CHECK_MODULES(GTK3, [
        gtk+-3.0 >= gtk3_required_version
    ])
    if test x"$enable_wayland" = x"yes"; then
        PKG_CHECK_EXISTS([gdk-wayland-3.0],
            [enable_gdk3_wayland=yes],
            [enable_gdk3_wayland=no]
        )
    else
        enable_gdk3_wayland=no
    fi

    gtk3_binary_version=`$PKG_CONFIG --variable=gtk_binary_version gtk+-3.0`
    GTK3_IM_MODULEDIR="$libdir"/gtk-3.0/$gtk3_binary_version/immodules
else
    enable_gdk3_wayland=no
fi
if test x"$enable_gdk3_wayland" != x"yes"; then
    enable_gdk3_wayland="no (disabled, need to install gdk-wayland-3.0.pc)"
fi
AM_CONDITIONAL([ENABLE_GDK3_WAYLAND], [test x"$enable_gdk3_wayland" = x"yes"])
IBUS_UI_WAYLAND_DESKTOP="org.freedesktop.IBus.Panel.Wayland.Gtk3.desktop"
AC_SUBST(IBUS_UI_WAYLAND_DESKTOP)
AC_DEFINE_UNQUOTED(UI_WAYLAND_DESKTOP, "$IBUS_UI_WAYLAND_DESKTOP",
                   [Desktop filename of IBus Wayland UI.])
AC_DEFINE_UNQUOTED(DATADIR, "$ibus_datadir", [Datadir.])

if test x"$enable_gtk4" = x"yes"; then
    # check for gtk4
    PKG_CHECK_MODULES(GTK4, [
        gtk4
    ])

    gtk4_binary_version=`$PKG_CONFIG --variable=gtk_binary_version gtk4`
    GTK4_IM_MODULEDIR="$libdir"/gtk-4.0/$gtk4_binary_version/immodules
    enable_gtk4="yes (enabled, use --disable-gtk4 to disable)"
fi

if test x"$enable_xim" = x"yes"; then
    # Check for x11
    PKG_CHECK_MODULES(X11, [
        x11
    ])
    AC_CHECK_HEADERS([X11/XKBlib.h])

    # if we don't have gtk3 yet, check for gtk2
    if test x"$enable_gtk3" != x"yes"; then
        # check for gtk2
        PKG_CHECK_MODULES(GTK2, [
            gtk+-2.0
        ])
    fi
    AC_SUBST([XIM_CLI_ARG], [--xim])
    AC_DEFINE(HAVE_XIM, 1, [Define to enable XIM])
    enable_xim="yes (enabled, use --disable-xim to disable)"
fi
if test x"$enable_gtk3" = x"yes"; then
    enable_gtk3="yes (enabled, use --disable-gtk3 to disable)"
fi

if $PKG_CONFIG --exists x11; then
    X11_PREFIX="`$PKG_CONFIG --variable=prefix x11`"
elif test x"$prefix" != xNONE; then
    X11_PREFIX="$prefix"
else
    X11_PREFIX="$ac_default_prefix"
fi
AC_SUBST(X11_PREFIX)

# Check locale dir for Compose files.
if test x"$cross_compiling" != xyes; then
    AC_CHECK_FILE($X11_PREFIX/share/X11/locale/locale.dir,
                  X11_LOCALEDATADIR="$X11_PREFIX/share/X11/locale",
                  [AC_CHECK_FILE($X11_PREFIX/lib/X11/locale/locale.dir,
                                 X11_LOCALEDATADIR="$X11_PREFIX/lib/X11/locale",
                                 X11_LOCALEDATADIR="$(datadir)/X11/locale")])
else
    if test x"$X11_LOCALEDATADIR" = x; then
        X11_LOCALEDATADIR="$X11_PREFIX/share/X11/locale"
    fi
    AC_MSG_RESULT([Skipping X11 locale directory check when cross compiling. Using: $X11_LOCALEDATADIR])
fi
AC_SUBST(X11_LOCALEDATADIR)

if test x"$enable_wayland" = x"yes"; then
    # Check for wayland
    PKG_CHECK_MODULES(WAYLAND, [
        wayland-client >= 1.2.0
        xkbcommon
    ])
    PKG_CHECK_EXISTS([wayland-protocols],
        [WAYLAND_PRTCLS_DIR="`$PKG_CONFIG --variable=pkgdatadir wayland-protocols`"],
        [WAYLAND_PRTCLS_DIR="/usr/share/wayland-protocols"]
    )
    WAYLAND_PRTCLS_DIR="`echo $WAYLAND_PRTCLS_DIR`"
    m4_ifdef([WAYLAND_SCANNER_RULES],
             [WAYLAND_IM_PRTCL_SUBDIR='unstable/input-method'
              WAYLAND_TEXT_PRTCL_SUBDIR='unstable/text-input'
              AC_SUBST(WAYLAND_PRTCLS_DIR)
              AC_SUBST(WAYLAND_IM_PRTCL_SUBDIR)
              AC_SUBST(WAYLAND_TEXT_PRTCL_SUBDIR)
              WAYLAND_SCANNER_RULES(['$(WAYLAND_PRTCLS_DIR)/$(WAYLAND_IM_PRTCL_SUBDIR)'])],
             [AC_SUBST(wayland_scanner_rules)])
    enable_wayland="yes (enabled, use --disable-wayland to disable)"
else
    wayland_scanner_rules="./wayland-scanner.mk"
    AC_SUBST(wayland_scanner_rules)
fi

# GObject introspection
GOBJECT_INTROSPECTION_CHECK([0.6.8])

IBUS_GIR_SCANNERFLAGS=
if test x"$found_introspection" = x"yes" ; then
    IBUS_GIR_SCANNERFLAGS="--warn-all --identifier-prefix=IBus --symbol-prefix=ibus --c-include=ibus.h"
    PKG_CHECK_EXISTS([gobject-introspection-1.0 >= 0.9.6],
                     [gir_symbol_prefix=yes],
                     [gir_symbol_prefix=no])
    if test x"$gir_symbol_prefix" = x"no" ; then
        IBUS_GIR_SCANNERFLAGS="--strip-prefix=IBus"
    fi
fi
AC_SUBST(IBUS_GIR_SCANNERFLAGS)

# Check vapigen.
VAPIGEN_CHECK([0.16])

# Check for gtk-doc.
GTK_DOC_CHECK(1.9)
if test x"$enable_gtk_doc" = x"no"; then
    enable_gtk_doc="no (disabled, use --enable-gtk-doc to enable)"
fi

# Check for dbus.
PKG_CHECK_MODULES(DBUS, [
    dbus-1
])

# --enable-memconf option.
AC_ARG_ENABLE(memconf,
    AS_HELP_STRING([--enable-memconf],
                   [Enable configure base on memory]),
    [enable_memconf=$enableval],
    [enable_memconf=no]
)
AM_CONDITIONAL([ENABLE_MEMCONF], [test "x$enable_memconf" = "xyes"])

AC_ARG_ENABLE(dconf,
    AS_HELP_STRING([--disable-dconf],
                   [Disable configure base on dconf]),
    [enable_dconf=$enableval],
    [enable_dconf=yes]
)
AM_CONDITIONAL([ENABLE_DCONF], [test x"$enable_dconf" = x"yes"])

if test x"$enable_dconf" = x"yes"; then
    # check dconf
    PKG_CHECK_MODULES(DCONF,
        [dconf >= 0.7.5],
    )
    PKG_CHECK_EXISTS([dconf >= 0.13.4],
        [AC_DEFINE(DCONF_0_13_4, TRUE, [dconf is 0.13.4 or later])],
        [])
    # check glib-compile-schemas
    GLIB_GSETTINGS
    enable_dconf="yes (enabled, use --disable-dconf to disable)"
fi

AC_ARG_ENABLE(systemd-services,
    AS_HELP_STRING([--disable-systemd-services],
                   [Disable systemd services installation]),
    [enable_systemd=$enableval],
    [enable_systemd=yes]
)
AM_CONDITIONAL([ENABLE_SYSTEMD], [test x"$enable_systemd" = x"yes"])

if test x"$enable_systemd" = x"yes"; then
    PKG_CHECK_MODULES(SYSTEMD, [
        systemd >= 0.7.5
    ])
    AC_SUBST([SYSTEMD_USER_UNIT_DIR], [`$PKG_CONFIG --define-variable prefix='${prefix}' --variable systemduserunitdir systemd`])
    enable_systemd="yes (enabled, use --disable-systemd-services to disable)"
fi

# Check env.
AC_PATH_PROG(ENV_IBUS_TEST, env)
AC_SUBST(ENV_IBUS_TEST)

AC_ARG_ENABLE(python2,
    AS_HELP_STRING([--disable-python2],
                   [Do not install bindings/pygobject/gi and ibus for python2.
                    '--disable-python2' bring '--disable-python-library'.]),
    [enable_python2=$enableval],
    [enable_python2=yes]
)

AC_ARG_ENABLE(python-library,
    AS_HELP_STRING([--enable-python-library],
                   [Use ibus python library]),
    [enable_python_library=$enableval],
    [enable_python_library=no]
)

AC_ARG_ENABLE(setup,
    AS_HELP_STRING([--disable-setup],
                   [Do not use setup ui.]),
    [enable_setup=$enableval],
    [enable_setup=yes]
)

# Define python version
AC_ARG_WITH(python,
    AS_HELP_STRING([--with-python[=PATH]],
        [Select python2 or python3]),
    [PYTHON=$with_python], []
)

AM_PATH_PYTHON([2.5])

if test x"$enable_python2" != x"yes"; then
    enable_python_library=no
    PYTHON2=
    enable_python2="no (disabled, use --enable-python2 to enable)"
else
    AC_PATH_PROG(PYTHON2, python2)

    if test x"$PYTHON2" = x""; then
        PYTHON2=$PYTHON
    fi
fi

AM_CONDITIONAL([ENABLE_PYTHON2], [test x"$enable_python2" = x"yes"])
AM_CONDITIONAL([ENABLE_PYTHON_LIBRARY], [test x"$enable_python_library" = x"yes"])
AM_CONDITIONAL([ENABLE_SETUP], [test x"$enable_setup" = x"yes"])
AM_CONDITIONAL([ENABLE_DAEMON], [true])

PYGOBJECT_REQUIRED=3.0.0

# Define Python overrides dir.
# When IBus is built with the Json files in test directory, the build
# tries to install IBus.py file into pyoverridesdir.
# The default GI directory is /usr in Flatpak but the directory is readonly
# with `flatpak-builder` and the Flatpak specific GI files are installed in
# /app GI directory.
AC_ARG_WITH(python-overrides-dir,
    AS_HELP_STRING([--with-python-overrides-dir[=DIR]],
        [Select Python overrides dir]),
    pyoverridesdir=$with_python_overrides_dir
)

PKG_CHECK_EXISTS([pygobject-3.0 >= $PYGOBJECT_REQUIRED],
                 [enable_pygobject=yes], [enable_pygobject=no])

if test "x$enable_pygobject" = "xyes"; then
    PKG_CHECK_MODULES(PYTHON, [pygobject-3.0 >= $PYGOBJECT_REQUIRED])

    if test x"$pyoverridesdir" = x""; then
        pyoverridesdir=`$PYTHON -c "import gi; print(gi._overridesdir)"`
    fi
    AC_SUBST(pyoverridesdir)

    if test x"$enable_python2" = x"yes"; then
        py2overridesdir=`$PYTHON2 -c "import gi; print(gi._overridesdir)"`
        AC_SUBST(py2overridesdir)
    fi
fi

AM_CONDITIONAL(ENABLE_PYGOBJECT, test x"$enable_pygobject" = "xyes")

if test x"$enable_python_library" = x"yes"; then
    # Check python.
    AC_PATH_PROG(PYTHON_CONFIG, python$PYTHON_VERSION-config)
    if test x"$PYTHON_CONFIG" = x""; then
        AC_PATH_PROG(PYTHON_CONFIG, python-config)
    fi
    if test x"$PYTHON_CONFIG" != x""; then
        PYTHON_CFLAGS=`$PYTHON_CONFIG --includes`
        PYTHON_LIBS=`$PYTHON_CONFIG --libs`
    else
        PYTHON_CFLAGS=`$PYTHON $srcdir/python-config.py --includes`
        PYTHON_LIBS=`$PYTHON $srcdir/python-config.py --libs`
    fi
    PYTHON_INCLUDES="$PYTHON_CFLAGS"
    AC_SUBST(PYTHON_CFLAGS)
    AC_SUBST(PYTHON_INCLUDES)
    AC_SUBST(PYTHON_LIBS)

    PYTHON2_PREFIX=`$PYTHON2 -c "import sys; sys.stdout.write(sys.prefix)"`
    PYTHON2_VERSION=`$PYTHON2 -c "import sys; sys.stdout.write(sys.version[[:3]])"`
    PYTHON2_LIBDIR="$PYTHON2_PREFIX/lib/python$PYTHON2_VERSION"
    python2dir="$PYTHON2_LIBDIR/site-packages"
    pkgpython2dir="$python2dir/ibus"
    AC_SUBST(pkgpython2dir)
else
    enable_python_library="no (disabled, use --enable-python-library to enable)"
fi

# Define gtk2 immodule dir.
AC_ARG_WITH(gtk2-im-module-dir,
    AS_HELP_STRING([--with-gtk2-im-module-dir[=DIR]],
        [Select gtk2 immodule dir]),
    GTK2_IM_MODULEDIR=$with_gtk2_im_module_dir
)
AC_SUBST(GTK2_IM_MODULEDIR)

# Define gtk3 immodule dir.
AC_ARG_WITH(gtk3-im-module-dir,
    AS_HELP_STRING([--with-gtk3-im-module-dir[=DIR]],
        [Select gtk3 immodule dir]),
    GTK3_IM_MODULEDIR=$with_gtk3_im_module_dir
)
AC_SUBST(GTK3_IM_MODULEDIR)

# Define gtk4 immodule dir.
AC_ARG_WITH(gtk4-im-module-dir,
    AS_HELP_STRING([--with-gtk4-im-module-dir[=DIR]],
        [Select gtk4 immodule dir]),
    GTK4_IM_MODULEDIR=$with_gtk4_im_module_dir
)
AC_SUBST(GTK4_IM_MODULEDIR)

AC_MSG_CHECKING([for $PYTHON GI GnomeDesktop module])
if AC_RUN_LOG([$PYTHON -c "def _configure_test():
    from gi import require_version as gi_require_version
    gi_require_version('GnomeDesktop', '4.0')
    from gi.repository import GnomeDesktop
_configure_test()"]); then
    enable_pygnome_desktop="yes"
else
    enable_pygnome_desktop="no"
fi
AC_MSG_RESULT([$enable_pygnome_desktop])
AM_CONDITIONAL([ENABLE_PYGNOME_DESKTOP],
               [test x"$enable_pygnome_desktop" = x"yes"])

if test x"$enable_python" = x"yes"; then
    # Check for dbus-python.
    AC_ARG_ENABLE(dbus-python-check,
        AS_HELP_STRING([--disable-dbus-python-check],
            [Do not check dbus-python]),
        [enable_dbus_python_check=$enableval],
        [enable_dbus_python_check=yes]
    )
    PKG_CHECK_MODULES(DBUS_PYTHON,
        [dbus-python >= 0.83.0],
        [IBUS_HAS_DBUS_PYTHON=yes],
        [IBUS_HAS_DBUS_PYTHON=no]
    )
    if test x"$IBUS_HAS_DBUS_PYTHON" != x"yes"; then
        if test x"$enable_dbus_python_check" != x"no"; then
            AC_MSG_ERROR([can not find dbus-python >= 0.83.0. Please install or update dbus-python.])
        else
            AC_MSG_WARN([can not find dbus-python >= 0.83.0. It is required.])
        fi
    fi
fi

# Option for always disable snooper applications.
AC_ARG_ENABLE(key-snooper,
    AS_HELP_STRING([--disable-key-snooper],
        [Always disable key snooper in gtk im module]),
    [enable_key_snooper=$enableval],
    [enable_key_snooper=yes]
)
if test x"$enable_key_snooper" = x"yes"; then
    AC_DEFINE(ENABLE_SNOOPER, TRUE, [Enable key snooper])
else
    AC_DEFINE(ENABLE_SNOOPER, FALSE, [Enable key snooper])
    enable_key_snooper="no (disabled, use --enable-key-snooper to enable)"
fi

# Option for no snooper applications.
AC_ARG_WITH(no-snooper-apps,
    AS_HELP_STRING([--with-no-snooper-apps[=regex1,regex2]],
        [Does not enable keyboard snooper in those applications (like: .*chrome.*,firefox.*)]),
    NO_SNOOPER_APPS=$with_no_snooper_apps,
    NO_SNOOPER_APPS=[firefox.*,.*chrome.*,.*chromium.*,terminator]
)
AC_DEFINE_UNQUOTED(NO_SNOOPER_APPS, "$NO_SNOOPER_APPS",
    [Does not enbale keyboard snooper in those applications])

# GNOME 3 uses the theme's icon.
AC_ARG_WITH(panel-icon-keyboard,
    AS_HELP_STRING([--with-panel-icon-keyboard[=icon_name]],
        [Set the default panel icon (default: "ibus-keyboard")]),
    [if test x"$with_panel_icon_keyboard" = x"yes" -o \
             x"$with_panel_icon_keyboard" = x; then
         with_panel_icon_keyboard="input-keyboard-symbolic"
     fi
     if test x"$with_panel_icon_keyboard" = x"legacy"; then
         with_panel_icon_keyboard="ibus-keyboard"
     fi
     IBUS_ICON_KEYBOARD=$with_panel_icon_keyboard
    ],
    IBUS_ICON_KEYBOARD="ibus-keyboard"
)
AC_SUBST(IBUS_ICON_KEYBOARD)

# --disable-surrounding-text option.
AC_ARG_ENABLE(surrounding-text,
    AS_HELP_STRING([--disable-surrounding-text],
        [Disable surrounding-text support]),
    [enable_surrounding_text=$enableval],
    [enable_surrounding_text=yes]
)
if test x"$enable_surrounding_text" = x"yes"; then
    AC_DEFINE(ENABLE_SURROUNDING, TRUE, [Enable surrounding-text support])
    enable_surrounding_text="yes (enabled, use --disable-surrounding-text to disable)"
fi

# --disable-ui
AC_ARG_ENABLE(ui,
    AS_HELP_STRING([--disable-ui],
                   [Disable ibus default user interface]),
    [enable_ui=$enableval],
    [enable_ui=yes]
)
AM_CONDITIONAL([ENABLE_UI], [test x"$enable_ui" = x"yes"])
if test x"$enable_ui" = x"yes"; then
    enable_ui="yes (enabled, use --disable-ui to disable)"
fi

if test x"$enable_appindicator" = x"yes"; then
    PKG_CHECK_MODULES(DBUSMENU_GLIB, [dbusmenu-glib-0.4])
    PKG_CHECK_MODULES(DBUSMENU_GTK3, [dbusmenu-gtk3-0.4])
    enable_appindicator="yes (enabled, use --disable-appindicator to disable)"
fi

# --disable-engine
AC_ARG_ENABLE(engine,
    AS_HELP_STRING([--disable-engine],
                   [Disable ibus simple engine]),
    [enable_engine=$enableval],
    [enable_engine=yes]
)
AM_CONDITIONAL([ENABLE_ENGINE], [test x"$enable_engine" = x"yes"])
if test x"$enable_engine" = x"yes"; then
    enable_engine="yes (enabled, use --disable-engine to disable)"
fi

# --disable-libnotify
AC_ARG_ENABLE(libnotify,
    AS_HELP_STRING([--disable-libnotify],
                   [Disable to link libnotify]),
    [enable_libnotify=$enableval],
    [enable_libnotify=yes]
)
AM_CONDITIONAL([ENABLE_LIBNOTIFY], [test x"$enable_libnotify" = x"yes"])
if test x"$enable_libnotify" = x"yes"; then
    PKG_CHECK_MODULES(LIBNOTIFY, [
        libnotify >= 0.7
    ])
    enable_libnotify="yes (enabled, use --disable-libnotify to disable)"
fi

PKG_CHECK_MODULES(LIBEVDEV,
    [libevdev libudev],
    [enable_libevdev=yes],
    [enable_libevdev=no]
)
AM_CONDITIONAL([ENABLE_LIBEVDEV], [test x"$enable_libevdev" = x"yes"])

PKG_CHECK_MODULES(XFIXES,
    [x11 xfixes >= 6],
    [AC_DEFINE([HAVE_XFIXES], [1], [Define to enable XFixes])
     have_xfixes=yes],
    [have_xfixes="no (libXfixes version is lower than 6)"]
)

# --enable-install-tests
AC_ARG_ENABLE(install-tests,
    AS_HELP_STRING([--enable-install-tests],
                   [Enable to install tests]),
    [enable_install_tests=$enableval],
    [enable_install_tests=no]
)
AM_CONDITIONAL([ENABLE_INSTALL_TESTS], [test x"$enable_install_tests" = x"yes"])
if test x"$enable_install_tests" = x"no"; then
    enable_install_tests="no (disabled, use --enable-install-tests to enable)"
fi


# --disable-emoji-dict option.
AC_ARG_ENABLE(emoji-dict,
    AS_HELP_STRING([--disable-emoji-dict],
                   [Do not build Emoji dict files]),
    [enable_emoji_dict=$enableval],
    [enable_emoji_dict=yes]
)
AM_CONDITIONAL([ENABLE_EMOJI_DICT], [test x"$enable_emoji_dict" = x"yes"])

AC_ARG_WITH(unicode-emoji-dir,
    AS_HELP_STRING([--with-unicode-emoji-dir[=DIR]],
        [Set the directory of Unicode Emoji.
         (default: "/usr/share/unicode/emoji")]),
    UNICODE_EMOJI_DIR=$with_unicode_emoji_dir,
    UNICODE_EMOJI_DIR="/usr/share/unicode/emoji"
)
AC_SUBST(UNICODE_EMOJI_DIR)

AC_ARG_WITH(emoji-annotation-dir,
    AS_HELP_STRING([--with-emoji-annotation-dir[=DIR]],
        [Set the directory of CLDR annotation files.
         (default: "/usr/share/unicode/cldr/common/annotations")]),
    EMOJI_ANNOTATION_DIR=$with_emoji_annotation_dir,
    EMOJI_ANNOTATION_DIR="/usr/share/unicode/cldr/common/annotations"
)
AC_SUBST(EMOJI_ANNOTATION_DIR)

if test x"$enable_emoji_dict" = x"yes"; then
    if test ! -f $UNICODE_EMOJI_DIR/emoji-test.txt ; then
        AC_MSG_ERROR(Not found $UNICODE_EMOJI_DIR/emoji-test.txt. You can get \
the emoji files from http://www.unicode.org/Public/emoji/4.0/)
    else
        # POSIX SHELL has no ${FOO:0:1}
        head=`echo "$UNICODE_EMOJI_DIR" | cut -c1`;
        if test $head != "/" ; then
            UNICODE_EMOJI_DIR=`realpath "$UNICODE_EMOJI_DIR"`
        fi
    fi
    if test ! -f $EMOJI_ANNOTATION_DIR/en.xml ; then
        AC_MSG_ERROR(Not found $EMOJI_ANNOTATION_DIR/en.xml. You can get \
https://github.com/fujiwarat/cldr-emoji-annotation)
    else
        head=`echo "$EMOJI_ANNOTATION_DIR" | cut -c1`;
        if test $head != "/" ; then
            EMOJI_ANNOTATION_DIR=`realpath "$EMOJI_ANNOTATION_DIR"`
        fi
    fi
    enable_emoji_dict="yes (enabled, use --disable-emoji-dict to disable)"
fi

# --disable-unicode-dict option.
AC_ARG_ENABLE(unicode-dict,
    AS_HELP_STRING([--disable-unicode-dict],
                   [Do not build Unicode dict files]),
    [enable_unicode_dict=$enableval],
    [enable_unicode_dict=yes]
)
AM_CONDITIONAL([ENABLE_UNICODE_DICT], [test x"$enable_unicode_dict" = x"yes"])

AC_ARG_WITH(ucd-dir,
    AS_HELP_STRING([--with-ucd-dir[=DIR]],
        [Set the directory of UCD (Unicode Character Database) files.
         (default: "/usr/share/unicode/ucd")]),
    UCD_DIR=$with_ucd_dir,
    UCD_DIR="/usr/share/unicode/ucd"
)
AC_SUBST(UCD_DIR)

if test x"$enable_unicode_dict" = x"yes"; then
    if test ! -f $UCD_DIR/NamesList.txt ; then
        AC_MSG_ERROR(Not found $UCD_DIR/NamesList.txt. You can get \
the UCD files from https://www.unicode.org/Public/UNIDATA/)
    elif test ! -f $UCD_DIR/Blocks.txt ; then
        AC_MSG_ERROR(Not found $UCD_DIR/Blocks.txt. You can get \
the UCD files from https://www.unicode.org/Public/UNIDATA/)
    else
        # POSIX SHELL has no ${FOO:0:1}
        head=`echo "$UCD_DIR" | cut -c1`;
        if test $head != "/" ; then
            UCD_DIR=`realpath "$UCD_DIR"`
        fi
    fi
    enable_unicode_dict="yes (enabled, use --disable-unicode-dict to disable)"
fi

AC_ARG_WITH(socket-dir,
    AS_HELP_STRING([--with-socket-dir[=DIR]],
        [Set the default socket directory to connect ibus-daemon with D-Bus
         connections (default: "$XDG_CACHE_HOME/ibus").
         The best practice of the socket directory would be unique per user
         not to modify by malicious users but XDG_RUNTIME_DIR is not integrated
         in BSD systems and the BSD's default is "/tmp".
         XDG_RUMTIME_DIR is not useful now because XDG_RUMTIME_DIR is available
         by login but not `su` command but ibus-daemon can be run with su.
         "$XDG_RUMTIME_DIR", "$XDG_CACHE_HOME", "$UID" are extracted by
         ibus-daemon.
         ibus-daemon also runs mkdir for the socket directory since BSD
         systems do not support abstract socket paths.
         The socket path on a NFS mount would not be a good idea likes
         "/home/$USER" because the directory is not sometimes accessible
         with the network condition.]),
    [IBUS_SOCKET_DIR=$with_socket_dir],
    [case $host in
         *linux*) IBUS_SOCKET_DIR='unix:tmpdir=$XDG_CACHE_HOME/ibus';;
         *)       IBUS_SOCKET_DIR='unix:tmpdir=/tmp';;
     esac]

)
AC_DEFINE_UNQUOTED(IBUS_SOCKET_DIR, "$IBUS_SOCKET_DIR",
                   [The default socket directory to connect ibus-daemon.])

# Check iso-codes.
PKG_CHECK_MODULES(ISOCODES, [
    iso-codes
])
ISOCODES_PREFIX=`$PKG_CONFIG iso-codes --variable=prefix`
AC_SUBST(ISOCODES_PREFIX)

PKG_CHECK_MODULES(XKBCONFIG,
    [xkeyboard-config],
    [XKBCONFIG_BASE=`$PKG_CONFIG xkeyboard-config --variable=xkb_base`],
    [XKBCONFIG_BASE='$(datarootdir)/X11/xkb']
)
AC_SUBST(XKBCONFIG_BASE)

AC_SUBST([GDBUS_CODEGEN], [`$PKG_CONFIG --variable gdbus_codegen gio-2.0`])

OUTPUT_TAIL=''
HAS_OUTPUT_TAIL=0
AS_IF([test x"$enable_wayland" = x"no"],
      [OUTPUT_TAIL="$OUTPUT_TAIL
You invoked the \`configure\` script with disabled wayland option
so you have to run
\`make -C tools maintainer-clean-generic\` to regenerate C files with VALA
before run \`make\`.
"
       HAS_OUTPUT_TAIL=1
      ],
      []
)
AS_IF([test x"$enable_ui" != x"no" &&
       test -f "$ac_confdir/ui/gtk3/application.c"
      ],
      [AS_IF([test x"$enable_appindicator" = x"no" ||
              test x"$enable_xim" = x"no" ||
              test x"$enable_gdk3_wayland" != x"yes" ||
              test x"$enable_emoji_dict" = x"no"
             ],
             [OUTPUT_TAIL="$OUTPUT_TAIL
You invoked the \`configure\` script with either disabled appinicator, xim,
gdk3_wayland or emoji_dict option so you have to run
\`make -C ui/gtk3 maintainer-clean-generic\` to regenerate C files with VALA
before run \`make\`.
"
              HAS_OUTPUT_TAIL=1
             ],
             [])
      ],
      []
)


# OUTPUT files
AC_CONFIG_FILES([
po/Makefile.in
bindings/Makefile
bindings/pygobject/Makefile
bindings/vala/Makefile
bus/Makefile
bus/ibus-daemon.1
bus/services/Makefile
client/Makefile
client/gtk2/Makefile
client/gtk3/Makefile
client/gtk4/Makefile
client/x11/Makefile
client/wayland/Makefile
conf/Makefile
conf/dconf/Makefile
conf/memconf/Makefile
data/Makefile
data/annotations/Makefile
data/icons/Makefile
data/its/Makefile
data/keymaps/Makefile
data/dconf/Makefile
data/dconf/00-upstream-settings.5
data/dconf/ibus.5
docs/Makefile
docs/reference/Makefile
docs/reference/ibus/ibus-docs.sgml
docs/reference/ibus/Makefile
engine/Makefile
ibus/_config.py
ibus/Makefile
ibus/interface/Makefile
m4/Makefile
portal/Makefile
setup/Makefile
setup/ibus-setup.1
src/Makefile
src/compose/Makefile
src/ibusversion.h
src/tests/Makefile
tools/Makefile
tools/ibus.1
ui/Makefile
ui/gtk3/Makefile
ui/gtk3/ibus-emoji.7
util/Makefile
util/IMdkit/Makefile
Makefile
ibus-1.0.pc
ibus.spec
])

AC_OUTPUT
AC_MSG_RESULT([
Build options:
  Version                       $IBUS_VERSION
  Install prefix                $prefix
  Build shared libs             $enable_shared
  Build static libs             $enable_static
  Build endianness              $ENDIAN_MSG
  CFLAGS                        $CFLAGS
  PYTHON                        $PYTHON
  PYTHON2                       $PYTHON2
  Python overrides dir          $pyoverridesdir
  Python GI GnomeDesktop module $enable_pygnome_desktop
  Enable python2                $enable_python2
  Gtk2 immodule dir             $GTK2_IM_MODULEDIR
  Gtk3 immodule dir             $GTK3_IM_MODULEDIR
  Gtk4 immodule dir             $GTK4_IM_MODULEDIR
  Build gtk2 immodule           $enable_gtk2
  Build gtk3 immodule           $enable_gtk3
  Build gtk4 immodule           $enable_gtk4
  Build XIM agent server        $enable_xim
  Build wayland support         $enable_wayland
  Build gdk3 wayland support    $enable_gdk3_wayland
  Build appindicator support    $enable_appindicator
  Build python library          $enable_python_library
  Build memconf modules         $enable_memconf
  Build dconf modules           $enable_dconf
  Build introspection           $found_introspection
  IBus-1.0.gir scannerflags     "$IBUS_GIR_SCANNERFLAGS"
  Build vala binding            $enable_vala
  Build document                $enable_gtk_doc
  Build UI                      $enable_ui
  Build engine                  $enable_engine
  Enable key snooper            $enable_key_snooper
  No snooper regexes            "$NO_SNOOPER_APPS"
  Panel icon                    "$IBUS_ICON_KEYBOARD"
  Enable surrounding-text       $enable_surrounding_text
  Enable libnotify              $enable_libnotify
  Enable Emoji dict             $enable_emoji_dict
  Unicode Emoji directory       $UNICODE_EMOJI_DIR
  CLDR annotation directory     $EMOJI_ANNOTATION_DIR
  Enable Unicode dict           $enable_unicode_dict
  UCD directory                 $UCD_DIR
  Socket directory              "$IBUS_SOCKET_DIR"
  XFixes client disconnect      $have_xfixes
  Install systemd service       $enable_systemd
  Run test cases                $enable_tests
  Install tests                 $enable_install_tests
])
AS_IF([test $HAS_OUTPUT_TAIL -eq 1],
      [AC_MSG_WARN([$OUTPUT_TAIL])],
      []
)
