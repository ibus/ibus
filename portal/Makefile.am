# vim:set noet ts=4:
#
# ibus - The Input Bus
#
# Copyright (c) 2007-2013 Peng Huang <shawn.p.huang@gmail.com>
# Copyright (c) 2007-2013 Red Hat, Inc.
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

NULL =

libibus = $(top_builddir)/src/libibus-@IBUS_API_VERSION@.la

AM_CPPFLAGS = \
	-I$(top_srcdir)/src   \
	-I$(top_builddir)/src \
	$(NULL)

AM_CFLAGS = \
	@GLIB2_CFLAGS@ \
	@GIO2_CFLAGS@ \
	@GTHREAD2_CFLAGS@ \
	-DG_LOG_DOMAIN=\"IBUS\" \
	-DPKGDATADIR=\"$(pkgdatadir)\" \
	-DLIBEXECDIR=\"$(libexecdir)\" \
	-DBINDIR=\"@bindir@\" \
	-DIBUS_DISABLE_DEPRECATED \
	$(NULL)
AM_LDADD = \
	@GOBJECT2_LIBS@ \
	@GLIB2_LIBS@ \
	@GIO2_LIBS@ \
	@GTHREAD2_LIBS@ \
	$(libibus) \
	$(NULL)

ibus_dbus_built_sources = ibus-portal-dbus.c ibus-portal-dbus.h
BUILT_SOURCES = $(ibus_dbus_built_sources)

libexec_PROGRAMS = ibus-portal
ibus_portal_DEPENDENCIES = \
	$(libibus) \
	$(NULL)
ibus_portal_SOURCES = \
	portal.c \
	$(ibus_dbus_built_sources) \
	$(NULL)
ibus_portal_CFLAGS = \
	$(AM_CFLAGS) \
	$(NULL)
ibus_portal_LDADD = \
	$(AM_LDADD) \
	$(NULL)

$(libibus):
	$(MAKE) -C $(top_builddir)/src

dbusservice_in_files = org.freedesktop.portal.IBus.service.in
dbusservice_DATA = $(dbusservice_in_files:.service.in=.service)
dbusservicedir=${datadir}/dbus-1/services

org.freedesktop.portal.IBus.service: org.freedesktop.portal.IBus.service.in
	$(AM_V_GEN) sed -e "s|\@libexecdir\@|$(libexecdir)|"  $< > $@.tmp && mv $@.tmp $@

$(ibus_dbus_built_sources) : org.freedesktop.IBus.Portal.xml
	$(AM_V_GEN) $(GDBUS_CODEGEN)                            \
		--interface-prefix org.freedesktop.IBus.        \
		--c-namespace IBusDbus                          \
		--generate-c-code $(builddir)/ibus-portal-dbus  \
		$^ \
		$(NULL)

EXTRA_DIST = \
    $(dbusservice_in_files) \
    org.freedesktop.IBus.Portal.xml \
    $(NULL)

CLEANFILES = \
    $(dbusservice_DATA) \
    $(NULL)


-include $(top_srcdir)/git.mk
