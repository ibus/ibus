# vim:set noet ts=4:
#
# ibus - The Input Bus
#
# Copyright (c) 2007-2015 Peng Huang <shawn.p.huang@gmail.com>
# Copyright (c) 2015-2025 Takao Fujiwara <takao.fujiwara1@gmail.com>
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

DEPS = $(top_builddir)/src/libibus-@IBUS_API_VERSION@.la

AM_CPPFLAGS = \
    -g                                      \
    @GLIB2_CFLAGS@                          \
    @GIO2_CFLAGS@                           \
    -DIBUS_DISABLE_DEPRECATION_WARNINGS     \
    -DX11_LOCALEDATADIR=\"$(X11_LOCALEDATADIR)\" \
    -I$(top_srcdir)/src                     \
    -I$(top_builddir)/src                   \
    $(NULL)

prog_ldadd =\
    @GLIB2_LIBS@                                                        \
    @GIO2_LIBS@                                                         \
    $(top_builddir)/src/libibus-@IBUS_API_VERSION@.la                   \
    $(NULL)

noinst_PROGRAMS = $(TESTS_C)
noinst_SCRIPTS = $(TESTS_SCRIPT)
TESTS_C = \
    ibus-bus                        \
    ibus-config                     \
    ibus-configservice              \
    ibus-factory                    \
    ibus-inputcontext               \
    ibus-inputcontext-create        \
    ibus-keynames                   \
    ibus-registry                   \
    ibus-serializable               \
    ibus-share                      \
    ibus-util                       \
    $(NULL)

TESTS_SCRIPT = \
    xkb-latin-layouts               \
    $(NULL)

TESTS = \
    $(TESTS_C)                      \
    $(TESTS_SCRIPT)                 \
    $(NULL)

CLEANFILES =

if ENABLE_ENGINE
TESTS_C += ibus-engine-switch
endif

if ENABLE_GTK3_OR_4
TESTS_C += ibus-compose
if ENABLE_LIBEVDEV
TESTS_C += ibus-keypress
noinst_PROGRAMS += uinput-replay
endif
endif

if ENABLE_GTK4
TESTS_C += ibus-keyval
endif

TESTS_ENVIRONMENT = \
    top_builddir=$(top_builddir) \
    top_srcdir=$(top_srcdir) \
    builddir=$(builddir) \
    srcdir=$(srcdir) \
    LD_LIBRARY_PATH="$(top_builddir)/src/.libs:$(top_builddir)/src" \
    DISABLE_GUI_TESTS="$(DISABLE_GUI_TESTS)" \
    DISABLE_DAEMONIZE_IN_TESTS="$(DISABLE_DAEMONIZE_IN_TESTS)" \
    $(NULL)

LOG_COMPILER = $(srcdir)/runtest
test_metas_in = meta.test.in

if ENABLE_INSTALL_TESTS
bin_SCRIPTS = ibus-desktop-testing-runner
_test_execs = $(TESTS:ibus-compose=ibus-compose-locales)
test_execs = $(_test_execs)
test_metas = $(addsuffix .test, $(_test_execs))
test_sources_DATA = \
    $(test_metas) \
    ibus-compose.basic \
    ibus-compose.emoji \
    ibus-compose.env \
    libinput-test.yml \
    $(NULL)
test_sourcesdir = $(datadir)/installed-tests/ibus

CLEANFILES += \
    $(test_metas) \
    ibus-desktop-testing-autostart \
    ibus-desktop-testing-runner \
    org.freedesktop.IBus.Desktop.Testing.desktop \
    $(NULL)

test_execs_PROGRAMS = $(TESTS_C)
test_execs_SCRIPTS = $(TESTS_SCRIPT)
if ENABLE_GTK3_OR_4
test_execs_SCRIPTS += ibus-compose-locales
CLEANFILES += ibus-compose-locales
endif
test_execsdir = $(libexecdir)/installed-tests/ibus
libexec_SCRIPTS = \
  ibus-desktop-testing-autostart \
  ibus-desktop-testing-module \
  $(NULL)

test_frame_DATA = org.freedesktop.IBus.Desktop.Testing.desktop
test_framedir = $(pkgdatadir)/tests
org.freedesktop.IBus.Desktop.Testing.desktop: ibus-desktop-testing.desktop.in
	$(AM_V_GEN) sed -e "s|\@libexecdir\@|$(libexecdir)|g" \
	$< > $@.tmp && \
	mv $@.tmp $@
endif

$(test_metas): $(test_metas_in)
	f=`echo $@ | sed -e 's/\.test//'`; \
	TEST_EXEC=$(test_execsdir)/$$f; \
	TEST_SRCDIR=$(test_sourcesdir); \
	sed -e "s|@TEST_EXEC[@]|$$TEST_EXEC|g" \
	    -e "s|@TEST_SRCDIR[@]|$$TEST_SRCDIR|g" \
	    $(srcdir)/$(test_metas_in) \
	    > $@.tmp && mv $@.tmp $@; \
	$(NULL)

ibus-compose-locales: ibus-compose-locales.in
	SRCDIR=$(test_sourcesdir); \
	sed -e "s|@SRCDIR[@]|$$SRCDIR|g" $< > $@.tmp; \
	mv $@.tmp $@; \
	chmod a+x $@; \
	$(NULL)

