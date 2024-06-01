#!/bin/sh
# Run this to generate all the initial makefiles, etc.

: ${srcdir=$(dirname $0)}
: ${srcdir:=.}
: ${SAVE_DIST_FILES:=0}
: ${MAKE:=make}

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

CFLAGS=${CFLAGS-"-Wall -Wformat -Werror=format-security"}
(test $GCC_MAJOR_VERSION -ge 10) && {
    CFLAGS="$CFLAGS -fanalyzer -fsanitize=address -fsanitize=leak"
    FEDORA_PKG1="$FEDORA_PKG1 libasan"
}

cd "$srcdir"

(test -f configure.ac \
  && test -f README ) || {
    echo -n "**Error**: Directory "\`$srcdir\'" does not look like the"
    echo " top-level $PKG_NAME directory"
    exit 1
}

(test $(grep -q "^GTK_DOC_CHECK" configure.ac)) || {
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

CONFIGFLAGS="$@"
(test "$#" = 0 -a "x$NOCONFIGURE" = "x" ) && {
    echo "*** WARNING: I am going to run 'configure' with no arguments." >&2
    echo "*** If you wish to pass any to it, please specify them on the" >&2
    echo "*** '$0' command line." >&2
    echo "" >&2
    (test $WANT_GTK_DOC -eq 1) && CONFIGFLAGS="--enable-gtk-doc $@"
}

(test $WANT_GTK_DOC -eq 1) && gtkdocize --copy

ACLOCAL_FLAGS="$ACLOCAL_FLAGS -I m4" REQUIRED_AUTOMAKE_VERSION=1.11 \
autoreconf --verbose --force --install || exit 1

cd "$olddir"
(test "x$NOCONFIGURE" = "x" ) && {
    echo "$srcdir/configure $CONFIGFLAGS"
    $srcdir/configure $CONFIGFLAGS || exit 1
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
