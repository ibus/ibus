# vim:set noet ts=4:
#
# ibus - The Input Bus
#
# Copyright (c) 2007-2010 Peng Huang <shawn.p.huang@gmail.com>
# Copyright (c) 2007-2010 Red Hat, Inc.
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

libibus = $(top_builddir)/src/libibus-@IBUS_API_VERSION@.la

libexec_PROGRAMS = \
	ibus-dconf \
	$(NULL)

ibus_dconf_SOURCES = \
	main.c \
	config.c \
	config-private.h \
	$(NULL)
ibus_dconf_CFLAGS = \
	@GLIB2_CFLAGS@ \
	@GIO2_CFLAGS@ \
	@DCONF_CFLAGS@ \
	-DG_LOG_DOMAIN=\"IBUS\" \
	-I$(top_srcdir)/src \
	-I$(top_builddir)/src \
	$(NULL)
ibus_dconf_LDADD = \
	@GOBJECT2_LIBS@ \
	@GLIB2_LIBS@ \
	@GIO2_LIBS@ \
	@DCONF_LIBS@ \
	$(libibus) \
	$(NULL)
ibus_dconf_DEPENDENCIES = \
	$(libibus) \
	$(NULL)

component_DATA = \
	dconf.xml \
	$(NULL)

componentdir = $(pkgdatadir)/component

CLEANFILES = \
	dconf.xml \
	*.pyc \
	$(NULL)

DISTCLEANFILES = \
	dconf-override-db \
	$(NULL)

EXTRA_DIST = \
	dconf.xml.in \
	$(NULL)

dconf.xml: dconf.xml.in
	$(AM_V_GEN) sed \
		-e 's|@VERSION[@]|$(VERSION)|g' \
		-e 's|@libexecdir[@]|$(libexecdir)|g' $< > $@.tmp && \
		mv $@.tmp $@

$(libibus):
	$(MAKE) -C $(top_builddir)/src

-include $(top_srcdir)/git.mk
