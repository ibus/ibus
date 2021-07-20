# vim:set noet ts=4:
#
# ibus - The Input Bus
#
# Copyright (c) 2007-2013 Peng Huang <shawn.p.huang@gmail.com>
# Copyright (c) 2019 Takao Fujiwara <takao.fujiwara1@gmail.com>
# Copyright (c) 2007-2019 Red Hat, Inc.
#
# This library is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as
# published by the Free Software Foundation; either version 2.1 of the
# License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301 USA.

libibus = $(top_builddir)/src/libibus-@IBUS_API_VERSION@.la

libexec_PROGRAMS = ibus-wayland

protocol_sources = \
    input-method-unstable-v1-client-protocol.h \
    input-method-unstable-v1-protocol.c \
    input-method-unstable-v2-client-protocol.h \
    input-method-unstable-v2-protocol.c \
    virtual-keyboard-unstable-v1-client-protocol.h \
    virtual-keyboard-unstable-v1-protocol.c \
    $(NULL)

ibus_wayland_SOURCES = \
    $(protocol_sources) \
    main.c \
    $(NULL)

ibus_wayland_DEPENDENCIES = \
    $(libibus) \
    $(NULL)

ibus_wayland_LDADD = \
    $(libibus) \
    $(GIO2_LIBS) \
    $(WAYLAND_LIBS) \
    $(NULL)

ibus_wayland_CFLAGS = \
    $(GIO2_CFLAGS) \
    $(WAYLAND_CFLAGS) \
    -I$(top_srcdir)/src \
    -I$(top_builddir)/src \
    $(NULL)

$(libibus):
	(cd $(top_builddir)/src; make)

EXTRA_DIST = \
    README \
    $(protocol_sources) \
    $(NULL)

-include @wayland_scanner_rules@

-include $(top_srcdir)/git.mk
