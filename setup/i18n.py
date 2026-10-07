# vim:set et sts=4 sw=4:
#
# ibus - The Input Bus
#
# Copyright(c) 2007-2015 Peng Huang <shawn.p.huang@gmail.com>
# Copyright(c) 2012-2021 Takao Fujiwara <takao.fujiwara1@gmail.com>
# Copyright(c) 2007-2015 Google, Inc.
# Copyright(c) 2012-2021 Red Hat, Inc.
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

import locale
import gettext
import os
import sys

DOMAINNAME = "ibus10"

_ = lambda a: gettext.dgettext(DOMAINNAME, a)
N_ = lambda a: a

PY3K = sys.version_info >= (3, 0)
LOCALEDIR = os.getenv("IBUS_LOCALEDIR")

def init_textdomain(domainname):
    if domainname == '':
        return
    # Python's locale module doesn't provide all methods on some
    # operating systems like FreeBSD
    try:
        locale.bindtextdomain(domainname, LOCALEDIR)
        locale.bind_textdomain_codeset(domainname, 'UTF-8')
    except AttributeError:
        pass
    gettext.bindtextdomain(domainname, LOCALEDIR)
    # https://docs.python.org/3/library/gettext.html#gettext.lgettext
    if not PY3K:
        gettext.bind_textdomain_codeset(domainname, 'UTF-8')

def gettext_engine_longname(engine):
    name = engine.get_name()
    if (name.startswith('xkb:')):
        return gettext.dgettext('xkeyboard-config', engine.get_longname())
    textdomain = engine.get_textdomain()
    if textdomain == '':
        return engine.get_longname()
    return gettext.dgettext(textdomain, engine.get_longname())

def gettext_engine_description(engine):
    name = engine.get_name()
    if (name.startswith('xkb:')):
        return gettext.dgettext('xkeyboard-config', engine.get_description())
    textdomain = engine.get_textdomain()
    if textdomain == '':
        return engine.get_description()
    return gettext.dgettext(textdomain, engine.get_description())
