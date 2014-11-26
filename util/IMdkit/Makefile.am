# vim:set noet ts=4:
#
# ibus - The Input Bus
#
# Copyright (c) 2007-2010 Peng Huang <shawn.p.huang@gmail.com>
# Copyright (c) 2007-2010 Red Hat, Inc.
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

noinst_LTLIBRARIES = libIMdkit.la

libIMdkit_la_SOURCES = \
	FrameMgr.c \
	i18nAttr.c \
	i18nClbk.c \
	i18nIc.c \
	i18nIMProto.c \
	i18nMethod.c \
	i18nOffsetCache.c \
	i18nPtHdr.c \
	i18nUtil.c \
	i18nX.c \
	IMConn.c \
	IMMethod.c \
	IMValues.c \
	$(NULL)

noinst_HEADERS = \
	FrameMgr.h \
	IMdkit.h \
	Xi18n.h \
	Xi18nX.h \
	XimFunc.h \
	XimProto.h \
	Xtrans.h \
	$(NULL)

libIMdkit_la_CFLAGS = \
	@X11_CFLAGS@ \
	$(NULL)

-include $(top_srcdir)/git.mk
