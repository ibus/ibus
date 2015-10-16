# vim:set noet ts=4:
#
# ibus - The Input Bus
#
# Copyright (c) 2007-2010 Peng Huang <shawn.p.huang@gmail.com>
# Copyright (c) 2011 Daiki Ueno <ueno@unixuser.org>
# Copyright (c) 2007-2011 Red Hat, Inc.
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

gsettings_schemas_in_files = org.freedesktop.ibus.gschema.xml.in
gsettingsconvertdir = $(datadir)/GConf/gsettings
dconfprofiledir = $(sysconfdir)/dconf/profile
dconfdbdir = $(sysconfdir)/dconf/db/ibus.d

if ENABLE_DCONF
gsettings_SCHEMAS = $(gsettings_schemas_in_files:.gschema.xml.in=.gschema.xml)
dist_gsettingsconvert_DATA = ibus.convert

dconfprofile_DATA = profile/ibus
dconfdb_DATA = 00-upstream-settings

@GSETTINGS_RULES@
@INTLTOOL_XML_NOMERGE_RULE@

install-data-hook:
	if test -z "$(DESTDIR)"; then \
		dconf update; \
	fi
endif

EXTRA_DIST = \
	$(gsettings_schemas_in_files) \
	make-dconf-override-db.sh \
	profile/ibus \
	00-upstream-settings \
	$(NULL)

CLEANFILES = \
	$(gsettings_SCHEMAS) \
	$(NULL)

MAINTAINERCLEANFILES = \
	$(gsettings_schemas_in_files) \
	00-upstream-settings \
	$(NULL)

org.freedesktop.ibus.gschema.xml.in: $(top_srcdir)/data/ibus.schemas.in
	$(AM_V_GEN) gsettings-schema-convert --force --gconf --xml \
		--schema-id "org.freedesktop.ibus" \
		--output $@ $<

00-upstream-settings: $(srcdir)/make-dconf-override-db.sh | $(gsettings_SCHEMAS)
	@$(MKDIR_P) db
	$(AM_V_GEN) $(srcdir)/make-dconf-override-db.sh > $@ || \
		{ rc=$$?; $(RM) -rf $@; exit $$rc; }

-include $(top_srcdir)/git.mk