%.sh: %.sh.in
	sed -e "s|@builddir[@]|$(builddir)|g" \
	    -e "s|@libexecdir[@]|$(libexecdir)|g" \
	    -e "s|@srcdir[@]|$(srcdir)|g" \
	    -e "s|@test_execsdir[@]|$(test_execsdir)|g" \
	    -e "s|@test_sourcesdir[@]|$(test_sourcesdir)|g" \
	$< > $@.tmp; \
	mv $@.tmp $@; \
	chmod a+x $@; \
	$(NULL)

ibus-desktop-testing-autostart: ibus-desktop-testing-autostart.in
	INSTALLEDDIR=$(datadir)/installed-tests; \
	sed -e "s|@DATADIR[@]|$(datadir)|g" \
	    -e "s|@INSTALLEDDIR[@]|$$INSTALLEDDIR|g" \
	    -e "s|@LIBEXECDIR[@]|$(libexecdir)|g" \
	    $< > $@.tmp; \
	mv $@.tmp $@; \
	chmod a+x $@; \
	$(NULL)

ibus-desktop-testing-runner: ibus-desktop-testing-runner.in
	INSTALLEDDIR=$(datadir)/installed-tests; \
	sed -e "s|@DATADIR[@]|$(datadir)|g" \
	    -e "s|@INSTALLEDDIR[@]|$$INSTALLEDDIR|g" \
	    -e "s|@LIBEXECDIR[@]|$(libexecdir)|g" \
	    $< > $@.tmp; \
	mv $@.tmp $@; \
	chmod a+x $@; \
	$(NULL)

EXTRA_DIST = \
    $(test_metas_in) \
    $(TESTS_SCRIPT) \
    runtest \
    ibus-compose.basic \
    ibus-compose.emoji \
    ibus-compose.env \
    ibus-compose-locales.in \
    ibus-desktop-testing.desktop.in \
    ibus-desktop-testing-autostart.in \
    ibus-desktop-testing-module \
    ibus-desktop-testing-runner.in \
    libinput-test.yml \
    $(NULL)

ibus_bus_SOURCES = ibus-bus.c
ibus_bus_LDADD = $(prog_ldadd)

ibus_compose_SOURCES = ibus-compose.c
if ENABLE_GTK4
ibus_compose_CFLAGS = @GTK4_CFLAGS@
ibus_compose_LDADD = $(prog_ldadd) @GTK4_LIBS@
else
if ENABLE_GTK3
ibus_compose_CFLAGS = @GTK3_CFLAGS@
ibus_compose_LDADD = $(prog_ldadd) @GTK3_LIBS@
endif
endif

ibus_config_SOURCES = ibus-config.c
ibus_config_LDADD = $(prog_ldadd)

ibus_configservice_SOURCES = ibus-configservice.c
ibus_configservice_LDADD = $(prog_ldadd)

ibus_engine_switch_SOURCES = ibus-engine-switch.c
ibus_engine_switch_LDADD = $(prog_ldadd)

ibus_factory_SOURCES = ibus-factory.c
ibus_factory_LDADD = $(prog_ldadd)

ibus_inputcontext_SOURCES = ibus-inputcontext.c
ibus_inputcontext_LDADD = $(prog_ldadd)

ibus_inputcontext_create_SOURCES = ibus-inputcontext-create.c
ibus_inputcontext_create_LDADD = $(prog_ldadd)

ibus_keynames_SOURCES = ibus-keynames.c
ibus_keynames_LDADD = $(prog_ldadd)

uinput_replay_SOURCES = uinput-replay.c uinput-replay.h
uinput_replay_CFLAGS = @GLIB2_CFLAGS@ @LIBEVDEV_CFLAGS@ -DUINPUT_REPLAY_MAIN
uinput_replay_LDADD = @GLIB2_LIBS@ @LIBEVDEV_LIBS@

ibus_keypress_SOURCES = \
    ibus-keypress.c \
    ibus-keypress-data.h \
    uinput-replay.c \
    uinput-replay.h \
    $(NULL)
if ENABLE_GTK4
ibus_keypress_CFLAGS = @GTK4_CFLAGS@ @LIBEVDEV_CFLAGS@
ibus_keypress_LDADD = @GTK4_LIBS@ @LIBEVDEV_LIBS@ $(prog_ldadd)
else
if ENABLE_GTK3
ibus_keypress_CFLAGS = @GTK3_CFLAGS@ @LIBEVDEV_CFLAGS@
ibus_keypress_LDADD = @GTK3_LIBS@ @LIBEVDEV_LIBS@ $(prog_ldadd)
endif
endif

ibus_keyval_SOURCES = ibus-keyval.c
ibus_keyval_CFLAGS = @GTK4_CFLAGS@
ibus_keyval_LDADD = $(prog_ldadd) @GTK4_LIBS@

ibus_registry_SOURCES = ibus-registry.c
ibus_registry_LDADD = $(prog_ldadd)

ibus_serializable_SOURCES = ibus-serializable.c
ibus_serializable_LDADD = $(prog_ldadd)

ibus_share_SOURCES = ibus-share.c
ibus_share_CFLAGS = @DBUS_CFLAGS@
ibus_share_LDADD = $(prog_ldadd) @DBUS_LIBS@

ibus_util_SOURCES = ibus-util.c
ibus_util_LDADD = $(prog_ldadd)

-include $(top_srcdir)/git.mk
