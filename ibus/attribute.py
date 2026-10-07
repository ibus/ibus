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
        "ATTR_TYPE_UNDERLINE",
        "ATTR_TYPE_FOREGROUND",
        "ATTR_TYPE_BACKGROUND",
        "ATTR_UNDERLINE_NONE",
        "ATTR_UNDERLINE_SINGLE",
        "ATTR_UNDERLINE_DOUBLE",
        "ATTR_UNDERLINE_LOW",
        "ATTR_UNDERLINE_ERROR",
        "Attribute",
        "AttributeUnderline",
        "AttributeForeground",
        "AttributeBackground",
        "AttrList",
        "ARGB", "RGB"
    )

import dbus
from exception import IBusException
from serializable import *

ATTR_TYPE_UNDERLINE = 1
ATTR_TYPE_FOREGROUND = 2
ATTR_TYPE_BACKGROUND = 3

ATTR_UNDERLINE_NONE = 0
ATTR_UNDERLINE_SINGLE = 1
ATTR_UNDERLINE_DOUBLE = 2
ATTR_UNDERLINE_LOW = 3
ATTR_UNDERLINE_ERROR = 4

class Attribute(Serializable):
    __gtype_name__ = "PYIBusAttribute"
    __NAME__ = "IBusAttribute"
    def __init__ (self, type=0, value=0, start_index=0, end_index=0):
        super(Attribute, self).__init__()
        self.__type = type
        self.__value = value
        self.__start_index = start_index
        self.__end_index = end_index

    def get_type(self):
        return self.__type

    def get_value(self):
        return self.__value

    def get_start_index(self):
        return self.__start_index

    def get_end_index(self):
        return self.__end_index

    type        = property(get_type)
    value       = property(get_value)
    start_index = property(get_start_index)
    end_index   = property(get_end_index)

    def serialize(self, struct):
        super(Attribute, self).serialize(struct)
        struct.append (dbus.UInt32(self.__type))
        struct.append (dbus.UInt32(self.__value))
        struct.append (dbus.UInt32(self.__start_index))
        struct.append (dbus.UInt32(self.__end_index))

    def deserialize(self, struct):
        super(Attribute, self).deserialize(struct)
        if len(struct) < 4:
            raise IBusException ("Can not deserialize IBusAttribute")

        self.__type = struct.pop(0)
        self.__value = struct.pop(0)
        self.__start_index = struct.pop(0)
        self.__end_index = struct.pop(0)

class AttributeUnderline (Attribute):
    def __init__(self, value, start_index, end_index):
        Attribute.__init__ (self, ATTR_TYPE_UNDERLINE, value, start_index, end_index)

class AttributeForeground (Attribute):
    def __init__(self, value, start_index, end_index):
        Attribute.__init__ (self, ATTR_TYPE_FOREGROUND, value, start_index, end_index)

class AttributeBackground (Attribute):
    def __init__(self, value, start_index, end_index):
        Attribute.__init__ (self, ATTR_TYPE_BACKGROUND, value, start_index, end_index)

def ARGB (a, r, g, b):
    return ((a & 0xff)<<24) + ((r & 0xff) << 16) + ((g & 0xff) << 8) + (b & 0xff)

def RGB (r, g, b):
    return ARGB (255, r, g, b)

class AttrList(Serializable):
    __gtype_name__ = "PYIBusAttrList"
    __NAME__ = "IBusAttrList"
    def __init__ (self, attrs = []):
        super(AttrList, self).__init__()
        self._attrs = []
        for attr in attrs:
            self.append (attr)

    def append (self, attr):
        assert isinstance (attr, Attribute)
        self._attrs.append (attr)

    def serialize (self, struct):
        super(AttrList, self).serialize (struct)
        array = map (lambda a: serialize_object(a), self._attrs)
        array = dbus.Array (array, signature = "v")
        struct.append(array)

    def deserialize (self, struct):
        super(AttrList, self).deserialize(struct)
        attrs = map(lambda v: deserialize_object(v), struct.pop(0))
        self._attrs = attrs

    def __iter__ (self):
        return self._attrs.__iter__ ()

def test():
    attr_list = AttrList()
    attr_list.append (Attribute())
    attr_list.append (Attribute())
    attr_list.append (Attribute())
    attr_list.append (Attribute())
    attr_list.append (Attribute())
    value = serialize_object(attr_list)
    attr_list = deserialize_object(value)

if __name__ == "__main__":
    test()
