# vim:set noet ts=4:
#
# ibus - The Input Bus
#
# Copyright (c) 2007-2015 Peng Huang <shawn.p.huang@gmail.com>
# Copyright (c) 2015-2025 Takao Fujwiara <takao.fujiwara1@gmail.com>
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

NULL =

libibus = $(top_builddir)/src/libibus-@IBUS_API_VERSION@.la
libibuswaylandim = $(top_builddir)/client/wayland/libibuswaylandim.la

component_DATA = \
    gtkpanel.xml \
    $(NULL)
componentdir = $(pkgdatadir)/component

%.xml: %.xml.in
	$(AM_V_GEN) sed \
		-e 's|@VERSION[@]|$(VERSION)|g' \
		-e 's|@libexecdir[@]|$(libexecdir)|g' $< > $@.tmp && \
		mv $@.tmp $@
	$(NULL)
%.desktop: %.desktop.in
	$(AM_V_GEN) sed \
		-e 's|@libexecdir[@]|$(libexecdir)|g' \
		-e 's|^#Name=|Name=|' $< > $@.tmp && \
		mv $@.tmp $@
	$(NULL)

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
    @GTK3_CFLAGS@ \
    -DG_LOG_DOMAIN=\"IBUS\" \
    -DBINDIR=\"$(bindir)\" \
    -DPKGDATADIR=\"$(pkgdatadir)\" \
    -DIBUS_DISABLE_DEPRECATED \
    -Wno-unused-variable \
    -Wno-unused-but-set-variable \
    -Wno-unused-function \
    $(NULL)

AM_LDADD = \
    @GOBJECT2_LIBS@ \
    @GLIB2_LIBS@ \
    @GIO2_LIBS@ \
    @GTHREAD2_LIBS@ \
    @GTK3_LIBS@ \
    $(libibus) \
    $(NULL)

AM_VALAFLAGS = \
    --vapidir=$(top_builddir)/bindings/vala \
    --vapidir=$(top_srcdir)/bindings/vala \
    --pkg=posix \
    --pkg=gtk+-3.0 \
    --pkg=ibus-1.0 \
    --pkg=config \
    --pkg=glibmacro \
    --pkg-config="\"$(PKG_CONFIG) --with-path=$(PKG_CONFIG_PATH)\"" \
    --target-glib="$(VALA_TARGET_GLIB_VERSION)" \
    $(NULL)

libexec_PROGRAMS = ibus-ui-gtk3

ibus_ui_gtk3_SOURCES = \
    application.vala \
    bindingcommon.vala \
    candidatearea.vala \
    candidatepanel.vala \
    emojier.vala \
    handle.vala \
    iconwidget.vala \
    keybindingmanager.vala \
    panel.vala \
    pango.vala \
    property.vala \
    propertypanel.vala \
    separator.vala \
    switcher.vala \
    xkblayout.vala \
    gen-notification-item.xml.c \
    gen-notification-watcher.xml.c \
    $(NULL)

ibus_ui_gtk3_LDADD = \
    $(AM_LDADD) \
    $(NULL)

if ENABLE_XIM
AM_CFLAGS += \
    @X11_CFLAGS@ \
    $(NULL)

AM_LDADD += \
    @X11_LIBS@ \
    -lXi \
    $(NULL)

AM_VALAFLAGS += \
    --pkg=gdk-x11-3.0 \
    --pkg=xi \
    -D ENABLE_XIM \
    $(NULL)
endif

if ENABLE_LIBNOTIFY
AM_CFLAGS += \
       @LIBNOTIFY_CFLAGS@ \
       $(NULL)

AM_LDADD += \
       @LIBNOTIFY_LIBS@ \
       $(NULL)

AM_VALAFLAGS += \
       --pkg=libnotify \
       -D ENABLE_LIBNOTIFY \
       $(NULL)
endif

if ENABLE_APPINDICATOR
AM_VALAFLAGS += \
    --define=INDICATOR \
    --pkg=Dbusmenu-0.4 \
    --pkg=DbusmenuGtk3-0.4 \
    $(NULL)
