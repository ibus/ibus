#!/bin/sh
# Run this to generate all the initial makefiles, etc.

: ${srcdir=$(dirname $0)}
: ${srcdir:=.}
: ${SAVE_DIST_FILES:=0}
: ${CFLAGS:='-g -O2'}
: ${MAKE:=make}
: ${GTKDOCIZE:=gtkdocize}

olddir=$(pwd)
# shellcheck disable=SC2016
PKG_NAME=$(autoconf --trace 'AC_INIT:$1' configure.ac)
WANT_GTK_DOC=0
GCC_VERSION=$(gcc --version | head -1 | awk '{print $3}')
GCC_MAJOR_VERSION=$(echo "$GCC_VERSION" | awk -F. '{print $1}')
FEDORA_PKG1='autoconf automake libtool gettext-devel'
FEDORA_PKG2='glib2-devel gtk2-devel gtk3-devel
 wayland-devel vala'
FEDORA_PKG3='cldr-emoji-annotation iso-codes-devel unicode-emoji unicode-ucd
 xkeyboard-config-devel'

(test $GCC_MAJOR_VERSION -ge 0) && {
    CFLAGS="-Wall -Wformat -Werror=format-security $CFLAGS"
} || :
(test "x$ENABLE_ANALYZER" != "x") && {
    (test $GCC_MAJOR_VERSION -ge 10) && {
        CFLAGS="-fanalyzer -fsanitize=address -fsanitize=leak $CFLAGS"
        FEDORA_PKG1="$FEDORA_PKG1 libasan"
    } || :
} || :

cd "$srcdir"

(test -f configure.ac \
  && test -f README ) || {
    echo -n "**Error**: Directory "\`$srcdir\'" does not look like the"
    echo " top-level $PKG_NAME directory"
    exit 1
}

CONFIGFLAGS="$@"
(test "x$NOCONFIGURE" = "x" ) &&
$(grep -q "^GTK_DOC_CHECK" configure.ac) && {
    # $WANT_GTK_DOC: If the source files require gtk-doc
    # Specify "--disable-gtk-doc" option for autogen.sh if you wish to disable
    # the gtk-doc builds.
    WANT_GTK_DOC=1
    FEDORA_PKG2="$FEDORA_PKG2 gtk-doc"
}

(test -f ChangeLog) || {
    touch ChangeLog
}

(test "x$DISABLE_INSTALL_PKGS" = "x") && (test "x$FLATPAK_ID" = "x" ) && {
    (test -f /etc/fedora-release ) && {
        rpm -q $FEDORA_PKG1 || exit 1
        rpm -q $FEDORA_PKG2 || exit 1
        rpm -q $FEDORA_PKG3 || exit 1
        DNF=dnf
        $DNF update --assumeno $FEDORA_PKG1 || exit 1
        $DNF update --assumeno $FEDORA_PKG2 || exit 1
        $DNF update --assumeno $FEDORA_PKG3 || exit 1
    }
}

(test "$#" = 0 -a "x$NOCONFIGURE" = "x" ) && {
    echo "*** WARNING: I am going to run 'configure' with no arguments." >&2
    echo "*** If you wish to pass any to it, please specify them on the" >&2
    echo "*** '$0' command line." >&2
    echo "" >&2
}

(test "x$NOCONFIGURE" = "x" ) && {
    $(echo "x$CONFIGFLAGS" | grep -q disable-gtk-doc) || {
        (test $WANT_GTK_DOC -eq 1) && CONFIGFLAGS="--enable-gtk-doc $@"
        (test -f ./m4/gtk-doc-dummy.m4) && $(rm ./m4/gtk-doc-dummy.m4)
    }
}

# If $WANT_GTK_DOC is 1, gtkdocize should run basically. You could apply
# GTKDOCIZE=echo for the workaround if you disable to run dtkdocize likes
# autoreconf.
(test $WANT_GTK_DOC -eq 1) && $GTKDOCIZE --copy

(test "x$NOCONFIGURE" = "x" ) &&
$(echo "x$CONFIGFLAGS" | grep -q disable-gtk-doc) &&
(test ! -f ./m4/gtk-doc.m4 ) && {
    # The license of gtk-doc.make and m4/gtk-doc.m4 is GPL but not LGPL
    # and IBus does not save the static files.
    cat > ./gtk-doc.make <<_EOF_MAKE
    EXTRA_DIST=
    CLEANFILES=
_EOF_MAKE
    cat > ./m4/gtk-doc-dummy.m4 <<_EOF_M4
AC_DEFUN([GTK_DOC_CHECK],
[
    have_gtk_doc=no
    AC_MSG_CHECKING([for gtk-doc])
    AC_MSG_RESULT($have_gtk_doc)

    dnl enable/disable documentation building
    AC_ARG_ENABLE([gtk-doc],
    AS_HELP_STRING([--enable-gtk-doc],
                   [use gtk-doc to build documentation [[default=no]]]),,
    [enable_gtk_doc=no])

    AC_MSG_CHECKING([whether to build gtk-doc documentation])
    AC_MSG_RESULT($enable_gtk_doc)
    AM_CONDITIONAL([ENABLE_GTK_DOC], [test "x$enable_gtk_doc" = "xyes"])

    if test "x$enable_gtk_doc" = "xyes" && test "$have_gtk_doc" = "no"; then
        AC_MSG_ERROR([Something wrong in the build.])
    fi
])
_EOF_M4
}

ACLOCAL_FLAGS="$ACLOCAL_FLAGS -I m4" REQUIRED_AUTOMAKE_VERSION=1.11 \
autoreconf --verbose --force --install || exit 1

cd "$olddir"
(test "x$NOCONFIGURE" = "x" ) && {
    echo "$srcdir/configure $CONFIGFLAGS"
    $srcdir/configure $CONFIGFLAGS CFLAGS="$CFLAGS" || exit 1
    (test "$1" = "--help" ) && {
        exit 0
    } || {
        echo "Now type '$MAKE' to compile $PKG_NAME" || exit 1
    }
} || {
    echo "Skipping configure process."
}

cd "$srcdir"
(test "x$SAVE_DIST_FILES" = "x0" ) && {
    # rm engine/simple.xml.in src/ibusemojigen.h src/ibusunicodegen.h
    for d in engine src src/compose; do
        echo "$MAKE -C $d maintainer-clean-generic"
        $MAKE -C $d maintainer-clean-generic
   done
} || :
cd "$olddir"
