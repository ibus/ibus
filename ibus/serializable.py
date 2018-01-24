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
        "Serializable",
        "serialize_object",
        "deserialize_object",
    )

from object import Object
import dbus
from gi.repository import GObject as gobject
from gi.types import GObjectMeta

__serializable_name_dict = dict()

def serializable_register(classobj):
    # if not issubclass(classobj, Serializable):
    #     raise "%s is not a sub-class of Serializable" % str(classobj)
    __serializable_name_dict[classobj.__NAME__] = classobj

def serialize_object(o):
    if isinstance(o, Serializable):
        l = [o.__NAME__]
        o.serialize(l)
        return dbus.Struct(l)
    else:
        return o

def deserialize_object(v):
    if isinstance(v, tuple):
        struct = list(v)
        type_name = struct.pop(0)
        type_class = __serializable_name_dict[type_name]
        o = type_class()
        o.deserialize (struct)
        return o
    return v

class SerializableMeta(GObjectMeta):
    def __init__(cls, name, bases, dict_):
        super(SerializableMeta, cls).__init__(name, bases, dict_)
        if "__NAME__" in cls.__dict__:
            serializable_register(cls)

class Serializable(Object):
    __metaclass__ = SerializableMeta
    __gtype_name__ = "PYIBusSerializable"
    __NAME__ = "IBusSerializable"
    def __init__(self):
        super(Serializable, self).__init__()
        self.__attachments = dict()

    def set_attachment(self, name, value):
        self.__attachments[name] = value

    def get_attachment(self, name):
        return self.__attachments.get(name, None)

    def serialize(self, struct):
        d = dbus.Dictionary(signature="sv")
        for k, v in self.__attachments.items():
            d[k] = serialize_object(v)
        struct.append(d)

    def deserialize(self, struct):
        d = struct.pop(0)
        self.__attachments = dict()
        for k, v in d.items():
            self.__attachments[k] = deserialize_object(v)

    def do_destroy(self):
        self.__attachments = None
        super(Serializable, self).do_destroy()

__serializable_name_dict["IBusSerializable"] = Serializable
