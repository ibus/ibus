# vim:set noet ts=4:
#
# ibus - The Input Bus
#
# Copyright (c) 2007-2013 Peng Huang <shawn.p.huang@gmail.com>
# Copyright (c) 2015-2025 Takao Fujiwara <takao.fujiwara1@gmail.com>
# Copyright (c) 2007-2017 Red Hat, Inc.
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

NULL =

libibus = $(top_builddir)/src/libibus-@IBUS_API_VERSION@.la
libibusimmodule = libibusimmodule.la
ibusimmodule_gir = IBusIMModule-1.0.gir
ibus_immodule_vapi = ibus-immodule-1.0.vapi
libibus_emoji_dialog = \
    $(top_builddir)/ui/gtk3/libibus-emoji-dialog-@IBUS_API_VERSION@.la

noinst_LTLIBRARIES = $(libibusimmodule)
noinst_DATA =
INTROSPECTION_GIRS =
MAINTAINERCLEANFILES =
DISTCLEANFILES =
VAPIGEN_VAPIS =

# force include config.h before gi18n.h.
AM_CPPFLAGS = \
    -I$(top_srcdir)/src \
    -I$(top_builddir)/src \
    -include $(CONFIG_HEADER) \
    $(NULL)

AM_CFLAGS = \
    @GLIB2_CFLAGS@ \
    @GIO2_CFLAGS@ \
    @GTHREAD2_CFLAGS@ \
    -DG_LOG_DOMAIN=\"IBUS\" \
    -DLIBEXECDIR=\"$(libexecdir)\" \
    -DIBUS_DISABLE_DEPRECATED \
    -DIBUS_COMPILATION \
    -Wno-unused-variable \
    -Wno-unused-but-set-variable \
    -Wno-unused-function \
    $(NULL)

AM_LDADD = \
    @GOBJECT2_LIBS@ \
    @GLIB2_LIBS@ \
    @GIO2_LIBS@ \
    @GTHREAD2_LIBS@ \
    $(libibus) \
    $(libibusimmodule) \
    $(LIBDL) \
    $(NULL)

AM_VALAFLAGS = \
    --vapidir=$(top_builddir)/bindings/vala \
    --vapidir=$(top_srcdir)/bindings/vala \
    --vapidir=$(builddir) \
    --vapidir=$(srcdir) \
    --pkg=dbus-1 \
    --pkg=gio-2.0 \
    --pkg=ibus-1.0 \
    --pkg=ibus-immodule-1.0 \
    --pkg=posix \
    --pkg=config \
    --pkg-config="\"$(PKG_CONFIG) --with-path=$(PKG_CONFIG_PATH)\"" \
    --target-glib="$(VALA_TARGET_GLIB_VERSION)" \
    $(NULL)

bin_PROGRAMS = ibus

ibus_SOURCES = \
	krcfile.vala \
	main.vala \
	$(NULL)

ibus_LDADD = \
	$(AM_LDADD) \
	$(NULL)

bash_completion_DATA= \
	ibus.bash \
	$(NULL)
bash_completiondir=@datadir@/bash-completion/completions

libibusimmodule_la_SOURCES = \
    ibusimmodule.c                                      \
    ibusimmodule.h                                      \
    $(NULL)
libibusimmodule_la_CFLAGS = \
    @GLIB2_CFLAGS@                                      \
    -DGTK2_IM_MODULEDIR=\"$(GTK2_IM_MODULEDIR)\"        \
    -DGTK3_IM_MODULEDIR=\"$(GTK3_IM_MODULEDIR)\"        \
    -DGTK4_IM_MODULEDIR=\"$(GTK4_IM_MODULEDIR)\"        \
    $(NULL)
libibusimmodule_la_LIBADD = \
    @GLIB2_LIBS@                                        \
    $(NULL)
libibusimmodule_la_LDFLAGS = \
    -no-undefined                                       \
    -export-symbols-regex "ibus_.*"                     \
    $(NULL)

man_one_DATA = ibus.1
man_onedir = $(mandir)/man1

EXTRA_DIST = \
    $(ibus_immodule_vapi) \
    $(ibusimmodule_gir) \
    ibus.bash \
    IBusIMModule-1.0.metadata \
    $(NULL)

if ENABLE_EMOJI_DICT
if ENABLE_UI
AM_VALAFLAGS += \
    --define=EMOJI_DICT \
    $(NULL)
endif
endif

if ENABLE_WAYLAND
AM_VALAFLAGS += \
    --define=IBUS_WAYLAND \
    --pkg=wayland-client \
    $(NULL)
AM_CFLAGS += @WAYLAND_CFLAGS@
AM_LDADD += @WAYLAND_LIBS@
endif

if HAVE_INTROSPECTION
BUILT_SOURCES = $(INTROSPECTION_GIRS) $(VAPIGEN_VAPIS)

-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_SCANNER_ARGS =
INTROSPECTION_COMPILER_ARGS = \
    --includedir=$(srcdir)                              \
    --includedir=.                                      \
    $(NULL)

IBusIMModule-1.0.gir: $(libibusimmodule) Makefile
IBusIMModule_1_0_gir_SCANNERFLAGS = \
    --pkg=glib-2.0                                      \
    $(IBUS_GIR_SCANNERFLAGS)                            \
    $(NULL)
IBusIMModule_1_0_gir_INCLUDES = GLib-2.0
IBusIMModule_1_0_gir_LIBS = $(libibusimmodule)
IBusIMModule_1_0_gir_FILES = ibusimmodule.h
IBusIMModule_1_0_gir_CFLAGS = \
    -I$(srcdir)                                         \
    -I$(builddir)                                       \
    $(NULL)

INTROSPECTION_GIRS += $(ibusimmodule_gir)
noinst_DATA += $(ibusimmodule_gir)
MAINTAINERCLEANFILES += $(ibusimmodule_gir)
DISTCLEANFILES += $(ibusimmodule_gir)

-include $(VAPIGEN_MAKEFILE)
ibus-immodule-1.0.vapi: $(ibusimmodule_gir) IBusIMModule-1.0.metadata
ibus_immodule_1_0_vapi_DEPS = glib-2.0
ibus_immodule_1_0_vapi_METADATADIRS = $(srcdir)
ibus_immodule_1_0_vapi_FILES = IBusIMModule-1.0.gir
VAPIGEN_VAPIS += $(ibus_immodule_vapi)
noinst_DATA += $(ibus_immodule_vapi)
MAINTAINERCLEANFILES += $(ibus_immodule_vapi)
DISTCLEANFILES += $(ibus_immodule_vapi)
endif

-include $(top_srcdir)/git.mk
