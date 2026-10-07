# vim:set noet ts=4:
#
# ibus - The Input Bus
#
# Copyright (c) 2010, Google Inc. All rights reserved.
# Copyright (c) 2007-2010 Peng Huang <shawn.p.huang@gmail.com>
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

memconf_programs = \
	ibus-memconf \
	$(NULL)

if ENABLE_MEMCONF
libexec_PROGRAMS = $(memconf_programs)
else
noinst_PROGRAMS = $(memconf_programs)
endif

ibus_memconf_SOURCES = \
	main.c \
	config.c \
	config.h \
	$(NULL)
ibus_memconf_CFLAGS = \
	@GLIB2_CFLAGS@ \
	@GIO2_CFLAGS@ \
	-DG_LOG_DOMAIN=\"IBUS\" \
	-I$(top_srcdir)/src \
	-I$(top_builddir)/src \
	$(NULL)
ibus_memconf_LDADD = \
	@GOBJECT2_LIBS@ \
	@GLIB2_LIBS@ \
	@GIO2_LIBS@ \
	$(libibus) \
	$(NULL)
ibus_memconf_DEPENDENCIES = \
	$(libibus) \
	$(NULL)

if ENABLE_MEMCONF
component_DATA = \
	memconf.xml \
	$(NULL)

componentdir = $(pkgdatadir)/component
else
noinst_DATA = \
	memconf.xml \
	$(NULL)
endif

CLEANFILES = \
	memconf.xml \
	$(NULL)

EXTRA_DIST = \
	memconf.xml.in \
	$(NULL)

memconf.xml: memconf.xml.in
	$(AM_V_GEN) sed \
		-e 's|@VERSION[@]|$(VERSION)|g' \
		-e 's|@libexecdir[@]|$(libexecdir)|g' $< > $@.tmp && \
		mv $@.tmp $@

$(libibus):
	$(MAKE) -C $(top_builddir)/src

-include $(top_srcdir)/git.mk
