# vim:set noet ts=4:
#
# ibus - The Input Bus
#
# Copyright (c) 2007-2016 Peng Huang <shawn.p.huang@gmail.com>
# Copyright (c) 2017-2023 Takao Fujiwara <takao.fujiwara1@gmail.com>
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

-include $(VAPIGEN_MAKEFILE)

libibus = $(top_builddir)/src/libibus-@IBUS_API_VERSION@.la

noinst_LTLIBRARIES =
noinst_DATA =
INTROSPECTION_GIRS =
girdir = $(datadir)/gir-1.0

AM_CPPFLAGS = \
    -I$(top_srcdir)/src \
    -I$(top_builddir)/src \
    -include $(CONFIG_HEADER) \
    $(NULL)
AM_CFLAGS = \
    -DG_LOG_DOMAIN=\"IBUS\" \
    -DPKGDATADIR=\"$(pkgdatadir)\" \
    -DIBUS_DISABLE_DEPRECATED \
    -Wno-unused-variable \
    -Wno-unused-but-set-variable \
    -Wno-unused-function \
    $(NULL)
AM_VALAFLAGS = \
    --vapidir=$(builddir) \
    --vapidir=$(srcdir) \
    --pkg=posix \
    --pkg=gtk+-3.0 \
    --pkg=gdk-x11-3.0 \
    --pkg=ibus-1.0 \
    --pkg=config \
    --pkg=xi \
    --target-glib="$(VALA_TARGET_GLIB_VERSION)" \
    $(NULL)

vapi_deps = \
	IBus-1.0.metadata \
	$(top_builddir)/src/IBus-1.0.gir \
	$(NULL)

ibus-1.0.vapi: $(vapi_deps)

ibus_vapi = ibus-1.0.vapi
VAPIGEN_VAPIS = $(ibus_vapi)

ibus_1_0_vapi_DEPS = gio-2.0
ibus_1_0_vapi_METADATADIRS = $(srcdir)
ibus_1_0_vapi_FILES = \
	$(top_builddir)/src/IBus-1.0.gir \
	$(srcdir)/IBus-1.0-custom.vala \
	$(NULL)

vapidir = $(datadir)/vala/vapi
vapi_DATA = $(ibus_vapi) $(ibus_vapi:.vapi=.deps)

MAINTAINERCLEANFILES = $(ibus_vapi)
DISTCLEANFILES = $(ibus_vapi)

EXTRA_DIST =                                    \
    $(ibus_vapi)                                \
    IBus-1.0.metadata                           \
    IBus-1.0-custom.vala                        \
    IBusEmojiDialog-1.0.metadata                \
    ibus-1.0.deps                               \
    ibus-emoji-dialog-1.0.deps                  \
    config.vapi                                 \
    gdk-wayland.vapi                            \
    glibmacro.vapi                              \
    xi.vapi                                     \
    $(NULL)

if ENABLE_EMOJI_DICT
AM_VALAFLAGS += --define=EMOJI_DICT

libibus_emoji_dialog = libibus-emoji-dialog-1.0.la
noinst_LTLIBRARIES += $(libibus_emoji_dialog)

libibus_emoji_dialog_1_0_la_SOURCES =           \
    candidatearea.vala                          \
    emojier.vala                                \
    iconwidget.vala                             \
    pango.vala                                  \
    separator.vala                              \
    $(NULL)
libibus_emoji_dialog_1_0_la_CFLAGS =            \
    $(AM_CFLAGS)                                \
    @GLIB2_CFLAGS@                              \
    @GIO2_CFLAGS@                               \
    @GTHREAD2_CFLAGS@                           \
    @GTK3_CFLAGS@                               \
    @X11_CFLAGS@                                \
    -DBINDIR=\"$(bindir)\"                      \
    $(NULL)
libibus_emoji_dialog_1_0_la_LIBADD =            \
    @GLIB2_LIBS@                                \
    @GIO2_LIBS@                                 \
    @GTHREAD2_LIBS@                             \
    @GTK3_LIBS@                                 \
    @X11_LIBS@                                  \
    -lXi                                        \
    $(libibus)                                  \
    $(NULL)
libibus_emoji_dialog_1_0_la_LDFLAGS =           \
    -no-undefined                               \
    -export-symbols-regex "ibus_.*"             \
    $(NULL)

# per file setting is needed to avoid conflicting LN_S by calling
# duplicated times in parallel make
%.vala: $(ibus_vapi)
	if test ! -f $@ ; then                                              \
	    $(LN_S) $(top_srcdir)/ui/gtk3/$@ .;                             \
	fi;

MAINTAINERCLEANFILES += $(libibus_emoji_dialog_1_0_la_SOURCES)
DISTCLEANFILES += $(libibus_emoji_dialog_1_0_la_SOURCES)

if HAVE_INTROSPECTION
-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_SCANNER_ARGS =
INTROSPECTION_COMPILER_ARGS =                   \
    --includedir=$(srcdir)                      \
    --includedir=.                              \
    --includedir=$(top_srcdir)/src              \
    $(NULL)


emoji_headers =                                 \
    $(top_srcdir)/ui/gtk3/ibusemojidialog.h     \
    $(NULL)

IBusEmojiDialog-1.0.gir: $(libibus_emoji_dialog) Makefile
IBusEmojiDialog_1_0_gir_SCANNERFLAGS =          \
    --pkg-export=ibus-1.0                       \
    --pkg=gtk+-3.0                              \
    $(IBUS_GIR_SCANNERFLAGS)                    \
    $(NULL)
IBusEmojiDialog_1_0_gir_INCLUDES = Gtk-3.0 GLib-2.0 GObject-2.0 Gio-2.0
IBusEmojiDialog_1_0_gir_LIBS = $(libibus_emoji_dialog) $(libibus)
IBusEmojiDialog_1_0_gir_FILES = $(emoji_headers)
IBusEmojiDialog_1_0_gir_CFLAGS =                \
    -I$(srcdir)                                 \
    -I$(builddir)                               \
    -I$(top_srcdir)/src                         \
    $(NULL)

ibus_emoji_dialog_gir = IBusEmojiDialog-1.0.gir
INTROSPECTION_GIRS += $(ibus_emoji_dialog_gir)
noinst_DATA += $(ibus_emoji_dialog_gir)
EXTRA_DIST += $(ibus_emoji_dialog_gir)
MAINTAINERCLEANFILES += $(ibus_emoji_dialog_gir)
DISTCLEANFILES += $(ibus_emoji_dialog_gir)

ibus-emoji-dialog-1.0.vapi: $(ibus_emoji_dialog_gir) IBusEmojiDialog-1.0.metadata
ibus_emoji_dialog_vapi = ibus-emoji-dialog-1.0.vapi
ibus_emoji_dialog_1_0_vapi_DEPS = gtk+-3.0 gio-2.0
ibus_emoji_dialog_1_0_vapi_METADATADIRS = $(srcdir)
ibus_emoji_dialog_1_0_vapi_FILES = IBusEmojiDialog-1.0.gir
VAPIGEN_VAPIS += $(ibus_emoji_dialog_vapi)
noinst_DATA += $(ibus_emoji_dialog_vapi)
EXTRA_DIST += $(ibus_emoji_dialog_vapi)
MAINTAINERCLEANFILES += $(ibus_emoji_dialog_vapi)
DISTCLEANFILES += $(ibus_emoji_dialog_vapi)

endif
#end of HAVE_INTROSPECTION
endif
# end of ENABLE_EMOJI_DICT

-include $(top_srcdir)/git.mk
