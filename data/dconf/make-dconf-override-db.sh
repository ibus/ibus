#!/bin/bash

set -e

# gnome-continuous doesn't have a machine-id set, which
# breaks dbus-launch.  There's dbus-run-session which is
# better, but not everyone has it yet.
export DBUS_FATAL_WARNINGS=0
export TMPDIR=$(TMPDIR="$PWD" mktemp -d)
export XDG_CONFIG_HOME="$TMPDIR/config"
export XDG_CACHE_HOME="$TMPDIR/cache"
export GSETTINGS_SCHEMA_DIR="$TMPDIR/schemas"
mkdir -p $XDG_CONFIG_HOME $XDG_CACHE_HOME $GSETTINGS_SCHEMA_DIR

eval `dbus-launch --sh-syntax`

trap 'rm -rf $TMPDIR; kill $DBUS_SESSION_BUS_PID' ERR

# in case that schema is not installed on the system
glib-compile-schemas --targetdir "$GSETTINGS_SCHEMA_DIR" "$PWD"

cat <<EOF
# This file is a part of the IBus packaging and should not be changed.
#
# Instead create your own file next to it with a higher numbered prefix,
# and run
#
#       dconf update
#
EOF

# Loop over top level schemas since "gsettings list-recursively" only
# looks for direct children.
schemas="org.freedesktop.ibus.general org.freedesktop.ibus.panel"
current_schema=
for schema in $schemas; do
  gsettings list-recursively $schema | \
  while read schema key val; do
    if test "$schema" != "$current_schema"; then
      echo
      echo $schema | sed 's/org\.freedesktop\(.*\)/[desktop\1]/' | tr '.' '/'
      current_schema="$schema"
    fi
    echo "$key=$val"
  done
done

rm -rf $TMPDIR

kill $DBUS_SESSION_BUS_PID
