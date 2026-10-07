# vim:set noet ts=4:
#
# ibus - The Input Bus
#
# Copyright (c) 2007-2015 Peng Huang <shawn.p.huang@gmail.com>
# Copyright (c) 2007-2015 Red Hat, Inc.
#
# SPDX-License-Identifier: LGPL-2.1-or-later
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
# License along with this library; if not, see <https://www.gnu.org/licenses/>.


libIMdkit = $(top_builddir)/util/IMdkit/libIMdkit.la
libibus = $(top_builddir)/src/libibus-@IBUS_API_VERSION@.la

libexec_PROGRAMS = ibus-x11

if ENABLE_GTK3
GTK_CFLAGS = @GTK3_CFLAGS@
GTK_LIBS = @GTK3_LIBS@
else
GTK_CFLAGS = @GTK2_CFLAGS@
GTK_LIBS = @GTK2_LIBS@
endif

ibus_x11_SOURCES = \
	main.c \
	gdk-private.c \
	$(NULL)

ibus_x11_DEPENDENCIES = \
	$(libIMdkit) \
	$(libibus) \
	$(NULL)

ibus_x11_LDADD = \
	$(libIMdkit) \
	$(libibus) \
	@X11_LIBS@ \
	$(GTK_LIBS) \
	$(XFIXES_LIBS) \
	$(NULL)

ibus_x11_CFLAGS = \
	@X11_CFLAGS@ \
	$(GTK_CFLAGS) \
	$(XFIXES_CFLAGS) \
	@DBUS_CFLAGS@ \
	-I$(top_srcdir)/util/IMdkit \
	-I$(top_srcdir)/src \
	-I$(top_builddir)/src \
	$(NULL)

noinst_HEADERS = \
	gdk-private.h \
	locales.h \
	$(NULL)

xwaylandsessiond_in_files = 10-ibus-x11.in
xwaylandsessiond_DATA = $(xwaylandsessiond_in_files:.in=)
xwaylandsessionddir=$(sysconfdir)/xdg/Xwayland-session.d

10-ibus-x11: 10-ibus-x11.in
	$(AM_V_GEN) sed -e "s|\@libexecdir\@|$(libexecdir)|"  $< > $@.tmp && mv $@.tmp $@

install-data-hook:
	chmod 755 $(DESTDIR)$(xwaylandsessionddir)/10-ibus-x11

$(libIMdkit):
	(cd $(top_builddir)/util/IMdkit; make)

$(libibus):
	(cd $(top_builddir)/src; make)

locales.h:
	( \
		locale -a | \
		gawk -F _ '{ if (length($$1) <= 3) print $$1 }' | \
		uniq | sort | \
		xargs python -c 'import sys;print "#define LOCALES_STRING \"%s\"" % ",".join(sys.argv[1:])' \
	 ) > $@

CLEANFILES =                    \
    $(xwaylandsessiond_DATA)    \
    $(NULL)

EXTRA_DIST =                    \
    10-ibus-x11.in              \
    $(NULL)

-include $(top_srcdir)/git.mk
