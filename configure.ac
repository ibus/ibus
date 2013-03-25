# vim:set et ts=4:
#
# ibus - The Input Bus
#
# Copyright (c) 2007-2010 Peng Huang <shawn.p.huang@gmail.com>
# Copyright (c) 2007-2010 Red Hat, Inc.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this program; if not, write to the
# Free Software Foundation, Inc., 59 Temple Place, Suite 330,
# Boston, MA  02111-1307  USA


# If ibus_released is 0, append datestamp to the version number.
m4_define([ibus_released], [1])

m4_define([ibus_major_version], [1])
m4_define([ibus_minor_version], [4])
m4_define([ibus_micro_version], [2])
m4_define([ibus_interface_age], [0])
m4_define([ibus_binary_age],
          [m4_eval(100 * ibus_minor_version + ibus_micro_version)])
m4_define([ibus_maybe_datestamp],
    m4_esyscmd([test x]ibus_released[ != x1 && date +.%Y%m%d | tr -d '\n\r']))
m4_define([ibus_version],
    ibus_major_version.ibus_minor_version.ibus_micro_version[]ibus_maybe_datestamp)
m4_define([ibus_api_version], [1.0])

AC_PREFEQ([2.62])
AC_INIT([ibus], [ibus_version],
        [http://code.google.com/p/ibus/issues/entry],
        [ibus])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

m4_define([ibus_binary_version], [1.0.0])

# Required versions of other packages.
m4_define([glib_required_version], [2.26.0])


# Init automake.
AM_INIT_AUTOMAKE([1.10])
AM_MAINTAINER_MODE([enable])
AC_GNU_SOURCE

# Support silent build rules. Disable
# by either passing --disable-silent-rules to configure or passing V=1
# to make
AM_SILENT_RULES([yes])

# Define sustituted variables:
IBUS_MAJOR_VERSION=ibus_major_version
IBUS_MINOR_VERSION=ibus_minor_version
IBUS_MICRO_VERSION=ibus_micro_version
IBUS_INTERFACE_AGE=ibus_interface_age
IBUS_BINARY_AGE=ibus_binary_age
IBUS_VERSION=ibus_version
IBUS_API_VERSION=ibus_api_version
IBUS_BINARY_VERSION=ibus_binary_version
AC_SUBST(IBUS_MAJOR_VERSION)
AC_SUBST(IBUS_MINOR_VERSION)
AC_SUBST(IBUS_MICRO_VERSION)
AC_SUBST(IBUS_INTERFACE_AGE)
AC_SUBST(IBUS_BINARY_AGE)
AC_SUBST(IBUS_API_VERSION)
AC_SUBST(IBUS_VERSION)
AC_SUBST(IBUS_BINARY_VERSION)

# libtool versioning
m4_define([lt_current],
          [m4_eval(100 * ibus_minor_version + ibus_micro_version - ibus_interface_age)])
m4_define([lt_revision], [ibus_interface_age])
m4_define([lt_age], [m4_eval(ibus_binary_age - ibus_interface_age)])
LT_VERSION_INFO="lt_current:lt_revision:lt_age"
LT_CURRENT_MINUS_AGE=m4_eval(lt_current - lt_age)
AC_SUBST(LT_VERSION_INFO)
AC_SUBST(LT_CURRENT_MINUS_AGE)

# Define GETTEXT_* variables.
GETTEXT_PACKAGE=ibus10
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE",
                   [The prefix for out gettext translation domains.])

# For dislpay date.
m4_define(ibus_datedisplay,
    m4_esyscmd(date '+%a %b %d %Y' | tr -d '\n\r'))
DATE_DISPLAY="ibus_datedisplay"
AC_SUBST(DATE_DISPLAY)


# Check for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CC_STDC
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# i18n stuff
AM_GLIB_GNU_GETTEXT

# Define PACKAGE_VERSION_* variables.
AM_DISABLE_STATIC
AC_ISC_POSIX
AC_HEADER_STDC
AM_PROG_LIBTOOL
IT_PROG_INTLTOOL([0.35.0])

# Check functions.
AC_CHECK_FUNCS(daemon)

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
    gio-2.0 >= glib_required_version
])
PKG_CHECK_MODULES(GTHREAD2, [
    gthread-2.0 >= glib_required_version
])

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

# --disable-xim option.
AC_ARG_ENABLE(xim,
    AS_HELP_STRING([--disable-xim],
                   [Do not build xim server]),
    [enable_xim=$enableval],
    [enable_xim=yes]
)
AM_CONDITIONAL([ENABLE_XIM], [test x"$enable_xim" = x"yes"])

# --disable-vala option.
AC_ARG_ENABLE(vala,
    AS_HELP_STRING([--disable-vala],
                   [Do not build ibus vala binding]),
    [enable_vala=$enableval],
    [enable_vala=yes]
)
if test x"$enable_vala" = x"yes"; then
    AC_PATH_PROG([VAPIGEN], [vapigen], [false])