AM_CFLAGS += \
    @DBUSMENU_GLIB_CFLAGS@ \
    @DBUSMENU_GTK3_CFLAGS@ \
    $(NULL)
AM_LDADD += \
    @DBUSMENU_GLIB_LIBS@ \
    @DBUSMENU_GTK3_LIBS@ \
    $(NULL)
ibus_ui_gtk3_SOURCES += \
    indicator.vala \
    $(NULL)
endif

gen-%.xml.c: %.xml
	echo "Building $@ from $<"
	echo "const char * _$(subst -,_,$(subst .,_,$(basename $(notdir $<)))) = " > $@
	sed -e "s:\":\\\\\":g" -e s:^:\": -e s:\$$:\\\\n\": $< >> $@
	echo ";" >> $@

emoji_headers =         \
    ibusemojidialog.h   \
    $(NULL)

desktop_in_files = \
    ibus-ui-emojier.desktop.in \
    ibus-ui-wayland.desktop.in \
    ibus-extension-gtk3.desktop.in \
    $(NULL)
desktop_notrans_files = $(desktop_in_files:.desktop.in=.desktop)
desktopdir = $(datadir)/applications
desktop_DATA =

CLEANFILES =            \
    $(desktop_notrans_files) \
    $(desktop_DATA)     \
    gtkpanel.xml        \
    $(NULL)

MAINTAINERCLEANFILES =

# References:
# libappindicator/src/notification-item.xml
# libappindicator/src/notification-watcher.xml
# kdelibs/kdeui/knotifications/src/org.kde.StatusNotifierItem.xml
# kdelibs/kdeui/knotifications/src/org.kde.StatusNotifierWatcher.xml
EXTRA_DIST =                            \
    $(emoji_headers)                    \
    $(desktop_in_files)                 \
    emojierapp.vala                     \
    extension.vala                      \
    gtkextension.xml.in                 \
    gtkpanel.xml.in                     \
    notification-item.xml               \
    notification-watcher.xml            \
    panelbinding.vala                   \
    $(NULL)

if ENABLE_GDK3_WAYLAND
AM_CPPFLAGS += \
    -I$(top_srcdir)/client/wayland \
    -I$(top_builddir)/client/wayland \
    $(NULL)
AM_VALAFLAGS += \
    --vapidir=$(top_builddir)/client/wayland \
    --vapidir=$(top_srcdir)/client/wayland \
    --pkg=ibus-wayland-im-1.0 \
    --pkg=gdk-wayland --define=USE_GDK_WAYLAND \
    $(NULL)
AM_LDADD += \
    $(libibuswaylandim) \
    $(NULL)
desktop_DATA += \
    $(IBUS_UI_WAYLAND_DESKTOP) \
    $(NULL)
$(IBUS_UI_WAYLAND_DESKTOP): ibus-ui-wayland.desktop
	$(AM_V_GEN) $(MSGFMT) --desktop --template $< -d $(top_srcdir)/po -o $@
	$(NULL)
endif

if ENABLE_EMOJI_DICT
AM_VALAFLAGS += --define=EMOJI_DICT

libexec_PROGRAMS += ibus-ui-emojier

ibus_ui_emojier_VALASOURCES =                   \
    emojierapp.vala                             \
    emojier.vala                                \
    iconwidget.vala                             \
    separator.vala                              \
    pango.vala                                  \
    $(NULL)
ibus_ui_emojier_SOURCES =                       \
    $(ibus_ui_emojier_VALASOURCES:.vala=.c)     \
    $(NULL)

ibus_ui_emojier_LDADD =                         \
    $(AM_LDADD)                                 \
    $(NULL)

ibus_ui_emojier_VALAFLAGS =                     \
    $(AM_VALAFLAGS)                             \
    $(NULL)

