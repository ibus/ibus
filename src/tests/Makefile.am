# vim:set noet ts=4:
#
# ibus - The Input Bus
#
# Copyright (c) 2007-2015 Peng Huang <shawn.p.huang@gmail.com>
# Copyright (c) 2015-2022 Takao Fujiwara <takao.fujiwara1@gmail.com>
# Copyright (c) 2007-2018 Red Hat, Inc.
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
    -DX11_DATA_PREFIX=\"$(X11_PREFIX)\"     \
    -I$(top_srcdir)/src                     \
    -I$(top_builddir)/src                   \
    $(NULL)

prog_ldadd =\
    @GLIB2_LIBS@                                                        \
    @GIO2_LIBS@                                                         \
    $(top_builddir)/src/libibus-@IBUS_API_VERSION@.la                   \
    $(NULL)

noinst_PROGRAMS = $(TESTS)
TESTS = \
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

CLEANFILES =

if ENABLE_ENGINE
TESTS += ibus-engine-switch
endif

if ENABLE_GTK3
TESTS += ibus-compose
if ENABLE_XTEST
TESTS += ibus-keypress
endif
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
test_execs = $(TESTS:ibus-compose=ibus-compose-locales)
test_metas = $(addsuffix .test, $(test_execs))
test_sources_DATA = \
    $(test_metas) \
    ibus-compose.emoji \
    ibus-compose.env \
    $(NULL)
test_sourcesdir = $(datadir)/installed-tests/ibus

CLEANFILES += \
    $(test_metas) \
    ibus-desktop-testing-runner \
    org.freedesktop.IBus.Desktop.Testing.desktop \
    $(NULL)

test_execs_PROGRAMS = $(TESTS)
if ENABLE_GTK3
test_execs_SCRIPTS = ibus-compose-locales
CLEANFILES += \
    ibus-compose-locales \
    $(NULL)
endif
test_execsdir = $(libexecdir)/installed-tests/ibus
libexec_SCRIPTS = ibus-desktop-testing-autostart

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
	sed -e "s|@TEST_EXEC[@]|$$TEST_EXEC|g" $(srcdir)/$(test_metas_in) \
	    > $@.tmp && mv $@.tmp $@; \
	$(NULL)

ibus-compose-locales: ibus-compose-locales.in
	SRCDIR=$(test_sourcesdir); \
	sed -e "s|@SRCDIR[@]|$$SRCDIR|g" $< > $@.tmp; \
	mv $@.tmp $@; \
	$(NULL)

ibus-desktop-testing-runner: ibus-desktop-testing-runner.in
	INSTALLEDDIR=$(datadir)/installed-tests; \
	sed -e "s|@INSTALLEDDIR[@]|$$INSTALLEDDIR|g" $< > $@.tmp; \
	mv $@.tmp $@; \
	$(NULL)

EXTRA_DIST = \
    $(test_metas_in) \
    runtest \
    ibus-compose.emoji \
    ibus-compose.env \
    ibus-compose-locales.in \
    ibus-desktop-testing.desktop.in \
    ibus-desktop-testing-autostart \
    ibus-desktop-testing-runner.in \
    $(NULL)

ibus_bus_SOURCES = ibus-bus.c
ibus_bus_LDADD = $(prog_ldadd)

ibus_compose_SOURCES = ibus-compose.c
ibus_compose_CFLAGS = @GTK3_CFLAGS@
ibus_compose_LDADD = $(prog_ldadd) @GTK3_LIBS@

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

ibus_keypress_SOURCES = ibus-keypress.c
ibus_keypress_CFLAGS = @GTK3_CFLAGS@ @XTEST_CFLAGS@
ibus_keypress_LDADD = $(prog_ldadd) @GTK3_LIBS@ @XTEST_LIBS@

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
