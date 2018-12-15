# vim:set et sts=4 sw=4:
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

__all__ = (
        "get_language_name",
    )

import xml.parsers.expat
import locale
import gettext

_ = lambda a: gettext.dgettext("ibus", a)
__languages_dict = {}

def get_language_name(_locale):
    lang = _locale.split("_")[0]
    lang = lang.lower()
    if lang in __languages_dict:
        lang = __languages_dict[lang]
        lang = gettext.dgettext("iso_639-3", lang)
    else:
        lang = _(u"Other")
        lang = gettext.dgettext("ibus", lang)
    return lang

def __start_element(name, attrs):
    global __languages_dict
    try:
        name = attrs[u"name"]
        for attr_name in (u"id", u"part1_code", u"part2_code"):
            if attr_name in attrs:
                attr_value = attrs[attr_name]
                __languages_dict[attr_value] = name
    except:
        pass

def __end_element(name):
    pass

def __char_data(data):
    pass

def __load_lang():
    import os
    import _config
    iso_639_3_xml = os.path.join(_config.ISOCODES_PREFIX, "share/xml/iso-codes/iso_639-3.xml")
    p = xml.parsers.expat.ParserCreate()
    p.StartElementHandler = __start_element
    p.EndElementHandler = __end_element
    p.CharacterDataHandler = __char_data
    p.ParseFile(file(iso_639_3_xml))

__load_lang()

if __name__ == "__main__":
    print get_language_name("mai")
    print get_language_name("zh")
    print get_language_name("ja")
    print get_language_name("ko")
