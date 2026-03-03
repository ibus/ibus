# vim:set noet ts=4:
#
# ibus - The Input Bus
#
# Copyright (c) 2010-2016, Google Inc. All rights reserved.
# Copyright (c) 2007-2016 Peng Huang <shawn.p.huang@gmail.com>
# Copyright (c) 2013-2025 Takao Fujiwara <takao.fujiwara1@gmail.com>
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

AM_CFLAGS = \
    @GLIB2_CFLAGS@ \
    @GIO2_CFLAGS@ \
    @GTHREAD2_CFLAGS@ \
    -DG_LOG_DOMAIN=\"IBUS\" \
    -DPKGDATADIR=\"$(pkgdatadir)\" \
    -DLIBEXECDIR=\"$(libexecdir)\" \
    -DBINDIR=\"@bindir@\" \
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
    $(libibus) \
    $(NULL)

AM_VALAFLAGS = \
    --vapidir=$(top_builddir)/bindings/vala \
    --pkg=ibus-1.0 \
    --pkg-config="\"$(PKG_CONFIG) --with-path=$(PKG_CONFIG_PATH)\"" \
    --target-glib="$(VALA_TARGET_GLIB_VERSION)" \
    $(NULL)

TESTS =

if ENABLE_PYGNOME_DESKTOP
TESTS += test-gnome.py
endif

libexec_PROGRAMS = \
    ibus-engine-simple \
    $(NULL)

ibus_engine_simple_SOURCES = \
    main.vala \
    $(NULL)
ibus_engine_simple_CFLAGS = \
    $(AM_CFLAGS) \
    $(NULL)
ibus_engine_simple_LDADD = \
    $(AM_LDADD) \
    $(NULL)
ibus_engine_simple_DEPENDENCIES = \
    $(libibus) \
    $(NULL)

component_DATA = \
    component.dtd \
    simple.xml \
    $(NULL)

componentdir = $(pkgdatadir)/component

MAINTAINERCLEANFILES = \
    simple.xml.in \
    $(NULL)

CLEANFILES = \
    simple.xml \
    $(NULL)

EXTRA_DIST = \
    component.dtd \
    denylist.txt \
    gensimple.py \
    iso639converter.py \
    simple.xml.in \
    test-gnome.py \
    $(NULL)

simple.xml: simple.xml.in
	$(AM_V_GEN) sed \
	    -e 's|@libexecdir[@]|$(libexecdir)|g' $< > $@.tmp && \
	    mv $@.tmp $@

simple.xml.in:
	$(PYTHON) $(srcdir)/gensimple.py \
	    --input=$(XKBCONFIG_BASE)/rules/evdev.xml \
	    --output=$@ \
	    --version=$(VERSION).`date '+%Y%m%d'` \
	    --exec-path=@libexecdir\@/ibus-engine-simple \
	    --iso-path=$(ISOCODES_PREFIX)/share/xml/iso-codes/iso_639.xml \
	    --first-language \
	$(NULL)

$(libibus):
	$(MAKE) -C $(top_builddir)/src

check-am: all-am simple.xml
	$(MAKE) $(AM_MAKEFLAGS) check-TESTS
	cp_dtd=0; \
	if test ! -f $(builddir)/component.dtd ; then \
	cp $(srcdir)/component.dtd $(builddir)/.; \
	cp_dtd=1; \
	fi; \
	xmllint --noout --valid --postvalid $(builddir)/simple.xml && \
	if test $$cp_dtd -eq 1 ; then \
	rm $(builddir)/component.dtd; \
	fi

-include $(top_srcdir)/git.mk