# This line and foo_VALASOURCES line can delete the duplicated entries
# of emojier.c: emojier.vala
emojierapp.c: $(ibus_ui_emojier_VALASOURCES)
	$(AM_V_VALAC)$(am__cd) $(srcdir) && $(VALAC) $(AM_VALAFLAGS) \
$(VALAFLAGS) -C $(ibus_ui_emojier_VALASOURCES)
	$(NULL)
# make dist creates .c files in a different srcdir
emojierapp.o: $(srcdir)/emojierapp.c
	$(AM_V_CC)source='$<' object='$@' libtool=no \
	DEPDIR=$(DEPDIR) $(CCDEPMODE) $(depcomp) \
	$(AM_V_CC_no)$(COMPILE) -c -o $@ $<
	$(NULL)

MAINTAINERCLEANFILES += emojierapp.c

component_DATA += gtkextension.xml
CLEANFILES += gtkextension.xml
libexec_PROGRAMS += ibus-extension-gtk3

ibus_extension_gtk3_VALASOURCES =               \
    bindingcommon.vala                          \
    emojier.vala                                \
    extension.vala                              \
    iconwidget.vala                             \
    keybindingmanager.vala                      \
    panelbinding.vala                           \
    pango.vala                                  \
    $(NULL)
ibus_extension_gtk3_SOURCES =                   \
    $(ibus_extension_gtk3_VALASOURCES:.vala=.c) \
    $(NULL)

ibus_extension_gtk3_LDADD =                     \
    $(AM_LDADD)                                 \
    $(NULL)
ibus_extension_gtk3_VALAFLAGS =                 \
    $(AM_VALAFLAGS)                             \
    $(NULL)

# This line and foo_VALASOURCES line can delete the duplicated entries
# of emojier.c: emojier.vala
extension.c: $(ibus_extension_gtk3_VALASOURCES)
	$(AM_V_VALAC)$(am__cd) $(srcdir) && $(VALAC) $(AM_VALAFLAGS) \
$(VALAFLAGS) -C $(ibus_extension_gtk3_VALASOURCES)
	$(NULL)
# make dist creates .c files in a different srcdir
extension.o: $(srcdir)/extension.c
	$(AM_V_CC)source='$<' object='$@' libtool=no \
	DEPDIR=$(DEPDIR) $(CCDEPMODE) $(depcomp) \
	$(AM_V_CC_no)$(COMPILE) -c -o $@ $<
	$(NULL)
# of emojier.c: emojier.vala
panelbinding.c: $(ibus_extension_gtk3_VALASOURCES)
	$(AM_V_VALAC)$(am__cd) $(srcdir) && $(VALAC) $(AM_VALAFLAGS) \
$(VALAFLAGS) -C $(ibus_extension_gtk3_VALASOURCES)
	$(NULL)
# make dist creates .c files in a different srcdir
panelbinding.o: $(srcdir)/panelbinding.c
	$(AM_V_CC)source='$<' object='$@' libtool=no \
	DEPDIR=$(DEPDIR) $(CCDEPMODE) $(depcomp) \
	$(AM_V_CC_no)$(COMPILE) -c -o $@ $<
	$(NULL)

MAINTAINERCLEANFILES += extension.c panelbinding.c

man_seven_DATA = ibus-emoji.7
man_sevendir = $(mandir)/man7

desktop_DATA += \
    org.freedesktop.IBus.Panel.Emojier.desktop \
    org.freedesktop.IBus.Panel.Extension.Gtk3.desktop \
    $(NULL)
org.freedesktop.IBus.Panel.Emojier.desktop: ibus-ui-emojier.desktop
	$(AM_V_GEN) $(MSGFMT) --desktop --template $< -d $(top_srcdir)/po -o $@
	$(NULL)
org.freedesktop.IBus.Panel.Extension.Gtk3.desktop: ibus-extension-gtk3.desktop
	$(AM_V_GEN) $(MSGFMT) --desktop --template $< -d $(top_srcdir)/po -o $@
	$(NULL)
endif
# end of ENABLE_EMOJI_DICT

-include $(top_srcdir)/git.mk
