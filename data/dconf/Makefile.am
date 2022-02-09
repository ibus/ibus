# vim:set noet ts=4:
#
# ibus - The Input Bus
#
# Copyright (c) 2007-2010 Peng Huang <shawn.p.huang@gmail.com>
# Copyright (c) 2011 Daiki Ueno <ueno@unixuser.org>
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

gsettings_SCHEMAS = org.freedesktop.ibus.gschema.xml
gsettingsconvertdir = $(datadir)/GConf/gsettings
dist_gsettingsconvert_DATA = ibus.convert
@GSETTINGS_RULES@

dconfprofiledir = $(sysconfdir)/dconf/profile
dconfprofile_DATA = profile/ibus

dconfdbdir = $(sysconfdir)/dconf/db/ibus.d
dconfdb_DATA = 00-upstream-settings

00-upstream-settings: $(srcdir)/make-dconf-override-db.sh | $(gsettings_SCHEMAS)
	@$(MKDIR_P) db
	$(AM_V_GEN) $(srcdir)/make-dconf-override-db.sh > $@ || \
		{ rc=$$?; $(RM) -rf $@; exit $$rc; }

man_5_in_files = 00-upstream-settings.5.in ibus.5.in
man_5_DATA = $(man_5_in_files:.5.in=.5)
man_5dir = $(mandir)/man5
%.5: %.5.in
	$(AM_V_GEN) sed \
	    -e 's|@VERSION[@]|$(VERSION)|g' $< > $@.tmp && \
	    mv $@.tmp $@

install-data-hook:
	if test -z "$(DESTDIR)"; then \
	    dconf update; \
	fi

EXTRA_DIST = \
    $(gsettings_SCHEMAS) \
    $(man_5_in_files) \
    make-dconf-override-db.sh \
    profile/ibus \
    00-upstream-settings \
    $(NULL)

CLEANFILES = \
    $(man_5_DATA) \
    $(NULL)

MAINTAINERCLEANFILES = \
    00-upstream-settings \
    $(NULL)

-include $(top_srcdir)/git.mk
