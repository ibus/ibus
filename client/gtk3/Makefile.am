# vim:set noet ts=4:
#
# ibus - The Input Bus
#
# Copyright (c) 2007-2010 Peng Huang <shawn.p.huang@gmail.com>
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

libibus = $(top_builddir)/src/libibus-@IBUS_API_VERSION@.la

AM_CPPFLAGS = \
	-I$(top_srcdir)/src \
	-I$(top_builddir)/src \
	$(NULL)

immoduledir = @GTK3_IM_MODULEDIR@
immodule_LTLIBRARIES = im-ibus.la

im_ibus_la_SOURCES = \
	ibusim.c \
	ibusimcontext.c \
	ibusimcontext.h \
	iconwidget.c \
	iconwidget.h \
	$(NULL)

im_ibus_la_DEPENDENCIES = $(libibus)

im_ibus_la_CFLAGS = \
	@GTK3_CFLAGS@ \
	@DBUS_CFLAGS@ \
	-DG_LOG_DOMAIN=\"IBUS\" \
	$(NULL)

im_ibus_la_LIBADD = \
	@GTK3_LIBS@ \
	@DBUS_LIBS@ \
	$(libibus) \
	$(NULL)
im_ibus_la_LDFLAGS = \
	-avoid-version \
	-module \
	$(NULL)

$(libibus):
	(cd $(top_builddir)/src; make )

EXTRA_DIST = \
	$(NULL)

test: all
	GTK_IM_MODULE=ibus gedit

-include $(top_srcdir)/git.mk
