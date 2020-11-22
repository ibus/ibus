#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

PKG_NAME="ibus"
DIST_FILES="
engine/simple.xml.in
src/ibusemojigen.h
src/ibusunicodegen.h
"
FEDORA_PKG1='autoconf automake libtool gettext-devel'
FEDORA_PKG2='glib2-devel gtk2-devel gtk3-devel
 wayland-devel'
FEDORA_PKG3='cldr-emoji-annotation iso-codes-devel unicode-emoji unicode-ucd'

(test -z "$DISABLE_INSTALL_PKGS") && {
    (test -f /etc/fedora-release ) && {
        dnf update --assumeno $FEDORA_PKG1 || exit 1
        dnf update --assumeno $FEDORA_PKG2 || exit 1
        dnf update --assumeno $FEDORA_PKG3 || exit 1
    }
}

(test -f $srcdir/configure.ac \
  && test -f $srcdir/README ) || {
    echo -n "**Error**: Directory "\`$srcdir\'" does not look like the"
    echo " top-level $PKG_NAME directory"
    exit 1
}

which gnome-autogen.sh || {
    echo "Not found gnome-autogen.sh. You may need to install gnome-common"
    exit 1
}

(test -f $srcdir/ChangeLog) || {
    touch $srcdir/ChangeLog
}

CFLAGS=${CFLAGS-"-Wall -Wformat -Werror=format-security"}

# need --enable-gtk-doc for gnome-autogen.sh to make dist
ACLOCAL_FLAGS="$ACLOCAL_FLAGS -I m4" REQUIRED_AUTOMAKE_VERSION=1.11 CFLAGS="$CFLAGS" . gnome-autogen.sh "$@"

(test -z "$SAVE_DIST_FILES") && {
    for f in $DIST_FILES ; do
        echo "rm $f"
        rm $f
    done
}