fi
AM_CONDITIONAL([ENABLE_VALA], [test x"$enable_vala" = x"yes"])
if test x"$enable_vala" != x"yes"; then
    enable_vala="no (disabled, use --enable-vala to enable)"
fi

if test x"$enable_gtk2" = x"yes" -o x"$enable_xim" = x"yes" ; then
    # check for gtk2
    PKG_CHECK_MODULES(GTK2, [
        gtk+-2.0
    ])
fi

if test x"$enable_gtk2" = x"yes"; then
    # check for gdk2
    gtk2_binary_version=`$PKG_CONFIG --variable=gtk_binary_version gtk+-2.0`
    GTK2_IM_MODULEDIR="$libdir"/gtk-2.0/$gtk2_binary_version/immodules

    PKG_CHECK_MODULES(GDK2, [
        gdk-2.0
    ])
else
    enable_gtk2="no (disabled, use --enable-gtk2 to enable)"
fi

if test x"$enable_gtk3" = x"yes"; then
    # check for gtk3, gdk3
    PKG_CHECK_MODULES(GTK3, [
        gtk+-3.0
    ])

    gtk3_binary_version=`$PKG_CONFIG --variable=gtk_binary_version gtk+-3.0`
    GTK3_IM_MODULEDIR="$libdir"/gtk-3.0/$gtk3_binary_version/immodules

    PKG_CHECK_MODULES(GDK3, [
        gdk-3.0
    ])
else
    enable_gtk3="no (disabled, use --enable-gtk3 to enable)"
fi

if test x"$enable_xim" = x"yes"; then
    # Check for x11
    PKG_CHECK_MODULES(X11, [
        x11
    ])
    AC_CHECK_HEADERS([X11/XKBlib.h])
else
    enable_xim="no (disabled, use --enable-xim to enable)"
fi

# GObject introspection
GOBJECT_INTROSPECTION_CHECK([0.6.8])

IBUS_GIR_SCANNERFLAGS=
if test x"$found_introspection" = x"yes" ; then
    IBUS_GIR_SCANNERFLAGS="--identifier-prefix=IBus --symbol-prefix=ibus"
    PKG_CHECK_EXISTS([gobject-introspection-1.0 >= 0.9.6],
                     [gir_symbol_prefix=yes],
                     [gir_symbol_prefix=no])
    if test x"$gir_symbol_prefix" = x"no" ; then
        IBUS_GIR_SCANNERFLAGS="--strip-prefix=IBus"
    fi
fi
AC_SUBST(IBUS_GIR_SCANNERFLAGS)

# Check for gtk-doc.
GTK_DOC_CHECK(1.9)
if test x"$enable_gtk_doc" = x"no"; then
    enable_gtk_doc="no (disabled, use --enable-gtk-doc to enable)"
fi

# Check for dbus.
PKG_CHECK_MODULES(DBUS, [
    dbus-1
])

# --disable-gconf option.
AC_ARG_ENABLE(gconf,
    AS_HELP_STRING([--disable-gconf],
                   [Do not use GConf code]),
    [enable_gconf=$enableval],
    [enable_gconf=yes]
)
AM_CONDITIONAL([ENABLE_GCONF], [test x"$enable_gconf" = x"yes"])

if test x"$enable_gconf" = x"yes"; then
    # check gconf
    PKG_CHECK_MODULES(GCONF,
        [gconf-2.0 >= 2.12],
    )

    AC_PATH_PROG(GCONFTOOL, gconftool-2, no)
    if test x"$GCONFTOOL" = xno; then
        AC_MSG_ERROR([gconftool-2 executable not found in your path - should be installed with GConf])
    fi

    AM_GCONF_SOURCE_2
    # GCONF_SCHEMAS_INSTALL should be set in macro AM_GCONF_SOURCE_2
else
    AM_CONDITIONAL([GCONF_SCHEMAS_INSTALL], [false])
    enable_gconf="no (disabled, use --enable-gconf to enable)"
fi

# --enable-memconf option.
AC_ARG_ENABLE(memconf,
    AS_HELP_STRING([--enable-memconf],
                   [Enable configure base on memory]),
    [enable_memconf=$enableval],
    [enable_memconf=no]
)
AM_CONDITIONAL([ENABLE_MEMCONF], [test "x$enable_memconf" = "xyes"])

AC_ARG_ENABLE(dconf,
    AS_HELP_STRING([--enable-dconf],
                   [Enable configure base on dconf]),
    [enable_dconf=$enableval],
    [enable_dconf=no]
)

if test x"$enable_dconf" = x"yes"; then
    # check dconf
    PKG_CHECK_MODULES(DCONF,
        [dconf >= 0.7.5], ,
        enable_dconf=no
    )
    PKG_CHECK_EXISTS([dconf >= 0.13.4],
        [AC_DEFINE(DCONF_0_13_4, TRUE, [dconf is 0.13.4 or later])],
        [])
    # check glib-compile-schemas
    GLIB_GSETTINGS
