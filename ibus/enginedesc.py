# vim:set et sts=4 sw=4:
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

__all__ = (
        "EngineDesc",
    )

import dbus
from exception import IBusException
from serializable import *

class EngineDesc(Serializable):
    __gtype_name__ = "PYIBusEngineDesc"
    __NAME__ = "IBusEngineDesc"
    def __init__(self, name="", longname="", description="", language="",
                 license="", author="", icon="", layout="us", hotkeys="",
                 rank=0, symbol="", setup="",
                 layout_variant="", layout_option="",
                 version=""):
        super(EngineDesc, self).__init__()
        self.__name = name
        self.__longname = longname
        self.__description = description
        self.__language = language
        self.__license = license
        self.__author = author
        self.__icon = icon
        self.__layout = layout
        self.__layout_variant = layout_variant
        self.__layout_option = layout_option
        self.__rank = rank
        self.__hotkeys = hotkeys
        self.__symbol = symbol
        self.__setup = setup
        self.__version = version

    def get_name(self):
        return self.__name

    def get_longname(self):
        return self.__longname

    def get_description(self):
        return self.__description

    def get_language(self):
        return self.__language

    def get_license(self):
        return self.__license

    def get_author(self):
        return self.__author

    def get_icon(self):
        return self.__icon

    def get_layout(self):
        return self.__layout

    def get_layout_variant(self):
        return self.__layout_variant

    def get_layout_option(self):
        return self.__layout_option

    def get_rank(self):
        return self.__rank

    def get_hotkeys(self):
        return self.__hotkeys

    def get_symbol(self):
        return self.__symbol

    def get_setup(self):
        return self.__setup

    def get_version(self):
        return self.__version

    name                = property(get_name)
    longname            = property(get_longname)
    description         = property(get_description)
    language            = property(get_language)
    license             = property(get_license)
    author              = property(get_author)
    icon                = property(get_icon)
    layout              = property(get_layout)
    layout_variant      = property(get_layout_variant)
    layout_option       = property(get_layout_option)
    rank                = property(get_rank)
    hotkeys             = property(get_hotkeys)
    symbol              = property(get_symbol)
    setup               = property(get_setup)
    version             = property(get_version)

    def serialize(self, struct):
        super(EngineDesc, self).serialize(struct)
        struct.append(dbus.String(self.__name))
        struct.append(dbus.String(self.__longname))
        struct.append(dbus.String(self.__description))
        struct.append(dbus.String(self.__language))
        struct.append(dbus.String(self.__license))
        struct.append(dbus.String(self.__author))
        struct.append(dbus.String(self.__icon))
        struct.append(dbus.String(self.__layout))
        struct.append(dbus.UInt32(self.__rank))
        # Keep the serialize order.
        struct.append(dbus.String(self.__hotkeys))
        struct.append(dbus.String(self.__symbol))
        struct.append(dbus.String(self.__setup))
        struct.append(dbus.String(self.__layout_variant))
        struct.append(dbus.String(self.__layout_option))
        struct.append(dbus.String(self.__version))

    def deserialize(self, struct):
        super(EngineDesc, self).deserialize(struct)
        self.__name             = struct.pop(0)
        self.__longname         = struct.pop(0)
        self.__description      = struct.pop(0)
        self.__language         = struct.pop(0)
        self.__license          = struct.pop(0)
        self.__author           = struct.pop(0)
        self.__icon             = struct.pop(0)
        self.__layout           = struct.pop(0)
        self.__rank             = struct.pop(0)
        # Keep the serialize order.
        self.__hotkeys          = struct.pop(0)
        self.__symbol           = struct.pop(0)
        self.__setup            = struct.pop(0)
        if len(struct) < 2:
            return
        self.__layout_variant   = struct.pop(0)
        self.__layout_option    = struct.pop(0)
        if len(struct) < 1:
            return
        self.__version          = struct.pop(0)

def test():
    engine = EngineDesc("Hello", "", "", "", "", "", "", "", "", 0, "", "")
    value = serialize_object(engine)
    engine = deserialize_object(value)

if __name__ == "__main__":
    test()
