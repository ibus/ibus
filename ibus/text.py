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
        "Text",
    )

import dbus
from exception import IBusException
from serializable import *
from attribute import AttrList

class Text(Serializable):
    __gtype_name__ = "PYIBusText"
    __NAME__ = "IBusText"
    def __init__ (self, text="", attrs=None):
        super(Text, self).__init__()
        self.__text = text
        self.__attrs = attrs

    def get_text(self):
        return self.__text

    def get_attributes(self):
        return self.__attrs

    text        = property(get_text)
    attributes  = property(get_attributes)

    def serialize(self, struct):
        super(Text, self).serialize(struct)
        struct.append (dbus.String(self.__text))
        if self.__attrs == None:
            self.__attrs = AttrList()
        struct.append (serialize_object(self.__attrs))

    def deserialize(self, struct):
        super(Text, self).deserialize(struct)

        self.__text = struct.pop(0)
        self.__attrs = deserialize_object(struct.pop(0))

def test():
    text = Text("Hello")
    value = serialize_object(text)
    text = deserialize_object(value)

if __name__ == "__main__":
    test()
