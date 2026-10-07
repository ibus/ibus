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
        "Component",
    )

import dbus
from exception import IBusException
from serializable import *
from enginedesc import *
from observedpath import *

class Component(Serializable):
    __gtype_name__ = "PYIBusComponent"
    __NAME__ = "IBusComponent"
    def __init__ (self, name="", description="", version="", license="", author="", homepage="", _exec="", textdomain=""):
        super(Component, self).__init__()
        self.__name = name
        self.__description = description
        self.__version = version
        self.__license = license
        self.__author = author
        self.__homepage = homepage
        self.__exec = _exec
        self.__textdomain = textdomain
        self.__observed_paths = []
        self.__engines = []

    def get_name(self):
        return self.__name

    def get_description(self):
        return self.__description

    def get_version(self):
        return self.__version

    def get_license(self):
        return self.__license

    def get_author(self):
        return self.__author

    def get_homepage(self):
        return self.__homepage

    def get_exec(self):
        return self.__exec

    def get_textdomain(self):
        return self.__textdomain

    def get_observed_paths(self):
        return self.__observed_paths[:]

    def get_engines(self):
        return self.__engines[:]

    name        = property(get_name)
    description = property(get_description)
    version     = property(get_version)
    license     = property(get_license)
    author      = property(get_author)
    homepage    = property(get_homepage)
    _exec       = property(get_exec)
    textdomain  = property(get_textdomain)
    observed_paths = property(get_observed_paths)
    engines     = property(get_engines)

    def add_observed_path(self, path):
        self.__observed_paths.append(ObservedPath(path))

    def add_engine(self, name="", longname="", description="", language="", license="", author="", icon="", layout="", hotkeys=""):
        engine = EngineDesc(name, longname, description, language, license, author, icon, layout, hotkeys)
        self.__engines.append(engine)

    def add_engines(self, engines):
        if not isinstance(engines, list):
            raise TypeError("engines must be an instance of list")
        self.__engines.extend(engines)

    def serialize(self, struct):
        super(Component, self).serialize(struct)
        struct.append (dbus.String(self.__name))
        struct.append (dbus.String(self.__description))
        struct.append (dbus.String(self.__version))
        struct.append (dbus.String(self.__license))
        struct.append (dbus.String(self.__author))
        struct.append (dbus.String(self.__homepage))
        struct.append (dbus.String(self.__exec))
        struct.append (dbus.String(self.__textdomain))
        struct.append (dbus.Array(map(serialize_object,self.__observed_paths), signature="v"))
        struct.append (dbus.Array(map(serialize_object,self.__engines), signature="v"))

    def deserialize(self, struct):
        super(Component, self).deserialize(struct)

        self.__name = struct.pop(0)
        self.__description = struct.pop(0)
        self.__version = struct.pop(0)
        self.__license = struct.pop(0)
        self.__author = struct.pop(0)
        self.__homepage = struct.pop(0)
        self.__exec = struct.pop(0)
        self.__textdomain = struct.pop(0)

        self.__observed_paths = map(deserialize_object, struct.pop(0))
        self.__engines = map(deserialize_object, struct.pop(0))

def test():
    text = Component("Hello", "", "", "", "", "", "", "")
    value = serialize_object(text)
    text=  deserialize_object(value)

if __name__ == "__main__":
    test()
