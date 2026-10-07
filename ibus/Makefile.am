# vim:set noet ts=4:
#
# ibus - The Input Bus
#
# Copyright (c) 2007-2014 Peng Huang <shawn.p.huang@gmail.com>
# Copyright (c) 2007-2014 Red Hat, Inc.
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
SUBDIRS = \
	interface \
	$(NULL)

PYTHON = $(PYTHON2)

ibus_PYTHON = \
	ascii.py \
	application.py \
	attribute.py \
	common.py \
	bus.py \
	inputcontext.py \
	engine.py \
	exception.py \
	factory.py \
	_gtk.py \
	__init__.py \
	keysyms.py \
	lang.py \
	lookuptable.py \
	modifier.py \
	object.py \
	panel.py \
	notifications.py \
	property.py \
	serializable.py \
	text.py \
	component.py \
	enginedesc.py \
	observedpath.py \
	utility.py \
	config.py \
	$(NULL)

nodist_ibus_PYTHON = \
	_config.py \
	$(NULL)

ibusdir = @pkgpython2dir@

EXTRA_DIST = \
	_config.py.in \
	$(NULL)

CLEANFILES = \
	*.pyc \
	$(NULL)

DISTCLEANFILES = \
	_config.py \
	$(NULL)

-include $(top_srcdir)/git.mk
