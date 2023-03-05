# vim:set noet ts=4:
#
# ibus - The Input Bus
#
# Copyright (c) 2007-2016 Peng Huang <shawn.p.huang@gmail.com>
# Copyright (c) 2015-2019 Takao Fujiwara <takao.fujiwara1@gmail.com>
# Copyright (c) 2007-2019 Red Hat, Inc.
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

if ENABLE_UI
UI_DIR = ui
endif

if ENABLE_ENGINE
ENGINE_DIR = engine
endif

if ENABLE_SETUP
SETUP_DIR = setup
endif

if ENABLE_PYTHON_LIBRARY
PYTHON_LIB_DIRS = ibus
endif

if ENABLE_DAEMON
DAEMON_DIR = \
	bus \
	$(NULL)
endif

SUBDIRS = \
	src \
	bindings \
	util \
	conf \
	client \
	portal \
	data \
	m4 \
	po \
	$(ENGINE_DIR) \
	$(UI_DIR) \
	tools \
	$(DAEMON_DIR) \
	$(PYTHON_LIB_DIRS) \
	$(SETUP_DIR) \
	docs \
	$(NULL)

ACLOCAL_AMFLAGS = -I m4

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = ibus-@IBUS_API_VERSION@.pc

ibus_pc_in = ibus-@IBUS_API_VERSION@.pc.in
EXTRA_DIST = \
	COPYING.unicode \
	autogen.sh \
	$(ibus_pc_in) \
	ibus.spec.in \
	python-config.py \
	xinput-ibus \
	$(NULL)

noinst_DIST = \
	$(NULL)

DISTCLEANFILES = \
	po/stamp-it \
	$(NULL)

install-data-hook:
	$(MKDIR_P) $(DESTDIR)${pkgdatadir}/engine

DISTCHECK_CONFIGURE_FLAGS = \
	--enable-gtk-doc \
	--disable-schemas-install \
	--enable-memconf \
	$(NULL)

dist-hook:
	if test -d .git ; then \
		git log --name-status --date=iso > $(distdir)/ChangeLog ; \
		IBUS_PREV_MICRO_VERSION=`expr $(IBUS_MICRO_VERSION) - 1`; \
		IBUS_PREV_VERSION=\
"$(IBUS_MAJOR_VERSION).$(IBUS_MINOR_VERSION).$$IBUS_PREV_MICRO_VERSION"; \
		echo "Changes in IBus $(IBUS_VERSION)" > $(distdir)/NEWS; \
		echo "" >> $(distdir)/NEWS; \
		git shortlog $$IBUS_PREV_VERSION...$(IBUS_VERSION) \
		        >> $(distdir)/NEWS; \
		echo "" >> $(distdir)/NEWS; \
		git log $$IBUS_PREV_VERSION...$(IBUS_VERSION) --reverse \
		        --pretty=format:'%s (%an) %h' >> $(distdir)/NEWS; \
	fi

distclean-local:
	if test "x$(srcdir)" = "x."; then :; else \
		rm -f ChangeLog; \
		rm -f NEWS; \
	fi

MAINTAINERCLEANFILES = \
	$(srcdir)/ChangeLog \
	$(NULL)

rpm: dist @PACKAGE_NAME@.spec
	rpmbuild -bb \
			--define "_sourcedir `pwd`" \
			--define "_builddir `pwd`/rpm" \
			--define "_srcrpmdir `pwd`/rpm" \
			--define "_rpmdir `pwd`"/rpm \
			--define "_specdir `pwd`/rpm" \
			@PACKAGE_NAME@.spec

srpm: dist @PACKAGE_NAME@.spec
	rpmbuild -bs \
			--define "_sourcedir `pwd`" \
			--define "_builddir `pwd`/rpm" \
			--define "_srcrpmdir `pwd`/rpm" \
			--define "_rpmdir `pwd`/rpm" \
			--define "_specdir `pwd`/rpm" \
			@PACKAGE_NAME@.spec

clean-rpm:
	-$(RM) -r "`uname -i`"

clean-local: clean-rpm

git-tag:
	git tag -s @PACKAGE_VERSION@

git-clean-tree:
	git clean -d -f -x

GITIGNOREFILES = \
	INSTALL \
	aclocal.m4 \
	compile \
	config.guess \
	config.h.in \
	config.sub \
	depcomp \
	gtk-doc.make \
	install-sh \
	ltmain.sh \
	missing \
	mkinstalldirs \
	py-compile \
	stamp-h* \
	$(NULL)

-include $(top_srcdir)/git.mk