fi
AM_CONDITIONAL([ENABLE_DCONF], [test x"$enable_dconf" = x"yes"])

# Check env.
AC_PATH_PROG(ENV_IBUS_TEST, env)
AC_SUBST(ENV_IBUS_TEST)

AC_ARG_ENABLE(python,
    AS_HELP_STRING([--disable-python],
                   [Do not use Python code]),
    [enable_python=$enableval],
    [enable_python=yes]
)

AM_CONDITIONAL([ENABLE_PYTHON], [test x"$enable_python" = x"yes"])
AM_CONDITIONAL([ENABLE_DAEMON], [true])

if test x"$enable_python" = x"yes"; then
    # Check python.
    AM_PATH_PYTHON([2.5])
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
else
    enable_python="no (disabled, use --enable-python to enable)"
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
    NO_SNOOPER_APPS=[.*chrome.*,.*chromium.*,firefox.*,Do.*]
)
AC_DEFINE_UNQUOTED(NO_SNOOPER_APPS, "$NO_SNOOPER_APPS",
    [Does not enbale keyboard snooper in those applications])

# GNOME 3 uses the theme's icon.
AC_ARG_WITH(panel-icon-keyboard,
    AS_HELP_STRING([--with-panel-icon-keyboard[=icon_name]],
        [Set the default panel icon (default: "input-keyboard-symbolic")]),
    [if test x"$with_panel_icon_keyboard" = x"yes" -o \
             x"$with_panel_icon_keyboard" = x; then
         with_panel_icon_keyboard="input-keyboard-symbolic"
     fi
     if test x"$with_panel_icon_keyboard" = x"legacy"; then
         with_panel_icon_keyboard="ibus-keyboard"
     fi
     IBUS_ICON_KEYBOARD=$with_panel_icon_keyboard
    ],
    IBUS_ICON_KEYBOARD="input-keyboard-symbolic"
)
AC_SUBST(IBUS_ICON_KEYBOARD)

# --enable-surrounding-text option.
AC_ARG_ENABLE(surrounding-text,
    AS_HELP_STRING([--enable-surrounding-text],
        [Enable surrounding-text support]),
    [enable_surrounding_text=$enableval],
    [enable_surrounding_text=no]
)
if test x"$enable_surrounding_text" = x"yes"; then
    AC_DEFINE(ENABLE_SURROUNDING, TRUE, [Enable surrounding-text support])
else
    enable_surrounding_text="no (disabled, use --enable-surrounding-text to enable)"
fi

# Check iso-codes.
PKG_CHECK_MODULES(ISOCODES, [
    iso-codes
])
ISOCODES_PREFIX=`pkg-config iso-codes --variable=prefix`
AC_SUBST(ISOCODES_PREFIX)

# OUTPUT files
AC_CONFIG_FILES([ po/Makefile.in
Makefile
ibus-1.0.pc
ibus.spec
xinput-ibus
memconf/Makefile
memconf/memconf.xml.in
client/Makefile
client/gtk2/Makefile
client/gtk3/Makefile
client/x11/Makefile
src/Makefile
src/ibusversion.h
src/tests/Makefile
bus/Makefile
util/Makefile
util/IMdkit/Makefile
data/Makefile
data/icons/Makefile
data/keymaps/Makefile
data/dconf/Makefile
docs/Makefile
docs/reference/Makefile
docs/reference/ibus/ibus-docs.sgml
docs/reference/ibus/Makefile
m4/Makefile
ibus/_config.py
ibus/Makefile
ibus/interface/Makefile
ui/Makefile
ui/gtk/Makefile
ui/gtk/ibus-ui-gtk
ui/gtk/gtkpanel.xml.in
setup/Makefile
setup/ibus-setup
gconf/Makefile
gconf/gconf.xml.in
bindings/Makefile
bindings/vala/Makefile
dconf/Makefile
dconf/dconf.xml.in
])

AC_OUTPUT
AC_MSG_RESULT([
Build options:
  Version                   $IBUS_VERSION
  Install prefix            $prefix
  Build shared libs         $enable_shared
  Build static libs         $enable_static
  CFLAGS                    $CFLAGS
  Gtk2 immodule dir         $GTK2_IM_MODULEDIR
  Gtk3 immodule dir         $GTK3_IM_MODULEDIR
  Build gtk2 immodule       $enable_gtk2
  Build gtk3 immodule       $enable_gtk3
  Build XIM agent server    $enable_xim
  Build python modules      $enable_python
  Build gconf modules       $enable_gconf
  Build memconf modules     $enable_memconf
  Build dconf modules       $enable_dconf
  Build introspection       $found_introspection
  IBus-1.0.gir scannerflags "$IBUS_GIR_SCANNERFLAGS"
  Build vala binding        $enable_vala
  Build document            $enable_gtk_doc
  Enable key snooper        $enable_key_snooper
  No snooper regexes        "$NO_SNOOPER_APPS"
  Panel icon                "$IBUS_ICON_KEYBOARD"
  Enable surrounding-text   $enable_surrounding_text
])

