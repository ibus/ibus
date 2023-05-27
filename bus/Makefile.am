# vim:set noet ts=4:
#
# ibus - The Input Bus
#
# Copyright (c) 2007-2013 Peng Huang <shawn.p.huang@gmail.com>
# Copyright (c) 2013-2022 Takao Fujiwara <takao.fujiwara1@gmail.com>
# Copyright (c) 2007-2022 Red Hat, Inc.
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
SUBDIRS = . services

libibus = $(top_builddir)/src/libibus-@IBUS_API_VERSION@.la

AM_CPPFLAGS =                \
	-I$(top_srcdir)/src   \
	-I$(top_builddir)/src \
	$(NULL)

AM_CFLAGS = \
	@GLIB2_CFLAGS@ \
	@GIO2_CFLAGS@ \
	@GTHREAD2_CFLAGS@ \
	-DG_LOG_DOMAIN=\"IBUS\" \
	-DPKGDATADIR=\"$(pkgdatadir)\" \
	-DLIBEXECDIR=\"$(libexecdir)\" \
	-DBINDIR=\"@bindir@\" \
	-DIBUS_DISABLE_DEPRECATED \
	$(NULL)
AM_LDADD = \
	@GOBJECT2_LIBS@ \
	@GLIB2_LIBS@ \
	@GIO2_LIBS@ \
	@GTHREAD2_LIBS@ \
	$(libibus) \
	$(NULL)

commonsrc = \
	component.c \
	component.h \
	dbusimpl.c \
	dbusimpl.h \
	ibusimpl.c \
	ibusimpl.h \
	inputcontext.c \
	inputcontext.h \
	engineproxy.c \
	engineproxy.h \
	panelproxy.c \
	panelproxy.h \
	factoryproxy.c \
	factoryproxy.h \
	global.c \
	global.h \
	server.c \
	server.h \
	connection.c \
	connection.h \
	matchrule.c \
	matchrule.h \
	marshalers.c \
	marshalers.h \
	types.h \
	$(NULL)

bin_PROGRAMS = ibus-daemon
ibus_daemon_DEPENDENCIES = \
	$(libibus) \
	$(NULL)
ibus_daemon_SOURCES = \
	$(commonsrc) \
	main.c \
	$(NULL)
ibus_daemon_CFLAGS = \
	$(AM_CFLAGS) \
	$(NULL)
ibus_daemon_LDADD = \
	$(AM_LDADD) \
	$(NULL)

BUILT_SOURCES =             \
	marshalers.h            \
	marshalers.c            \
	$(NULL)

# gen marshal
marshalers.h: marshalers.list
	$(AM_V_GEN) $(GLIB_GENMARSHAL) --prefix=bus_marshal $(srcdir)/marshalers.list --header --internal > $@.tmp && \
	mv $@.tmp $@

marshalers.c: marshalers.h marshalers.list
	$(AM_V_GEN) (echo "#include \"marshalers.h\""; \
	$(GLIB_GENMARSHAL) --prefix=bus_marshal $(srcdir)/marshalers.list --body --internal) > $@.tmp && \
	mv $@.tmp $@

if ENABLE_EMOJI_DICT
AM_CFLAGS += -DEMOJI_DICT
endif

if ENABLE_TESTS
TESTS = \
	test-matchrule \
	test-stress	\
	$(NULL)
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

LOG_COMPILER = $(top_srcdir)/src/tests/runtest

noinst_PROGRAMS = $(TESTS)

test_matchrule_DEPENDENCIES = \
	$(libibus) \
	$(NULL)
test_matchrule_SOURCES = \
	$(commonsrc) \
	test-matchrule.c \
	$(NULL)
test_matchrule_CFLAGS = \
	$(AM_CFLAGS) \
	$(NULL)
test_matchrule_LDADD = \
	$(AM_LDADD) \
	$(NULL)

test_stress_SOURCES = \
	test-client.c \
	test-client.h \
	test-stress.c \
	$(NULL)
test_stress_CFLAGS = \
	$(AM_CFLAGS) \
	@GTK2_CFLAGS@ \
	@X11_CFLAGS@ \
	$(NULL)
test_stress_LDADD = \
	$(AM_LDADD) \
	@GTK2_LIBS@ \
	@X11_LIBS@ \
	$(NULL)

EXTRA_DIST =                \
	marshalers.list         \
	$(NULL)

$(libibus):
	$(MAKE) -C $(top_builddir)/src

test: ibus-daemon
	$(ENV_IBUS_TEST) \
		G_DEBUG=fatal_warnings \
		$(builddir)/ibus-daemon -v

man_one_DATA = ibus-daemon.1
man_onedir = $(mandir)/man1

-include $(top_srcdir)/git.mk
