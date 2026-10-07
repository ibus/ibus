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
        "EngineFactoryBase",
        "FactoryInfo"
    )
import dbus
import object
import interface
from serializable import *
from exception import *

class EngineFactoryBase(object.Object):
    def __init__(self, bus):
        super(EngineFactoryBase, self).__init__()
        self.__proxy = EngineFactoryProxy (self, bus.get_dbusconn(), "/org/freedesktop/IBus/Factory")
        self.__bus = bus

    def initialize(self):
        pass

    def uninitialize(self):
        pass

    def create_engine(self, engine_name):
        raise IBusException("Can not create engine %s" % engine_name)

    def do_destroy(self):
        self.__proxy = None
        self.__bus = None
        super(EngineFactoryBase, self).do_destroy()


class EngineFactoryProxy(interface.IEngineFactory):
    def __init__(self, factory, conn, object_path):
        super(EngineFactoryProxy, self).__init__(conn, object_path)
        self.__factory = factory

    def GetInfo(self):
        return self.__factory.get_info()

    def Initialize(self):
        return self.__factory.initialize()

    def Uninitialize(self):
        return self.__factory.uninitialize()

    def CreateEngine(self, engine_name):
        engine = self.__factory.create_engine(engine_name)
        return engine.get_dbus_object()

    def Destroy(self):
        self.__factory.destroy()
        self.__factory = None
        self.remove_from_connection ()

class FactoryInfo(Serializable):
    __gtype_name__ = "PYIBusFactoryInfo"
    __NAME__ = "IBusFactoryInfo"
    def __init__ (self, path=None, name=None, lang=None, icon=None, authors=None, credits=None):
        super(FactoryInfo, self).__init__()
        self.__path = path
        self.__name = name
        self.__lang = lang
        self.__icon = icon
        self.__authors = authors
        self.__credits = credits

    def get_path(self):
        return self.__path

    def get_name(self):
        return self.__name

    def get_lang(self):
        return self.__lang

    def get_icon(self):
        return self.__icon
    def get_authors(self):
        return self.__authors

    def get_credits(self):
        return self.__credits

    path = property(get_path)
    name = property(get_name)
    lang = property(get_lang)
    icon = property(get_icon)
    authors = property(get_authors)
    credits = property(get_credits)

    def serialize(self, struct):
        super(FactoryInfo, self).serialize(struct)
        struct.append (dbus.ObjectPath(self.__path))
        struct.append (dbus.String(self.__name))
        struct.append (dbus.String(self.__lang))
        struct.append (dbus.String(self.__icon))
        struct.append (dbus.String(self.__authors))
        struct.append (dbus.String(self.__credits))

    def deserialize(self, struct):
        super(FactoryInfo, self).deserialize(struct)
        if len(struct) < 5:
            raise IBusException ("Can not deserialize IBusFactoryInfo")

        self.__path = struct.pop(0)
        self.__name = struct.pop(0)
        self.__lang = struct.pop(0)
        self.__icon = struct.pop(0)
        self.__authors = struct.pop(0)
        self.__credits = struct.pop(0)

