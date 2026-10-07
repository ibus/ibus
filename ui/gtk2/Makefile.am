# vim:set noet ts=4:
#
# ibus - The Input Bus
#
# Copyright (c) 2007-2010 Peng Huang <shawn.p.huang@gmail.com>
# Copyright (c) 2007-2010 Red Hat, Inc.
#
# SPDX-License-Identifier: LGPL-2.1-or-later
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
# License along with this library; if not, see <https://www.gnu.org/licenses/>.

ui_gtk_PYTHON = \
	candidatepanel.py \
	handle.py \
	i18n.py \
	icon.py \
	languagebar.py \
	main.py \
	menu.py \
	notifications.py \
	panel.py \
	propitem.py \
	toolitem.py \
	engineabout.py \
	$(NULL)
ui_gtkdir = $(pkgdatadir)/ui/gtk

libexec_SCRIPTS = \
	ibus-ui-gtk \
	$(NULL)

component_DATA = \
	gtkpanel.xml \
	$(NULL)
componentdir = $(pkgdatadir)/component

CLEANFILES = \
	gtkpanel.xml \
	*.pyc \
	$(NULL)

EXTRA_DIST = \
	ibus-ui-gtk.in \
	gtkpanel.xml.in \
	$(NULL)

gtkpanel.xml: gtkpanel.xml.in
	$(AM_V_GEN) sed \
		-e 's|@VERSION[@]|$(VERSION)|g' \
		-e 's|@libexecdir[@]|$(libexecdir)|g' $< > $@.tmp && \
		mv $@.tmp $@

test:
	$(ENV) DBUS_DEBUG=true \
		IBUS_PREFIX=@prefix@ \
		IBUS_DATAROOTDIR=@datarootdir@ \
		IBUS_LOCALEDIR=@localedir@ \
		PYTHONPATH=$(top_srcdir) \
		$(PYTHON) $(srcdir)/main.py --replace

-include $(top_srcdir)/git.mk
