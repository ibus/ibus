# vim:set noet ts=4:
#
# ibus - The Input Bus
#
# Copyright (c) 2007-2013 Peng Huang <shawn.p.huang@gmail.com>
# Copyright (c) 2019-2025 Takao Fujiwara <takao.fujiwara1@gmail.com>
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
noinst_DATA =
MAINTAINERCLEANFILES =
DISTCLEANFILES =

protocol_sources = \
    input-method-unstable-v1-client-protocol.h \
    input-method-unstable-v1-protocol.c \
    input-method-unstable-v2-client-protocol.h \
    input-method-unstable-v2-protocol.c \
    text-input-unstable-v1-client-protocol.h \
    virtual-keyboard-unstable-v1-client-protocol.h \
    virtual-keyboard-unstable-v1-protocol.c \
    $(NULL)

ibus_wayland_SOURCES = \
    $(protocol_sources) \
    main.c \
    ibuswaylandim.c \
    ibuswaylandim.h \
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
    -DIBUS_COMPILATION \
    -I$(top_srcdir)/src \
    -I$(top_builddir)/src \
    $(NULL)

$(libibus):
	(cd $(top_builddir)/src; make)

EXTRA_DIST = \
    $(protocol_sources) \
    IBusWaylandIM-1.0.metadata \
    ibus-wayland-im-custom.vala \
    input-method-unstable-v2.xml \
    virtual-keyboard-unstable-v1.xml \
    README \
    $(NULL)

if ENABLE_WAYLAND
# Do not use wayland_scanner_rules = /usr/share/wayland/wayland-scanner.mk
# for multiple protocols.
wayland_text_protocoldir = $(WAYLAND_PRTCLS_DIR)/$(WAYLAND_TEXT_PRTCL_SUBDIR)

input-method-unstable-v1-client-protocol.h: $(wayland_protocoldir)/input-method-unstable-v1.xml
	$(AM_V_GEN)$(wayland_scanner) client-header $< $@

input-method-unstable-v1-protocol.c: $(wayland_protocoldir)/input-method-unstable-v1.xml
	$(AM_V_GEN)$(wayland_scanner) code $< $@

input-method-unstable-v2-client-protocol.h: $(srcdir)/input-method-unstable-v2.xml
	$(AM_V_GEN)$(wayland_scanner) client-header $< $@

input-method-unstable-v2-protocol.c: $(srcdir)/input-method-unstable-v2.xml
	$(AM_V_GEN)$(wayland_scanner) code $< $@

text-input-unstable-v1-client-protocol.h: $(wayland_text_protocoldir)/text-input-unstable-v1.xml
	$(AM_V_GEN)$(wayland_scanner) client-header $< $@

virtual-keyboard-unstable-v1-client-protocol.h: $(srcdir)/virtual-keyboard-unstable-v1.xml
	$(AM_V_GEN)$(wayland_scanner) client-header $< $@

virtual-keyboard-unstable-v1-protocol.c: $(srcdir)/virtual-keyboard-unstable-v1.xml
	$(AM_V_GEN)$(wayland_scanner) code $< $@
endif


# `make distdir` can go to ibus/client/wayland
if ENABLE_WAYLAND
if HAVE_INTROSPECTION
-include $(INTROSPECTION_MAKEFILE)

libibuswaylandim = libibuswaylandim.la
noinst_LTLIBRARIES = $(libibuswaylandim)

libibuswaylandim_la_CPPFLAGS = \
    $(GIO2_CFLAGS) \
    $(WAYLAND_CFLAGS) \
    -DIBUS_COMPILATION \
    -I$(top_srcdir)/src \
    -I$(top_builddir)/src \
    $(NULL)
libibuswaylandim_la_LIBADD = \
    $(GIO2_LIBS) \
    $(WAYLAND_LIBS) \
    $(libibus) \
    $(NULL)
libibuswaylandim_la_LDFLAGS =                   \
    -no-undefined                               \
    -export-symbols-regex "ibus_wayland_im_.*"  \
    $(NULL)
libibuswaylandim_la_SOURCES =  \
    $(protocol_sources)        \
    ibuswaylandim.c            \
    ibuswaylandim.h            \
    $(NULL)

INTROSPECTION_GIRS = IBusWaylandIM-1.0.gir
INTROSPECTION_SCANNER_ARGS = \
    --add-include-path=$(top_srcdir)/src \
    --add-include-path=$(top_builddir)/src \
    $(NULL)
INTROSPECTION_COMPILER_ARGS = \
    --includedir=$(srcdir)    \
    --includedir=.            \
    $(NULL)
noinst_DATA += $(INTROSPECTION_GIRS)
EXTRA_DIST += $(INTROSPECTION_GIRS)
MAINTAINERCLEANFILES += $(INTROSPECTION_GIRS)
DISTCLEANFILES += $(INTROSPECTION_GIRS)
$(INTROSPECTION_GIRS): $(libibus) $(libibuswaylandim) Makefile
IBusWaylandIM_1_0_gir_SCANNERFLAGS =    \
    --pkg-export=ibus-1.0               \
    --pkg=gobject-2.0                   \
    $(IBUS_GIR_SCANNERFLAGS)            \
    $(NULL)
IBusWaylandIM_1_0_gir_INCLUDES = GLib-2.0 GObject-2.0 Gio-2.0
IBusWaylandIM_1_0_gir_LIBS = $(libibus) $(libibuswaylandim)
IBusWaylandIM_1_0_gir_FILES =      \
    ibuswaylandim.h                \
    $(NULL)
IBusWaylandIM_1_0_gir_CFLAGS =                  \
    -DIBUS_COMPILATION                          \
    -I$(srcdir)                                 \
    -I$(builddir)                               \
    -I$(top_srcdir)/src                         \
    -I$(top_builddir)/src                       \
    $(NULL)

-include $(VAPIGEN_MAKEFILE)
VAPIGEN_VAPIS = ibus-wayland-im-1.0.vapi
VAPIGEN_VAPIDIRS = \
    $(top_srcdir)/bindings/vala \
    $(top_builddir)/bindings/vala \
    $(NULL)
VAPIGEN_GIRDIRS = \
    $(top_srcdir)/src \
    $(NULL)
noinst_DATA += $(VAPIGEN_VAPIS)
EXTRA_DIST += $(VAPIGEN_VAPIS)
MAINTAINERCLEANFILES += $(VAPIGEN_VAPIS)
DISTCLEANFILES += $(VAPIGEN_VAPIS)
$(VAPIGEN_VAPIS): $(INTROSPECTION_GIRS)
ibus_wayland_im_1_0_vapi_DEPS = glib-2.0 gobject-2.0 gio-2.0 ibus-1.0
ibus_waylayd_im_1_0_vapi_METADATADIRS = $(srcdir) $(top_srcdir)/bindings/vala
#ibus_waylayd_im_1_0_vapi_VAPIDIRS = $(top_srcdir)/bindings/vala
ibus_wayland_im_1_0_vapi_FILES = \
    $(INTROSPECTION_GIRS) \
    $(srcdir)/ibus-wayland-im-custom.vala \
    $(NULL)

endif
#end of HAVE_INTROSPECTION
endif
#end of ENABLE_WAYLAND

-include $(top_srcdir)/git.mk
